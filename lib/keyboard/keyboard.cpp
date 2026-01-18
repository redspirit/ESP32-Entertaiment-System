#include "keyboard.h"
#include "scancodes.h"
#include "LOG.h"
#include <Arduino.h>

#define PS2_CLK   38
#define PS2_DATA  39

#define BUF_SIZE  32
#define BUF_MASK  (BUF_SIZE - 1)

#define EVENT_BUF_SIZE 32
#define EVENT_BUF_MASK (EVENT_BUF_SIZE - 1)

#define KEY_BIT(k)   ((k) & 0x1FF)
#define KEY_BYTE(k)  (KEY_BIT(k) >> 3)
#define KEY_MASK(k)  (1 << (KEY_BIT(k) & 7))

namespace keyboard {

    static KeyEvent eventBuf[EVENT_BUF_SIZE];
    static volatile uint8_t eventHead = 0;
    static volatile uint8_t eventTail = 0;

    // ===== Ring buffer =====
    static volatile uint8_t buffer[BUF_SIZE];
    static volatile uint8_t head = 0;
    static volatile uint8_t tail = 0;

    // ===== PS/2 state =====
    static volatile uint8_t bitCount = 0;
    static volatile uint8_t incoming = 0;
    static volatile uint32_t lastTickUs = 0;

    static uint8_t keyBits[64]; // 512 keys
    static uint8_t prevKeyBits[64]; // предыдущее состояние

    static bool capsLock = false;
    static volatile bool ledsDirty = false;

    static volatile uint32_t ps2WriteErrors = 0;
    static volatile uint32_t ps2AckCount = 0;

    // ===== Fast GPIO read =====
    static inline bool readData() {
        // if (PS2_DATA < 32)
        //     return GPIO.in & (1UL << PS2_DATA);
        // else
            return GPIO.in1.val & (1UL << (PS2_DATA - 32));
    }

    // ===== ISR =====
    static void IRAM_ATTR ps2_isr() {
        uint32_t now = micros();

        // ресинхронизация при паузе > 2 ms
        if (now - lastTickUs > 2000) {
            bitCount = 0;
            incoming = 0;
        }
        lastTickUs = now;

        bool val = readData();

        // ===== START BIT =====
        // ждём start bit (DATA = 0)
        if (bitCount == 0) {
            if (val) return;          // не start bit
            incoming = 0;
            bitCount = 1;
            return;
        }

        // ===== DATA BITS 1..8 (LSB first) =====
        if (bitCount >= 1 && bitCount <= 8) {
            if (val)
                incoming |= (1 << (bitCount - 1));
            bitCount++;
            return;
        }

        // ===== PARITY BIT (9) =====
        if (bitCount == 9) {
            // parity игнорируем, но шаг считаем
            bitCount++;
            return;
        }

        // ===== STOP BIT (10) =====
        if (bitCount == 10) {
            // stop bit ОБЯЗАН быть 1
            if (!val) {
                // битый кадр — жёсткий сброс
                bitCount = 0;
                incoming = 0;
                return;
            }

            // кадр корректен — сохраняем байт
            uint8_t next = (head + 1) & BUF_MASK;
            if (next != tail) {
                buffer[head] = incoming;
                head = next;
            }

            bitCount = 0;
            return;
        }
    }

    static bool waitPinState(uint8_t pin, uint8_t state) {
        uint32_t start = micros();
        while (digitalRead(pin) != state) {
            if (micros() - start > 10000) { // PS2_TIMEOUT_US
                return false; // Timeout
            }
        }
        return true;
    }

    // ===== Host → Keyboard write =====
    // Возвращает true, если отправка прошла успешно
    static bool ps2_write(uint8_t data) {
        // Отключаем ISR, чтобы прерывания не мешали отправке
        detachInterrupt(PS2_CLK);

        // 1. Расчет Parity (Odd parity - нечетность)
        // __builtin_parity возвращает 1, если число бит нечетное.
        // Для Odd Parity сумма бит данных + бит четности должна быть нечетной.
        // Если в данных четное кол-во бит (0), нам нужен 1.
        // Если нечетное (1), нам нужен 0.
        bool parity = !(__builtin_parity(data) & 1);

        // 2. Инициализация (Host Request to Send)
        pinMode(PS2_CLK, OUTPUT);
        digitalWrite(PS2_CLK, LOW);
        delayMicroseconds(120); // Держим CLK > 100 мкс

        pinMode(PS2_DATA, OUTPUT);
        digitalWrite(PS2_DATA, LOW);
        delayMicroseconds(10); 

        // Отпускаем CLK, клавиатура должна начать генерировать такты
        pinMode(PS2_CLK, INPUT_PULLUP);

        // --- Отправка 8 бит данных ---
        for (uint8_t i = 0; i < 8; i++) {
            // Ждем, пока клавиатура опустит CLK (считывание хостом готовности)
            if (!waitPinState(PS2_CLK, LOW)) goto error_exit;

            // Выставляем бит данных
            digitalWrite(PS2_DATA, (data & 1) ? HIGH : LOW);
            data >>= 1;

            // Ждем, пока клавиатура поднимет CLK (клавиатура прочитала бит)
            if (!waitPinState(PS2_CLK, HIGH)) goto error_exit;
        }

        // --- Отправка Parity бита ---
        if (!waitPinState(PS2_CLK, LOW)) goto error_exit;
        digitalWrite(PS2_DATA, parity ? HIGH : LOW);
        if (!waitPinState(PS2_CLK, HIGH)) goto error_exit;

        // --- Отправка Stop бита (отпускаем DATA) ---
        if (!waitPinState(PS2_CLK, LOW)) goto error_exit;
        pinMode(PS2_DATA, INPUT_PULLUP); // Отпускаем линию (High)
        if (!waitPinState(PS2_CLK, HIGH)) goto error_exit;

        // Успех: восстанавливаем прерывание
        attachInterrupt(PS2_CLK, ps2_isr, FALLING);
        return true;

    error_exit:
        // Ошибка: переводим пины в безопасное состояние и возвращаем прерывание
        pinMode(PS2_CLK, INPUT_PULLUP);
        pinMode(PS2_DATA, INPUT_PULLUP);
        attachInterrupt(PS2_CLK, ps2_isr, FALLING);
        
        LOG.println("PS/2 Write Timeout Error!");
        return false;
    }

    // Вспомогательная функция для ожидания конкретного байта прямо из буфера
    static bool waitForByte(uint8_t target, uint32_t timeoutMs) {
        uint32_t start = millis();
        while (millis() - start < timeoutMs) {
            // Проверяем, есть ли что-то в буфере ISR
            if (available()) {
                uint8_t b = read(); // Читаем байт, сдвигаем хвост буфера
                if (b == target) {
                    return true;
                }
                // Если пришло что-то другое (например, сканкод нажатой клавиши 
                // прямо перед отправкой команды), мы его "проглотим".
                // В простом варианте это допустимо, чтобы не усложнять логику.
            }
        }
        return false;
    }

    static void setLeds() {
        uint8_t leds = 0;
        if (capsLock) leds |= (1 << 2);

        // Попытка 1: Команда ED
        if (!ps2_write(0xED)) {
            ps2WriteErrors++;
            return; // Аппаратная ошибка (обрыв/таймаут), выходим сразу
        }

        // Ждем ACK на команду
        if (!waitForByte(0xFA, 200)) {
            ps2WriteErrors++;
            LOG.println("No ACK for 0xED");
            // Важно: даже если нет ACK, можно попробовать послать данные, 
            // чтобы сбросить стейт-машину клавиатуры, но часто это бесполезно.
        }

        // Попытка 2: Данные
        if (!ps2_write(leds)) {
            ps2WriteErrors++;
            return;
        }

        // Ждем ACK на данные
        if (!waitForByte(0xFA, 200)) {
             ps2WriteErrors++;
             LOG.println("No ACK for LED data");
        }
    }

    // ===== Public API =====
    void init() {
        initKeyMap();
        pinMode(PS2_CLK, INPUT_PULLUP);
        pinMode(PS2_DATA, INPUT_PULLUP);

        attachInterrupt(PS2_CLK, ps2_isr, FALLING);

        delay(300); // питание стабилизируется

        // RESET
        ps2_write(0xFF);

        bool ack = false;
        bool bat = false;
        uint32_t t = millis();

        while (millis() - t < 300) {
            if (available()) {
                uint8_t b = read();
                if (b == 0xFA) ack = true;
                if (b == 0xAA) bat = true;
            }
        }

        // LEDs test
        if (ack && bat) {
            ps2_write(0xED);
            t = millis();
            while (millis() - t < 100) {
                if (available() && read() == 0xFA) break;
            }
            ps2_write(0x03); // Scroll + Num + Caps
        }
    }

    bool available() {
        return head != tail;
    }

    static void pushEvent(uint16_t key, bool pressed, bool extended) {
        // обновляем состояние ВСЕГДА
        if (pressed)
            keyBits[KEY_BYTE(key)] |=  KEY_MASK(key);
        else
            keyBits[KEY_BYTE(key)] &= ~KEY_MASK(key);

        uint8_t next = (eventHead + 1) & EVENT_BUF_MASK;
        if (next == eventTail) {
            // очередь переполнена — событие не сохраняем
            return;
        }

        eventBuf[eventHead] = { key, pressed, extended };
        eventHead = next;
    }

    static void processScanCode(uint8_t sc) {
        // LOG.print("[RAW] ");
        // LOG.println(sc, HEX);
        static bool release  = false;
        static bool extended = false;

        // ACK от клавиатуры — НЕ scan-code
        if (sc == 0xFA) {
            ps2AckCount++;
            return;
        }

        if (sc == 0xE0) {
            extended = true;
            return;
        }

        if (sc == 0xF0) {
            release = true;
            return;
        }

        uint16_t key = sc | (extended ? 0x100 : 0);
        bool pressed = !release;

        pushEvent(key, pressed, extended);

        // CapsLock — переключатель по НАЖАТИЮ
        if (key == Key::CAPS && pressed) {
            capsLock = !capsLock;
            ledsDirty = true;   // только помечаем
        }

        release  = false;
        extended = false;
    }

    bool readKey(KeyEvent& ev) {
        if (eventHead == eventTail)
            return false;

        noInterrupts();
        ev = eventBuf[eventTail];
        eventTail = (eventTail + 1) & EVENT_BUF_MASK;
        interrupts();

        return true;
    }

    uint8_t read() {
        noInterrupts();
        uint8_t v = buffer[tail];
        tail = (tail + 1) & BUF_MASK;
        interrupts();
        return v;
    }

    bool isPressed(uint16_t key) {
        return keyBits[KEY_BYTE(key)] & KEY_MASK(key);
    }

    void poll() {
        // переработать ВСЕ raw scan-codes
        while (available()) {
            processScanCode(read());
        }

        if (ledsDirty) {
            setLeds();
            ledsDirty = false;
        }        
    }
    
    void beginFrame() {
        // сохранить состояние клавиш в начале кадра
        memcpy(prevKeyBits, keyBits, sizeof(keyBits));
    }

    bool isJustPressed(uint16_t key) {
        uint8_t b = KEY_BYTE(key);
        uint8_t m = KEY_MASK(key);

        return (keyBits[b] & m) && !(prevKeyBits[b] & m);
    }

    bool isJustReleased(uint16_t key) {
        uint8_t b = KEY_BYTE(key);
        uint8_t m = KEY_MASK(key);

        return !(keyBits[b] & m) && (prevKeyBits[b] & m);
    }

    bool getChar(char& out) {
        KeyEvent ev;
        if (!readKey(ev))
            return false;

        if (!ev.pressed)
            return false;

        // не печатаем модификаторы
        if (ev.key == Key::SHIFT_LEFT  ||
            ev.key == Key::SHIFT_RIGHT ||
            ev.key == Key::CTRL_LEFT   ||
            ev.key == Key::CTRL_RIGHT  ||
            ev.key == Key::ALT_LEFT    ||
            ev.key == Key::ALT_RIGHT)
            return false;

        KeyChar kc = KeyMap[ev.key];
        if (!kc.normal)
            return false;

        bool shift =
            isPressed(Key::SHIFT_LEFT) ||
            isPressed(Key::SHIFT_RIGHT);

        bool upper = capsLock ^ shift;
        out = upper ? kc.shifted : kc.normal;
        return true;
    }

    uint32_t getPs2AckCount() {
        return ps2AckCount;
    }

    uint32_t getPs2WriteErrors() {
        return ps2WriteErrors;
    }

}

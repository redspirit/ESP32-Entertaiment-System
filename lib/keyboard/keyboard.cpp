#include "keyboard.h"
#include <Arduino.h>

#define PS2_CLK   38
#define PS2_DATA  39

#define BUF_SIZE  32
#define BUF_MASK  (BUF_SIZE - 1)

#define EVENT_BUF_SIZE 16
#define EVENT_BUF_MASK (EVENT_BUF_SIZE - 1)

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

    // ===== Fast GPIO read =====
    static inline bool readData() {
        if (PS2_DATA < 32)
            return GPIO.in & (1UL << PS2_DATA);
        else
            return GPIO.in1.val & (1UL << (PS2_DATA - 32));
    }

    // ===== ISR =====
    static void IRAM_ATTR ps2_isr() {
        uint32_t now = micros();

        // ресинхронизация при паузе > 2 ms
        if (now - lastTickUs > 2000) {
            bitCount = 0;
        }
        lastTickUs = now;

        bool val = readData();

        // ждём start bit (DATA = 0)
        if (bitCount == 0) {
            if (val) return;          // не start bit
            incoming = 0;
            bitCount = 1;
            return;
        }

        // data bits 1..8 (LSB first)
        if (bitCount >= 1 && bitCount <= 8) {
            if (val)
                incoming |= (1 << (bitCount - 1));
        }

        // parity bit (9) — игнорируем, но учитываем
        // stop bit (10) — должен быть 1

        bitCount++;

        if (bitCount == 11) {
            uint8_t next = (head + 1) & BUF_MASK;
            if (next != tail) {
                buffer[head] = incoming;
                head = next;
            }
            bitCount = 0;
        }
    }

    // ===== Host → Keyboard write =====
    static void ps2_write(uint8_t data) {
        detachInterrupt(PS2_CLK);

        pinMode(PS2_CLK, OUTPUT);
        pinMode(PS2_DATA, OUTPUT);

        digitalWrite(PS2_CLK, LOW);
        delayMicroseconds(150);
        digitalWrite(PS2_DATA, LOW);
        delayMicroseconds(10);

        pinMode(PS2_CLK, INPUT_PULLUP); // keyboard clocks

        // 8 data bits
        for (uint8_t i = 0; i < 8; i++) {
            while (digitalRead(PS2_CLK));
            digitalWrite(PS2_DATA, (data & 1) ? HIGH : LOW);
            while (!digitalRead(PS2_CLK));
            data >>= 1;
        }

        // parity (odd)
        while (digitalRead(PS2_CLK));
        digitalWrite(PS2_DATA, HIGH);
        while (!digitalRead(PS2_CLK));

        // stop bit
        pinMode(PS2_DATA, INPUT_PULLUP);

        attachInterrupt(PS2_CLK, ps2_isr, FALLING);
    }

    // ===== Public API =====
    void init() {
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

    static void processScanCode(uint8_t sc) {
        static bool release  = false;
        static bool extended = false;

        if (sc == 0xE0) {
            extended = true;
            return;
        }

        if (sc == 0xF0) {
            release = true;
            return;
        }

        uint8_t next = (eventHead + 1) & EVENT_BUF_MASK;
        if (next == eventTail) {
            // очередь переполнена — событие теряем
            release = false;
            extended = false;
            return;
        }

        eventBuf[eventHead] = {
            .key = uint16_t(sc | (extended ? 0x100 : 0)),
            .pressed = !release,
            .extended = extended
        };
        eventHead = next;

        release = false;
        extended = false;
    }

    bool readKey(KeyEvent& ev) {
        // сначала переработать все raw scan-codes
        while (available()) {
            processScanCode(read());
        }

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
}

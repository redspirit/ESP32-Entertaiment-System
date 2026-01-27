#include "keyboard.h"
#include "LOG.h"
#include <Arduino.h>

// ===== PS/2 pins =====
#define PS2_CLK   38
#define PS2_DATA  39

#define BUF_MASK        (BUF_SIZE - 1)
#define EVENT_BUF_MASK  (EVENT_BUF_SIZE - 1)

#define KEY_BIT(k)   ((k) & 0x1FF)
#define KEY_BYTE(k)  (KEY_BIT(k) >> 3)
#define KEY_MASK(k)  (1 << (KEY_BIT(k) & 7))

// ===== Singleton for ISR =====
Keyboard* Keyboard::instance_ = nullptr;

// ===== Fast GPIO read =====
static inline bool readDataFast() {
    return GPIO.in1.val & (1UL << (PS2_DATA - 32));
}

// ===== ISR trampoline =====
void IRAM_ATTR Keyboard::isrTrampoline() {
    if (instance_)
        instance_->handleIsr();
}

// ===== Ctor / Dtor =====
Keyboard::Keyboard() {
}

Keyboard::~Keyboard() {
    detachInterrupt(PS2_CLK);
    instance_ = nullptr;
}

// ===== ISR body =====
void IRAM_ATTR Keyboard::handleIsr() {
    uint32_t now = micros();

    // ресинхронизация при паузе > 2 ms
    if (now - lastTickUs_ > 2000) {
        bitCount_ = 0;
        incoming_ = 0;
    }
    lastTickUs_ = now;

    bool val = readDataFast();

    // START BIT
    if (bitCount_ == 0) {
        if (val) return;
        incoming_ = 0;
        bitCount_ = 1;
        return;
    }

    // DATA BITS 1..8
    if (bitCount_ >= 1 && bitCount_ <= 8) {
        if (val)
            incoming_ |= (1 << (bitCount_ - 1));
        bitCount_++;
        return;
    }

    // PARITY (ignored)
    if (bitCount_ == 9) {
        bitCount_++;
        return;
    }

    // STOP BIT
    if (bitCount_ == 10) {
        if (!val) {
            bitCount_ = 0;
            incoming_ = 0;
            return;
        }

        uint8_t next = (head_ + 1) & BUF_MASK;
        if (next != tail_) {
            buffer_[head_] = incoming_;
            head_ = next;
        }

        bitCount_ = 0;
    }
}

// ===== PS/2 host write helpers =====
static bool waitPinState(uint8_t pin, uint8_t state) {
    uint32_t start = micros();
    while (digitalRead(pin) != state) {
        if (micros() - start > 10000)
            return false;
    }
    return true;
}

bool Keyboard::ps2Write(uint8_t data) {
    detachInterrupt(PS2_CLK);
    bool parity = !(__builtin_parity(data) & 1);

    pinMode(PS2_CLK, OUTPUT);
    digitalWrite(PS2_CLK, LOW);
    delayMicroseconds(120);
    pinMode(PS2_DATA, OUTPUT);
    digitalWrite(PS2_DATA, LOW);
    delayMicroseconds(10);
    pinMode(PS2_CLK, INPUT_PULLUP);

    for (uint8_t i = 0; i < 8; i++) {
        if (!waitPinState(PS2_CLK, LOW)) goto error;
        digitalWrite(PS2_DATA, (data & 1) ? HIGH : LOW);
        if (!waitPinState(PS2_CLK, HIGH)) goto error;
        data >>= 1;
    }

    if (!waitPinState(PS2_CLK, LOW)) goto error;
    digitalWrite(PS2_DATA, parity ? HIGH : LOW);
    if (!waitPinState(PS2_CLK, HIGH)) goto error;

    if (!waitPinState(PS2_CLK, LOW)) goto error;
    pinMode(PS2_DATA, INPUT_PULLUP);

    lastTickUs_ = micros();
    bitCount_ = 0;
    attachInterrupt(PS2_CLK, isrTrampoline, FALLING);

    waitPinState(PS2_CLK, HIGH);
    return true;

error:
    pinMode(PS2_CLK, INPUT_PULLUP);
    pinMode(PS2_DATA, INPUT_PULLUP);
    attachInterrupt(PS2_CLK, isrTrampoline, FALLING);
    return false;
}

// ===== Init =====
void Keyboard::init() {
    instance_ = this;

    initKeyMap();

    pinMode(PS2_CLK, INPUT_PULLUP);
    pinMode(PS2_DATA, INPUT_PULLUP);
    attachInterrupt(PS2_CLK, isrTrampoline, FALLING);

    delay(300);

    // RESET
    ps2Write(0xFF);

    bool ack = false;
    bool bat = false;
    uint32_t t = millis();

    while (millis() - t < 300) {
        if (available()) {
            uint8_t b = readRaw();
            if (b == 0xFA) ack = true;
            if (b == 0xAA) bat = true;
        }
    }

    if (ack && bat) {
        ps2Write(0xED);
        t = millis();
        while (millis() - t < 100) {
            if (available() && readRaw() == 0xFA)
                break;
        }
        ps2Write(0x03);
    }
}

// ===== Key map =====
void Keyboard::initKeyMap() {
    keyMap_[A] = {'a','A'};
    keyMap_[B] = {'b','B'};
    keyMap_[C] = {'c','C'};
    keyMap_[D] = {'d','D'};
    keyMap_[E] = {'e','E'};
    keyMap_[F] = {'f','F'};
    keyMap_[G] = {'g','G'};
    keyMap_[H] = {'h','H'};
    keyMap_[I] = {'i','I'};
    keyMap_[J] = {'j','J'};
    keyMap_[K] = {'k','K'};
    keyMap_[L] = {'l','L'};
    keyMap_[M] = {'m','M'};
    keyMap_[N] = {'n','N'};
    keyMap_[O] = {'o','O'};
    keyMap_[P] = {'p','P'};
    keyMap_[Q] = {'q','Q'};
    keyMap_[R] = {'r','R'};
    keyMap_[S] = {'s','S'};
    keyMap_[T] = {'t','T'};
    keyMap_[U] = {'u','U'};
    keyMap_[V] = {'v','V'};
    keyMap_[W] = {'w','W'};
    keyMap_[X] = {'x','X'};
    keyMap_[Y] = {'y','Y'};
    keyMap_[Z] = {'z','Z'};

    keyMap_[NUM_1] = {'1','!'};
    keyMap_[NUM_2] = {'2','@'};
    keyMap_[NUM_3] = {'3','#'};
    keyMap_[NUM_4] = {'4','$'};
    keyMap_[NUM_5] = {'5','%'};
    keyMap_[NUM_6] = {'6','^'};
    keyMap_[NUM_7] = {'7','&'};
    keyMap_[NUM_8] = {'8','*'};
    keyMap_[NUM_9] = {'9','('};
    keyMap_[NUM_0] = {'0',')'};

    keyMap_[SPACE] = {' ',' '};
    keyMap_[MINUS] = {'-','_'};
    keyMap_[EQUAL] = {'=','+'};
    keyMap_[LBRACKET] = {'[','{'};
    keyMap_[RBRACKET] = {']','}'};
    keyMap_[BACKSLASH] = {'\\','|'};
    keyMap_[SEMI] = {';',':'};
    keyMap_[QUOTE] = {'\'','"'};
    keyMap_[COMMA] = {',','<'};
    keyMap_[DOT] = {'.','>'};
    keyMap_[SLASH] = {'/','?'};
    keyMap_[GRAVE] = {'`','~'};
}

// ===== Raw buffer =====
bool Keyboard::available() const {
    return head_ != tail_;
}

uint8_t Keyboard::readRaw() {
    noInterrupts();
    uint8_t v = buffer_[tail_];
    tail_ = (tail_ + 1) & BUF_MASK;
    interrupts();
    return v;
}

// ===== Scan code processing =====
void Keyboard::pushEvent(uint16_t key, bool pressed, bool extended) {
    if (pressed)
        keyBits_[KEY_BYTE(key)] |=  KEY_MASK(key);
    else
        keyBits_[KEY_BYTE(key)] &= ~KEY_MASK(key);

    uint8_t next = (eventHead_ + 1) & EVENT_BUF_MASK;
    if (next == eventTail_)
        return;

    eventBuf_[eventHead_] = { key, pressed, extended };
    eventHead_ = next;
}

void Keyboard::processScanCode(uint8_t sc) {
    static bool release = false;
    static bool extended = false;

    if (sc == 0xFA) {
        ps2AckCount_++;
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

    if (key == CAPS && pressed) {
        capsLock_ = !capsLock_;
        ledsDirty_ = true;
    }

    release = false;
    extended = false;
}

bool Keyboard::waitForAck(uint32_t timeoutMs) {
    uint32_t start = millis();
    uint32_t initialAck = ps2AckCount_; 
    
    while (millis() - start < timeoutMs) {
        // Если в буфере что-то появилось - обрабатываем
        while (available()) {
            processScanCode(readRaw());
        }
        // Если счетчик ACK увеличился - значит мы получили 0xFA
        if (ps2AckCount_ > initialAck) return true;
        
        yield(); // Даем ESP32 заняться фоновыми задачами
    }
    return false;
}

void Keyboard::setLeds() {
    uint8_t leds = 0;
    if (capsLock_)
        leds |= (1 << 2);

    if (ps2Write(0xED)) {
        if (!waitForAck(200))
            ps2WriteErrors_++;
    } else {
        ps2WriteErrors_++;
        return;
    }

    if (ps2Write(leds)) {
        if (!waitForAck(200))
            ps2WriteErrors_++;
    } else {
        ps2WriteErrors_++;
    }
}

void Keyboard::poll() {
    while (available())
        processScanCode(readRaw());

    if (ledsDirty_) {
        setLeds();
        ledsDirty_ = false;
    }
}

void Keyboard::beginFrame() {
    memcpy(prevKeyBits_, keyBits_, sizeof(keyBits_));
}

bool Keyboard::readKey(KeyEvent& ev) {
    if (eventHead_ == eventTail_)
        return false;

    noInterrupts();
    ev = eventBuf_[eventTail_];
    eventTail_ = (eventTail_ + 1) & EVENT_BUF_MASK;
    interrupts();
    return true;
}

bool Keyboard::isPressed(uint16_t key) const {
    return keyBits_[KEY_BYTE(key)] & KEY_MASK(key);
}

bool Keyboard::isJustPressed(uint16_t key) const {
    uint8_t b = KEY_BYTE(key), m = KEY_MASK(key);
    return (keyBits_[b] & m) && !(prevKeyBits_[b] & m);
}

bool Keyboard::isJustReleased(uint16_t key) const {
    uint8_t b = KEY_BYTE(key), m = KEY_MASK(key);
    return !(keyBits_[b] & m) && (prevKeyBits_[b] & m);
}

bool Keyboard::getChar(char& out) {
    KeyEvent ev;
    if (!readKey(ev) || !ev.pressed)
        return false;

    if (ev.key == SHIFT_LEFT || ev.key == SHIFT_RIGHT ||
        ev.key == CTRL_LEFT  || ev.key == CTRL_RIGHT  ||
        ev.key == ALT_LEFT   || ev.key == ALT_RIGHT)
        return false;

    KeyChar kc = keyMap_[ev.key];
    if (!kc.normal)
        return false;

    bool shift = isPressed(SHIFT_LEFT) || isPressed(SHIFT_RIGHT);
    bool upper = capsLock_ ^ shift;
    out = upper ? kc.shifted : kc.normal;
    return true;
}

uint32_t Keyboard::getPs2AckCount() const {
    return ps2AckCount_;
}

uint32_t Keyboard::getPs2WriteErrors() const {
    return ps2WriteErrors_;
}

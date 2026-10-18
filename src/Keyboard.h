#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "Arduino_FreeRTOS.h"
#include <avr/pgmspace.h>

constexpr uint8_t keyNone = 255;
constexpr uint8_t keyEsc = 0;
constexpr uint8_t key1 = 1;
constexpr uint8_t key2 = 2;
constexpr uint8_t key3 = 3;
constexpr uint8_t key4 = 4;
constexpr uint8_t key5 = 5;
constexpr uint8_t key6 = 6;
constexpr uint8_t key7 = 7;
constexpr uint8_t key8 = 8;
constexpr uint8_t key9 = 9;
constexpr uint8_t key0 = 10;

constexpr uint8_t keyMinus = 11;
constexpr uint8_t keyEqual = 12;
constexpr uint8_t keyBackSlash = 13;
constexpr uint8_t keySemicolon = 14;
constexpr uint8_t keySingleQuote = 15;
constexpr uint8_t keySqrBracketLeft = 16;
constexpr uint8_t keySqrBracketRight = 17;
constexpr uint8_t keySlash = 18;
constexpr uint8_t keyIns = 19;
constexpr uint8_t keyDel = 20;
constexpr uint8_t keyBackspace = 21;

constexpr uint8_t keyTab = 22;
constexpr uint8_t keyQ = 23;
constexpr uint8_t keyW = 24;
constexpr uint8_t keyE = 25;
constexpr uint8_t keyR = 26;
constexpr uint8_t keyT = 27;
constexpr uint8_t keyY = 28;
constexpr uint8_t keyU = 29;
constexpr uint8_t keyI = 30;
constexpr uint8_t keyO = 31;
constexpr uint8_t keyP = 32;

constexpr uint8_t keyCaps = 33;
constexpr uint8_t keyA = 34;
constexpr uint8_t keyS = 35;
constexpr uint8_t keyD = 36;
constexpr uint8_t keyF = 37;
constexpr uint8_t keyG = 38;
constexpr uint8_t keyH = 39;
constexpr uint8_t keyJ = 40;
constexpr uint8_t keyK = 41;
constexpr uint8_t keyL = 42;
constexpr uint8_t keyEnter = 43;

constexpr uint8_t keyShift = 44;
constexpr uint8_t keyZ = 45;
constexpr uint8_t keyX = 46;
constexpr uint8_t keyC = 47;
constexpr uint8_t keyV = 48;
constexpr uint8_t keyB = 49;
constexpr uint8_t keyN = 50;
constexpr uint8_t keyM = 51;
constexpr uint8_t keyHome = 52;
constexpr uint8_t keyUp = 53;
constexpr uint8_t keyEnd = 54;

constexpr uint8_t keyCtrl = 55;
constexpr uint8_t keyFn = 56;
constexpr uint8_t keySpace = 57; // Длинный пробел ломает порядок сканкодов
constexpr uint8_t keyAlt = 60;
constexpr uint8_t keyComma = 61;
constexpr uint8_t keyDot = 62;
constexpr uint8_t keyLeft = 63;
constexpr uint8_t keyDown = 64;
constexpr uint8_t keyRight = 65;


struct KBState
{
    uint8_t pressedKey;
    char pressedChar;
    uint8_t modifiers; // Биты: 0=Shift, 1=Ctrl, 2=Alt, 3=Fn

    // Проверка отдельных модификаторов
    inline bool shift() const { return modifiers & 0x01; }
    inline bool ctrl() const { return modifiers & 0x02; }
    inline bool alt() const { return modifiers & 0x04; }
    inline bool fn() const { return modifiers & 0x08; }

    // Проверка наличия хотя бы одного модификатора
    inline bool anyModifier() const { return modifiers & 0x0F; }

    // Проверка конкретной клавиши
    inline bool is(uint8_t key) const { return pressedKey == key; }

    // Проверка клавиши с модификатором
    inline bool isCtrl(uint8_t key) const { return pressedKey == key && ctrl() && !(modifiers & 0x05); }
    inline bool isShift(uint8_t key) const { return pressedKey == key && shift() && !(modifiers & 0x0E); }
    inline bool isAlt(uint8_t key) const { return pressedKey == key && alt() && !(modifiers & 0x0B); }
    inline bool isFn(uint8_t key) const { return pressedKey == key && fn(); }

    // Проверка клавиши без модификаторов
    inline bool isPlain(uint8_t key) const { return pressedKey == key && !anyModifier(); }
};

class Keyboard
{
private:
    uint8_t keyStates[11];
    uint8_t stableKeyStates[11];
    uint8_t stableCounter;
    uint8_t prevKey;
    TickType_t prevKeyTick;
    bool capsLockState;
    KBState getKBStateInternal(bool consume);
    bool readKeyboardAllowed();
public:
    Keyboard();
    void scan();
    KBState getKBState();
    bool isKeyPressed(uint8_t key);
    KBState peekKBState();
    KBState consumeKBState();
    bool check(uint8_t key = keyNone, bool requireCtrl = false, bool requireShift = false,
               bool requireAlt = false, bool requireFn = false);
    bool take(uint8_t key = keyNone, bool requireCtrl = false, bool requireShift = false,
              bool requireAlt = false, bool requireFn = false);
};

extern Keyboard keyboard;
#endif // KEYBOARD_H

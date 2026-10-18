// Keyboard.cpp
#include "Keyboard.h"
#include "Common.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <util/delay.h>

const uint8_t STABLE_COUNT = 3;
const uint16_t DELAY_SECOND_READ = 500;
const uint16_t DELAY_NEXT_READ = 200;

const uint8_t KEY_COLS = 11;
const uint8_t KEY_ROWS = 6;
const uint8_t ROW_MASK = (1 << KEY_ROWS) - 1; // 00111111
const uint8_t COL_MASK = 0x0F;                // биты PL0-PL3

Keyboard keyboard;

// Таблица символов в PROGMEM
const unsigned char kb_chars[66] PROGMEM = {
    27,   // 0: keyEsc
    '1',  // 1: key1
    '2',  // 2: key2
    '3',  // 3: key3
    '4',  // 4: key4
    '5',  // 5: key5
    '6',  // 6: key6
    '7',  // 7: key7
    '8',  // 8: key8
    '9',  // 9: key9
    '0',  // 10: key0
    '-',  // 11: keyMinus
    '=',  // 12: keyEqual
    '\\', // 13: keyBackSlash
    ';',  // 14: keySemicolon
    '\'', // 15: keySingleQuote
    '[',  // 16: keySqrBracketLeft
    ']',  // 17: keySqrBracketRight
    '/',  // 18: keySlash
    0,    // 19: keyIns (нет символа)
    0,    // 20: keyDel (нет символа)
    '\b', // 21: keyBackspace
    '\t', // 22: keyTab
    'q',  // 23: keyQ
    'w',  // 24: keyW
    'e',  // 25: keyE
    'r',  // 26: keyR
    't',  // 27: keyT
    'y',  // 28: keyY
    'u',  // 29: keyU
    'i',  // 30: keyI
    'o',  // 31: keyO
    'p',  // 32: keyP
    0,    // 33: keyCaps (нет символа)
    'a',  // 34: keyA
    's',  // 35: keyS
    'd',  // 36: keyD
    'f',  // 37: keyF
    'g',  // 38: keyG
    'h',  // 39: keyH
    'j',  // 40: keyJ
    'k',  // 41: keyK
    'l',  // 42: keyL
    '\n', // 43: keyEnter
    0,    // 44: keyShift (нет символа)
    'z',  // 45: keyZ
    'x',  // 46: keyX
    'c',  // 47: keyC
    'v',  // 48: keyV
    'b',  // 49: keyB
    'n',  // 50: keyN
    'm',  // 51: keyM
    0,    // 52: keyHome (нет символа)
    0,    // 53: keyUp (нет символа)
    0,    // 54: keyEnd (нет символа)
    0,    // 55: keyCtrl (нет символа)
    0,    // 56: keyFn (нет символа)
    ' ',  // 57: keySpace
    0,    // 58: зарезервировано
    0,    // 59: зарезервировано
    0,    // 60: keyAlt (нет символа)
    ',',  // 61: keyComma
    '.',  // 62: keyDot
    0,    // 63: keyLeft (нет символа)
    0,    // 64: keyDown (нет символа)
    0     // 65: keyRight (нет символа)
};

// Таблица для Shift-символов
struct ShiftMap
{
    uint8_t key;
    char normal;
    char shifted;
};

const ShiftMap shiftMap[] PROGMEM = {
    {key1, '1', '!'}, {key2, '2', '@'}, {key3, '3', '#'}, {key4, '4', '$'}, {key5, '5', '%'}, {key6, '6', '^'}, {key7, '7', '&'}, {key8, '8', '*'}, {key9, '9', '('}, {key0, '0', ')'}, {keyMinus, '-', '_'}, {keyEqual, '=', '+'}, {keyBackSlash, '\\', '|'}, {keySemicolon, ';', ':'}, {keySingleQuote, '\'', '"'}, {keySqrBracketLeft, '[', '{'}, {keySqrBracketRight, ']', '}'}, {keySlash, '/', '?'}, {keyComma, ',', '<'}, {keyDot, '.', '>'}};

Keyboard::Keyboard()
{
    capsLockState = false;
    DDRC = 0x00;  // строки на вход
    PORTC = 0xFF; // подтяжка
    DDRL = 0xFF;  // дешифратор на выход
    memset(keyStates, 0xFF, KEY_COLS);
    memset(stableKeyStates, 0xFF, KEY_COLS);
    prevKey = keyNone;
    stableCounter = 0;
}

void Keyboard::scan()
{
    bool noPressed = true;

    if (stableCounter < STABLE_COUNT)
    {
        stableCounter++;
    }
    else
    {
        memcpy(stableKeyStates, keyStates, KEY_COLS);
    }

    for (uint8_t col = 0; col < KEY_COLS; col++)
    {
        PORTL = (PORTL & ~COL_MASK) | (col & COL_MASK);
        _delay_us(2); // Небольшая задержка для стабилизации
        uint8_t colData = ~PINC;
        colData = colData & 0x3F;

        if (keyStates[col] != colData)
        {
            stableCounter = 0;
            keyStates[col] = colData;
        }

        if (colData != 0)
            noPressed = false;
    }
}

KBState Keyboard::peekKBState()
{
    return getKBStateInternal(false);
}

KBState Keyboard::consumeKBState()
{
    return getKBStateInternal(true);
}

KBState Keyboard::getKBStateInternal(bool consume)
{
    KBState kbstate = {keyNone, '\0', 0};

    if (isKeyPressed(keyShift))
        kbstate.modifiers |= 0x01;
    if (isKeyPressed(keyCtrl))
        kbstate.modifiers |= 0x02;
    if (isKeyPressed(keyAlt))
        kbstate.modifiers |= 0x04;
    if (isKeyPressed(keyFn))
        kbstate.modifiers |= 0x08;

    TickType_t now = xTaskGetTickCount();

    if (stableCounter == STABLE_COUNT)
    {
        bool anyKeyPressed = false;
        for (uint8_t col = 0; col < 11; col++)
        {
            if (stableKeyStates[col] != 0)
            {
                anyKeyPressed = true;
                break;
            }
        }

        if (!anyKeyPressed)
        {
            prevKey = keyNone;
            prevKeyTick = 0;
        }
    }

    if (stableCounter == STABLE_COUNT)
    {
        for (uint8_t col = 0; col < 11; col++)
        {
            uint8_t colData = stableKeyStates[col];

            if (colData != 0)
            {
                uint8_t row = __builtin_ctz(colData);

                if (row < 6)
                {
                    uint8_t keyCode = row * 11 + col;

                    if (keyCode == keyShift || keyCode == keyCtrl || keyCode == keyAlt || keyCode == keyFn)
                        continue;

                    if (keyCode != prevKey)
                    {
                        if (consume)
                        {
                            prevKey = keyCode;
                            prevKeyTick = now + pdMS_TO_TICKS(DELAY_SECOND_READ);
                        }
                        kbstate.pressedKey = keyCode;
                        kbstate.pressedChar = pgm_read_byte(&kb_chars[keyCode]);

                        if (keyCode == keyCaps)
                        {
                            capsLockState = !capsLockState;
                            kbstate.pressedKey = keyNone;
                        }
                    }
                    else if (prevKeyTick != 0 && now >= prevKeyTick)
                    {
                        bool stillPressed = (stableKeyStates[col] & (1 << row)) != 0;
                        if (!stillPressed)
                            break;

                        if (consume)
                        {
                            prevKeyTick = now + pdMS_TO_TICKS(DELAY_NEXT_READ);
                        }
                        kbstate.pressedKey = keyCode;
                        kbstate.pressedChar = pgm_read_byte(&kb_chars[keyCode]);
                    }

                    break;
                }
            }
        }
    }

    // Обработка регистра и shift-символов
    if (kbstate.pressedChar >= 'a' && kbstate.pressedChar <= 'z')
    {
        if (capsLockState ^ kbstate.shift())
        {
            kbstate.pressedChar -= 32;
        }
    }
    else
    {
        for (uint8_t i = 0; i < sizeof(shiftMap) / sizeof(ShiftMap); i++)
        {
            ShiftMap m;
            memcpy_P(&m, &shiftMap[i], sizeof(ShiftMap));
            if (m.key == kbstate.pressedKey)
            {
                kbstate.pressedChar = kbstate.shift() ? m.shifted : m.normal;
                break;
            }
        }
    }

    return kbstate;
}

bool Keyboard::readKeyboardAllowed()
{
    TaskHandle_t cur = xTaskGetCurrentTaskHandle();
    return cur == activeTask || cur == shellTask;
}

KBState Keyboard::getKBState()
{
    KBState kbstate = {keyNone, '\0', 0};
    if (!readKeyboardAllowed())
        return kbstate;
    else
        return consumeKBState();
}

bool Keyboard::isKeyPressed(uint8_t key)
{
    if (!readKeyboardAllowed())
        return false;
    uint8_t row = key / KEY_COLS;
    uint8_t col = key % KEY_COLS;

    if (row >= KEY_ROWS || col >= KEY_COLS)
        return false;
    return (stableKeyStates[col] & (1 << row)) != 0;
}

bool Keyboard::check(uint8_t key, bool requireCtrl, bool requireShift,
                     bool requireAlt, bool requireFn)
{
    if (xTaskGetCurrentTaskHandle() != activeTask)
        return false;

    KBState kbstate = peekKBState();

    if (kbstate.pressedKey != key)
        return false;

    if (kbstate.ctrl() != requireCtrl ||
        kbstate.shift() != requireShift ||
        kbstate.alt() != requireAlt ||
        kbstate.fn() != requireFn)
        return false;

    return true;
}

bool Keyboard::take(uint8_t key, bool requireCtrl, bool requireShift,
                    bool requireAlt, bool requireFn)
{
    if (!check(key, requireCtrl, requireShift, requireAlt, requireFn))
        return false;

    consumeKBState();
    return true;
}
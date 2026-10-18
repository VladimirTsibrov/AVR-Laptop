#ifndef TETRIS_H
#define TETRIS_H

#include "../Task.h"
#include "../Common.h"
#include "../VS1053.h"
#include <string.h>

namespace nsTetris
{
    class Tetris : public Task
    {
        enum Screen : uint8_t
        {
            screenSplash,
            screenGame,
            screenGameOver
        };

        enum : uint16_t
        {
            BOARD_COLUMNS = 10,
            BOARD_ROWS = 20,
            CELL_WIDTH = 3,
            CELL_HEIGHT = 3,
            BOARD_X = 12,
            BOARD_Y = 2,
            BOARD_PIXEL_WIDTH = BOARD_COLUMNS * CELL_WIDTH,
            BOARD_PIXEL_HEIGHT = BOARD_ROWS * CELL_HEIGHT,
            INFO_PANEL_X = 64,
            INFO_PANEL_Y = 1,
            NEXT_X = INFO_PANEL_X,
            NEXT_Y = INFO_PANEL_Y + 10,
            SCORE_X = INFO_PANEL_X,
            SCORE_Y = INFO_PANEL_Y + 31,
            LINES_Y = INFO_PANEL_Y + 48,
            PRESS_SPACE_X = 36,
            PRESS_SPACE_Y = 61,
            PAUSE_X = BOARD_X + 1,
            PAUSE_Y = 28,
            GAME_OVER_X = 33,
            GAME_OVER_Y = 28,
            PIECE_COUNT = 7,
            START_FALL_MS = 900,
            MIN_FALL_MS = 100,
            LEVEL_LINES = 10,
            BLINK_MS = 1000,
            STACK_SIZE = 260
        };

        enum : uint16_t
        {
            FULL_ROW = 0x03FF
        };

        struct Piece
        {
            uint8_t type;
            uint8_t rotation;
        };

        inline static const char caption[] PROGMEM = "Tetris";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0,
            0x00, 0x0c, 0xf0, 0x00, 0x0c, 0xf0, 0x00, 0x0f, 0xf0, 0x00, 0x0f, 0xf0,
            0x00, 0x03, 0xf0, 0x00, 0x03, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0,
            0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0xc3, 0x30, 0xf0, 0xc3, 0x30, 0xf0,
            0xf3, 0xfc, 0xf3, 0xf3, 0xfc, 0xf3, 0xf3, 0xff, 0xff, 0xf3, 0xff, 0xff};

        inline static const uint8_t startBitmap[1024] PROGMEM = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0xC0,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
            0x00, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0xB0, 0x00, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xC0,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90,
            0x01, 0x00, 0x00, 0xC0, 0xF3, 0x3F, 0xFF, 0xFD, 0xCF, 0x1F, 0x18, 0xFC,
            0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0xC0, 0x03, 0x03, 0x03, 0xC0,
            0xC0, 0x30, 0x18, 0x86, 0x01, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0xC0,
            0x03, 0x03, 0x03, 0xC0, 0xC0, 0x60, 0x18, 0x03, 0x03, 0x00, 0x00, 0xC0,
            0x00, 0x00, 0x00, 0xC0, 0x03, 0x03, 0x03, 0xC0, 0xC0, 0x60, 0x18, 0x03,
            0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0xC0, 0x03, 0x03, 0x03, 0xC0,
            0xC0, 0x60, 0x18, 0x06, 0x00, 0x00, 0x00, 0xC0, 0x01, 0x00, 0x00, 0xC0,
            0x03, 0x03, 0xFF, 0xC0, 0xC0, 0x30, 0x18, 0xFC, 0x70, 0x00, 0x00, 0xB8,
            0x01, 0x00, 0x18, 0xC0, 0x03, 0x03, 0x03, 0xC0, 0xC0, 0x1F, 0x18, 0x80,
            0x61, 0x00, 0x60, 0xA8, 0x02, 0x00, 0x18, 0xC0, 0x03, 0x03, 0x03, 0xC0,
            0xC0, 0x30, 0x18, 0x00, 0x63, 0x00, 0x60, 0x9C, 0x07, 0x00, 0x18, 0xC0,
            0x03, 0x03, 0x03, 0xC0, 0xC0, 0x60, 0x18, 0x00, 0x73, 0x00, 0x60, 0xFE,
            0x0D, 0x00, 0x18, 0xC0, 0x03, 0x03, 0x03, 0xC0, 0xC0, 0x60, 0x18, 0x03,
            0x63, 0x00, 0x60, 0xFC, 0x07, 0x02, 0x18, 0xC0, 0x03, 0x03, 0x03, 0xC0,
            0xC0, 0x60, 0x18, 0x86, 0x61, 0x00, 0x60, 0x12, 0x05, 0x06, 0x26, 0xC0,
            0x03, 0x03, 0xFF, 0xC1, 0xC0, 0xC0, 0x18, 0xFC, 0x10, 0x00, 0xE0, 0x57,
            0x0D, 0x82, 0xBD, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x70, 0x60, 0x90, 0x8D, 0x1B, 0x86, 0xB6, 0xC3, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xD8, 0x40, 0x4C, 0x0F, 0x1A, 0xE2, 0xB9, 0xC7,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x41, 0x7B, 0xDD,
            0x6A, 0xB6, 0x99, 0xC5, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xBB, 0x63, 0xB6, 0x66, 0x3F, 0xB6, 0xCF, 0xCC, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x80, 0xB6, 0x4F, 0xA3, 0xE8, 0xBF, 0x40, 0xCC, 0xCE,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x96, 0xB3, 0x99, 0xD9,
            0x18, 0xF1, 0xD6, 0xCE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
            0x9B, 0xB3, 0x08, 0xD1, 0x1F, 0xF9, 0x31, 0xC6, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0xA0, 0xCD, 0x92, 0x0C, 0x93, 0xAF, 0x30, 0x33, 0xCB,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0xC5, 0x9E, 0x0D, 0x93,
            0xBF, 0xA2, 0x49, 0xCB, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0,
            0xC3, 0xBF, 0x09, 0xDB, 0x91, 0xF8, 0x8C, 0xC4, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x60, 0x63, 0x9F, 0x99, 0xD9, 0x49, 0xD2, 0xEC, 0xC3,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xF1, 0x65, 0x93, 0xEC,
            0x4D, 0x96, 0xFE, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0,
            0xD8, 0xE6, 0xF7, 0xFE, 0x04, 0x2A, 0x7E, 0xC0, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xEC, 0xB2, 0xFF, 0xDF, 0x12, 0x69, 0x00, 0xC0,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xF1, 0x09, 0xF9,
            0x92, 0xF8, 0x81, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xDE, 0xD9, 0x89, 0xF9, 0x8E, 0xE4, 0x67, 0xC0, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xC0, 0x1E, 0x13, 0x09, 0x48, 0xE4, 0x7F, 0xC0,
            0x03, 0x40, 0x40, 0x00, 0x18, 0xCE, 0x3F, 0x06, 0x81, 0xBF, 0x09, 0x89,
            0x49, 0xF6, 0x81, 0xC1, 0x03, 0x00, 0x00, 0x00, 0x10, 0x8C, 0x24, 0x05,
            0xE7, 0xE7, 0x09, 0xF9, 0x37, 0xFF, 0x7E, 0xC0, 0x63, 0x7E, 0x7C, 0x8F,
            0x11, 0x94, 0x14, 0x81, 0x3C, 0x83, 0x0D, 0x7B, 0xDE, 0xFD, 0xE6, 0xC1,
            0x93, 0x44, 0x4A, 0x12, 0x12, 0xA4, 0x1C, 0x82, 0x60, 0x86, 0xED, 0x3E,
            0x07, 0x4C, 0x00, 0xC2, 0x93, 0x44, 0x4E, 0x12, 0x13, 0xC4, 0x14, 0xC4,
            0xCC, 0x7D, 0x10, 0x60, 0x3E, 0xE6, 0xFF, 0xC3, 0x93, 0x44, 0x44, 0x92,
            0x12, 0xC4, 0x24, 0xC5, 0xC4, 0x79, 0x00, 0x61, 0xB6, 0x6D, 0xC6, 0xC0,
            0x63, 0xEE, 0xE2, 0xB7, 0x3F, 0x8E, 0x3E, 0xC3, 0xE4, 0xFC, 0x08, 0xB0,
            0x17, 0xCF, 0x80, 0xC1, 0x03, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x40,
            0xB4, 0x7C, 0x00, 0xE0, 0x19, 0xDE, 0xE0, 0xC3, 0x03, 0x00, 0x12, 0x00,
            0x00, 0x00, 0x00, 0xC0, 0x75, 0x7C, 0x11, 0x69, 0x79, 0x9C, 0xE0, 0xC2,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3C, 0xEF, 0xFF, 0xFF,
            0x83, 0xBC, 0xE2, 0xC7, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0xC0,
            0x13, 0xCF, 0xAE, 0xF3, 0x9E, 0xEF, 0xFF, 0xC7, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xE0, 0xE3, 0x00, 0xF0, 0x38, 0xC7, 0xDC, 0xCB,
            0x03, 0x00, 0xDE, 0x6C, 0x3E, 0x06, 0x00, 0xA0, 0xE1, 0x7F, 0x04, 0xF0,
            0x10, 0x44, 0x78, 0xCE, 0x03, 0x00, 0x24, 0x49, 0x21, 0x05, 0x00, 0x00,
            0xFE, 0x2F, 0x00, 0x60, 0xFF, 0xFF, 0xE8, 0xDF, 0x03, 0x00, 0x24, 0x49,
            0x26, 0x01, 0x00, 0xC8, 0x53, 0xEE, 0xFF, 0x6F, 0x1C, 0xEE, 0xF7, 0xD7,
            0x03, 0x00, 0x24, 0x49, 0x28, 0x01, 0x00, 0xC0, 0x50, 0x7C, 0x70, 0x6C,
            0x08, 0x48, 0x30, 0xCC, 0x03, 0x00, 0x6E, 0xF3, 0x77, 0x06, 0x00, 0xF0,
            0xCF, 0x39, 0x67, 0x13, 0x08, 0x18, 0x03, 0xE0, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0xE0, 0xFC, 0x0F, 0x8C, 0xE3, 0xDF, 0xCE, 0x03, 0xC6,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7C, 0x46, 0x12, 0x86,
            0xF7, 0x1A, 0xF3, 0xE6, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0,
            0x3F, 0xF0, 0xFF, 0x3D, 0x09, 0x52, 0xF1, 0xEF, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0xC0, 0xB4, 0x3B, 0x04, 0x04, 0x5D, 0x47, 0xF1, 0xEF,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x10, 0x11, 0x00, 0x06,
            0x55, 0x55, 0xF7, 0xCF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60,
            0x14, 0x31, 0x26, 0x00, 0x5D, 0x15, 0x7F, 0xE7, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0xC0, 0x14, 0x19, 0x62, 0x06, 0x57, 0x87, 0x9F, 0xE4,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x1C, 0x31, 0xF6, 0xC6,
            0x51, 0x58, 0x8E, 0xC8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90,
            0xEB, 0xF7, 0x9F, 0xFF, 0xFF, 0x3F, 0x60, 0xDB, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF};

        // Each rotation is a row-major 4x4 bitmap.
        inline static const uint16_t SHAPES[PIECE_COUNT][4] PROGMEM = {
            {0x0F00, 0x2222, 0x0F00, 0x2222}, // I
            {0x0660, 0x0660, 0x0660, 0x0660}, // O
            {0x0270, 0x0262, 0x0072, 0x0232}, // T
            {0x0360, 0x0462, 0x0360, 0x0462}, // S
            {0x0630, 0x0264, 0x0630, 0x0264}, // Z
            {0x0470, 0x0226, 0x0071, 0x0322}, // J
            {0x0170, 0x0223, 0x0074, 0x0622}  // L
        };

        Screen screen = screenSplash;
        uint16_t board[BOARD_ROWS] = {};
        Piece current = {};
        Piece next = {};
        int8_t pieceX = BOARD_COLUMNS / 2 - 2;
        int8_t pieceY = 0;
        uint32_t score = 0;
        uint16_t lines = 0;
        uint16_t fallInterval = START_FALL_MS;
        uint32_t lastFallAt = 0;
        uint32_t nextBlinkAt = 0;
        bool promptVisible = false;
        bool paused = false;

        FsFile musicDirectory;
        FsFile musicFile;
        char musicName[32];
        bool audioAcquired = false;

        static uint16_t shapeAt(const Piece &piece)
        {
            return pgm_read_word(&SHAPES[piece.type][piece.rotation & 3]);
        }

        void drawCell(uint8_t x, uint8_t y)
        {
            const uint8_t left = BOARD_X + x * CELL_WIDTH;
            const uint8_t top = BOARD_Y + y * CELL_HEIGHT;
            lcd.drawRect(left, top, left + CELL_WIDTH - 1, top + CELL_HEIGHT - 1, modeSet, true);
        }

        void renderSplash()
        {
            lcd.cls();
            lcd.drawXBM(startBitmap, 128, 64);
            if (promptVisible)
            {
                lcd.setPrintPos(PRESS_SPACE_X, PRESS_SPACE_Y);
                lcd.print(F("press space"), modeSet);
            }
            lcd.repaint();
        }

        void enterSplash()
        {
            screen = screenSplash;
            promptVisible = false;
            nextBlinkAt = millis() + BLINK_MS;
            renderSplash();
        }

        void updateSplash()
        {
            const uint32_t now = millis();
            if ((int32_t)(now - nextBlinkAt) < 0)
                return;
            promptVisible = !promptVisible;
            nextBlinkAt = now + BLINK_MS;
            renderSplash();
        }

        bool canPlace(int8_t x, int8_t y, uint8_t rotation) const
        {
            const uint16_t shape = pgm_read_word(&SHAPES[current.type][rotation & 3]);
            for (uint8_t row = 0; row < 4; ++row)
            {
                for (uint8_t col = 0; col < 4; ++col)
                {
                    if (!(shape & (0x8000 >> (row * 4 + col))))
                        continue;
                    const int8_t bx = x + col;
                    const int8_t by = y + row;
                    if (bx < 0 || bx >= BOARD_COLUMNS || by >= BOARD_ROWS)
                        return false;
                    if (by >= 0 && (board[by] & ((uint16_t)1 << bx)))
                        return false;
                }
            }
            return true;
        }

        void renderGame()
        {
            lcd.cls();
            lcd.drawRect(BOARD_X - 1, BOARD_Y - 1,
                         BOARD_X + BOARD_PIXEL_WIDTH, BOARD_Y + BOARD_PIXEL_HEIGHT, modeSet, false);
            for (uint8_t row = 0; row < BOARD_ROWS; ++row)
                for (uint8_t col = 0; col < BOARD_COLUMNS; ++col)
                    if (board[row] & ((uint16_t)1 << col))
                        drawCell(col, row);

            const uint16_t fallingShape = shapeAt(current);
            for (uint8_t row = 0; row < 4; ++row)
                for (uint8_t col = 0; col < 4; ++col)
                    if ((fallingShape & (0x8000 >> (row * 4 + col))) && pieceY + row < BOARD_ROWS)
                        drawCell(pieceX + col, pieceY + row);

            lcd.setPrintPos(NEXT_X, NEXT_Y);
            lcd.print(F("NEXT"), modeSet);
            const uint16_t preview = pgm_read_word(&SHAPES[next.type][0]);
            for (uint8_t row = 0; row < 4; ++row)
                for (uint8_t col = 0; col < 4; ++col)
                    if (preview & (0x8000 >> (row * 4 + col)))
                    {
                        const uint8_t x = NEXT_X + col * 3;
                        const uint8_t y = NEXT_Y + row * 3;
                        lcd.drawRect(x, y, x + 2, y + 2, modeSet, true);
                    }

            lcd.setPrintPos(SCORE_X, SCORE_Y);
            lcd.print(F("SCORE"), modeSet);
            lcd.setPrintPos(SCORE_X, SCORE_Y + 8);
            lcd.print((unsigned long)score, modeSet);
            lcd.setPrintPos(INFO_PANEL_X, LINES_Y);
            lcd.print(F("LINES"), modeSet);
            lcd.setPrintPos(INFO_PANEL_X, LINES_Y + 8);
            lcd.print((unsigned int)lines, modeSet);

            if (paused)
            {
                //lcd.drawRect(PAUSE_X - 2, PAUSE_Y - 1, PAUSE_X + 31, PAUSE_Y + 7, modeInvert, true);
                lcd.setPrintPos(PAUSE_X, PAUSE_Y + 7);
                lcd.print(F("PAUSE"), modeSet);
            }
            lcd.repaint();
        }

        void renderGameOver()
        {
            lcd.cls();
            lcd.setPrintPos(GAME_OVER_X, GAME_OVER_Y);
            lcd.print(F("GAME OVER"), modeSet);
            lcd.setPrintPos(GAME_OVER_X + 3, GAME_OVER_Y + 14);
            lcd.print(F("SCORE:"), modeSet);
            lcd.setPrintPos(GAME_OVER_X + 42, GAME_OVER_Y + 14);
            lcd.print((unsigned long)score, modeSet);
            lcd.repaint();
        }

        static Piece randomPiece()
        {
            Piece piece = {static_cast<uint8_t>(random(PIECE_COUNT)), 0};
            return piece;
        }

        void spawnNext()
        {
            current = next;
            pieceX = BOARD_COLUMNS / 2 - 2;
            pieceY = 0;
            next = randomPiece();
        }

        void beginGame()
        {
            memset(board, 0, sizeof(board));
            score = 0;
            lines = 0;
            fallInterval = START_FALL_MS;
            paused = false;
            next = randomPiece();
            spawnNext();
            lastFallAt = millis();
            if (!canPlace(pieceX, pieceY, current.rotation))
            {
                screen = screenGameOver;
                renderGameOver();
                return;
            }
            screen = screenGame;
            renderGame();
        }

        void movePiece(int8_t dx, int8_t dy)
        {
            const int8_t x = pieceX + dx;
            const int8_t y = pieceY + dy;
            if (canPlace(x, y, current.rotation))
            {
                pieceX = x;
                pieceY = y;
            }
        }

        void rotatePiece()
        {
            const uint8_t rotation = (current.rotation + 1) & 3;
            if (canPlace(pieceX, pieceY, rotation))
                current.rotation = rotation;
        }

        void settlePiece()
        {
            const uint16_t shape = shapeAt(current);
            for (uint8_t row = 0; row < 4; ++row)
                for (uint8_t col = 0; col < 4; ++col)
                    if (shape & (0x8000 >> (row * 4 + col)))
                    {
                        const int8_t y = pieceY + row;
                        if (y >= 0 && y < BOARD_ROWS)
                            board[y] |= (uint16_t)1 << (pieceX + col);
                    }

            uint8_t cleared = 0;
            int8_t row = BOARD_ROWS - 1;
            while (row >= 0)
            {
                if (board[row] == FULL_ROW)
                {
                    ++cleared;
                    for (int8_t above = row; above > 0; --above)
                        board[above] = board[above - 1];
                    board[0] = 0;
                }
                else
                    --row;
            }

            static const uint16_t points[] = {0, 40, 100, 300, 1200};
            if (cleared > 4)
                cleared = 4;
            score += (uint32_t)points[cleared] * (1 + lines / LEVEL_LINES);
            lines += cleared;
            const uint16_t fasterBy = (lines / LEVEL_LINES) * 80;
            fallInterval = fasterBy >= START_FALL_MS - MIN_FALL_MS
                               ? MIN_FALL_MS
                               : START_FALL_MS - fasterBy;

            spawnNext();
            lastFallAt = millis();
            if (!canPlace(pieceX, pieceY, current.rotation))
            {
                screen = screenGameOver;
                renderGameOver();
            }
            else
                renderGame();
        }

        static bool isMp3(const char *name)
        {
            const size_t length = strlen(name);
            return length >= 5 && name[length - 4] == '.' &&
                   (name[length - 3] | 0x20) == 'm' &&
                   (name[length - 2] | 0x20) == 'p' &&
                   (name[length - 1] | 0x20) == '3';
        }

        void closeMusic()
        {
            if (musicFile)
                musicFile.close();
            if (musicDirectory)
                musicDirectory.close();
            if (audioAcquired)
            {
                vs1053.clearBuffer();
                vs1053.release();
                audioAcquired = false;
            }
        }

        bool openNextMusic()
        {
            SchedulerSuspender lock;
            if (musicFile)
                musicFile.close();
            for (uint8_t pass = 0; pass < 2; ++pass)
            {
                while (musicFile.openNext(&musicDirectory, O_RDONLY))
                {
                    musicFile.getName(musicName, sizeof(musicName));
                    musicName[sizeof(musicName) - 1] = '\0';
                    if (!musicFile.isDirectory() && isMp3(musicName))
                        return true;
                    musicFile.close();
                }
                musicDirectory.rewindDirectory();
            }
            return false;
        }

        void startMusic()
        {
            if (!vs1053.acquire())
                return;
            audioAcquired = true;
            if (!musicDirectory.open("/games/tetris/music", O_RDONLY) || !openNextMusic())
                closeMusic();
        }

        void feedMusic()
        {
            if (!audioAcquired || !musicFile)
                return;
            if (!musicFile.available())
            {
                if (!openNextMusic())
                    closeMusic();
                return;
            }
            vs1053.feedFile(musicFile);
        }

    public:
        static const __FlashStringHelper *getCaption()
        {
            return reinterpret_cast<const __FlashStringHelper *>(caption);
        }
        static const uint8_t *getIcon() { return icon; }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }
        uint16_t getStackSize() const override { return STACK_SIZE; }
        static bool canOpen(const char *) { return false; }

        ~Tetris() override { closeMusic(); }

        void fullRepaint() override
        {
            if (screen == screenSplash)
                renderSplash();
            else if (screen == screenGame)
                renderGame();
            else
                renderGameOver();
        }

    protected:
        void setup() override
        {
            randomSeed(analogRead(A0) ^ micros());
            startMusic();
            enterSplash();
        }

        void onClosing() override { closeMusic(); }

        void loop() override
        {
            if (screen == screenSplash)
            {
                updateSplash();
                if (keyboard.take(keySpace))
                    beginGame();
                else if (keyboard.take(keyEsc))
                    requestClose();
            }
            else if (screen == screenGame)
            {
                if (keyboard.take(keyEsc))
                    enterSplash();
                else if (keyboard.take(keySpace))
                {
                    paused = !paused;
                    if (!paused)
                        lastFallAt = millis();
                    renderGame();
                }
                else if (!paused)
                {
                    bool changed = false;
                    if (keyboard.take(keyLeft))
                    {
                        movePiece(-1, 0);
                        changed = true;
                    }
                    else if (keyboard.take(keyRight))
                    {
                        movePiece(1, 0);
                        changed = true;
                    }
                    else if (keyboard.take(keyDown))
                    {
                        movePiece(0, 1);
                        changed = true;
                    }
                    else if (keyboard.take(keyUp))
                    {
                        rotatePiece();
                        changed = true;
                    }
                    else if (keyboard.take(keyEnter))
                    {
                        while (canPlace(pieceX, pieceY + 1, current.rotation))
                            ++pieceY;
                        settlePiece();
                    }

                    if (changed)
                        renderGame();

                    const uint32_t now = millis();
                    if (screen == screenGame && (uint32_t)(now - lastFallAt) >= fallInterval)
                    {
                        lastFallAt = now;
                        if (canPlace(pieceX, pieceY + 1, current.rotation))
                        {
                            ++pieceY;
                            renderGame();
                        }
                        else
                            settlePiece();
                    }
                }
            }
            else if (keyboard.peekKBState().pressedKey != keyNone)
            {
                keyboard.consumeKBState();
                enterSplash();
            }

            feedMusic();
            taskYIELD();
        }
    };
}

#endif // TETRIS_H

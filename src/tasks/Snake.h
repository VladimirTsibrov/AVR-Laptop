#ifndef SNAKE_H
#define SNAKE_H

#include "../Task.h"

namespace nsSnake
{
    class Snake : public Task
    {
    private:
        static const uint8_t cellSize = 3;
        static const uint8_t borderSize = 1;
        static const uint8_t fieldWidth = (Display::width() - 2 * borderSize) / cellSize;
        static const uint8_t fieldHeight = (Display::height() - 2 * borderSize) / cellSize;
        static const uint8_t maxLength = fieldWidth * fieldHeight;
        static const uint16_t movePeriod = 180;

        enum Screen : uint8_t
        {
            startScreen,
            gameScreen,
            completeScreen
        };

        enum Direction : uint8_t
        {
            up,
            down,
            left,
            right
        };

        inline static const char caption[] PROGMEM = "Snake";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0xF0, 0xF0, 0x00, 0xF8, 0xFB, 0x00, 0x1C, 0xF7, 0x00, 0x0C, 0xF0,
            0x00, 0x0C, 0xF0, 0x00, 0x18, 0xF0, 0x00, 0x30, 0xF0, 0x00, 0x60, 0xF0,
            0x00, 0xE0, 0xF0, 0x00, 0xC0, 0xF0, 0x00, 0xC7, 0xF1, 0x80, 0x8F, 0xF1,
            0xC0, 0xDD, 0xF1, 0xE0, 0xF8, 0xF0, 0xA3, 0x70, 0xF0, 0x64, 0x00, 0xF0,
            0x0E, 0x00, 0xF0, 0x11, 0x00, 0xF0, 0x11, 0x00, 0xF0, 0x0E, 0x00, 0xF0};

        Screen screen = startScreen;
        int8_t snakeX[maxLength];
        int8_t snakeY[maxLength];
        uint8_t snakeLength = 0;
        int8_t foodX = 0;
        int8_t foodY = 0;
        Direction direction = right;
        Direction nextDirection = right;
        uint32_t nextMove = 0;
        bool startPromptVisible = false;
        uint32_t nextStartPromptToggle = 0;

        void renderStart(bool showPrompt)
        {
            lcd.cls();
            lcd.drawRect(0, 0, Display::width() - 1, Display::height() - 1, modeSet, false);
            lcd.setPrintPos(45, 17);
            lcd.print(F("SNAKE"));
            lcd.setPrintPos(24, 31);
            lcd.print(F("ARROWS TO MOVE"));
            lcd.setPrintPos(31, 43);
            lcd.print(F("ESC: EXIT"));
            if (showPrompt)
            {
                lcd.setPrintPos(32, 61);
                lcd.print(F("press space"));
            }
        }

        void resetStartSplash()
        {
            screen = startScreen;
            startPromptVisible = false;
            nextStartPromptToggle = millis() + 1000UL;
            renderStart(false);
        }

        void updateStartSplash()
        {
            if ((int32_t)(millis() - nextStartPromptToggle) < 0)
                return;
            startPromptVisible = !startPromptVisible;
            nextStartPromptToggle = millis() + 1000UL;
            renderStart(startPromptVisible);
        }

        void drawCell(uint8_t x, uint8_t y, bool filled)
        {
            const uint8_t pixelX = borderSize + x * cellSize;
            const uint8_t pixelY = borderSize + y * cellSize;
            lcd.drawRect(pixelX, pixelY, pixelX + cellSize - 1, pixelY + cellSize - 1,
                         filled ? modeSet : modeClear, true);
        }

        void drawFood()
        {
            const uint8_t pixelX = borderSize + foodX * cellSize;
            const uint8_t pixelY = borderSize + foodY * cellSize;
            lcd.drawRect(pixelX, pixelY, pixelX + cellSize - 1, pixelY + cellSize - 1,
                         modeSet, false);
        }

        void renderGame()
        {
            lcd.cls();
            lcd.drawRect(0, 0, Display::width() - 1, Display::height() - 1, modeSet, false);
            for (uint8_t i = 0; i < snakeLength; i++)
                drawCell(snakeX[i], snakeY[i], true);
            drawFood();
        }

        void renderComplete()
        {
            lcd.cls();
            lcd.setPrintPos(33, 22);
            lcd.print(F("GAME OVER"));
            lcd.setPrintPos(22, 40);
            lcd.print(F("PRESS SPACE"));
        }

        bool hasSnakeAt(int8_t x, int8_t y, uint8_t length) const
        {
            for (uint8_t i = 0; i < length; i++)
                if (snakeX[i] == x && snakeY[i] == y)
                    return true;
            return false;
        }

        bool placeFood()
        {
            if (snakeLength >= maxLength)
                return false;
            for (uint16_t attempt = 0; attempt < maxLength * 2; attempt++)
            {
                int8_t x = random(fieldWidth);
                int8_t y = random(fieldHeight);
                if (!hasSnakeAt(x, y, snakeLength))
                {
                    foodX = x;
                    foodY = y;
                    return true;
                }
            }
            return false;
        }

        void startGame()
        {
            snakeLength = 3;
            snakeX[0] = fieldWidth / 2;
            snakeY[0] = fieldHeight / 2;
            snakeX[1] = snakeX[0] - 1;
            snakeY[1] = snakeY[0];
            snakeX[2] = snakeX[1] - 1;
            snakeY[2] = snakeY[0];
            direction = right;
            nextDirection = right;
            if (!placeFood())
            {
                screen = completeScreen;
                renderComplete();
                return;
            }
            screen = gameScreen;
            nextMove = millis() + movePeriod;
            renderGame();
        }

        bool isOpposite(Direction candidate) const
        {
            return (direction == up && candidate == down) ||
                   (direction == down && candidate == up) ||
                   (direction == left && candidate == right) ||
                   (direction == right && candidate == left);
        }

        void moveSnake()
        {
            if (!isOpposite(nextDirection))
                direction = nextDirection;

            int8_t x = snakeX[0];
            int8_t y = snakeY[0];
            if (direction == up)
                y--;
            else if (direction == down)
                y++;
            else if (direction == left)
                x--;
            else
                x++;

            const bool ateFood = x == foodX && y == foodY;
            const uint8_t collisionLength = ateFood ? snakeLength : snakeLength - 1;
            if (x < 0 || x >= fieldWidth || y < 0 || y >= fieldHeight ||
                hasSnakeAt(x, y, collisionLength))
            {
                screen = completeScreen;
                renderComplete();
                return;
            }

            const uint8_t newLength = ateFood ? snakeLength + 1 : snakeLength;
            for (uint8_t i = newLength - 1; i > 0; i--)
            {
                snakeX[i] = snakeX[i - 1];
                snakeY[i] = snakeY[i - 1];
            }
            snakeX[0] = x;
            snakeY[0] = y;
            snakeLength = newLength;
            if (ateFood && !placeFood())
            {
                screen = completeScreen;
                renderComplete();
                return;
            }
            renderGame();
        }

    public:
        static const __FlashStringHelper *getCaption() { return reinterpret_cast<const __FlashStringHelper *>(caption); }
        static const uint8_t *getIcon() { return icon; }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }
        uint16_t getStackSize() const override { return 384; }

        void fullRepaint() override
        {
            if (screen == startScreen)
                renderStart(startPromptVisible);
            else if (screen == gameScreen)
                renderGame();
            else
                renderComplete();
        }

    protected:
        void setup() override
        {
            randomSeed(analogRead(A0) ^ micros());
            resetStartSplash();
        }

        void loop() override
        {
            if (keyboard.take(keyEsc))
            {
                if (screen == startScreen)
                    requestClose();
                else
                    resetStartSplash();
                taskYIELD();
                return;
            }

            if (screen == startScreen)
            {
                updateStartSplash();
                if (keyboard.take(keySpace) || keyboard.take(keyEnter))
                    startGame();
            }
            else if (screen == completeScreen)
            {
                if (keyboard.take(keySpace) || keyboard.take(keyEnter))
                    resetStartSplash();
            }
            else
            {
                if (keyboard.take(keyUp))
                    nextDirection = up;
                else if (keyboard.take(keyDown))
                    nextDirection = down;
                else if (keyboard.take(keyLeft))
                    nextDirection = left;
                else if (keyboard.take(keyRight))
                    nextDirection = right;

                if ((int32_t)(millis() - nextMove) >= 0)
                {
                    moveSnake();
                    nextMove += movePeriod;
                }
            }
            taskYIELD();
        }
    };
}

#endif // SNAKE_H

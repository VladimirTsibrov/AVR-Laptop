#ifndef CALC_H
#define CALC_H

#include "../Task.h"
#include "../ui/Form.h"
#include "../ui/elements/Button.h"
#include "../ui/elements/IconButton.h"
#include <stdlib.h>
#include <string.h>

namespace nsCalc
{
    class Calc;

    inline static const uint8_t signIcon7x7[] PROGMEM =
        {0x04, 0x0E, 0x24, 0x10, 0x08, 0x04, 0x72};

    // Отдельный шрифт индикатора калькулятора: '-', '.', пробел и цифры 0-9.
    // clang-format off
    inline static const uint8_t displayFont8x12[] PROGMEM = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x38, 0x60, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x7C, 0xC6, 0xCE, 0xDE, 0xD6, 0xF6, 0xE6, 0xC6, 0x7C, 0x00, 0x00,
        0x00, 0x10, 0x30, 0xF0, 0x30, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00, 0x00,
        0x00, 0x78, 0xCC, 0xCC, 0x0C, 0x18, 0x30, 0x60, 0xCC, 0xFC, 0x00, 0x00,
        0x00, 0x78, 0xCC, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0xCC, 0x78, 0x00, 0x00,
        0x00, 0x0C, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x1E, 0x00, 0x00,
        0x00, 0xFC, 0xC0, 0xC0, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00, 0x00,
        0x00, 0x38, 0x60, 0xC0, 0xC0, 0xF8, 0xCC, 0xCC, 0xCC, 0x78, 0x00, 0x00,
        0x00, 0xFE, 0xC6, 0xC6, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00, 0x00,
        0x00, 0x78, 0xCC, 0xCC, 0xCC, 0x78, 0xCC, 0xCC, 0xCC, 0x78, 0x00, 0x00,
        0x00, 0x78, 0xCC, 0xCC, 0xCC, 0x7C, 0x18, 0x18, 0x30, 0x70, 0x00, 0x00,
    };
    // clang-format on
    class CalcForm : public Form
    {
    public:
        CalcForm(Calc &owner) : owner(owner) {}
        void draw() override;
        bool onKey(const KBState &kb) override;

    private:
        Calc &owner;
    };

    class Calc : public Task
    {
    private:
        enum State : uint8_t
        {
            inputA,
            inputB,
            gotB,
            showResult
        };

        inline static const char caption[] PROGMEM = "Calc";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0xFE, 0xFF, 0xF7, 0x02, 0x00, 0xF4,
            0x02, 0x00, 0xF4, 0x02, 0x00, 0xF4, 0xFE, 0xFF, 0xF7, 0x02, 0x00, 0xF4,
            0x02, 0xB6, 0xF5, 0x02, 0xB6, 0xF5, 0x02, 0x00, 0xF4, 0x02, 0xB6, 0xF5,
            0x02, 0xB6, 0xF5, 0x02, 0x00, 0xF4, 0x02, 0xB6, 0xF5, 0x02, 0xB6, 0xF5,
            0x02, 0x00, 0xF4, 0xFE, 0xFF, 0xF7, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0};

        // clang-format off
        inline static const char buttonValues[20] PROGMEM = {
            '1', '2', '3', '+', '\b', 
            '4', '5', '6', '-', 'C', 
            '7', '8', '9', '*', '=', 
            'i', '0', '.', '/', '='};
            
        inline static const char buttonCaptions[] PROGMEM =
            "1\0" "2\0" "3\0" "+\0" "<\0"
            "4\0" "5\0" "6\0" "-\0" "C\0"
            "7\0" "8\0" "9\0" "*\0" "=\0"
            " \0" "0\0" ".\0" "/\0" "=\0";
        // clang-format on

        CalcForm *form = nullptr;
        UIElement *buttons[20] = {};
        char value[16] = "0";
        double a = 0;
        double b = 0;
        char operation = 0;
        State state = inputA;
        bool hasDot = false;
        bool error = false;
        uint8_t displayFontIndex = 0;

        void reset(bool full = true);
        void refreshDisplay()
        {
            if (form)
                form->draw();
        }
        void calculate();
        void setOperation(char op);
        void input(char ch);
        void moveFocus(int8_t dx, int8_t dy);

        void onEvent(UIElement *sender, uint8_t event, uint8_t value) override;
        friend class CalcForm;

    public:
        static const __FlashStringHelper *getCaption() { return reinterpret_cast<const __FlashStringHelper*>(caption); }
        static const uint8_t *getIcon() { return icon; }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }
        uint16_t getStackSize() const override { return 320; }

    protected:
        void setup() override;
        void loop() override;
    };

    inline void CalcForm::draw()
    {
        Form::draw();
        lcd.drawRect(2, 1, 125, 14, modeClear, true);
        lcd.drawRect(2, 1, 125, 14, modeSet, false);
        if (owner.error)
        {
            setPrintRect(4, 2, 123, 13);
            print("Error");
            return;
        }

        uint8_t length = strlen(owner.value);
        uint8_t width = length ? (uint8_t)(length * 9 - 1) : 0;
        uint8_t x = (width < 120) ? (uint8_t)(123 - width) : 3;
        setPrintRect(3, 2, 123, 13);
        setPrintPos(x, 13);
        lcd.print(owner.value, length, owner.displayFontIndex);
    }

    inline bool CalcForm::onKey(const KBState &kb)
    {
        if (kb.pressedKey == keyLeft)
        {
            owner.moveFocus(-1, 0);
            return true;
        }
        if (kb.pressedKey == keyRight)
        {
            owner.moveFocus(1, 0);
            return true;
        }
        if (kb.pressedKey == keyUp)
        {
            owner.moveFocus(0, -1);
            return true;
        }
        if (kb.pressedKey == keyDown)
        {
            owner.moveFocus(0, 1);
            return true;
        }
        if (kb.pressedKey == keyEsc)
        {
            owner.requestClose();
            return true;
        }
        if (kb.pressedKey == keyBackspace)
        {
            owner.input('\b');
            return true;
        }
        if (kb.pressedKey == keyEnter || kb.pressedKey == keySpace)
            return Form::onKey(kb);
        if (kb.pressedChar != 0)
        {
            owner.input(kb.pressedChar);
            return true;
        }
        return Form::onKey(kb);
    }

    inline void Calc::reset(bool full)
    {
        strcpy(value, "0");
        hasDot = false;
        error = false;
        if (full)
        {
            a = 0;
            b = 0;
            operation = 0;
            state = inputA;
        }
    }

    inline void Calc::calculate()
    {
        if (operation == '+')
            a += b;
        else if (operation == '-')
            a -= b;
        else if (operation == '*')
            a *= b;
        else if (operation == '/')
        {
            if (b == 0)
            {
                error = true;
                return;
            }
            a /= b;
        }

        char formatted[24];
        char *start = nullptr;
        for (int8_t precision = 6; precision >= 0; precision--)
        {
            dtostrf(a, 0, precision, formatted);
            start = formatted;
            while (*start == ' ')
                start++;
            if (strlen(start) < sizeof(value))
                break;
        }
        if (!start || strlen(start) >= sizeof(value))
        {
            error = true;
            return;
        }
        strcpy(value, start);
        char *dot = strchr(value, '.');
        if (dot)
        {
            char *end = value + strlen(value) - 1;
            while (end > dot && *end == '0')
                *end-- = '\0';
            if (*end == '.')
                *end = '\0';
        }
        hasDot = strchr(value, '.') != nullptr;
    }

    inline void Calc::setOperation(char op)
    {
        if (error)
            return;
        if (state == inputA || state == showResult)
            a = strtod(value, nullptr);
        else if (state == gotB)
        {
            b = strtod(value, nullptr);
            calculate();
            if (error)
            {
                refreshDisplay();
                return;
            }
        }
        operation = op;
        state = inputB;
        reset(false);
        refreshDisplay();
    }

    inline void Calc::input(char ch)
    {
        if (ch == 'c' || ch == 'C')
        {
            reset();
            refreshDisplay();
            return;
        }
        if (ch == 'i')
        {
            if (value[0] == '-')
                memmove(value, value + 1, strlen(value));
            else if (strlen(value) < sizeof(value) - 1 && strcmp(value, "0") != 0)
            {
                memmove(value + 1, value, strlen(value) + 1);
                value[0] = '-';
            }
            refreshDisplay();
            return;
        }
        if (ch == '+' || ch == '-' || ch == '*' || ch == '/')
        {
            setOperation(ch);
            return;
        }
        if (ch == '=')
        {
            if (state == gotB || state == showResult)
            {
                if (state == gotB)
                    b = strtod(value, nullptr);
                calculate();
                state = showResult;
                refreshDisplay();
            }
            return;
        }
        if (ch == '\b')
        {
            uint8_t length = strlen(value);
            if (length > 1)
                value[length - 1] = '\0';
            else
                strcpy(value, "0");
            hasDot = strchr(value, '.') != nullptr;
            refreshDisplay();
            return;
        }
        if (ch == ',' || ch == '.')
        {
            if (!hasDot && strlen(value) < sizeof(value) - 1)
            {
                strcat(value, ".");
                hasDot = true;
            }
        }
        else if (ch >= '0' && ch <= '9')
        {
            if (state == showResult)
            {
                reset(false);
                state = inputA;
            }
            if (state == inputB)
            {
                reset(false);
                state = gotB;
            }
            uint8_t length = strlen(value);
            if (length < sizeof(value) - 1)
            {
                if (strcmp(value, "0") == 0)
                {
                    value[0] = ch;
                    value[1] = '\0';
                }
                else
                {
                    value[length] = ch;
                    value[length + 1] = '\0';
                }
            }
        }
        else
            return;
        refreshDisplay();
    }

    inline void Calc::moveFocus(int8_t dx, int8_t dy)
    {
        UIElement *focused = form->getFocused();
        uint8_t index = 0;
        for (; index < 20 && buttons[index] != focused; index++)
            ;
        if (index == 20)
            return;
        int8_t row = index / 5;
        int8_t col = index % 5;
        int8_t newRow = row + dy;
        int8_t newCol = col + dx;
        if (newCol < 0 || newCol > 4 || newRow < 0 || newRow > 3)
            return;
        if (col == 4 && row == 2 && dy > 0)
            newRow = 2;
        if (newCol == 4 && newRow == 3)
            newRow = 2;
        form->setFocusedElement(buttons[newRow * 5 + newCol]);
    }

    inline void Calc::onEvent(UIElement *sender, uint8_t event, uint8_t value)
    {
        (void)value;
        if (event != EVENT_CLICK)
            return;
        for (uint8_t i = 0; i < 20; i++)
            if (buttons[i] == sender)
            {
                input(pgm_read_byte(buttonValues + i));
                return;
            }
    }

    inline void Calc::setup()
    {
        displayFontIndex = lcd.addFont("calc8x12", 8, 12, displayFont8x12, '-');
        form = new CalcForm(*this);
        if (!form)
        {
            requestClose();
            return;
        }
        form->setDrawBorder(false);
        form->setShowCaption(false);

        for (uint8_t row = 0; row < 4; row++)
        {
            for (uint8_t col = 0; col < 5; col++)
            {
                uint8_t index = row * 5 + col;
                uint8_t height = (col == 4 && row == 2) ? 22 : 10;
                if (col == 4 && row == 3)
                {
                    buttons[index] = buttons[14];
                    continue;
                }
                if (index == 15)
                    buttons[index] = form->addIconButton(68, 52, 9, 10, signIcon7x7, 7, 7);
                else
                {
                    const __FlashStringHelper *label = reinterpret_cast<const __FlashStringHelper *>(buttonCaptions + index * 2);
                    buttons[index] = form->addButton(68 + col * 12, 16 + row * 12, 9, height, label);
                }
                if (buttons[index])
                    buttons[index]->setEventMask(EVENT_CLICK);
            }
        }
        reset();
        ui.showForm(form);
    }

    inline void Calc::loop()
    {
        taskYIELD();
    }
}

#endif

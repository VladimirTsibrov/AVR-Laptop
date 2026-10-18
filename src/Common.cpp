#include "Common.h"
#include "Display.h"
#include <string.h>

TaskHandle_t activeTask = NULL;
uint8_t activeTaskIndex = 255;
TaskHandle_t shellTask = NULL;

uint16_t getTextWidth(const char *text, uint8_t maxChars)
{
    return getTextWidth(TextRef(text), maxChars);
}

uint16_t getTextWidth(TextRef text, uint8_t maxChars)
{
    if (text.isNull())
        return 0;

    const uint8_t fontIndex = lcd.getSystemFontIndex();
    uint8_t charWidth = lcd.fonts[fontIndex].width;
    uint8_t count = 0;
    
    for (uint8_t i = 0; text.charAt(i) && i < maxChars; i++)
        count++;

    if (count == 0)
        return 0;

    // Ширина = количество символов * ширина + промежутки между символами
    return (uint16_t)count * charWidth + (count > 1 ? count - 1 : 0);
}

uint8_t getVisibleChars(const char *text, uint16_t maxWidth)
{
    return getVisibleChars(TextRef(text), maxWidth);
}

uint8_t getVisibleChars(TextRef text, uint16_t maxWidth)
{
    if (text.isNull() || maxWidth == 0)
        return 0;

    const uint8_t fontIndex = lcd.getSystemFontIndex();
    uint8_t charWidth = lcd.fonts[fontIndex].width;
    uint8_t visibleChars = 0;
    uint16_t currentWidth = 0;

    for (uint8_t i = 0; text.charAt(i); i++)
    {
        // Добавляем символ и промежуток
        uint16_t symbolWidth = charWidth + (i > 0 ? 1 : 0);
        if (currentWidth + symbolWidth > maxWidth)
            break;
        currentWidth += symbolWidth;
        visibleChars++;
    }

    return visibleChars;
}

void calculateTextPosition(const char *text,
                          uint8_t rectLeft, uint8_t rectTop,
                          uint8_t rectWidth, uint8_t rectHeight,
                          uint8_t hAlign, uint8_t vAlign,
                          uint8_t &outLeft, uint8_t &outTop)
{
    calculateTextPosition(TextRef(text), rectLeft, rectTop, rectWidth, rectHeight,
                          hAlign, vAlign, outLeft, outTop);
}

void calculateTextPosition(TextRef text,
                          uint8_t rectLeft, uint8_t rectTop,
                          uint8_t rectWidth, uint8_t rectHeight,
                          uint8_t hAlign, uint8_t vAlign,
                          uint8_t &outLeft, uint8_t &outTop)
{
    if (text.isNull())
    {
        outLeft = rectLeft;
        outTop = rectTop;
        return;
    }

    const uint8_t fontIndex = lcd.getSystemFontIndex();
    uint8_t charHeight = lcd.fonts[fontIndex].height;
    uint8_t charWidth = lcd.fonts[fontIndex].width;
    uint8_t padding = 1;

    // Получаем видимый текст (который поместится)
    uint8_t maxChars = getVisibleChars(text, rectWidth - 2 * padding);
    uint16_t textWidth = getTextWidth(text, maxChars);

    // Горизонтальное выравнивание
    uint8_t x = rectLeft + padding;
    if (hAlign == 1)  // центр
    {
        if (textWidth <= (rectWidth - 2 * padding))
            x = rectLeft + (rectWidth - textWidth) / 2;
    }
    else if (hAlign == 2)  // право
    {
        x = rectLeft + rectWidth - padding - textWidth;
    }

    // Вертикальное выравнивание
    uint8_t y = rectTop + padding;
    if (vAlign == 1)  // центр
    {
        if (charHeight <= (rectHeight - 2 * padding))
            y = rectTop + (rectHeight - charHeight) / 2;
    }
    else if (vAlign == 2)  // низ
    {
        y = rectTop + rectHeight - padding - charHeight;
    }

    outLeft = x;
    outTop = y;
}

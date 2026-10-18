#include "Button.h"
#include "../Form.h"
#include "../../Display.h"
#include "../../Keyboard.h"
#include "../UIEvents.h"
#include "../../Common.h"

Button::Button(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char *caption)
{
    this->left = left;
    this->top = top;
    this->width = (width < 3) ? 3 : width;
    this->height = (height < 3) ? 3 : height;
    this->flags = FLAG_VISIBLE;
    this->ownerForm = nullptr;
    this->eventMask = 0;
    setCaption(caption);
}

Button::Button(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const __FlashStringHelper *caption)
    : Button(left, top, width, height, TextRef(caption).c_str())
{
    setCaption(caption);
}

void Button::setCaption(const char *caption)
{
    this->caption = TextRef(caption);
}

void Button::setCaption(const __FlashStringHelper *caption)
{
    this->caption = TextRef(caption);
}

void Button::calcTextPosition(uint8_t &textLeft, uint8_t &textTop, uint8_t &textRight, uint8_t &textBottom)
{
    const uint8_t fontIndex = lcd.getSystemFontIndex();
    uint8_t charWidth = lcd.fonts[fontIndex].width;
    uint8_t padding = 1;
    uint8_t maxCaptionWidth = width - 2 * padding;

    uint8_t captionLength = caption.length();
    uint8_t totalCaptionWidth = (captionLength == 0) ? 0 : captionLength * charWidth + (captionLength - 1);

    uint8_t captionLeft = left + padding;
    if (totalCaptionWidth <= maxCaptionWidth)
    {
        captionLeft += (maxCaptionWidth - totalCaptionWidth) / 2;
    }
    else
    {
        uint8_t visibleChars = (maxCaptionWidth + 1) / (charWidth + 1);
        if (visibleChars > 0)
            captionLeft += (maxCaptionWidth - (visibleChars * charWidth + (visibleChars - 1))) / 2;
    }

    textLeft = captionLeft + padding;
    textTop = top + padding;
    textRight = left + width - padding;
    textBottom = top + height - padding;
}

void Button::draw()
{
    if (!isVisible() || ownerForm == nullptr)
        return;

    // Отрисовка рамки кнопки
    ownerForm->drawRect(left, top, left + width, top + height, modeClear, true);
    ownerForm->drawRect(left, top, left + width, top + height, modeSet, isFocused());

    // Вычисляем позицию текста
    uint8_t textLeft, textTop, textRight, textBottom;
    calcTextPosition(textLeft, textTop, textRight, textBottom);

    // Устанавливаем область печати и выводим текст
    ownerForm->setPrintRect(textLeft, textTop, textRight, textBottom);
    ownerForm->print(caption, modeInvert);
}

bool Button::onKey(uint8_t key)
{
    if (!isVisible() || !isFocused())
        return false;

    if (key == keyEnter || key == keySpace)
    {
        triggerEvent(EVENT_CLICK);
        return true; // true - забираем нажатие, false - оставляем для пользователя
    }
    return false;
}

void Button::onFocusChanged()
{
    draw();
    restoreTopLayer();
}

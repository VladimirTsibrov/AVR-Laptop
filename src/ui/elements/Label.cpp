#include "Label.h"
#include "../Form.h"
#include "../../Display.h"
#include "../../Common.h"

Label::Label(const char *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height,
             uint8_t hAlign, uint8_t vAlign)
    : hAlign(hAlign), vAlign(vAlign)
{
    this->left = left;
    this->top = top;
    this->width = (width == 0) ? 1 : width;
    this->height = (height == 0) ? 1 : height;
    this->flags = FLAG_VISIBLE;
    this->ownerForm = nullptr;
    this->eventMask = 0;
    setText(text);
}

Label::Label(const __FlashStringHelper *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height,
             uint8_t hAlign, uint8_t vAlign)
    : Label(TextRef(text).c_str(), left, top, width, height, hAlign, vAlign)
{
    setText(text);
}

Label::Label(TextRef text, uint8_t left, uint8_t top, uint8_t width, uint8_t height,
             uint8_t hAlign, uint8_t vAlign)
    : text(text), hAlign(hAlign), vAlign(vAlign)
{
    this->left = left;
    this->top = top;
    this->width = (width == 0) ? 1 : width;
    this->height = (height == 0) ? 1 : height;
    this->flags = FLAG_VISIBLE;
    this->ownerForm = nullptr;
    this->eventMask = 0;
}

void Label::setText(const char *text)
{
    this->text = TextRef(text);
}

void Label::setText(const __FlashStringHelper *text)
{
    this->text = TextRef(text);
}

void Label::setAlignment(uint8_t hAlign, uint8_t vAlign)
{
    this->hAlign = hAlign;
    this->vAlign = vAlign;
}

void Label::draw()
{
    if (!isVisible() || ownerForm == nullptr)
        return;

    // Очистка области
    ownerForm->drawRect(left, top, left + width, top + height, modeClear, true);

    // Вычисляем позицию текста с выравниванием
    uint8_t textX, textY;
    calculateTextPosition(text, left, top, width, height,
                          hAlign, vAlign, textX, textY);

    // Выводим текст
    ownerForm->setPrintRect(textX, textY, left + width, top + height, true);
    ownerForm->print(text, modeInvert);
}

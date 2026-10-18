#include "CheckBox.h"
#include "../Form.h"
#include "../../Display.h"
#include "../../Keyboard.h"
#include "../UIEvents.h"
#include "../../Common.h"

const uint8_t iconUnchecked[] PROGMEM = {0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff};
const uint8_t iconChecked[] PROGMEM = {0xff, 0x81, 0xc1, 0xe3, 0xb7, 0x9d, 0x89, 0xff};

CheckBox::CheckBox(uint8_t left, uint8_t top, bool checked, const char *caption)
{
    this->left = left;
    this->top = top;
    this->height = 10;
    this->flags = FLAG_VISIBLE;
    setFlag(FLAG_CHECKED, checked);
    this->ownerForm = nullptr;
    this->eventMask = 0;
    uint8_t captionLength = TextRef(caption).length();
    const uint8_t fontIndex = lcd.getSystemFontIndex();
    this->width = 10;
    if (captionLength > 0)
        this->width += captionLength * lcd.fonts[fontIndex].width + (captionLength - 1) + 1;
    setCaption(caption);
}

CheckBox::CheckBox(uint8_t left, uint8_t top, bool checked, const __FlashStringHelper *caption)
    : CheckBox(left, top, checked, TextRef(caption).c_str())
{
    setCaption(caption);
    uint8_t captionLength = this->caption.length();
    const uint8_t fontIndex = lcd.getSystemFontIndex();
    this->width = 10;
    if (captionLength > 0)
        this->width += captionLength * lcd.fonts[fontIndex].width + (captionLength - 1) + 1;
}

void CheckBox::setCaption(const char *caption)
{
    this->caption = TextRef(caption);
}

void CheckBox::setCaption(const __FlashStringHelper *caption)
{
    this->caption = TextRef(caption);
}

void CheckBox::draw()
{
    if (!isVisible() || ownerForm == nullptr)
        return;

    // Очистка области перед выводом чекбокса
    ownerForm->drawRect(left, top, left + width, top + height, modeClear, true);

    if (isFocused())
    {
        ownerForm->drawRect(left, top, left + width, top + height - 1, modeSet, true);
        //        lcd.drawRect(left, top, left + width, top + height - 1, modeSet);
    }

    if (isChecked())
    {
        ownerForm->drawXBM(iconChecked, 8, 8, left + 1, top + 1, modeSet);
    }
    else
    {
        ownerForm->drawXBM(iconUnchecked, 8, 8, left + 1, top + 1, modeSet);
    }

    // Устанавливаем область печати и выводим текст
    ownerForm->setPrintRect(left + 10, top, ownerForm->clientRight() - ownerForm->clientLeft(), top + 8, true);
    ownerForm->print(caption, modeInvert);
}

bool CheckBox::onKey(uint8_t key)
{
    if (!isVisible() || !isFocused())
        return false;

    if (key == keyEnter || key == keySpace)
    {
        setChecked(!isChecked());
        triggerEvent(EVENT_CLICK);
        return true;
    }
    return false;
}

void CheckBox::setChecked(bool checked)
{
    setFlag(FLAG_CHECKED, checked);
    draw();
    restoreTopLayer();
    triggerEvent(EVENT_VALUE_CHANGED);
}

void CheckBox::onFocusChanged()
{
    draw();
    restoreTopLayer();
}

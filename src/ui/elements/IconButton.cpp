#include "IconButton.h"
#include "../Form.h"
#include "../../Keyboard.h"
#include "../UIEvents.h"

IconButton::IconButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
                       const uint8_t *icon, uint8_t iconWidth, uint8_t iconHeight)
    : icon(icon), iconWidth(iconWidth), iconHeight(iconHeight)
{
    this->left = left;
    this->top = top;
    this->width = (width < 3) ? 3 : width;
    this->height = (height < 3) ? 3 : height;
    this->flags = FLAG_VISIBLE;
    this->ownerForm = nullptr;
    this->eventMask = 0;
}

IconButton::IconButton(uint8_t left, uint8_t top, const uint8_t *icon, uint8_t iconSize)
    : IconButton(left, top, iconSize + 2, iconSize + 2, icon, iconSize, iconSize)
{
}

void IconButton::draw()
{
    if (!isVisible() || !ownerForm)
        return;

    DrawMode iconMode = isFocused() ? modeInvert : modeSet;
    ownerForm->drawRect(left, top, left + width, top + height, modeClear, true);
    ownerForm->drawRect(left, top, left + width, top + height, modeSet, isFocused());
    if (icon && iconWidth && iconHeight)
    {
        uint8_t iconLeft = left + (width - iconWidth) / 2;
        uint8_t iconTop = top + (height - iconHeight) / 2;
        ownerForm->drawXBM(icon, iconWidth, iconHeight, iconLeft, iconTop, iconMode);
    }
}

void IconButton::setIcon(const uint8_t *icon, uint8_t iconWidth, uint8_t iconHeight)
{
    this->icon = icon;
    this->iconWidth = iconWidth;
    this->iconHeight = iconHeight;
    draw();
    restoreTopLayer();
}

bool IconButton::onKey(uint8_t key)
{
    if (!isVisible() || !isFocused())
        return false;

    if (key != keyEnter && key != keySpace)
        return false;

    triggerEvent(EVENT_CLICK);
    return true;
}

void IconButton::onFocusChanged()
{
    draw();
    restoreTopLayer();
}

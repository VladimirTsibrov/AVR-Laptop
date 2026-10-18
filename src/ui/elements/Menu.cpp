#include "Menu.h"
#include "../Form.h"
#include "../../Keyboard.h"
#include "../UIEvents.h"

Menu::Menu(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
    : ArrayList(left, top, width, height), resultIndex(-1)
{
    setDrawBorder(true);
    setScrollBarVisible(false);
}

void Menu::show()
{
    resetResult();
    setVisible(true);
    setFocus(true);
    draw();
}

void Menu::close()
{
    setFocus(false);
    setVisible(false);
    if (ownerForm)
        ownerForm->draw();
}

bool Menu::onKey(uint8_t key)
{
    if (!isVisible() || !isFocused())
        return false;

    if (key == keyEnter)
    {
        resultIndex = (getItemCount() > 0) ? (int16_t)getSelectedIndex() : -1;
        close();
        triggerEvent(EVENT_CLICK);
        return true;
    }

    if (key == keyEsc || key == keyBackspace)
    {
        resultIndex = -1;
        close();
        triggerEvent(EVENT_CLOSE);
        return true;
    }

    if (key == keyUp || key == keyDown || key == keyHome || key == keyEnd)
    {
        ArrayList::onKey(key);
        return true;
    }

    // Поглощаем все остальные клавиши, чтобы они не проваливались в другие элементы
    return true;
}

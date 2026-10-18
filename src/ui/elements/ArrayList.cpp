#include "ArrayList.h"
#include <string.h>

ArrayList::ArrayList(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
    : List(left, top, width, height)
{
    items = nullptr;
    source = ARRAYLIST_SOURCE_NONE;
}

void ArrayList::setRamItems(const char *const *items, uint16_t count)
{
    this->items = items;
    source = ARRAYLIST_SOURCE_RAM;
    setItemCount(items ? count : 0);
    draw();
    restoreTopLayer();
}

void ArrayList::setProgmemItems(PGM_P const *items, uint16_t count)
{
    this->items = items;
    source = ARRAYLIST_SOURCE_PROGMEM;
    setItemCount(items ? count : 0);
    draw();
    restoreTopLayer();
}

bool ArrayList::getItemText(uint16_t index, char *buffer, uint8_t bufferSize)
{
    if (bufferSize == 0 || !items || index >= itemCount)
        return false;

    buffer[0] = '\0';
    if (source == ARRAYLIST_SOURCE_RAM)
    {
        const char *const *ramItems = (const char *const *)items;
        const char *text = ramItems[index];
        if (text)
        {
            strncpy(buffer, text, bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
        }
        return true;
    }

    if (source == ARRAYLIST_SOURCE_PROGMEM)
    {
        PGM_P const *progmemItems = (PGM_P const *)items;
        PGM_P text = (PGM_P)pgm_read_ptr(&progmemItems[index]);
        if (text)
        {
            strncpy_P(buffer, text, bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
        }
        return true;
    }

    return false;
}

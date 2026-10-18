#include "FileList.h"
#include "../../Keyboard.h"
#include "../../Display.h"
#include "../UIEvents.h"
#include "../../Common.h"
#include <string.h>

namespace
{
    bool checkExt(const char *filename, const char *extension)
    {
        if (*extension == '.')
            extension++;

        const char *filenameEnd = filename;
        const char *extensionEnd = extension;

        while (*filenameEnd)
            filenameEnd++;
        while (*extensionEnd)
            extensionEnd++;

        while (extensionEnd > extension)
        {
            if (filenameEnd == filename)
                return false;

            extensionEnd--;
            filenameEnd--;
            if ((*filenameEnd | 0x20) != (*extensionEnd | 0x20))
                return false;
        }

        return filenameEnd > filename && *--filenameEnd == '.';
    }
}

FileList::FileList(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
    : List(left, top, width, height)
{
    currentPath[0] = '/';
    currentPath[1] = '\0';
    dirCount = 0;
    fileCount = 0;
    fileFlags = 0;
    navDepth = 0;
    extensionFilter = nullptr;
    directoryOpened = false;
}

FileList::~FileList()
{
    delete[] extensionFilter;
}

bool FileList::setExtensionFilter(TextRef extension)
{
    if (extension.isNull() || extension.isEmpty())
    {
        if (!extensionFilter)
            return true;

        delete[] extensionFilter;
        extensionFilter = nullptr;
    }
    else
    {
        size_t length = extension.length();
        char *filter = new char[length + 1];
        if (!filter)
            return false;

        for (size_t i = 0; i <= length; i++)
            filter[i] = extension.charAt(i);

        if (extensionFilter && strcmp(extensionFilter, filter) == 0)
        {
            delete[] filter;
            return true;
        }

        delete[] extensionFilter;
        extensionFilter = filter;
    }

    if (!directoryOpened)
        return true;

    if (!scanDir())
        return false;

    setSelectedIndex(0);
    draw();
    restoreTopLayer();
    return true;
}

bool FileList::openDir(const char *path)
{
    if (!path || !path[0])
        return false;

    strncpy(currentPath, path, MAX_PATH_LEN - 1);
    currentPath[MAX_PATH_LEN - 1] = '\0';

    uint8_t len = strlen(currentPath);
    if (len == 0)
        return false;
    if (currentPath[len - 1] != '/')
    {
        if (len + 1 >= MAX_PATH_LEN)
            return false;
        currentPath[len++] = '/';
        currentPath[len] = '\0';
    }

    navDepth = 0;
    if (!scanDir())
        return false;
    directoryOpened = true;
    setSelectedIndex(0);
    draw();
    restoreTopLayer();
    return true;
}

bool FileList::scanDir()
{
    char workName[LIST_TEXT_BUF_SIZE];
    
    // Выделяем FsFile в heap временно
    FsFile *scanDirectory = new FsFile();
    FsFile *scanFile = new FsFile();
    if (!scanDirectory || !scanFile)
    {
        delete scanDirectory;
        delete scanFile;
        return false;
    }
    
    dirCount = 0;
    fileCount = 0;
    fileFlags = (strcmp(currentPath, "/") != 0) ? 0x01 : 0;

    if (!scanDirectory->open(currentPath, O_RDONLY))
    {
        delete scanDirectory;
        delete scanFile;
        return false;
    }

    scanDirectory->rewindDirectory();
    while (scanFile->openNext(scanDirectory, O_RDONLY))
    {
        scanFile->getName(workName, sizeof(workName));
        workName[sizeof(workName) - 1] = '\0';
        if (workName[0] == '\0')
        {
            scanFile->close();
            continue;
        }

        if (scanFile->isDir())
            dirCount++;
        else if (matchesExtension(workName))
            fileCount++;
        scanFile->close();
    }
    scanDirectory->close();
    
    delete scanDirectory;
    delete scanFile;

    setItemCount((uint16_t)(hasParent() ? 1 : 0) + dirCount + fileCount);
    return true;
}

void FileList::pushNav()
{
    if (FILELIST_NAV_DEPTH == 0)
        return;

    if (navDepth >= FILELIST_NAV_DEPTH)
    {
        for (uint8_t i = 1; i < FILELIST_NAV_DEPTH; i++)
            navStack[i - 1] = navStack[i];
        navDepth = FILELIST_NAV_DEPTH - 1;
    }

    navStack[navDepth].offset = scrollOffset;
    navStack[navDepth].focused = focusedRow;
    navDepth++;
}

bool FileList::popNav(NavState &state)
{
    if (navDepth == 0)
        return false;
    navDepth--;
    state = navStack[navDepth];
    return true;
}

void FileList::trimToParent()
{
    uint8_t len = strlen(currentPath);
    if (len <= 1)
        return;

    if (currentPath[len - 1] == '/')
        len--;
    while (len > 1 && currentPath[len - 1] != '/')
        len--;
    currentPath[len] = '\0';
}

bool FileList::getEntryInfo(uint16_t index, char *name, uint8_t nameSize, bool *isDir)
{
    if (nameSize == 0)
        return false;
    name[0] = '\0';

    if (hasParent())
    {
        if (index == 0)
            return false;
        index--;
    }

    bool needDir = index < dirCount;
    if (!needDir)
        index -= dirCount;

    char workName[LIST_TEXT_BUF_SIZE];
    
    // Выделяем FsFile в heap временно
    FsFile *scanDirectory = new FsFile();
    FsFile *scanFile = new FsFile();
    if (!scanDirectory || !scanFile)
    {
        delete scanDirectory;
        delete scanFile;
        return false;
    }
    
    if (!scanDirectory->open(currentPath, O_RDONLY))
    {
        delete scanDirectory;
        delete scanFile;
        return false;
    }

    scanDirectory->rewindDirectory();
    while (scanFile->openNext(scanDirectory, O_RDONLY))
    {
        bool entryIsDir = scanFile->isDir();
        scanFile->getName(workName, sizeof(workName));
        workName[sizeof(workName) - 1] = '\0';
        if (workName[0] == '\0')
        {
            scanFile->close();
            continue;
        }

        if (entryIsDir == needDir && (entryIsDir || matchesExtension(workName)))
        {
            if (index == 0)
            {
                scanFile->getName(name, nameSize);
                name[nameSize - 1] = '\0';
                scanFile->close();
                scanDirectory->close();
                delete scanDirectory;
                delete scanFile;
                if (isDir)
                    *isDir = entryIsDir;
                return true;
            }
            index--;
        }
        scanFile->close();
    }

    scanDirectory->close();
    delete scanDirectory;
    delete scanFile;
    return false;
}

bool FileList::matchesExtension(const char *filename) const
{
    return !extensionFilter || checkExt(filename, extensionFilter);
}

uint8_t FileList::pathLen() const
{
    return strlen(currentPath);
}

bool FileList::getItemText(uint16_t index, char *buffer, uint8_t bufferSize)
{
    if (bufferSize == 0)
        return false;

    buffer[0] = '\0';
    if (hasParent() && index == 0)
    {
        strncpy(buffer, "[..]", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return true;
    }

    bool isDir = false;
    if (!getEntryInfo(index, buffer, bufferSize, &isDir))
        return false;

    if (isDir && bufferSize > 2)
    {
        uint8_t textWidth = getContentWidth();
        if (itemCount > getVisibleRows() && textWidth > 4)
            textWidth -= 4;

        const uint8_t fontIndex = lcd.getSystemFontIndex();
        const uint8_t charPitch = lcd.fonts[fontIndex].width + 1;
        uint8_t maxChars = charPitch ? (textWidth + 1) / charPitch : 0;
        if (maxChars > bufferSize - 1)
            maxChars = bufferSize - 1;

        if (maxChars <= 2)
        {
            strncpy(buffer, "[]", bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            return true;
        }

        uint8_t nameChars = maxChars - 2;
        buffer[nameChars] = '\0';
        memmove(buffer + 1, buffer, nameChars + 1);
        buffer[0] = '[';
        uint8_t len = strlen(buffer);
        buffer[len++] = ']';
        buffer[len] = '\0';
    }
    return true;
}

bool FileList::getSelectedName(char *buffer, uint8_t bufferSize)
{
    if (bufferSize == 0)
        return false;
    buffer[0] = '\0';

    if (hasParent() && getSelectedIndex() == 0)
    {
        strncpy(buffer, "..", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return true;
    }

    return getEntryInfo(getSelectedIndex(), buffer, bufferSize, nullptr);
}

bool FileList::selectByName(const char *name)
{
    if (!name || !name[0])
        return false;

    char entryName[LIST_TEXT_BUF_SIZE];
    for (uint16_t index = 0; index < getItemCount(); index++)
    {
        if (getEntryInfo(index, entryName, sizeof(entryName), nullptr) && strcmp(entryName, name) == 0)
        {
            setSelectedIndex(index);
            draw();
            restoreTopLayer();
            return true;
        }
    }
    return false;
}

bool FileList::selectedIsDir()
{
    if (hasParent() && getSelectedIndex() == 0)
        return true;

    bool result = false;
    char ignored[1];
    if (!getEntryInfo(getSelectedIndex(), ignored, sizeof(ignored), &result))
        return false;
    return result;
}

bool FileList::openParent()
{
    if (!hasParent())
        return false;

    trimToParent();
    if (!scanDir())
    {
        draw();
        restoreTopLayer();
        return false;
    }

    NavState state;
    if (popNav(state))
    {
        scrollOffset = state.offset;
        focusedRow = state.focused;
        normalizeSelection();
    }
    else
    {
        setSelectedIndex(0);
    }
    draw();
    restoreTopLayer();
    return true;
}

bool FileList::openSelected()
{
    if (itemCount == 0)
        return false;

    if (hasParent() && getSelectedIndex() == 0)
        return openParent();

    uint8_t oldLen = pathLen();
    if (oldLen >= MAX_PATH_LEN - 1)
        return false;

    bool isDir = false;
    if (!getEntryInfo(getSelectedIndex(), currentPath + oldLen, MAX_PATH_LEN - oldLen - 1, &isDir))
        return false;

    if (isDir)
    {
        uint8_t len = strlen(currentPath);
        if (len + 1 >= MAX_PATH_LEN)
        {
            currentPath[oldLen] = '\0';
            return false;
        }

        currentPath[len++] = '/';
        currentPath[len] = '\0';

        pushNav();
        if (!scanDir())
        {
            currentPath[oldLen] = '\0';
            NavState ignored;
            popNav(ignored);
            draw();
            restoreTopLayer();
            return false;
        }
        setSelectedIndex(0);
        draw();
        restoreTopLayer();
        return true;
    }

    currentPath[oldLen] = '\0';
    return onFileSelected();
}

bool FileList::getSelectedPath(char *buffer, uint8_t bufferSize)
{
    if (!buffer || bufferSize == 0)
        return false;

    uint8_t oldLen = pathLen();
    if (oldLen >= bufferSize)
        return false;

    memcpy(buffer, currentPath, oldLen);
    return getSelectedName(buffer + oldLen, bufferSize - oldLen);
}

bool FileList::onFileSelected()
{
    return triggerEvent(EVENT_CLICK);
}

bool FileList::onKey(uint8_t key)
{
    // Обработка Ctrl+Up/Down как Page Up/Page Down
    KBState kb = keyboard.peekKBState();
    if (kb.ctrl() && key == keyUp)
    {
        // Page Up - прокрутка на целую страницу вверх
        uint8_t rows = getVisibleRows();
        for (uint8_t i = 0; i < rows && (focusedRow > 0 || scrollOffset > 0); i++)
        {
            if (focusedRow > 0)
                focusedRow--;
            else if (scrollOffset > 0)
                scrollOffset--;
        }
        draw();
        restoreTopLayer();
        triggerEvent(EVENT_VALUE_CHANGED);
        return true;
    }
    if (kb.ctrl() && key == keyDown)
    {
        // Page Down - прокрутка на целую страницу вниз
        uint8_t rows = getVisibleRows();
        uint16_t maxIndex = itemCount > 0 ? itemCount - 1 : 0;
        for (uint8_t i = 0; i < rows && getSelectedIndex() < maxIndex; i++)
        {
            if (focusedRow + 1 < rows)
                focusedRow++;
            else
                scrollOffset++;
        }
        draw();
        restoreTopLayer();
        triggerEvent(EVENT_VALUE_CHANGED);
        return true;
    }

    if (key == keyEnter)
        return openSelected();
    if (key == keyBackspace)
        return openParent();
    return List::onKey(key);
}

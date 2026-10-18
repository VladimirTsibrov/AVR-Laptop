#ifndef XBMVIEWER_H
#define XBMVIEWER_H

#include "../Task.h"
#include "../Display.h"
#include "../Keyboard.h"
#include <ctype.h>

namespace nsXBMViewer
{
    class XBMViewer : public Task
    {
    private:
        FsFile xbmFile;
        uint8_t bitmapWidth = 0;
        uint8_t bitmapHeight = 0;
        volatile bool renderPending = false;

        void closeFile()
        {
            xbmFile.close();
            bitmapWidth = 0;
            bitmapHeight = 0;
        }

        static bool readDefine(const char *line, const char *suffix, uint16_t &value)
        {
            if (!strstr(line, "#define"))
                return false;

            const char *number = strstr(line, suffix);
            if (!number)
                return false;
            number += strlen(suffix);

            while (*number == ' ' || *number == '\t')
                number++;
            if (!isdigit((unsigned char)*number))
                return false;

            value = (uint16_t)strtoul(number, nullptr, 10);
            return true;
        }

        bool readDimensions()
        {
            xbmFile.rewind();
            char line[128];
            uint16_t width = 0;
            uint16_t height = 0;

            while (xbmFile.available())
            {
                int length = xbmFile.readBytesUntil('\n', line, sizeof(line) - 1);
                line[length] = '\0';
                readDefine(line, "_width", width);
                readDefine(line, "_height", height);
                if (width != 0 && height != 0)
                    break;
            }

            if (width == 0 || height == 0 || width > Display::width() || height > Display::height())
                return false;

            bitmapWidth = (uint8_t)width;
            bitmapHeight = (uint8_t)height;
            return true;
        }

        bool seekDataStart()
        {
            xbmFile.rewind();
            char line[128];
            bool bitsDeclared = false;

            while (xbmFile.available())
            {
                int length = xbmFile.readBytesUntil('\n', line, sizeof(line) - 1);
                line[length] = '\0';
                if (strstr(line, "_bits"))
                    bitsDeclared = true;
                if (bitsDeclared && strchr(line, '{'))
                    return true;
            }
            return false;
        }

        bool readDataByte(uint8_t &value)
        {
            while (xbmFile.available())
            {
                int c = xbmFile.read();
                if (c != '0' || !xbmFile.available() || xbmFile.read() != 'x')
                    continue;

                value = 0;
                uint8_t digits = 0;
                while (xbmFile.available())
                {
                    c = xbmFile.read();
                    uint8_t nibble;
                    if (c >= '0' && c <= '9')
                        nibble = c - '0';
                    else if (c >= 'a' && c <= 'f')
                        nibble = c - 'a' + 10;
                    else if (c >= 'A' && c <= 'F')
                        nibble = c - 'A' + 10;
                    else
                        return digits != 0;

                    value = (value << 4) | nibble;
                    digits++;
                }
                return digits != 0;
            }
            return false;
        }

        void renderXbm()
        {
            lcd.cls();
            if (!xbmFile || !seekDataStart())
                return;

            const uint8_t bytesPerRow = (bitmapWidth + 7) >> 3;
            const uint8_t left = (Display::width() - bitmapWidth) / 2;
            const uint8_t top = (Display::height() - bitmapHeight) / 2;
            uint8_t row[16];

            for (uint8_t y = 0; y < bitmapHeight; y++)
            {
                for (uint8_t x = 0; x < bytesPerRow; x++)
                {
                    if (!readDataByte(row[x]))
                        return;
                }
                lcd.drawXBM_RAM(row, bitmapWidth, 1, left, top + y);
            }
        }

    protected:
        void setup() override {}

        void loop() override
        {
            if (renderPending)
            {
                renderPending = false;
                renderXbm();
            }

            if (keyboard.take(keyEsc))
                requestClose();
            vTaskDelay(pdMS_TO_TICKS(50));
        }

    public:
        static const __FlashStringHelper *getCaption() { return F("XBM Viewer"); }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }
        uint16_t getStackSize() const override { return 512; }
        
        static bool canOpen(const char *extension)
        {
            return extension && strcasecmp(extension, ".xbm") == 0;
        }

        ~XBMViewer() override { closeFile(); }

        bool openFile(const char* path) override
        {
            closeFile();
            if (!path || !path[0])
                return false;

            xbmFile = sdCard.open(path, O_RDONLY);
            if (!xbmFile || xbmFile.isDirectory())
            {
                closeFile();
                return false;
            }

            if (readDimensions())
                return true;

            closeFile();
            return false;
        }

        void onForeground() override
        {
            renderPending = true;
        }
    };
}

#endif // XBMVIEWER_H

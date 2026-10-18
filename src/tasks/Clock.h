#ifndef CLOCK_H
#define CLOCK_H

#include "../Task.h"
#include "../Debug.h"
#include "../Display.h"
#include "../Keyboard.h"
#include "../ui/UI.h"
#include "../ui/elements/All.h"
#include "../DS3231.h"

namespace nsClock
{
#define CLOCK_CENTER_X 31
#define CLOCK_CENTER_Y 26
#define CLOCK_RADIUS 22
#define HOUR_LEN 12
#define MINUTE_LEN 16
#define SECOND_LEN 18

    class Clock : public Task
    {
    private:
        inline static const char caption[] PROGMEM = "Date and time";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0x00, 0xF0, 0x80, 0x1F, 0xF0, 0xE0, 0x7F, 0xF0, 0x70, 0xE0, 0xF0,
            0x18, 0x86, 0xF1, 0x0C, 0x06, 0xF3, 0x0C, 0x06, 0xF3, 0x06, 0x06, 0xF6,
            0x06, 0x06, 0xF6, 0x06, 0x06, 0xF6, 0x06, 0x3E, 0xF6, 0x06, 0x3E, 0xF6,
            0x06, 0x00, 0xF6, 0x0C, 0x00, 0xF3, 0x0C, 0x00, 0xF3, 0x18, 0x80, 0xF1,
            0x70, 0xE0, 0xF0, 0xE0, 0x7F, 0xF0, 0x80, 0x1F, 0xF0, 0x00, 0x00, 0xF0};
        Form *frmMain = nullptr;
        Button *btnSetDateTime = nullptr;
        Button *btnClose = nullptr;

        // Форма ввода времени/даты

        Form *frmSet = nullptr;
        Button *btnSetOk = nullptr;
        Button *btnSetCancel = nullptr;
        DateEdit *edtDate = nullptr;
        TimeEdit *edtTime = nullptr;
        RTCDateTime dateTime = RTCDateTime();

        void onEvent(UIElement *sender, uint8_t event, uint8_t value) override
        {
            (void)value;
            if (event == EVENT_CLICK && sender == btnSetDateTime)
            {
                // Показываем форму ввода даты и времени
                frmSet = new Form();
                frmSet->setCaption(F("Set new date & time"));
                frmSet->setDrawBorder(true);
                rtc.getDateTime(dateTime);
                edtTime = frmSet->addTimeEdit(2, 2, 64, 10, dateTime.time);
                edtDate = frmSet->addDateEdit(2, 16, 64, 10, dateTime.date);
                btnSetOk = frmSet->addButton(2, 34, 50, 12, F("Ok"));
                btnSetCancel = frmSet->addButton(72, 34, 50, 12, F("Cancel"));
                btnSetOk->setEventMask(EVENT_CLICK);
                btnSetCancel->setEventMask(EVENT_CLICK);
                ui.showForm(frmSet);
                return;
            }
            else if (event == EVENT_CLICK && sender == btnClose)
            {
                sender->getOwnerForm()->requestCloseOwnerTask();
            }

            else if (event == EVENT_CLICK && (sender == btnSetOk || sender == btnSetCancel))
            {
                if (sender == btnSetOk)
                {
                    //if (!edtDate->isValid() || !edtTime->isValid()) return;

                    dateTime.date = edtDate->getValue();
                    dateTime.time = edtTime->getValue();
                    rtc.setDateTime(dateTime);
                }
                // Закрыть форму
                ui.closeForm(frmSet);
                frmSet = nullptr;
                edtDate = nullptr;
                edtTime = nullptr;
                btnSetOk = nullptr;
                btnSetCancel = nullptr;
            }
        }

        void drawClockFace() const
        {
            frmMain->drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS, modeSet);
            frmMain->drawCircle(CLOCK_CENTER_X, CLOCK_CENTER_Y, CLOCK_RADIUS + 1, modeSet);

            for (uint8_t i = 0; i < 12; ++i)
            {
                float angle = ((float)i / 12.0f) * 2.0f * PI - PI / 2.0f;
                int16_t x1 = CLOCK_CENTER_X + (int16_t)(cos(angle) * (CLOCK_RADIUS - 3));
                int16_t y1 = CLOCK_CENTER_Y + (int16_t)(sin(angle) * (CLOCK_RADIUS - 3));
                int16_t x2 = CLOCK_CENTER_X + (int16_t)(cos(angle) * (CLOCK_RADIUS - 1));
                int16_t y2 = CLOCK_CENTER_Y + (int16_t)(sin(angle) * (CLOCK_RADIUS - 1));
                frmMain->drawLine(x1, y1, x2, y2, modeSet);
            }
        }

        void drawHands() const
        {
            RTCTime now;
            if (!rtc.getTime(now))
                return;

            uint8_t hour12 = now.hour % 12;
            float hourAngle = ((float)hour12 * 30.0f) + ((float)now.minute / 60.0f) * 30.0f;
            float minuteAngle = ((float)now.minute * 6.0f) + ((float)now.second / 60.0f) * 6.0f;
            float secondAngle = ((float)now.second * 6.0f);

            hourAngle = (hourAngle - 90.0f) * PI / 180.0f;
            minuteAngle = (minuteAngle - 90.0f) * PI / 180.0f;
            secondAngle = (secondAngle - 90.0f) * PI / 180.0f;

            int16_t hourX = CLOCK_CENTER_X + (int16_t)(cos(hourAngle) * HOUR_LEN);
            int16_t hourY = CLOCK_CENTER_Y + (int16_t)(sin(hourAngle) * HOUR_LEN);
            int16_t minuteX = CLOCK_CENTER_X + (int16_t)(cos(minuteAngle) * MINUTE_LEN);
            int16_t minuteY = CLOCK_CENTER_Y + (int16_t)(sin(minuteAngle) * MINUTE_LEN);
            int16_t secondX = CLOCK_CENTER_X + (int16_t)(cos(secondAngle) * SECOND_LEN);
            int16_t secondY = CLOCK_CENTER_Y + (int16_t)(sin(secondAngle) * SECOND_LEN);

            frmMain->drawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, hourX, hourY, modeSet);
            frmMain->drawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, minuteX, minuteY, modeSet);
            frmMain->drawLine(CLOCK_CENTER_X, CLOCK_CENTER_Y, secondX, secondY, modeSet);
        }

        void drawStatus() const
        {
            char timeBuf[9];
            RTCTime now;
            if (!rtc.getTime(now))
                return;

            snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u:%02u", now.hour, now.minute, now.second);

            frmMain->resetPrintRect();
            frmMain->setPrintPos(70, 12);
            frmMain->print(timeBuf, modeSet);
        }

    public:
        static const __FlashStringHelper *getCaption() { return reinterpret_cast<const __FlashStringHelper*>(caption); }
        static const uint8_t *getIcon() { return icon; }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }

        // uint16_t getStackSize() const override { return 512; }

        ~Clock() override
        {
            if (frmMain)
            {
                ui.closeForm(frmMain);
                frmMain = nullptr;
            }
            btnSetDateTime = nullptr;
            btnClose = nullptr;
        }

    protected:
        void setup() override
        {
            frmMain = new Form();
            frmMain->setCaption(reinterpret_cast<const __FlashStringHelper*>(caption));
            frmMain->setDrawBorder(true);

            btnSetDateTime = frmMain->addButton(68, 20, 52, 12, "Set");
            btnClose = frmMain->addButton(68, 38, 52, 12, "Close");
            btnSetDateTime->setEventMask(EVENT_CLICK);
            btnClose->setEventMask(EVENT_CLICK);

            ui.showForm(frmMain);
        }

        void loop() override
        {
            if (keyboard.take(keyEsc))
                requestClose();

            if (frmSet == nullptr)
            {
                frmMain->drawRect(0, 0, 54, 54, modeClear, true);
                drawClockFace();
                drawHands();
                drawStatus();
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    };
}

#endif // CLOCK_H

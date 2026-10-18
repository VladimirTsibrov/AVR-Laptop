#ifndef UITEST_H
#define UITEST_H

#include "../Task.h"
#include "../Display.h"
#include "../Keyboard.h"
#include "../ui/UI.h"
#include "../ui/elements/All.h"

namespace nsUITest
{
    class UITest : public Task
    {
    private:
        inline static const char caption[] PROGMEM = "UITest";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0xFE, 0xFF, 0xF7, 0x02, 0x00, 0xF4,
            0x1E, 0x00, 0xF4, 0x0A, 0x00, 0xF4, 0x0A, 0x00, 0xF4, 0x1E, 0x00, 0xF4,
            0x02, 0x00, 0xF4, 0xFE, 0xFF, 0xF7, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0,
            0xAA, 0xAA, 0xF2, 0x00, 0x00, 0xF4, 0x02, 0x00, 0xF4, 0x00, 0x00, 0xF4,
            0x02, 0x00, 0xF4, 0x00, 0x00, 0xF4, 0xFE, 0xFF, 0xF7, 0x00, 0x00, 0xF0};
        
        // Нестатические указатели - занимают SRAM только когда задача создана
        Form *form = nullptr;
        CheckBox *chkBorders = nullptr;
        CheckBox *chkTitle = nullptr;
        ProgressBar *progressBar = nullptr;
        Button *btnOk = nullptr;
        Button *btnCancel = nullptr;
        TextBox *textBox = nullptr;
        
        TickType_t lastUpdateTime = 0;
        const TickType_t UPDATE_INTERVAL = pdMS_TO_TICKS(50);
        bool dir = true;

        void onEvent(UIElement *sender, uint8_t event, uint8_t value) override
        {
            (void)value;
            if (sender == chkBorders && (event == EVENT_CLICK || event == EVENT_VALUE_CHANGED))
            {
                form->setDrawBorder(chkBorders->isChecked());
                return;
            }

            if (sender == chkTitle && (event == EVENT_CLICK || event == EVENT_VALUE_CHANGED))
            {
                form->setShowCaption(chkTitle->isChecked());
                return;
            }

            if (event == EVENT_CLICK)
            {
                if (sender == btnOk)
                {
                    ui.messageBox(F("Hello world!"), messageBoxOkCancel);
                    char *buffer = (char*)pvPortMalloc(40);
                    if (!buffer)
                        return;
                    buffer[0] = '\0';
                    ui.inputBox("Input filename", buffer, 40);
                    ui.openDialog(buffer, 40, ".vid");

                    ui.saveDialog(buffer, 40, F(".vid"));
                    vPortFree(buffer);
                }
                else if (sender == btnCancel)
                {
                    //
                }
            }
        }

    public:
        static const __FlashStringHelper *getCaption() { return reinterpret_cast<const __FlashStringHelper*>(caption); }
        static const uint8_t *getIcon() { return icon; }

        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }

        uint16_t getStackSize() const override { return 320; }

        ~UITest() override
        {
            if (form)
            {
                ui.closeForm(form);
                form = nullptr;
            }
            chkBorders = nullptr;
            chkTitle = nullptr;
            progressBar = nullptr;
            btnOk = nullptr;
            btnCancel = nullptr;
        }

    protected:
        void setup() override
        {
            form = new Form();
            form->setCaption(getTaskCaption());
            form->setDrawBorder(true);

            chkBorders = form->addCheckBox(1, 1, true, "Borders");
            chkTitle = form->addCheckBox(62, 1, true, "Title");
            progressBar = form->addProgressBar(1, 12, 120, 10, 0, 100);

            textBox = form->addTextBox(1, 24, 100, 10, "hello", 16);

            btnOk = form->addButton(2, 38, 57, 11, "Ok");
            btnCancel = form->addButton(64, 38, 57, 11, "Cancel");
            chkBorders->setEventMask(EVENT_CLICK | EVENT_VALUE_CHANGED);
            chkTitle->setEventMask(EVENT_CLICK);
            btnOk->setEventMask(EVENT_CLICK);
            btnCancel->setEventMask(EVENT_CLICK);

            lastUpdateTime = xTaskGetTickCount();
            ui.showForm(form);
        }

        void loop() override
        {
            if (keyboard.take(keyEsc))
                requestClose();

            TickType_t currentTime = xTaskGetTickCount();
            if ((currentTime - lastUpdateTime) >= UPDATE_INTERVAL)
            {
                lastUpdateTime = currentTime;
                uint8_t val = progressBar->getValue();
                if (val == 100)
                    dir = false;
                else if (val == 0)
                    dir = true;

                if (dir)
                    val++;
                else
                    val--;
                progressBar->setValue(val);
            }

            taskYIELD();
        }
    };
}

#endif // UITEST_H

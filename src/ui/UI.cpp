#include "UI.h"
#include "../Task.h"
#include "../Display.h"
#include "../Keyboard.h"
#include "elements/All.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <string.h>

SemaphoreHandle_t UI::fileDialogSemaphore = nullptr;

namespace
{
    class ModalForm : public Form
    {
    public:
        ModalForm(UI &ui) : owner(ui)
        {
            setDrawBorder(true);
        }

        bool onKey(const KBState &kb) override
        {
            if (kb.pressedKey == keyEsc)
                return owner.finishModal(this, dialogCancel);
            return Form::onKey(kb);
        }

        virtual void complete(DialogResult result)
        {
            owner.finishModal(this, result);
        }

    protected:
        UI &owner;
    };

    class ModalButton : public Button
    {
    public:
        ModalButton(ModalForm &form, uint8_t left, uint8_t top, uint8_t width,
                    uint8_t height, const char *caption, DialogResult result)
            : Button(left, top, width, height, caption), modalForm(form), modalResult(result) {}
        ModalButton(ModalForm &form, uint8_t left, uint8_t top, uint8_t width,
                    uint8_t height, const __FlashStringHelper *caption, DialogResult result)
            : Button(left, top, width, height, caption), modalForm(form), modalResult(result) {}

        bool onKey(uint8_t key) override
        {
            if (!isVisible() || !isFocused())
                return false;
            if (key != keyEnter && key != keySpace)
                return false;
            modalForm.complete(modalResult);
            return true;
        }

    private:
        ModalForm &modalForm;
        DialogResult modalResult;
    };

    class InputDialog : public ModalForm
    {
    public:
        InputDialog(UI &ui, const char *text, char *output, uint8_t outputSize)
            : InputDialog(ui, TextRef(text), output, outputSize)
        {
        }

        InputDialog(UI &ui, const __FlashStringHelper *text, char *output, uint8_t outputSize)
            : InputDialog(ui, TextRef(text), output, outputSize)
        {
        }

        InputDialog(UI &ui, TextRef text, char *output, uint8_t outputSize)
            : ModalForm(ui), output(output), outputSize(outputSize)
        {
            uint8_t buttonTop = clientHeight() - 16;
            constexpr uint8_t buttonWidth = 40;
            constexpr uint8_t buttonHeight = 10;
            constexpr uint8_t buttonOffset = 10;

            addLabel(text, 1, 1, clientWidth() - 2, 20);
            edit = addTextBox(1, 21, clientWidth() - 3, 10, output, outputSize - 1);
            addElement(new ModalButton(*this, buttonOffset, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textOk, dialogOk));
            addElement(new ModalButton(*this, clientWidth() - buttonOffset - buttonWidth - 2, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textCancel, dialogCancel));
        }

        void complete(DialogResult result) override
        {
            if (result == dialogOk && edit)
            {
                strncpy(output, edit->getText(), outputSize - 1);
                output[outputSize - 1] = '\0';
            }
            ModalForm::complete(result);
        }

    private:
        char *output;
        uint8_t outputSize;
        TextBox *edit = nullptr;
    };

    class OpenDialog : public ModalForm
    {
    public:
        OpenDialog(UI &ui, char *outputPath, uint8_t outputPathSize, bool directoryOnly,
                   TextRef extensionFilter)
            : ModalForm(ui), outputPath(outputPath), outputPathSize(outputPathSize),
              directoryOnly(directoryOnly), extensionFilter(extensionFilter)
        {
            constexpr uint8_t buttonWidth = 40;
            constexpr uint8_t buttonHeight = 10;
            constexpr uint8_t buttonOffset = 10;
            uint8_t buttonTop = clientHeight() - 16;
            const __FlashStringHelper *title = directoryOnly
                                                   ? (__FlashStringHelper *)textSelectDir
                                                   : (__FlashStringHelper *)textOpenFile;
            if (!addLabel(title, 1, 1, 100, 10))
            {
                ready = false;
                return;
            }
            list = static_cast<FileList *>(addElement(new FileList(1, 14, clientWidth() - 2, clientHeight() - 33)));
            if (!list || !list->setExtensionFilter(extensionFilter))
            {
                ready = false;
                return;
            }
            list->setDrawBorder(true);
            list->setEventMask(EVENT_CLICK);
            if (!addElement(new ModalButton(*this, buttonOffset, buttonTop, buttonWidth,
                                            buttonHeight, (__FlashStringHelper *)textOk, dialogOk)) ||
                !addElement(new ModalButton(*this, clientWidth() - buttonOffset - buttonWidth - 2,
                                            buttonTop, buttonWidth, buttonHeight,
                                            (__FlashStringHelper *)textCancel, dialogCancel)))
                ready = false;
        }

        bool isReady() const { return ready; }

        void update(TickType_t nowTicks) override
        {
            if (!listInitialized)
            {
                listInitialized = true;
                list->openDir((outputPath && outputPath[0]) ? outputPath : "/");
            }
            ModalForm::update(nowTicks);
        }

        bool onEvent(UIElement *sender, uint8_t event, uint8_t value) override
        {
            (void)value;
            if (sender == list && event == EVENT_CLICK && !directoryOnly)
            {
                complete(dialogOk);
                return true;
            }
            return false;
        }

        void complete(DialogResult result) override
        {
            if (result == dialogOk && list)
            {
                if (directoryOnly)
                {
                    strncpy(outputPath, list->getCurrentPath(), outputPathSize - 1);
                    outputPath[outputPathSize - 1] = '\0';
                }
                else
                {
                    if (!list->getSelectedPath(outputPath, outputPathSize))
                        return;
                }
            }
            ModalForm::complete(result);
        }

    private:
        char *outputPath;
        uint8_t outputPathSize;
        bool directoryOnly;
        TextRef extensionFilter;
        FileList *list = nullptr;
        bool listInitialized = false;
        bool ready = true;
    };

    class SaveFileList : public FileList
    {
    public:
        SaveFileList(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
            : FileList(left, top, width, height) {}

        bool onKey(uint8_t key) override
        {
            if (key == keyEnter && !selectedIsDir())
                return triggerEvent(EVENT_CLICK);
            return FileList::onKey(key);
        }
    };

    class SaveDialog : public ModalForm
    {
    public:
        SaveDialog(UI &ui, char *outputPath, uint8_t outputPathSize,
                   TextRef extensionFilter)
            : ModalForm(ui), outputPath(outputPath), outputPathSize(outputPathSize)
        {
            constexpr uint8_t buttonWidth = 40;
            constexpr uint8_t buttonHeight = 10;
            constexpr uint8_t buttonOffset = 10;
            uint8_t buttonTop = clientHeight() - 16;

            char *fileName = splitInitialPath();
            addLabel((__FlashStringHelper *)textSaveAs, 1, 1, 42, 10);
            uint8_t maxNameLength = outputPathSize - 1;
            if (maxNameLength > 32)
                maxNameLength = 32;
            edit = addTextBox(46, 1, clientWidth() - 48, 10, fileName, maxNameLength);
            list = static_cast<SaveFileList *>(addElement(new SaveFileList(1, 14, clientWidth() - 2, clientHeight() - 33)));
            list->setExtensionFilter(extensionFilter);
            list->setDrawBorder(true);
            list->setEventMask(EVENT_CLICK);
            addElement(new ModalButton(*this, buttonOffset, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textOk, dialogOk));
            addElement(new ModalButton(*this, clientWidth() - buttonOffset - buttonWidth - 2, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textCancel, dialogCancel));
        }

        void update(TickType_t nowTicks) override
        {
            if (!listInitialized)
            {
                listInitialized = true;
                list->openDir(outputPath[0] ? outputPath : "/");
            }
            ModalForm::update(nowTicks);
        }

        bool onEvent(UIElement *sender, uint8_t event, uint8_t value) override
        {
            (void)value;
            if (sender != list || event != EVENT_CLICK || !edit)
                return false;

            char name[LIST_TEXT_BUF_SIZE];
            if (list->getSelectedName(name, sizeof(name)))
                edit->setText(name);
            return true;
        }

        void complete(DialogResult result) override
        {
            if (result == dialogOk)
            {
                if (!list || !edit || !edit->getText()[0])
                    return;

                const char *directory = list->getCurrentPath();
                uint8_t directoryLength = strlen(directory);
                uint8_t nameLength = strlen(edit->getText());
                if (directoryLength + nameLength >= outputPathSize)
                    return;

                memcpy(outputPath, directory, directoryLength);
                memcpy(outputPath + directoryLength, edit->getText(), nameLength + 1);
            }
            ModalForm::complete(result);
        }

    private:
        char *splitInitialPath()
        {
            char *separator = strrchr(outputPath, '/');
            if (separator)
            {
                char *fileName = separator + 1;
                *separator = '\0';
                return fileName;
            }

            uint8_t nameLength = strlen(outputPath);
            if (nameLength >= outputPathSize - 1)
                nameLength = outputPathSize - 2;
            memmove(outputPath + 1, outputPath, nameLength + 1);
            outputPath[0] = '/';
            return outputPath + 1;
        }

        char *outputPath;
        uint8_t outputPathSize;
        SaveFileList *list = nullptr;
        TextBox *edit = nullptr;
        bool listInitialized = false;
    };

    DialogResult showMessageBox(UI &ui, TextRef text, MessageBoxButtons buttons)
    {
        constexpr uint8_t buttonWidth = 40;
        constexpr uint8_t buttonHeight = 10;
        constexpr uint8_t buttonOffset = 10;

        ModalForm *form = new ModalForm(ui);
        if (!form)
            return dialogCancel;

        uint8_t buttonTop = form->clientHeight() - 16;

        form->addLabel(text, 1, 1, form->clientWidth() - 2, form->clientHeight() - 16);
        if (buttons == messageBoxYesNo)
        {
            form->addElement(new ModalButton(*form, buttonOffset, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textYes, dialogYes));
            form->addElement(new ModalButton(*form, form->clientWidth() - buttonOffset - buttonWidth - 2, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textNo, dialogNo));
        }
        else if (buttons == messageBoxOkCancel)
        {
            form->addElement(new ModalButton(*form, buttonOffset, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textOk, dialogOk));
            form->addElement(new ModalButton(*form, form->clientWidth() - buttonOffset - buttonWidth - 2, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textCancel, dialogCancel));
        }
        else
            form->addElement(new ModalButton(*form, (form->clientWidth() - buttonWidth - 2) / 2, buttonTop, buttonWidth, buttonHeight, (__FlashStringHelper *)textOk, dialogOk));

        return ui.runModal(form);
    }
}

UI ui;

bool postUIEvent(Form *form, UIElement *sender, uint8_t event, uint8_t value)
{
    if (!form)
        return false;

    TaskForms *forms = ui.getTaskForms(form->getOwnerTask());
    if (forms && forms->stackTop && forms->stackTop->form == form &&
        form->onEvent(sender, event, value))
        return true;
    return forms && forms->task && forms->task->postEvent(sender, event, value);
}

DialogResult UI::runModal(Form *form)
{
    return runModal(form, xTaskGetCurrentTaskHandle());
}

DialogResult UI::runModal(Form *form, TaskHandle_t ownerTask)
{
    if (!form || modalSession)
    {
        delete form;
        return dialogCancel;
    }

    ModalSession session = {form, xTaskGetCurrentTaskHandle(), dialogCancel};
    modalSession = &session;
    modalFinishPending = false;
    if (!showForm(form, ownerTask))
    {
        modalSession = nullptr;
        delete form;
        return dialogCancel;
    }

    xTaskNotifyWait(0, UINT32_MAX, nullptr, portMAX_DELAY);
    return session.result;
}

bool UI::finishModal(Form *form, DialogResult result)
{
    if (!modalSession || modalFinishPending || modalSession->form != form)
        return false;

    modalSession->result = result;
    modalFinishPending = true;
    return true;
}

DialogResult UI::messageBox(const char *text, MessageBoxButtons buttons)
{
    return showMessageBox(*this, TextRef(text), buttons);
}

DialogResult UI::messageBox(const __FlashStringHelper *text, MessageBoxButtons buttons)
{
    return showMessageBox(*this, TextRef(text), buttons);
}

DialogResult UI::inputBox(const char *text, char *buffer, uint8_t bufferSize)
{
    if (!buffer || bufferSize < 2)
        return dialogCancel;
    InputDialog *form = new InputDialog(*this, text, buffer, bufferSize);
    return form ? runModal(form) : dialogCancel;
}

DialogResult UI::inputBox(const __FlashStringHelper *text, char *buffer, uint8_t bufferSize)
{
    if (!buffer || bufferSize < 2)
        return dialogCancel;
    InputDialog *form = new InputDialog(*this, text, buffer, bufferSize);
    return form ? runModal(form) : dialogCancel;
}

void UI::init()
{
    if (!fileDialogSemaphore)
        fileDialogSemaphore = xSemaphoreCreateCounting(1, 1);
}

DialogResult UI::openDialog(char *pathBuffer, uint8_t pathBufferSize, TextRef extensionFilter)
{
    return runFileDialog(pathBuffer, pathBufferSize, extensionFilter, false, false);
}

DialogResult UI::runFileDialog(char *pathBuffer, uint8_t pathBufferSize,
                               TextRef extensionFilter,
                               bool directoryOnly, bool save)
{
    if (!pathBuffer || pathBufferSize < 2)
        return dialogCancel;

    if (!fileDialogSemaphore)
        fileDialogSemaphore = xSemaphoreCreateCounting(1, 1);
    if (!fileDialogSemaphore ||
        xSemaphoreTake(fileDialogSemaphore, portMAX_DELAY) != pdTRUE)
        return dialogCancel;

    FileDialogRequest *request = new FileDialogRequest{
        xTaskGetCurrentTaskHandle(), pathBuffer, pathBufferSize, extensionFilter,
        directoryOnly, save, dialogCancel};
    if (!request)
    {
        xSemaphoreGive(fileDialogSemaphore);
        return dialogCancel;
    }

    if (xTaskCreate(fileDialogTask, "FileDlg", fileDialogStackSize, request, 3,
                    &fileDialogHandle) != pdPASS)
    {
        delete request;
        xSemaphoreGive(fileDialogSemaphore);
        return dialogCancel;
    }
    xTaskNotifyWait(0, UINT32_MAX, nullptr, portMAX_DELAY);
    DialogResult result = request->result;
    delete request;
    return result;
}

void UI::fileDialogTask(void *parameters)
{
    FileDialogRequest *request = static_cast<FileDialogRequest *>(parameters);
    Form *form = request->save
                     ? static_cast<Form *>(new SaveDialog(ui, request->pathBuffer,
                                                          request->pathBufferSize,
                                                          request->extensionFilter))
                     : static_cast<Form *>(new OpenDialog(ui, request->pathBuffer,
                                                          request->pathBufferSize,
                                                          request->directoryOnly,
                                                          request->extensionFilter));

    if (form)
    {
        OpenDialog *openForm = request->save ? nullptr : static_cast<OpenDialog *>(form);
        if (!openForm || openForm->isReady())
            request->result = ui.runModal(form, request->caller);
        else
            delete form;
    }

    xTaskNotify(request->caller, 0, eNoAction);
    if (fileDialogSemaphore)
        xSemaphoreGive(fileDialogSemaphore);
    ui.fileDialogHandle = nullptr;
    vTaskDelete(nullptr);
}

DialogResult UI::saveDialog(char *pathBuffer, uint8_t pathBufferSize,
                            TextRef extensionFilter)
{
    return runFileDialog(pathBuffer, pathBufferSize, extensionFilter, false, true);
}

DialogResult UI::selectDirectory(char *pathBuffer, uint8_t pathBufferSize)
{
    return runFileDialog(pathBuffer, pathBufferSize, TextRef(), true, false);
}

TaskForms *UI::getCurrentTaskForms()
{
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    TaskForms *forms = (TaskForms *)pvTaskGetThreadLocalStoragePointer(currentTask, 0);

    if (forms == NULL)
    {
        forms = createTaskFormsForCurrentTask();
    }

    return forms;
}

TaskForms *UI::createTaskFormsForCurrentTask()
{
    return createTaskFormsForTask(xTaskGetCurrentTaskHandle());
}

TaskForms *UI::createTaskFormsForTask(TaskHandle_t task)
{
    TaskForms *forms = (TaskForms *)pvPortMalloc(sizeof(TaskForms));
    if (forms == NULL)
        return NULL;

    forms->stackTop = NULL;
    forms->task = NULL;

    vTaskSetThreadLocalStoragePointer(task, 0, (void *)forms);

    return forms;
}

bool UI::showForm(Form *form)
{
    TaskHandle_t ownerTask = servicingTask ? servicingTask : xTaskGetCurrentTaskHandle();
    return showForm(form, ownerTask);
}

bool UI::showForm(Form *form, TaskHandle_t ownerTask)
{
    if (!form)
        return false;

    if (ownerTask == NULL)
        ownerTask = xTaskGetCurrentTaskHandle();

    form->setOwnerTask(ownerTask);

    TaskForms *forms = getTaskForms(ownerTask);
    if (!forms)
        forms = createTaskFormsForTask(ownerTask);

    if (!forms)
        return false;

    FormNode *curr = forms->stackTop;
    while (curr)
    {
        if (curr->form == form)
            return false;
        curr = curr->next;
    }

    FormNode *node = (FormNode *)pvPortMalloc(sizeof(FormNode));
    if (!node)
        return false;

    node->form = form;
    node->next = forms->stackTop;
    forms->stackTop = node;

    if (ownerTask == xTaskGetCurrentTaskHandle())
    {
        form->show();
    }
    else
    {
        // Дочерняя задача не имеет права писать в видеобуфер. Shell выполнит
        // первую полную отрисовку перед обслуживанием формы владельца.
        form->setVisible(true);
        pendingInitialDraw = form;
    }

    return true;
}

bool UI::closeForm(Form *form)
{
    if (!form)
        return false;

    TaskHandle_t ownerTask = form->getOwnerTask();
    if (ownerTask == NULL)
        ownerTask = xTaskGetCurrentTaskHandle();

    TaskForms *forms = getTaskForms(ownerTask);
    if (!forms)
    {
        form->hide();
        delete form;
        return true;
    }

    FormNode *prev = NULL;
    FormNode *curr = forms->stackTop;

    while (curr && curr->form != form)
    {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
    {
        form->hide();
        delete form;
        return true;
    }

    if (prev)
    {
        prev->next = curr->next;
    }
    else
    {
        forms->stackTop = curr->next;
    }

    Form *formToClose = curr->form;
    vPortFree(curr);

    if (formToClose)
    {
        if (formToClose == pendingInitialDraw)
            pendingInitialDraw = NULL;
        formToClose->hide();
        delete formToClose;
    }

    if (forms->stackTop)
    {
        repaintStack(forms->stackTop);
    }
    else if (forms->task == NULL)
    {
        vTaskSetThreadLocalStoragePointer(ownerTask, 0, NULL);
        vPortFree(forms);
    }

    return true;
}

Form *UI::getActiveForm()
{
    TaskForms *forms = getCurrentTaskForms();
    if (!forms || !forms->stackTop)
        return NULL;
    return forms->stackTop->form;
}

bool UI::isFormDrawingAllowed(const Form *form) const
{
    if (!form || !form->isVisible())
        return false;

    if (form == repaintingForm)
        return true;

    TaskForms *forms = getTaskForms(form->getOwnerTask());
    return forms && forms->stackTop && forms->stackTop->form == form;
}

void UI::registerTask(TaskHandle_t taskHandle, Task *task)
{
    TaskForms *forms = getTaskForms(taskHandle);
    if (!forms)
        forms = createTaskFormsForTask(taskHandle);
    if (forms)
        forms->task = task;
}

void UI::unregisterTask(TaskHandle_t taskHandle)
{
    TaskForms *forms = getTaskForms(taskHandle);
    if (forms)
    {
        forms->task = NULL;
        if (forms->stackTop == NULL)
        {
            vTaskSetThreadLocalStoragePointer(taskHandle, 0, NULL);
            vPortFree(forms);
        }
    }
}

void UI::requestCloseOwnerTask(TaskHandle_t taskHandle)
{
    TaskForms *forms = getTaskForms(taskHandle);
    if (forms && forms->task)
        forms->task->requestClose();
}

TaskForms *UI::getTaskForms(TaskHandle_t task)
{
    return (TaskForms *)pvTaskGetThreadLocalStoragePointer(task, 0);
}

void UI::repaintTask(TaskHandle_t task)
{
    TaskForms *forms = getTaskForms(task);
    if (forms && forms->stackTop)
        repaintStack(forms->stackTop);
}

void UI::repaintStack(FormNode *node)
{
    if (!node)
        return;

    repaintStack(node->next);

    if (node->form)
    {
        const Form *previousRepaintingForm = repaintingForm;
        repaintingForm = node->form;
        node->form->show();
        repaintingForm = previousRepaintingForm;
    }
}

void UI::service(TaskHandle_t task)
{
    TaskForms *forms = (TaskForms *)pvTaskGetThreadLocalStoragePointer(task, 0);
    if (!forms || !forms->stackTop)
        return;

    Form *active = forms->stackTop->form;
    if (!active->isVisible())
        return;

    servicingTask = task;

    if (active == pendingInitialDraw)
    {
        pendingInitialDraw = NULL;
        active->show();
    }

    active->update(xTaskGetTickCount());

    KBState kb = keyboard.peekKBState();
    if (kb.pressedKey != keyNone)
    {
        if (active->onKey(kb))
            keyboard.consumeKBState();
    }

    if (modalFinishPending && modalSession && modalSession->form == active)
    {
        TaskHandle_t modalTask = modalSession->task;
        modalSession = nullptr;
        modalFinishPending = false;
        closeForm(active);
        xTaskNotify(modalTask, 0, eNoAction);
    }

    servicingTask = NULL;
}

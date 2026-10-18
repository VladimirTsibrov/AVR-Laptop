#include "Shell.h"
#include "MemoryFree.h"
#include "ui/UI.h"
#include <string.h>

namespace nsShell
{
    bool needRepaint = true;
    volatile bool itsTimeToReadRTC = false;
    volatile bool itsTimeToScanKeyboard = false;

// Самостоятельные задачи: отображаются в главном меню.
#define MENU_TASK_TYPES(X)                     \
    X(FileManager, nsFileManager::FileManager) \
    X(Clock, nsClock::Clock)                   \
    X(Calc, nsCalc::Calc)                      \
    X(UITest, nsUITest::UITest)                \
    X(VideoPlayer, nsVideoPlayer::VideoPlayer) \
    X(Sokoban, nsSokoban::Sokoban)             \
    X(Life, nsLife::Life)                      \
    X(Snake, nsSnake::Snake)                   \
    X(Tetris, nsTetris::Tetris)                

    
//    X(Notepad, nsNotepad::Notepad)             \
//    X(MusicPlayer, nsMusicPlayer::MusicPlayer) \
//    X(Match3, nsMatch3::Match3)

// Обработчики файлов: не отображаются в меню, запускаются по расширению.
#define OPEN_WITH_TASK_TYPES(X) \
    X(XBMViewer, nsXBMViewer::XBMViewer)

#define ALL_TASK_TYPES(X) \
    MENU_TASK_TYPES(X)    \
    OPEN_WITH_TASK_TYPES(X)

    enum TaskType : uint8_t
    {
#define TASK_TYPE(name, type) taskType##name,
        MENU_TASK_TYPES(TASK_TYPE)
#undef TASK_TYPE
        taskTypeMenuCount,
#define TASK_TYPE(name, type) taskType##name,
        OPEN_WITH_TASK_TYPES(TASK_TYPE)
#undef TASK_TYPE
        taskTypeCount
    };

    TaskInfo availableTasks[taskTypeCount] = {};
    uint8_t tasksCount = taskTypeMenuCount;

    void switchToTask(TaskHandle_t target, TaskHandle_t shellHandle);

    TextRef getTaskCaption(uint8_t taskIndex)
    {
        switch (taskIndex)
        {
#define TASK_CAPTION(name, type) \
    case taskType##name:         \
        return TextRef(type::getCaption());
            ALL_TASK_TYPES(TASK_CAPTION)
#undef TASK_CAPTION
        default:
            return TextRef();
        }
    }

    const uint8_t *getTaskIcon(uint8_t taskIndex)
    {
        switch (taskIndex)
        {
#define TASK_ICON(name, type) \
    case taskType##name:      \
        return type::getIcon();
            MENU_TASK_TYPES(TASK_ICON)
#undef TASK_ICON
        default:
            return nullptr;
        }
    }

    Task *createTask(uint8_t taskIndex)
    {
        switch (taskIndex)
        {
#define CREATE_TASK(name, type) \
    case taskType##name:        \
        return new type();
            ALL_TASK_TYPES(CREATE_TASK)
#undef CREATE_TASK
        default:
            return nullptr;
        }
    }

    // Forward declaration
    bool startTask(uint8_t taskIndex, const char *path, TaskHandle_t shellHandle);

    bool canTaskOpen(uint8_t taskIndex, const char *extension)
    {
        switch (taskIndex)
        {
#define CAN_OPEN(name, type) \
    case taskType##name:     \
        return type::canOpen(extension);
            ALL_TASK_TYPES(CAN_OPEN)
#undef CAN_OPEN
        default:
            return false;
        }
    }

    bool requestOpenFile(const char *path, TaskHandle_t callerHandle)
    {
        if (!path || !path[0])
            return false;

        // Определяем расширение файла
        const char *extension = strrchr(path, '.');
        if (!extension || !extension[1])
            return false;

        // Ищем задачу, которая может открыть этот файл
        for (uint8_t i = 0; i < taskTypeCount; i++)
        {
            if (canTaskOpen(i, extension))
            {
                // Синхронно запускаем задачу, передавая путь напрямую
                // (путь живет на стеке вызывающей задачи до конца вызова)
                return startTask(i, path, callerHandle);
            }
        }
        return false;
    }

    bool startTask(uint8_t taskIndex, const char *path, TaskHandle_t shellHandle)
    {
        if (taskIndex >= taskTypeCount)
            return false;

        TaskInfo &taskInfo = availableTasks[taskIndex];
        if (taskInfo.taskPtr != nullptr)
        {
            if (eTaskGetState(taskInfo.handle) == eDeleted)
            {
                delete taskInfo.taskPtr;
                taskInfo.taskPtr = nullptr;
                taskInfo.handle = nullptr;
            }
            else
            {
                // Повторное открытие файла в существующей задаче
                if (path && !taskInfo.taskPtr->openFile(path))
                    return false;

                switchToTask(taskInfo.handle, shellHandle);
                return true;
            }
        }

        Task *newTask = createTask(taskIndex);
        if (!newTask)
            return false;

        // Задача сама открывает файл если передан путь
        if (path && !newTask->openFile(path))
        {
            delete newTask;
            return false;
        }

        TaskHandle_t newHandle = nullptr;
        const char *taskName = newTask->getCaptionForFreeRTOS(newTask->getTaskCaption());
        if (xTaskCreate(Task::start, taskName, newTask->getStackSize(),
                        newTask, 3, &newHandle) != pdPASS)
        {
            delete newTask;
            return false;
        }

        taskInfo.taskPtr = newTask;
        taskInfo.handle = newHandle;
        switchToTask(newHandle, shellHandle);
        return true;
    }

    uint8_t focusPos = 0;
    uint8_t scrolledCount = 0;
    uint8_t lastHour = 255;
    uint8_t lastMinute = 255;

    // История переключений задач (MRU)
    static const uint8_t HISTORY_SIZE = 8;
    static TaskHandle_t taskHistory[HISTORY_SIZE];
    static uint8_t historyCount = 0;

    // Состояние переключателя задач (без массива - работаем с taskHistory напрямую)
    uint8_t taskOverlayCount = 0; // количество живых задач в истории
    uint8_t taskOverlayPos = 0;   // текущая позиция в списке
    uint8_t taskOverlayScrolledCount = 0;
    Overlay *taskSwitchOverlay = nullptr;

    // Геометрия оверлея переключателя задач (X и ширина кратны 8)
    static const uint8_t swX = 16;
    static const uint8_t swY = 13;
    static const uint8_t swW = 96;
    static const uint8_t swH = 33;
    static const uint8_t swVisible = 3;
    static const uint8_t swLineH = 11;

    bool isActiveTask()
    {
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        return currentTask == activeTask;
    }

    // Добавить задачу в историю
    void addToHistory(TaskHandle_t task)
    {
        if (task == nullptr)
            return;

        // Удаляем из старой позиции
        for (uint8_t i = 0; i < historyCount; i++)
        {
            if (taskHistory[i] == task)
            {
                for (uint8_t j = i; j < historyCount - 1; j++)
                    taskHistory[j] = taskHistory[j + 1];
                historyCount--;
                break;
            }
        }

        if (historyCount >= HISTORY_SIZE)
            historyCount--;

        for (uint8_t i = historyCount; i > 0; i--)
            taskHistory[i] = taskHistory[i - 1];

        taskHistory[0] = task;
        historyCount++;
    }

    void removeFromHistory(TaskHandle_t task)
    {
        for (uint8_t i = 0; i < historyCount; i++)
        {
            if (taskHistory[i] == task)
            {
                for (uint8_t j = i; j < historyCount - 1; j++)
                    taskHistory[j] = taskHistory[j + 1];
                historyCount--;
                break;
            }
        }
    }

    bool isTaskAlive(TaskHandle_t task)
    {
        return task != nullptr && eTaskGetState(task) != eDeleted;
    }

    TaskHandle_t getMostRecentLiveTask(TaskHandle_t shellHandle)
    {
        for (uint8_t i = 0; i < historyCount; i++)
        {
            TaskHandle_t task = taskHistory[i];
            if (task == shellHandle || isTaskAlive(task))
                return task;
        }
        return shellHandle;
    }

    TextRef getHistoryTaskName(TaskHandle_t task, TaskHandle_t shellHandle)
    {
        if (task == shellHandle)
            return TextRef(F("Desktop"));

        for (uint8_t i = 0; i < taskTypeCount; i++)
        {
            if (availableTasks[i].handle == task && availableTasks[i].taskPtr != nullptr)
                return getTaskCaption(i);
        }

        return TextRef();
    }

    // Получить N-ю живую задачу из истории (для переключателя)
    TaskHandle_t getLiveTaskByIndex(uint8_t index, TaskHandle_t shellHandle)
    {
        uint8_t liveIdx = 0;
        for (uint8_t i = 0; i < historyCount; i++)
        {
            if (isTaskAlive(taskHistory[i]) && !getHistoryTaskName(taskHistory[i], shellHandle).isNull())
            {
                if (liveIdx == index)
                    return taskHistory[i];
                liveIdx++;
            }
        }
        return shellHandle;
    }

    // Найти индекс задачи в списке живых (для переключателя)
    uint8_t findLiveTaskIndex(TaskHandle_t task, TaskHandle_t shellHandle)
    {
        uint8_t liveIdx = 0;
        for (uint8_t i = 0; i < historyCount; i++)
        {
            if (isTaskAlive(taskHistory[i]) && !getHistoryTaskName(taskHistory[i], shellHandle).isNull())
            {
                if (taskHistory[i] == task)
                    return liveIdx;
                liveIdx++;
            }
        }
        return 0;
    }

    void repaintShell()
    {
        uint8_t x = 12;
        uint8_t y = 4;
        lcd.cls();
        lcd.drawRect(0, 54, 127, 63, modeSet, true);

        for (uint8_t pos = 0; pos < 8; pos++)
        {
            uint8_t taskIndex = scrolledCount + pos;
            if (taskIndex >= tasksCount)
                break;

            lcd.drawXBM(getTaskIcon(taskIndex), 20, 20, x, y);

            if (pos == focusPos)
            {
                lcd.drawRect(x - 2, y - 2, x + 21, y + 21, modeSet);
                lcd.drawRect(x - 3, y - 3, x + 22, y + 22, modeSet);
                lcd.setPrintPos(2, 62);
                lcd.print(getTaskCaption(taskIndex), modeInvert);
            }

            if ((pos + 1) % 4 == 0)
            {
                x = 12;
                y += 26;
            }
            else
            {
                x += 28;
            }
        }
    }

    void repaintClock(bool force = false)
    {
        uint8_t hour, minute;
        rtc.getTime(hour, minute);

        if (!force && hour == lastHour && minute == lastMinute)
            return;

        lastHour = hour;
        lastMinute = minute;

        char time[6];
        time[0] = (hour / 10) + '0';
        time[1] = (hour % 10) + '0';
        time[2] = ':';
        time[3] = (minute / 10) + '0';
        time[4] = (minute % 10) + '0';
        time[5] = '\0';

        lcd.drawRect(96, 54, 127, 63, modeSet, true);
        lcd.resetPrintRect();
        lcd.setPrintPos(96, 62);
        lcd.print(time, modeInvert);
    }

    void setupTimer4()
    {
        TCCR4A = 0;
        TCCR4B = 0;
        TCNT4 = 0;
        OCR4A = 78;
        TCCR4B |= (1 << WGM42) | (1 << CS42) | (1 << CS40);
        TIMSK4 |= (1 << OCIE4A);
    }

    ISR(TIMER4_COMPA_vect)
    {
        static uint8_t counter = 0;
        itsTimeToScanKeyboard = true;
        if (++counter >= 50)
        {
            counter = 0;
            itsTimeToReadRTC = true;
        }
    }

    // --- Переключатель задач ---------------------------------------------

    void ensureSwitcherVisible()
    {
        if (taskOverlayPos < taskOverlayScrolledCount)
            taskOverlayScrolledCount = taskOverlayPos;
        else if (taskOverlayPos >= taskOverlayScrolledCount + swVisible)
            taskOverlayScrolledCount = taskOverlayPos - (swVisible - 1);
    }

    void drawTaskSwitcher()
    {
        lcd.drawRect(swX, swY, swX + swW - 1, swY + swH - 1, modeClear, true);
        lcd.drawRect(swX, swY, swX + swW - 1, swY + swH - 1, modeSet, false);

        lcd.resetPrintRect();

        // Буфер для имени одной задачи
        char nameBuf[configMAX_TASK_NAME_LEN];

        for (uint8_t line = 0; line < swVisible; line++)
        {
            uint8_t idx = taskOverlayScrolledCount + line;
            if (idx >= taskOverlayCount)
                break;

            // Получаем handle N-й живой задачи
            TaskHandle_t taskHandle = getLiveTaskByIndex(idx, shellTask);
            if (taskHandle == nullptr)
                break;

            // Получаем имя задачи во временный буфер
            TextRef name = getHistoryTaskName(taskHandle, shellTask);
            if (name.isNull())
                continue;

            uint8_t nameLength = 0;
            while (nameLength < configMAX_TASK_NAME_LEN - 1 && name.charAt(nameLength))
            {
                nameBuf[nameLength] = name.charAt(nameLength);
                nameLength++;
            }
            nameBuf[nameLength] = '\0';

            uint8_t top = swY + 2 + line * swLineH;
            bool focused = (idx == taskOverlayPos);
            if (focused)
                lcd.drawRect(swX + 1, top - 1, swX + swW - 2, top + swLineH - 3, modeSet, true);

            lcd.setPrintPos(swX + 4, top + 7);
            lcd.print(nameBuf, focused ? modeInvert : modeSet);
        }
    }

    void buildHistoryOverlay(TaskHandle_t shellHandle)
    {
        // Считаем живые задачи в истории
        uint8_t aliveCount = 0;
        for (uint8_t i = 0; i < historyCount; i++)
        {
            if (isTaskAlive(taskHistory[i]) && !getHistoryTaskName(taskHistory[i], shellHandle).isNull())
                aliveCount++;
        }

        taskOverlayCount = aliveCount;
        if (taskOverlayCount == 0)
            return;

        // Находим позицию активной задачи в списке живых
        uint8_t activeOverlayPos = findLiveTaskIndex(activeTask, shellHandle);

        // Первое Ctrl+Tab должно выбрать предыдущую активную задачу
        taskOverlayPos = (taskOverlayCount > 1) ? (activeOverlayPos + 1) % taskOverlayCount : 0;
        taskOverlayScrolledCount = 0;
        ensureSwitcherVisible();
    }

    void switchToTask(TaskHandle_t target, TaskHandle_t shellHandle)
    {
        uint8_t idx = 255;
        if (target != shellHandle)
        {
            for (uint8_t i = 0; i < taskTypeCount; i++)
            {
                if (availableTasks[i].taskPtr != nullptr && availableTasks[i].handle == target)
                {
                    idx = i;
                    break;
                }
            }
            if (idx == 255)
                return;
        }

        if (target == activeTask)
        {
            if (target == shellHandle)
            {
                needRepaint = true;
                lastHour = 255;
                lastMinute = 255;
            }
            else
            {
                availableTasks[idx].taskPtr->onForeground();
                ui.repaintTask(target);
            }
            return;
        }

        // Фон для предыдущей задачи
        if (activeTask != shellHandle && activeTaskIndex < taskTypeCount &&
            availableTasks[activeTaskIndex].taskPtr != nullptr)
        {
            availableTasks[activeTaskIndex].taskPtr->onBackground();
        }

        // Добавляем в историю и задачи, и shell
        addToHistory(target);

        activeTask = target;

        if (target == shellHandle)
        {
            activeTaskIndex = 255;
            needRepaint = true;
            lastHour = 255;
            lastMinute = 255;
            return;
        }

        activeTaskIndex = idx;
        availableTasks[idx].taskPtr->onForeground();
        ui.repaintTask(target);
    }

    void drawFocusRect(uint8_t pos, DrawMode drawMode)
    {
        uint8_t row = pos / 4;
        uint8_t col = pos % 4;
        uint8_t x = 12 + col * 28;
        uint8_t y = (row == 0 ? 4 : 30);

        lcd.drawRect(x - 2, y - 2, x + 21, y + 21, drawMode);
        lcd.drawRect(x - 3, y - 3, x + 22, y + 22, drawMode);
    }

    void start(void *pvParameters)
    {
        DEBUG_INIT();
        TCCR5B = 0;
        lcd.begin();
        lcd.cls();
        sdCard.begin(28, SD_SCK_MHZ(12));
        setupTimer4();
        TaskHandle_t shellHandle = activeTask;
        shellTask = shellHandle;
        addToHistory(shellHandle);

        while (true)
        {
            if (itsTimeToScanKeyboard)
            {
                itsTimeToScanKeyboard = false;
                keyboard.scan();
            }

            KBState kbstate = keyboard.peekKBState();

            // Alt+Tab — переключатель задач
            if (kbstate.alt() && kbstate.pressedKey == keyTab)
            {
                keyboard.consumeKBState();

                if (taskOverlayCount == 0)
                {
                    buildHistoryOverlay(shellHandle);
                }
                else if (taskOverlayCount > 0)
                {
                    taskOverlayPos = (taskOverlayPos + 1) % taskOverlayCount;
                    ensureSwitcherVisible();
                }

                if (taskOverlayCount > 0)
                {
                    // Выделяем оверлей при первом Alt+Tab
                    if (taskSwitchOverlay == nullptr)
                        taskSwitchOverlay = lcd.addOverlay(swX, swY, swW, swH);

                    if (taskSwitchOverlay != nullptr)
                    {
                        uint8_t *base = lcd.getFrameBuffer();
                        const uint8_t startByte = swX >> 3;
                        const uint8_t byteWidth = swW >> 3;

                        {
                            SchedulerSuspender lock;

                            // Сохраняем текущий фон в буфер оверлея
                            for (uint8_t row = 0; row < swH; row++)
                                memcpy(taskSwitchOverlay->buf + (uint16_t)row * byteWidth,
                                       base + (uint16_t)(swY + row) * 16 + startByte,
                                       byteWidth);

                            // Рисуем переключатель задач поверх фона
                            drawTaskSwitcher();

                            // Построчный обмен содержимым: видеобуфер (переключатель задач) <-> оверлей (фон)
                            uint8_t tempLine[byteWidth];
                            for (uint8_t row = 0; row < swH; row++)
                            {
                                uint8_t *fbRow = base + (uint16_t)(swY + row) * 16 + startByte;
                                uint8_t *ovRow = taskSwitchOverlay->buf + (uint16_t)row * byteWidth;

                                // Временно сохраняем нарисованную строку
                                memcpy(tempLine, fbRow, byteWidth);
                                // Восстанавливаем фон в framebuffer
                                memcpy(fbRow, ovRow, byteWidth);
                                // Копируем нарисованную строку в оверлей
                                memcpy(ovRow, tempLine, byteWidth);
                            }
                        }
                    }
                }
            }
            // Отпустили Alt — переключаемся
            else if (taskOverlayCount > 0 && !kbstate.alt())
            {
                TaskHandle_t target = getLiveTaskByIndex(taskOverlayPos, shellHandle);

                if (taskSwitchOverlay != nullptr)
                {
                    lcd.removeOverlay(taskSwitchOverlay);
                    taskSwitchOverlay = nullptr;
                }

                taskOverlayCount = 0;

                switchToTask(target, shellHandle);
            }
            else if (kbstate.fn() && kbstate.pressedKey == keyUp)
            {
                keyboard.consumeKBState();
                lcd.increaseBacklight();
            }
            else if (kbstate.fn() && kbstate.pressedKey == keyDown)
            {
                keyboard.consumeKBState();
                lcd.decreaseBacklight();
            }

            if (activeTask == shellHandle)
            {
                KBState kbstate = keyboard.getKBState();
                if (kbstate.pressedKey != keyNone)
                {
                    uint8_t prevFocusPos = focusPos;
                    if (kbstate.pressedKey == keyLeft)
                    {
                        if (focusPos & 0x03)
                            focusPos--;
                        else if (scrolledCount >= 8)
                        {
                            scrolledCount -= 8;
                            focusPos = (focusPos >= 4 ? 7 : 3);
                            needRepaint = true;
                        }
                    }
                    else if (kbstate.pressedKey == keyRight)
                    {
                        uint8_t currentTaskIndex = scrolledCount + focusPos;
                        if ((focusPos & 0x03) != 3)
                        {
                            if (currentTaskIndex + 1 < tasksCount)
                                focusPos++;
                        }
                        else
                        {
                            uint8_t nextPageStart = scrolledCount + 8;
                            if (nextPageStart < tasksCount)
                            {
                                scrolledCount = nextPageStart;
                                bool hasSecondRow = (tasksCount - nextPageStart) > 4;
                                focusPos = (focusPos >= 4 && hasSecondRow) ? 4 : 0;
                                needRepaint = true;
                            }
                        }
                    }
                    else if (kbstate.pressedKey == keyUp)
                    {
                        if (focusPos >= 4)
                        {
                            uint8_t taskAboveIndex = scrolledCount + focusPos - 4;
                            if (taskAboveIndex < tasksCount)
                                focusPos -= 4;
                        }
                    }
                    else if (kbstate.pressedKey == keyDown)
                    {
                        if (focusPos < 4)
                        {
                            uint8_t taskBelowIndex = scrolledCount + focusPos + 4;
                            if (taskBelowIndex < tasksCount)
                                focusPos += 4;
                        }
                    }
                    else if (kbstate.pressedKey == keyEnter)
                    {
                        uint8_t taskIndex = scrolledCount + focusPos;
                        if (taskIndex < tasksCount)
                            startTask(taskIndex, nullptr, shellHandle);
                    }

                    if (prevFocusPos != focusPos)
                    {
                        drawFocusRect(prevFocusPos, modeClear);
                        drawFocusRect(focusPos);
                        lcd.drawRect(0, 54, 95, 63, modeSet, true);
                        lcd.setPrintPos(2, 62);
                        lcd.print(getTaskCaption(scrolledCount + focusPos), modeInvert);
                    }
                }

                if (needRepaint)
                {
                    repaintShell();
                    repaintClock(true);
                    needRepaint = false;
                }
                else if (itsTimeToReadRTC)
                {
                    itsTimeToReadRTC = false;
                    repaintClock(false);
                }
            }
            else
            {
                if (eTaskGetState(activeTask) == eDeleted &&
                    activeTaskIndex < taskTypeCount &&
                    availableTasks[activeTaskIndex].taskPtr != nullptr)
                {
                    TaskHandle_t finishedHandle = availableTasks[activeTaskIndex].handle;
                    Task *finishedTask = availableTasks[activeTaskIndex].taskPtr;

                    delete finishedTask;
                    availableTasks[activeTaskIndex].taskPtr = nullptr;
                    availableTasks[activeTaskIndex].handle = nullptr;

                    removeFromHistory(finishedHandle);

                    switchToTask(getMostRecentLiveTask(shellHandle), shellHandle);
                }
            }

            ui.service(activeTask);
            vs1053.service();
            lcd.service();

            if (TCCR5B || vs1053.isInUse())
                taskYIELD();
            else
                vTaskDelay(1);
        }
    }

    static void printTaskStackUsage(const __FlashStringHelper *caption, TaskHandle_t handle, uint16_t allocatedBytes)
    {
        const unsigned int freeBytes = getStackFreeBytes(handle);
        DEBUG_PRINT(caption);
        DEBUG_PRINT_F(": used max=");
        DEBUG_PRINT((freeBytes <= allocatedBytes) ? allocatedBytes - freeBytes : 0);
        DEBUG_PRINT_F(" B, free min=");
        DEBUG_PRINT(freeBytes);
        DEBUG_PRINT_F(" B, allocated=");
        DEBUG_PRINT(allocatedBytes);
        DEBUG_PRINTLN_F(" B");
    }

    void printStackUsage(TaskHandle_t shellHandle)
    {
        DEBUG_PRINTLN_F("Stack high-water marks:");
        if (shellHandle != nullptr)
            printTaskStackUsage(F("Shell"), shellHandle, SHELL_STACK_SIZE);

        TaskHandle_t dialogHandle = ui.getFileDialogTaskHandle();
        if (dialogHandle != nullptr && eTaskGetState(dialogHandle) != eDeleted)
            printTaskStackUsage(F("FileDlg"), dialogHandle, UI::fileDialogStackSize);

        for (uint8_t i = 0; i < taskTypeCount; i++)
        {
            const TaskInfo &taskInfo = availableTasks[i];
            if (taskInfo.taskPtr != nullptr && taskInfo.handle != nullptr &&
                eTaskGetState(taskInfo.handle) != eDeleted)
            {
                printTaskStackUsage(getTaskCaption(i).flashString(), taskInfo.handle,
                                    taskInfo.taskPtr->getStackSize());
            }
        }
    }
}

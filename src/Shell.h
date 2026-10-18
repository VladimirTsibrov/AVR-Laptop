#ifndef SHELL_H
#define SHELL_H

#include "Arduino_FreeRTOS.h"
#include "Debug.h"
#include "Display.h"
#include "Keyboard.h"
#include "Sdcard.h"
#include "Task.h"
#include "DS3231.h"
#include "Common.h"
#include "tasks/Clock.h"
#include "tasks/Calc.h"
//#include "tasks/MusicPlayer.h"
#include "tasks/UITest.h"
// #include "tasks/Notepad.h"
#include "tasks/FileManager.h"
#include "tasks/VideoPlayer.h"
#include "tasks/XBMViewer.h"
#include "tasks/Sokoban.h"
#include "tasks/Life.h"
#include "tasks/Snake.h"
#include "tasks/Tetris.h"
// #include "tasks/Match3.h"

#define SHELL_STACK_SIZE 480 // Размер стека для shell

namespace nsShell
{
    extern bool needRepaint;
    extern volatile bool itsTimeToReadRTC;
    extern volatile bool itsTimeToScanKeyboard;

    struct TaskInfo
    {
        Task *taskPtr;
        TaskHandle_t handle; // хэндл запущенной задачи (для переключателя)
    };

    // Список доступных для запуска задач
    extern TaskInfo availableTasks[];

    extern uint8_t tasksCount;
    extern uint8_t focusPos;
    extern uint8_t scrolledCount;

    bool isActiveTask();

    // Синхронно открывает файл - вызывается из задачи, передает управление в Shell
    bool requestOpenFile(const char *path, TaskHandle_t callerHandle);

    void repaintShell();
    void repaintClock(bool force = false);

    // Выводит low-water mark стека оболочки и всех созданных прикладных задач.
    void printStackUsage(TaskHandle_t shellHandle);

    void setupTimer4();
    void drawFocusRect(uint8_t pos, DrawMode drawMode = modeSet);
    void start(void *pvParameters);

}

#endif // SHELL_H

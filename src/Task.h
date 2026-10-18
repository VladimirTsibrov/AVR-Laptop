#ifndef TASK_H
#define TASK_H

#include "Arduino_FreeRTOS.h"
#include "Display.h"
#include "Keyboard.h"
#include "Sdcard.h"
#include "ui/UI.h"

class UIElement;

/**
 * @brief Структура события от UI элемента к задаче
 *
 * Содержит информацию о событии, произошедшем в UI элементе (клик кнопки,
 * изменение значения и т.д.), которое нужно обработать в задаче.
 *
 * @see Task::postEvent, Task::onEvent
 */
struct TaskEvent
{
    UIElement *sender; ///< Указатель на UI элемент, сгенерировавший событие
    uint8_t type;      ///< Тип события (EVENT_CLICK, EVENT_VALUE_CHANGED и т.д.)
    uint8_t value;     ///< Дополнительное значение события (зависит от типа)
};

/**
 * @brief Слот для хранения ожидающего события
 *
 * Обеспечивает потокобезопасную передачу одного события между UI потоком
 * и потоком задачи через критические секции FreeRTOS.
 *
 * Ограничение: очередь глубиной 1 событие. Если событие уже ожидает обработки
 * (pending = true), новое событие будет отброшено (postEvent вернёт false).
 *
 * @see Task::postEvent, Task::takeEvent
 */
struct TaskEventSlot
{
    TaskEvent event; ///< Сохранённое событие
    bool pending;    ///< true если событие ожидает обработки, false если слот свободен
};

/**
 * @brief Результат решения о закрытии задачи
 *
 * Возвращается из Task::onCloseRequest() для управления процессом закрытия.
 *
 * @see Task::onCloseRequest, Task::requestClose
 */
enum CloseDecision : uint8_t
{
    closeAccept, ///< Разрешить закрытие задачи (данные сохранены, можно завершать)
    closeReject  ///< Отклонить закрытие (есть несохранённые данные, пользователь отменил)
};

/**
 * @brief Базовый класс для всех задач
 *
 * Task представляет собой отдельное приложение, работающее в собственном потоке FreeRTOS.
 * Каждая задача имеет свой UI, жизненный цикл и может обрабатывать события от UI элементов.
 *
 * Жизненный цикл задачи:
 * 1. Создание: xTaskCreate(Task::start, ...) с параметром Task*
 * 2. setup() — инициализация UI, загрузка данных
 * 3. loop() — основной цикл + обработка событий через onEvent()
 * 4. requestClose() → onCloseRequest() — запрос на закрытие (можно отклонить)
 * 5. onClosing() — финальная очистка ресурсов
 * 6. vTaskDelete(NULL) — завершение потока
 *
 * События переднего/заднего плана:
 * - onForeground() — задача стала активной (пользователь переключился на неё)
 * - onBackground() — задача ушла в фон (пользователь переключился на другую)
 *
 * @see UI, Form, Shell
 */
class Task
{
protected:
    /**
     * @brief Инициализация задачи (аналог Arduino setup)
     *
     * Вызывается один раз при старте задачи. Здесь создаются UI формы
     * и элементы управления (при необходимости), загружаются данные, открываются файлы.
     *
     * Должен быть реализован в производном классе (чисто виртуальный).
     */
    virtual void setup() = 0;

    /**
     * @brief Основной цикл задачи (аналог Arduino loop)
     *
     * Вызывается повторно в бесконечном цикле. Здесь обрабатываются
     * анимации, обновление данных, периодические операции.
     *
     * Должен быть реализован в производном классе (чисто виртуальный).
     */
    virtual void loop() = 0;

    /**
     * @brief Обработчик событий от UI элементов
     * @param sender Указатель на UI элемент, сгенерировавший событие
     * @param event Тип события (EVENT_CLICK, EVENT_VALUE_CHANGED и т.д.)
     * @param value Дополнительное значение события
     *
     * Вызывается автоматически при получении события через postEvent().
     * По умолчанию ничего не делает — переопределите для обработки событий.
     *
     * Альтернатива: использовать setEventHandler() на UI элементах.
     *
     * @code
     * void onEvent(UIElement* sender, uint8_t event, uint8_t value) override {
     *     if (sender == btnSave && event == EVENT_CLICK) {
     *         saveFile();
     *     }
     * }
     * @endcode
     */
    virtual void onEvent(UIElement *sender, uint8_t event, uint8_t value)
    {
        (void)sender;
        (void)event;
        (void)value;
    }

    /**
     * @brief Запрос подтверждения закрытия задачи
     * @return closeAccept для разрешения закрытия, closeReject для отклонения
     *
     * Вызывается при попытке закрыть задачу через requestClose().
     * Здесь можно проверить несохранённые данные и показать диалог подтверждения.
     *
     * По умолчанию всегда разрешает закрытие (closeAccept).
     */
    virtual CloseDecision onCloseRequest() { return closeAccept; }

    /**
     * @brief Финальная очистка перед завершением задачи
     *
     * Вызывается после того, как closeRequested = true и loop() завершил работу.
     * Здесь освобождаются ресурсы, закрываются файлы, удаляются UI элементы.
     *
     * После onClosing() вызывается ui.unregisterTask() и vTaskDelete().
     * По умолчанию ничего не делает.
     */
    virtual void onClosing() {}

private:
    volatile bool closeRequested = false; ///< Флаг запроса на закрытие (устанавливается requestClose)
    TaskEventSlot *eventSlot = nullptr;   ///< Слот для потокобезопасной передачи событий
    char *captionCopy = nullptr;          ///< Копия caption из PROGMEM в SRAM для FreeRTOS

    /**
     * @brief Извлекает ожидающее событие из слота (потокобезопасно)
     * @param[out] event Структура для записи извлечённого события
     * @return true если событие было извлечено, false если слот пуст
     *
     * Используется внутри цикла start() для получения событий из UI потока.
     */
    bool takeEvent(TaskEvent &event)
    {
        taskENTER_CRITICAL();
        if (!eventSlot || !eventSlot->pending)
        {
            taskEXIT_CRITICAL();
            return false;
        }

        event = eventSlot->event;
        eventSlot->pending = false;
        taskEXIT_CRITICAL();
        return true;
    }

public:
    /**
     * @brief Деструктор, освобождает память событийного слота и caption
     *
     * Автоматически освобождает память, выделенную через pvPortMalloc
     * для eventSlot и captionCopy.
     */
    virtual ~Task()
    {
        if (eventSlot)
            vPortFree(eventSlot);
        if (captionCopy)
            vPortFree(captionCopy);
    }

    /**
     * @brief Отправляет событие в задачу
     * @param sender Указатель на UI элемент, генерирующий событие
     * @param event Тип события (EVENT_CLICK и т.д.)
     * @param value Дополнительное значение
     * @return true если событие успешно отправлено, false если слот занят или ошибка памяти
     *
     * Вызывается из UI потока (UIElement::triggerEvent). Создаёт eventSlot при первом вызове.
     *
     * Ограничение: очередь глубиной 1 событие. Если предыдущее событие ещё не обработано,
     * новое событие будет отброшено (возврат false).
     *
     * @see onEvent, takeEvent
     */
    bool postEvent(UIElement *sender, uint8_t event, uint8_t value)
    {
        if (!eventSlot)
        {
            TaskEventSlot *newSlot = static_cast<TaskEventSlot *>(pvPortMalloc(sizeof(TaskEventSlot)));
            if (!newSlot)
                return false;

            newSlot->pending = false;
            bool installed = false;
            taskENTER_CRITICAL();
            if (!eventSlot)
            {
                eventSlot = newSlot;
                installed = true;
            }
            taskEXIT_CRITICAL();

            if (!installed)
                vPortFree(newSlot);
        }

        if (!eventSlot)
            return false;

        taskENTER_CRITICAL();
        if (eventSlot->pending)
        {
            taskEXIT_CRITICAL();
            return false;
        }

        eventSlot->event.sender = sender;
        eventSlot->event.type = event;
        eventSlot->event.value = value;
        eventSlot->pending = true;
        taskEXIT_CRITICAL();
        return true;
    }

    /**
     * @brief Запрашивает закрытие задачи
     *
     * Вызывает onCloseRequest() для подтверждения. Если возвращён closeAccept,
     * устанавливает флаг closeRequested, что приводит к выходу из цикла loop()
     * и вызову onClosing().
     */
    void requestClose()
    {
        if (onCloseRequest() == closeAccept)
            closeRequested = true;
    }

    /**
     * @brief Полная перерисовка базового (не-форменного) слоя задачи (например, при
     * переключении задачи из фона на передний план).
     * UI формы перерисовываются автоматически через UI::repaintForms(). Всё остальное - 
     * в ответсвенности самой задачи.
     * По умолчанию ничего не делает.
     *
     * @code
     * // Пример для игры Snake
     * void fullRepaint() override {
     *     lcd.clear();
     *     drawGameField();
     *     drawScore();
     * }
     * @endcode
     */
    virtual void fullRepaint() {}

    /**
     * @brief Вызывается при выводе задачи на передний план
     *
     * Происходит при переключении пользователя на эту задачу через Shell
     * или при возврате из модального диалога.
     *
     * По умолчанию вызывает fullRepaint(). UI формы восстанавливаются UI движком.
     *
     * @see onBackground
     */
    virtual void onForeground() { fullRepaint(); }

    /**
     * @brief Вызывается при уходе задачи в фоновый режим
     *
     * Происходит при переключении пользователя на другую задачу или
     * открытии модального диалога.
     *
     * Используется для остановки анимаций, сохранения состояния, паузы игры и т.д.
     *
     * По умолчанию ничего не делает.
     *
     * @code
     * void onBackground() override {
     *     pauseGame();
     *     releaseAudioBuffer();
     * }
     * @endcode
     *
     * @see onForeground
     */
    virtual void onBackground() {}

    /**
     * @brief Возвращает размер стека для потока FreeRTOS
     * @return Размер стека в байтах
     *
     * Каждая задача запрашивает свой размер стека в зависимости от сложности.
     * По умолчанию 256 байт (достаточно для простых UI задач).
     */
    virtual uint16_t getStackSize() const { return 256; }

    /**
     * @brief Возвращает название задачи для отображения в Shell
     * @return F-строка с названием из PROGMEM или nullptr
     *
     * Используется Shell для отображения имени задачи в меню.
     * Должен быть переопределён в производном классе.
     *
     * @code
     * const __FlashStringHelper* getTaskCaption() const override {
     *     return F("Notepad");
     * }
     * @endcode
     */
    virtual const __FlashStringHelper *getTaskCaption() const { return nullptr; }

    /**
     * @brief Копирует caption из PROGMEM в SRAM для FreeRTOS
     * @param caption TextRef со строкой имени задачи
     * @return Указатель на скопированную C-строку в SRAM или nullptr при ошибке
     *
     * FreeRTOS требует имена задач в SRAM (не PROGMEM). Этот метод выделяет
     * память через pvPortMalloc и копирует строку побайтно.
     *
     * Память освобождается в деструкторе Task.
     * При повторном вызове возвращает ранее скопированную строку.
     */
    const char *getCaptionForFreeRTOS(TextRef caption)
    {
        if (caption.isNull())
            return nullptr;

        if (captionCopy) // Уже скопировано
            return captionCopy;

        uint8_t len = caption.length();

        captionCopy = static_cast<char *>(pvPortMalloc(len + 1));
        if (!captionCopy)
            return nullptr;

        for (uint8_t i = 0; i <= len; i++)
            captionCopy[i] = caption.charAt(i);

        return captionCopy;
    }

    /**
     * @brief Проверяет, может ли задача открыть файл с заданным расширением
     * @param extension Расширение файла (например, "txt", "xbm", "vid")
     * @return true если задача поддерживает это расширение, false иначе
     *
     * Статический метод, используемый Shell/FileManager для определения,
     * какие задачи могут открыть выбранный файл.
     *
     * По умолчанию всегда возвращает false (задача не работает с файлами).
     *
     * @see openFile
     */
    static bool canOpen(const char *extension)
    {
        (void)extension;
        return false;
    }

    /**
     * @brief Открывает файл по пути
     * @param path Абсолютный путь к файлу на SD-карте
     * @return true при успешном открытии, false при ошибке
     *
     * Виртуальный метод для задач-редакторов/просмотрщиков.
     * По умолчанию возвращает false (задача не работает с файлами).
     *
     * Задача сама открывает файл через sdCard.open() если нужно.
     * Путь валиден только в рамках этого вызова - если нужно сохранить,
     * копируйте его в собственный буфер задачи.
     *
     * @see canOpen
     */
    virtual bool openFile(const char *path)
    {
        (void)path;
        return false;
    }

    /**
     * @brief Статическая функция-обёртка для запуска задачи в FreeRTOS
     * @param parameters Указатель на экземпляр Task (передаётся в xTaskCreate)
     *
     * Точка входа для потока FreeRTOS. Выполняет полный жизненный цикл задачи:
     * 1. Регистрирует задачу в UI (ui.registerTask)
     * 2. Вызывает setup()
     * 3. Входит в цикл: обработка событий (takeEvent → onEvent) + loop()
     * 4. При closeRequested = true выходит из цикла
     * 5. Вызывает onClosing()
     * 6. Отменяет регистрацию (ui.unregisterTask)
     * 7. Удаляет поток (vTaskDelete)
     *
     * Используется так:
     * @code
     * Task* task = new MyTask();
     * xTaskCreate(
     *     Task::start,
     *     task->getCaptionForFreeRTOS(task->getTaskCaption()),
     *     task->getStackSize(),
     *     task,  // передаётся в parameters
     *     3,
     *     &handle
     * );
     * @endcode
     */
    static void start(void *parameters)
    {
        Task *self = static_cast<Task *>(parameters);
        if (self != nullptr)
        {
            ui.registerTask(xTaskGetCurrentTaskHandle(), self);
            self->setup();
            while (!self->closeRequested)
            {
                TaskEvent event;
                if (self->takeEvent(event))
                    self->onEvent(event.sender, event.type, event.value);
                self->loop();
            }
            self->onClosing();
            ui.unregisterTask(xTaskGetCurrentTaskHandle());
        }
        vTaskDelete(NULL);
    }
};
#endif // TASK_H

#ifndef UI_H
#define UI_H

#include <inttypes.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include "Form.h"
#include "../Sdcard.h"

class Task;

// Константы для стандартных кнопок диалогов (хранятся в PROGMEM)
const char textOk[] PROGMEM = "Ok";
const char textYes[] PROGMEM = "Yes";
const char textNo[] PROGMEM = "No";
const char textCancel[] PROGMEM = "Cancel";
const char textSelectDir[] PROGMEM = "Select dir";
const char textOpenFile[] PROGMEM = "Open file";
const char textSaveAs[] PROGMEM = "Save as";

/**
 * @brief Результаты модальных диалогов
 */
enum DialogResult : uint8_t
{
    dialogCancel = 0, ///< Отмена / закрытие диалога
    dialogOk,         ///< Нажата кнопка Ok
    dialogYes,        ///< Нажата кнопка Yes
    dialogNo          ///< Нажата кнопка No
};

/**
 * @brief Наборы кнопок для MessageBox
 */
enum MessageBoxButtons : uint8_t
{
    messageBoxOk,      ///< Одна кнопка Ok
    messageBoxYesNo,   ///< Две кнопки: Yes и No
    messageBoxOkCancel ///< Две кнопки: Ok и Cancel
};

/**
 * @brief Узел связанного списка форм
 */
struct FormNode
{
    Form *form;     ///< Указатель на форму
    FormNode *next; ///< Следующий узел в стеке
};

/**
 * @brief Стек форм для конкретной задачи
 */
struct TaskForms
{
    FormNode *stackTop; ///< Вершина стека форм (верхняя активная форма)
    Task *task;         ///< Указатель на задачу-владельца
};

/**
 * @brief Менеджер пользовательского интерфейса
 *
 * Центральный компонент UI системы. Управляет стеком форм для каждой задачи,
 * обрабатывает модальные диалоги, предоставляет высокоуровневые функции для
 * отображения сообщений, ввода текста и выбора файлов.
 *
 * Каждая задача (Task) имеет свой стек форм. Верхняя форма в стеке является
 * активной и получает события клавиатуры. Формы могут быть модальными — в этом
 * случае выполнение задачи блокируется до закрытия диалога.
 *
 * Глобальный экземпляр доступен через `extern UI ui`.
 */
class UI
{
public:
    void init();

    TaskHandle_t getFileDialogTaskHandle() const { return fileDialogHandle; }
    static const uint16_t fileDialogStackSize = 350;

    /**
     * @brief Обслуживает UI для указанной задачи (периодический вызов)
     *
     * Вызывается в цикле задачи для обработки событий клавиатуры,
     * обновления состояния элементов и перерисовки форм.
     *
     * @param task Handle задачи FreeRTOS
     */
    void service(TaskHandle_t task);

    // ============== Управление формами ==============

    /**
     * @brief Показывает форму (добавляет в стек текущей задачи)
     * @param form Указатель на форму для отображения
     * @return true если форма успешно добавлена в стек
     */
    bool showForm(Form *form);

    /**
     * @brief Закрывает форму (удаляет из стека)
     * @param form Указатель на форму для закрытия
     * @return true если форма была найдена и закрыта
     */
    bool closeForm(Form *form);

    /**
     * @brief Получает указатель на верхнюю активную форму текущей задачи
     * @return Указатель на форму или nullptr, если стек пуст
     */
    Form *getActiveForm();

    /**
     * @brief Проверяет, разрешена ли отрисовка для указанной формы
     *
     * Используется системой UI для определения, может ли форма рисовать на экране.
     *
     * @param form Указатель на форму
     * @return true если форма может рисовать
     */
    bool isFormDrawingAllowed(const Form *form) const;

    /**
     * @brief Регистрирует задачу в UI системе
     * @param taskHandle Handle задачи FreeRTOS
     * @param task Указатель на объект Task
     */
    void registerTask(TaskHandle_t taskHandle, Task *task);

    /**
     * @brief Отменяет регистрацию задачи и очищает её стек форм
     * @param taskHandle Handle задачи FreeRTOS
     */
    void unregisterTask(TaskHandle_t taskHandle);

    /**
     * @brief Запрашивает закрытие задачи-владельца форм
     * @param taskHandle Handle задачи для закрытия
     */
    void requestCloseOwnerTask(TaskHandle_t taskHandle);

    // ============== Модальные диалоги ==============

    /**
     * @brief Запускает модальный диалог (блокирует задачу до закрытия)
     *
     * Форма показывается поверх текущих форм. Управление возвращается только
     * после вызова finishModal() для этой формы.
     *
     * @param form Указатель на форму диалога
     * @return Результат диалога (DialogResult)
     */
    DialogResult runModal(Form *form);

    /**
     * @brief Завершает модальный диалог и возвращает результат
     * @param form Указатель на форму диалога
     * @param result Результат для возврата из runModal()
     * @return true если диалог успешно завершён
     */
    bool finishModal(Form *form, DialogResult result);

    // ============== Стандартные диалоги ==============

    /**
     * @brief Показывает окно с сообщением (строка из RAM)
     * @param text Текст сообщения
     * @param buttons Набор кнопок (по умолчанию только Ok)
     * @return Результат диалога (какая кнопка нажата)
     */
    DialogResult messageBox(const char *text, MessageBoxButtons buttons = messageBoxOk);

    /**
     * @brief Показывает окно с сообщением (строка из PROGMEM)
     * @param text Текст сообщения из Flash памяти
     * @param buttons Набор кнопок (по умолчанию только Ok)
     * @return Результат диалога (какая кнопка нажата)
     */
    DialogResult messageBox(const __FlashStringHelper *text, MessageBoxButtons buttons = messageBoxOk);

    /**
     * @brief Показывает диалог ввода текста (строка из RAM)
     * @param text Приглашение для ввода
     * @param[out] buffer Буфер для введённого текста
     * @param bufferSize Размер буфера
     * @return dialogOk если текст введён, dialogCancel если отменено
     */
    DialogResult inputBox(const char *text, char *buffer, uint8_t bufferSize);

    /**
     * @brief Показывает диалог ввода текста (строка из PROGMEM)
     * @param text Приглашение для ввода из Flash памяти
     * @param[out] buffer Буфер для введённого текста
     * @param bufferSize Размер буфера
     * @return dialogOk если текст введён, dialogCancel если отменено
     */
    DialogResult inputBox(const __FlashStringHelper *text, char *buffer, uint8_t bufferSize);

    /**
     * @brief Показывает диалог выбора файла для открытия
     * @param[out] pathBuffer Буфер для записи полного пути к выбранному файлу
     * @param pathBufferSize Размер буфера пути
     * @param extensionFilter Фильтр по расширению (например, ".txt") или пустой для всех файлов
     * @return dialogOk если файл выбран, dialogCancel если отменено
     */
    DialogResult openDialog(char *pathBuffer, uint8_t pathBufferSize,
                            TextRef extensionFilter = TextRef());

    /**
     * @brief Показывает диалог выбора файла для сохранения
     * @param[out] pathBuffer Буфер для записи полного пути (может включать имя файла для сохранения)
     * @param pathBufferSize Размер буфера пути
     * @param extensionFilter Фильтр по расширению или пустой
     * @return dialogOk если путь выбран, dialogCancel если отменено
     */
    DialogResult saveDialog(char *pathBuffer, uint8_t pathBufferSize,
                            TextRef extensionFilter = TextRef());

    /**
     * @brief Показывает диалог выбора директории
     * @param[out] pathBuffer Буфер для записи пути к выбранной директории
     * @param pathBufferSize Размер буфера пути
     * @return dialogOk если директория выбрана, dialogCancel если отменено
     */
    DialogResult selectDirectory(char *pathBuffer, uint8_t pathBufferSize);

    // ============== Прочее ==============

    /**
     * @brief Перерисовывает верхнюю форму указанной задачи
     *
     * Используется переключателем задач (Shell) при выводе задачи на передний план.
     *
     * @param task Handle задачи для перерисовки
     */
    void repaintTask(TaskHandle_t task);

    /**
     * @brief Получает стек форм для указанной задачи
     * @param task Handle задачи FreeRTOS
     * @return Указатель на TaskForms или nullptr, если задача не зарегистрирована
     */
    TaskForms *getTaskForms(TaskHandle_t task);

private:
    TaskHandle_t servicingTask = NULL; ///< Handle задачи, которая сейчас обслуживается
    const Form *repaintingForm = NULL; ///< Форма, которая в данный момент перерисовывается
    Form *pendingInitialDraw = NULL;   ///< Форма, чья первая отрисовка ожидает контекста Shell

    /**
     * @brief Сессия модального диалога
     */
    struct ModalSession
    {
        Form *form;          ///< Форма модального диалога
        TaskHandle_t task;   ///< Задача, запустившая диалог
        DialogResult result; ///< Результат диалога
    };

    ModalSession *modalSession = nullptr; ///< Текущая модальная сессия
    bool modalFinishPending = false;      ///< Флаг ожидающего завершения модального диалога
    TaskHandle_t fileDialogHandle = nullptr;

    struct FileDialogRequest
    {
        TaskHandle_t caller;
        char *pathBuffer;
        uint8_t pathBufferSize;
        TextRef extensionFilter;
        bool directoryOnly;
        bool save;
        DialogResult result;
    };

    /**
     * @brief Получает стек форм для текущей задачи (servicingTask)
     * @return Указатель на TaskForms или nullptr
     */
    TaskForms *getCurrentTaskForms();

    /**
     * @brief Создаёт новый стек форм для текущей задачи
     * @return Указатель на созданный TaskForms
     */
    TaskForms *createTaskFormsForCurrentTask();

    /**
     * @brief Создаёт новый стек форм для указанной задачи
     * @param task Handle задачи
     * @return Указатель на созданный TaskForms
     */
    TaskForms *createTaskFormsForTask(TaskHandle_t task);

    /**
     * @brief Внутренний метод для показа формы с указанием владельца
     * @param form Указатель на форму
     * @param ownerTask Handle задачи-владельца
     * @return true если форма показана успешно
     */
    bool showForm(Form *form, TaskHandle_t ownerTask);

    DialogResult runModal(Form *form, TaskHandle_t ownerTask);
    DialogResult runFileDialog(char *pathBuffer, uint8_t pathBufferSize,
                               TextRef extensionFilter,
                               bool directoryOnly, bool save);
    static void fileDialogTask(void *parameters);
    static SemaphoreHandle_t fileDialogSemaphore;

    /**
     * @brief Перерисовывает стек форм, начиная с указанного узла
     * @param node Узел стека форм для начала отрисовки
     */
    void repaintStack(FormNode *node);
};

/**
 * @brief Глобальный экземпляр менеджера UI
 */
extern UI ui;

#endif

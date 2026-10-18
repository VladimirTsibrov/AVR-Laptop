#ifndef UI_MENU_H
#define UI_MENU_H

#include "ArrayList.h"

/**
 * @brief Menu — всплывающее меню с выбором элемента из списка
 * 
 * Специализированный ArrayList для реализации контекстных меню, выпадающих списков
 * и диалогов выбора. Отображается поверх других элементов (isOnTop = true) с рамкой.
 * 
 * Особенности:
 * - Всегда отображается на верхнем слое UI (поверх других элементов)
 * - Автоматическое включение рамки и отключение скроллбара по умолчанию
 * - Запоминает индекс выбранного пункта (resultIndex)
 * - Закрывается по Enter (выбор) или Esc/Backspace (отмена)
 * - Генерирует EVENT_CLICK при выборе, EVENT_CLOSE при отмене
 * - Методы show()/close() для управления видимостью и фокусом
 * - Поддержка навигации клавишами Up/Down/PageUp/PageDown/Home/End
 * 
 * Типовой сценарий использования:
 * 1. Создать Menu, установить items через setItems()
 * 2. Вызвать show() для отображения
 * 3. Обработать EVENT_CLICK и прочитать getResultIndex()
 * 4. Menu автоматически закроется после выбора или отмены
 * 
 * @code
 * static const char* menuItems[] PROGMEM = {
 *     "Открыть", "Сохранить", "Закрыть"
 * };
 * 
 * Menu* menu = new Menu(20, 10, 60, 40);
 * menu->setItems(menuItems, 3);
 * menu->setEventHandler(EVENT_CLICK, [menu]() {
 *     int8_t choice = menu->getResultIndex();
 *     if (choice == 0) openFile();
 *     else if (choice == 1) saveFile();
 *     else if (choice == 2) closeFile();
 * });
 * form->addElement(menu);
 * 
 * // Показать меню по событию (например, по кнопке)
 * menu->show();
 * @endcode
 * 
 * @see ArrayList, List
 */
class Menu : public ArrayList
{
public:
    /**
     * @brief Создаёт всплывающее меню
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param width Ширина меню
     * @param height Высота меню
     * 
     * По умолчанию включает рамку (drawBorder = true) и отключает скроллбар.
     * Начальное состояние: resultIndex = -1 (ничего не выбрано).
     */
    Menu(uint8_t left, uint8_t top, uint8_t width, uint8_t height);

    /**
     * @brief Проверяет, находится ли элемент на верхнем слое UI
     * @return Всегда true (меню всегда поверх других элементов)
     * 
     * Переопределяет метод UIElement для гарантии отрисовки меню
     * поверх всех остальных элементов формы.
     */
    bool isOnTop() const override { return true; }
    
    /**
     * @brief Обрабатывает нажатие клавиш
     * @param key Код нажатой клавиши
     * @return true если клавиша обработана, false если игнорируется
     * 
     * Обрабатываемые клавиши:
     * - Enter: сохраняет выбранный индекс в resultIndex, закрывает меню, генерирует EVENT_CLICK
     * - Esc/Backspace: устанавливает resultIndex = -1, закрывает меню, генерирует EVENT_CLOSE
     * - Up/Down/PageUp/PageDown/Home/End: навигация по пунктам (делегируется ArrayList)
     */
    bool onKey(uint8_t key) override;
    
    /**
     * @brief Возвращает индекс выбранного пункта меню
     * @return Индекс выбранного пункта (0..itemCount-1) или -1 если выбор отменён
     * 
     * Значение -1 означает, что меню было закрыто по Esc или не было открыто.
     * После show() значение сбрасывается в -1 до момента выбора пользователем.
     */
    int8_t getResultIndex() const { return resultIndex; }
    
    /**
     * @brief Сбрасывает результат выбора
     * 
     * Устанавливает resultIndex = -1. Используется автоматически в show().
     */
    void resetResult() { resultIndex = -1; }
    
    /**
     * @brief Показывает меню с фокусом
     * 
     * Сбрасывает resultIndex, делает меню видимым, устанавливает фокус
     * и перерисовывает элемент. После этого меню готово к взаимодействию.
     */
    void show();
    
    /**
     * @brief Закрывает меню
     * 
     * Снимает фокус, делает меню невидимым и перерисовывает форму-владельца.
     * Вызывается автоматически при выборе пункта (Enter) или отмене (Esc).
     */
    void close();

private:
    int8_t resultIndex; ///< Индекс выбранного пункта меню (-1 = не выбран или отменён)
};

#endif

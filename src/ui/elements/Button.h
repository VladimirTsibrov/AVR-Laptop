#ifndef BUTTON_H
#define BUTTON_H

#include "../UIElement.h"
#include "../TextRef.h"

/**
 * @brief Кнопка — элемент управления с текстовым label для нажатия
 *
 * Кнопка представляет собой прямоугольный элемент с текстовой подписью (caption),
 * который реагирует на нажатие клавиш Enter или Space и генерирует событие EVENT_CLICK.
 *
 * Особенности:
 * - Текст автоматически центрируется внутри кнопки
 * - Визуальное выделение фокуса двойной рамкой
 * - Минимальный размер 3×3 пикселя
 * - Поддержка строк из RAM и PROGMEM (TextRef)
 * - Инвертированный текст для лучшей читаемости
 *
 * @code
 * Button* btnOK = new Button(10, 10, 30, 10, F("OK"));
 * btnOK->setEventHandler(EVENT_CLICK, []() { _обработка_ });
 * form->addElement(btnOK);
 * @endcode
 *
 * @see UIElement, TextRef
 */
class Button : public UIElement
{
public:
    /**
     * @brief Создаёт кнопку с текстом из RAM
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param width Ширина кнопки (минимум 3 пикселя)
     * @param height Высота кнопки (минимум 3 пикселя)
     * @param caption Текст кнопки (строка в RAM)
     */
    Button(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char *caption);

    /**
     * @brief Создаёт кнопку с текстом из PROGMEM
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param width Ширина кнопки (минимум 3 пикселя)
     * @param height Высота кнопки (минимум 3 пикселя)
     * @param caption Текст кнопки (F-строка из PROGMEM)
     */
    Button(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const __FlashStringHelper *caption);

    /**
     * @brief Отрисовывает кнопку на экране
     *
     * Рисует двойную рамку (тонкую/жирную в зависимости от фокуса),
     * вычисляет позицию и выводит центрированный инвертированный текст.
     * Если текст не помещается, обрезается по границам кнопки.
     */
    void draw() override;

    /**
     * @brief Обрабатывает нажатие клавиш
     * @param key Код нажатой клавиши
     * @return true если клавиша обработана (Enter/Space), false если игнорируется
     *
     * Реагирует на keyEnter и keySpace, генерирует событие EVENT_CLICK.
     */
    bool onKey(uint8_t key) override;

    /**
     * @brief Устанавливает новый текст кнопки (RAM строка)
     * @param text Новый текст для отображения
     */
    void setCaption(const char *text);

    /**
     * @brief Устанавливает новый текст кнопки (PROGMEM строка)
     * @param text Новый текст для отображения (F-строка)
     */
    void setCaption(const __FlashStringHelper *text);

    /**
     * @brief Возвращает указатель на текст кнопки как C-строку
     * @return Указатель на C-строку текста (может быть в RAM или PROGMEM)
     * @see getCaptionRef
     */
    const char *getCaption() const { return caption.c_str(); }

    /**
     * @brief Возвращает текст кнопки как TextRef объект
     * @return TextRef обёртка, позволяющая единообразно работать с RAM/PROGMEM
     * @see TextRef
     */
    TextRef getCaptionRef() const { return caption; }

private:
    /**
     * @brief Вычисляет область для вывода текста с центрированием
     * @param[out] textLeft Левая граница области текста
     * @param[out] textTop Верхняя граница области текста
     * @param[out] textRight Правая граница области текста
     * @param[out] textBottom Нижняя граница области текста
     *
     * Учитывает размер шрифта, отступы (padding = 1) и центрирует текст
     * горизонтально внутри кнопки. Если текст не помещается, вычисляет
     * количество видимых символов и центрирует обрезанную строку.
     */
    void calcTextPosition(uint8_t &textLeft, uint8_t &textTop, uint8_t &textRight, uint8_t &textBottom);

    TextRef caption;      ///< Текст кнопки (универсальная обёртка RAM/PROGMEM)
    uint8_t textX, textY; ///< Кэшированные координаты текста (не используются в текущей реализации)

protected:
    /**
     * @brief Вызывается при изменении фокуса
     *
     * Перерисовывает кнопку с новым состоянием фокуса (тонкая/жирная рамка)
     * и восстанавливает верхний слой UI.
     */
    void onFocusChanged() override;
};

#endif // BUTTON_H

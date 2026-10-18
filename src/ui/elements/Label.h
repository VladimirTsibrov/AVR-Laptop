#ifndef LABEL_H
#define LABEL_H

#include "../UIElement.h"
#include "../TextRef.h"

/**
 * @brief Label — текстовая надпись с выравниванием
 * 
 * Простой элемент для отображения статического или динамического текста
 * в заданной прямоугольной области с поддержкой выравнивания по горизонтали и вертикали.
 * 
 * Особенности:
 * - Не получает фокус (isFocusable = false)
 * - Не обрабатывает клавиатуру
 * - Поддержка горизонтального выравнивания: левое (0), центр (1), правое (2)
 * - Поддержка вертикального выравнивания: верхнее (0), центр (1), нижнее (2)
 * - Поддержка строк из RAM и PROGMEM (TextRef)
 * - Текст автоматически обрезается по границам области
 * - Используется для заголовков, подписей, информационных полей
 * 
 * @code
 * // Центрированный заголовок
 * Label* title = new Label(F("Настройки"), 0, 5, 128, 10, 1, 0);
 * form->addElement(title);
 * 
 * // Информационное поле справа
 * Label* status = new Label(F("OK"), 100, 50, 28, 8, 2, 1);
 * form->addElement(status);
 * 
 * // Динамическое обновление текста
 * status->setText(F("Error"));
 * @endcode
 * 
 * @see UIElement, TextRef
 */
class Label : public UIElement
{
public:
      /**
       * @brief Создаёт текстовую надпись с текстом из RAM
       * @param text Текст надписи (строка в RAM)
       * @param left Левая координата X области отображения
       * @param top Верхняя координата Y области отображения
       * @param width Ширина области отображения (минимум 1 пиксель)
       * @param height Высота области отображения (минимум 1 пиксель)
       * @param hAlign Горизонтальное выравнивание: 0=левое, 1=центр, 2=правое (по умолчанию 0)
       * @param vAlign Вертикальное выравнивание: 0=верхнее, 1=центр, 2=нижнее (по умолчанию 0)
       * 
       * Если текст не помещается в область, он обрезается по границам.
       */
      Label(const char *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height,
            uint8_t hAlign = 0, uint8_t vAlign = 0);
      
      /**
       * @brief Создаёт текстовую надпись с текстом из PROGMEM
       * @param text Текст надписи (F-строка из PROGMEM)
       * @param left Левая координата X области отображения
       * @param top Верхняя координата Y области отображения
       * @param width Ширина области отображения
       * @param height Высота области отображения
       * @param hAlign Горизонтальное выравнивание (0-2)
       * @param vAlign Вертикальное выравнивание (0-2)
       */
      Label(const __FlashStringHelper *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height,
            uint8_t hAlign = 0, uint8_t vAlign = 0);
      
      /**
       * @brief Создаёт текстовую надпись с TextRef объектом
       * @param text Текст надписи (TextRef обёртка)
       * @param left Левая координата X области отображения
       * @param top Верхняя координата Y области отображения
       * @param width Ширина области отображения
       * @param height Высота области отображения
       * @param hAlign Горизонтальное выравнивание (0-2)
       * @param vAlign Вертикальное выравнивание (0-2)
       * 
       * Универсальный конструктор, принимающий уже созданный TextRef.
       */
      Label(TextRef text, uint8_t left, uint8_t top, uint8_t width, uint8_t height,
            uint8_t hAlign = 0, uint8_t vAlign = 0);

      /**
       * @brief Отрисовывает текстовую надпись на экране
       * 
       * Очищает область отображения, вычисляет позицию текста с учётом
       * выравнивания (через calculateTextPosition) и выводит текст
       * инвертированным режимом с обрезкой по границам области.
       */
      void draw() override;
      
      /**
       * @brief Обработчик клавиш (не используется)
       * @param key Код нажатой клавиши
       * @return Всегда false (Label не обрабатывает клавиатуру)
       */
      bool onKey(uint8_t key) override { return false; }
      
      /**
       * @brief Проверяет, может ли элемент получить фокус
       * @return Всегда false (Label не получает фокус)
       */
      bool isFocusable() const override { return false; }

      /**
       * @brief Устанавливает новый текст (RAM строка)
       * @param text Новый текст для отображения
       * @note Не вызывает автоматическую перерисовку. Вызовите draw() вручную
       *       или invalidate() для обновления экрана.
       */
      void setText(const char *text);
      
      /**
       * @brief Устанавливает новый текст (PROGMEM строка)
       * @param text Новый текст для отображения (F-строка)
       */
      void setText(const __FlashStringHelper *text);
      
      /**
       * @brief Возвращает указатель на текст как C-строку
       * @return Указатель на C-строку текста (может быть в RAM или PROGMEM)
       * @see getTextRef
       */
      const char *getText() const { return text.c_str(); }
      
      /**
       * @brief Возвращает текст как TextRef объект
       * @return TextRef обёртка, позволяющая единообразно работать с RAM/PROGMEM
       * @see TextRef
       */
      TextRef getTextRef() const { return text; }

      /**
       * @brief Устанавливает новое выравнивание текста
       * @param hAlign Горизонтальное выравнивание: 0=левое, 1=центр, 2=правое
       * @param vAlign Вертикальное выравнивание: 0=верхнее, 1=центр, 2=нижнее
       * @note Не вызывает автоматическую перерисовку. Вызовите draw() вручную
       *       для обновления экрана.
       */
      void setAlignment(uint8_t hAlign, uint8_t vAlign);

private:
      TextRef text;    ///< Текст надписи (универсальная обёртка RAM/PROGMEM)
      uint8_t hAlign;  ///< Горизонтальное выравнивание: 0=левое, 1=центр, 2=правое
      uint8_t vAlign;  ///< Вертикальное выравнивание: 0=верхнее, 1=центр, 2=нижнее
};

#endif // LABEL_H

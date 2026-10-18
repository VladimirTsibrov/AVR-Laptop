#ifndef UI_TEXTREF_H
#define UI_TEXTREF_H

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

/**
 * @brief Универсальная обёртка для текстовых строк из RAM и Flash (PROGMEM)
 * 
 * Позволяет единообразно работать со строками, хранящимися как в оперативной памяти,
 * так и в программной памяти (Flash). Автоматически определяет тип строки и использует
 * соответствующие методы доступа (pgm_read_byte для PROGMEM).
 * 
 * Используется в UI элементах для экономии SRAM — строковые литералы можно хранить
 * в Flash, передавая их через F() макрос или указатель __FlashStringHelper*.
 */
class TextRef
{
public:
    /**
     * @brief Конструктор по умолчанию, создаёт пустую ссылку (nullptr)
     */
    TextRef() : ptr(nullptr), flash(false) {}
    
    /**
     * @brief Конструктор для строки из RAM
     * @param text Указатель на строку в оперативной памяти
     */
    TextRef(const char *text) : ptr(text), flash(false) {}
    
    /**
     * @brief Конструктор для строки из PROGMEM (Flash памяти)
     * @param text Указатель на строку в программной памяти (используйте F() макрос)
     */
    TextRef(const __FlashStringHelper *text) : ptr(text), flash(true) {}

    /**
     * @brief Проверяет, является ли ссылка пустой (nullptr)
     * @return true если указатель на строку равен nullptr
     */
    bool isNull() const { return ptr == nullptr; }
    
    /**
     * @brief Проверяет, хранится ли строка в Flash памяти
     * @return true если строка находится в PROGMEM
     */
    bool isFlash() const { return flash; }
    
    /**
     * @brief Проверяет, является ли строка пустой (первый символ '\0')
     * @return true если строка имеет нулевую длину
     */
    bool isEmpty() const { return charAt(0) == '\0'; }

    /**
     * @brief Получает указатель на строку в RAM
     * @return Указатель на C-строку или nullptr, если строка в PROGMEM
     */
    const char *c_str() const { return flash ? nullptr : static_cast<const char *>(ptr); }
    
    /**
     * @brief Получает указатель на строку в Flash памяти
     * @return Указатель __FlashStringHelper* или nullptr, если строка в RAM
     */
    const __FlashStringHelper *flashString() const { return flash ? static_cast<const __FlashStringHelper *>(ptr) : nullptr; }

    /**
     * @brief Получает символ по индексу (работает для RAM и PROGMEM)
     * 
     * Автоматически использует pgm_read_byte() для чтения из Flash памяти.
     * 
     * @param index Позиция символа в строке (0-based)
     * @return Символ на указанной позиции или '\0' для пустой ссылки
     */
    char charAt(uint8_t index) const
    {
        if (!ptr)
            return '\0';

        if (flash)
            return pgm_read_byte(reinterpret_cast<PGM_P>(ptr) + index);

        return static_cast<const char *>(ptr)[index];
    }

    /**
     * @brief Вычисляет длину строки
     * 
     * Работает корректно как для RAM, так и для PROGMEM строк.
     * 
     * @param maxChars Максимальное количество символов для проверки (защита от переполнения)
     * @return Длина строки в символах (без учёта '\0')
     */
    uint8_t length(uint8_t maxChars = 255) const
    {
        uint8_t count = 0;
        while (count < maxChars && charAt(count))
            count++;
        return count;
    }

private:
    const void *ptr;  ///< Указатель на строку (RAM или Flash)
    bool flash;       ///< true = строка в PROGMEM, false = строка в RAM
};

#endif

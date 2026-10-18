#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Enable/disable debug output
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 1  // 0 - disabled, 1 - enabled
#endif

#if DEBUG_ENABLED
  #define DEBUG_INIT()      Serial.begin(115200)
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINT_F(x)  Serial.print(F(x))
  #define DEBUG_PRINTLN_F(x) Serial.println(F(x))
#else
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT_F(x)
  #define DEBUG_PRINTLN_F(x)
#endif

#endif

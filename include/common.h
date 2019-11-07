#pragma once
#include <stdint.h>
#include <Arduino.h>


#define DEBUG

#define DEBUG_SERIAL  Serial
#ifdef DEBUG
  #define dbgInit()     DEBUG_SERIAL.begin(115200);
  #define dbg(x)        DEBUG_SERIAL.print(x)
  #define dbgF(x)       DEBUG_SERIAL.print(F(x))
  #define dbg_s(...)    DEBUG_SERIAL.printf(__VA_ARGS__)
  #define dbgFlush()    DEBUG_SERIAL.flush()

  #define EOL "\r\n"
#else
  #define dbg(x)    {}
  #define dbgln(x)  {}
  #define dbgF(x)   {}
  #define dbglnF(x) {}
  #define dbgf(...) {}
  #define dbgflush  {}
#endif


#define __version "1.1.0"

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <TelnetStream.h>
#include <Arduino.h>
#include "WString.h"
#include "Printable.h"

class Debug {

public:

  Debug();

  void print(const String &);
  void print(int);
  void print(unsigned int);
  void print(long);
  void print(unsigned long);
  void print(long long);
  void print(unsigned long long);
  void print(double);
  void print(char);
  void print(unsigned char);
  void print(const char[]);
  void print(struct tm *timeinfo, const char *format = NULL);
  void print(const Printable &);
  void print(const __FlashStringHelper *ifsh) {
    return print(reinterpret_cast<const char *>(ifsh));
  }

  void println(const String &);
  void println(int);
  void println(unsigned int);
  void println(long);
  void println(unsigned long);
  void println(long long);
  void println(unsigned long long);
  void println(double);
  void println(char);
  void println(unsigned char);
  void println(const char[]);
  void println(struct tm *timeinfo, const char *format = NULL);
  void println(const Printable &);
  void println(const __FlashStringHelper *ifsh) {
    return println(reinterpret_cast<const char *>(ifsh));
  }
};

#endif

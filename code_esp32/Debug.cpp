#include <TelnetStream.h>
#include <Arduino.h>
#include "WString.h"
#include "Printable.h"

#include "Debug.h"

Debug::Debug() {}

void Debug::print(const String &s) {
  Serial.print(s);
  TelnetStream.print(s);
}

void Debug::print(const char str[]) {
  Serial.print(str);
  TelnetStream.print(str);
}

void Debug::print(char c) {
  Serial.print(c);
  TelnetStream.print(c);
}

void Debug::print(unsigned char b) {
  Serial.print(b);
  TelnetStream.print(b);
}

void Debug::print(int n) {
  Serial.print(n);
  TelnetStream.print(n);
}

void Debug::print(unsigned int n) {
  Serial.print(n);
  TelnetStream.print(n);
}

void Debug::print(long n) {
  Serial.print(n);
  TelnetStream.print(n);
}

void Debug::print(unsigned long n) {
  Serial.print(n);
  TelnetStream.print(n);
}

void Debug::print(long long n) {
  Serial.print(n);
  TelnetStream.print(n);
}

void Debug::print(unsigned long long n) {
  Serial.print(n);
  TelnetStream.print(n);
}

void Debug::print(double n) {
  Serial.print(n);
  TelnetStream.print(n);
}

void Debug::print(struct tm *timeinfo, const char *format) {
  Serial.print(timeinfo, format);
  TelnetStream.print(timeinfo, format);
}

void Debug::print(const Printable &x) {
  Serial.print(x);
  TelnetStream.print(x);
}

void Debug::println(const String &s) {
  Serial.println(s);
  TelnetStream.println(s);
}

void Debug::println(const char str[]) {
  Serial.println(str);
  TelnetStream.println(str);
}

void Debug::println(char c) {
  Serial.println(c);
  TelnetStream.println(c);
}

void Debug::println(unsigned char b) {
  Serial.println(b);
  TelnetStream.println(b);
}

void Debug::println(int n) {
  Serial.println(n);
  TelnetStream.println(n);
}

void Debug::println(unsigned int n) {
  Serial.println(n);
  TelnetStream.println(n);
}

void Debug::println(long n) {
  Serial.println(n);
  TelnetStream.println(n);
}

void Debug::println(unsigned long n) {
  Serial.println(n);
  TelnetStream.println(n);
}

void Debug::println(long long n) {
  Serial.println(n);
  TelnetStream.println(n);
}

void Debug::println(unsigned long long n) {
  Serial.println(n);
  TelnetStream.println(n);
}

void Debug::println(double n) {
  Serial.println(n);
  TelnetStream.println(n);
}

void Debug::println(struct tm *timeinfo, const char *format) {
  Serial.println(timeinfo, format);
  TelnetStream.println(timeinfo, format);
}

void Debug::println(const Printable &x) {
  Serial.println(x);
  TelnetStream.println(x);
}

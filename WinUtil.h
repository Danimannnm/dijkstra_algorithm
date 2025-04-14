#pragma once

#include <Windows.h>
#include <string>

// Like assert, but displays a Message Box on assertion failure
#define MB_ASSERT(val) mb_assert(val, __FILE__, __LINE__)

// Displays the given message, along with the filename and line number of its
// origin
#define MB_WRITE(str) mb_write(str, __FILE__, __LINE__)

// Not intended to be called directly, use the MB_ASSERT macro instead
void mb_assert(bool val, const char *filename, int line_no);

// Not intended to be called directly, use the MB_WRITE macro instead
void mb_write(const char *msg, const char *filename, int line_no);

short rect_width(SMALL_RECT &rect);
long rect_width(RECT &rect);
short rect_height(SMALL_RECT &rect);
long rect_height(RECT &rect);

void clear(HANDLE h_output);

class ConsoleWriter {
  const HANDLE h_console;

public:
  ConsoleWriter(const HANDLE h_console);

  ConsoleWriter &write(const std::wstring &val);
  ConsoleWriter &write(const wchar_t *val);
  ConsoleWriter &write(int val);
  ConsoleWriter &write(long val);
  ConsoleWriter &write(short val);
  ConsoleWriter &write(unsigned int val);
  ConsoleWriter &write(unsigned long val);
  ConsoleWriter &write(unsigned short val);
  ConsoleWriter &write(size_t val);

  ConsoleWriter &writeline(const std::wstring &val);
  ConsoleWriter &writeline(const wchar_t *val);
  ConsoleWriter &writeline(int val);
  ConsoleWriter &writeline(long val);
  ConsoleWriter &writeline(short val);
  ConsoleWriter &writeline(unsigned int val);
  ConsoleWriter &writeline(unsigned long val);
  ConsoleWriter &writeline(unsigned short val);
  ConsoleWriter &writeline(size_t val);

  ConsoleWriter &writeline();
};

COORD get_console_dimensions(HANDLE h_output);

void hide_cursor(HANDLE h_output);

enum FG_COLORS : unsigned short {
  FG_BLACK = 0,
  FG_BLUE = 1,
  FG_GREEN = 2,
  FG_CYAN = 3,
  FG_RED = 4,
  FG_MAGENTA = 5,
  FG_BROWN = 6,
  FG_LIGHTGRAY = 7,
  FG_GRAY = 8,
  FG_LIGHTBLUE = 9,
  FG_LIGHTGREEN = 10,
  FG_LIGHTCYAN = 11,
  FG_LIGHTRED = 12,
  FG_LIGHTMAGENTA = 13,
  FG_YELLOW = 14,
  FG_WHITE = 15
};

enum BG_COLORS : unsigned short {
  BG_NAVYBLUE = 16,
  BG_GREEN = 32,
  BG_TEAL = 48,
  BG_MAROON = 64,
  BG_PURPLE = 80,
  BG_OLIVE = 96,
  BG_SILVER = 112,
  BG_GRAY = 128,
  BG_BLUE = 144,
  BG_LIME = 160,
  BG_CYAN = 176,
  BG_RED = 192,
  BG_MAGENTA = 208,
  BG_YELLOW = 224,
  BG_WHITE = 240
};

bool is_alphanumeric_vk_code(unsigned short vk_code);

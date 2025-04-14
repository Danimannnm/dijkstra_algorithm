#include "WinUtil.h"
#include <string>
#include <winuser.h>

using namespace std::string_literals;

void mb_assert(bool val, const char *filename, int line_no) {
  if (!val) {
    DWORD err = GetLastError();
    MessageBoxA(NULL,
                ("File: "s + filename + "\nLine: "s + std::to_string(line_no) +
                 "\nError: "s + std::to_string(err))
                    .c_str(),
                NULL, MB_OK | MB_ICONERROR);
  }
}

void mb_write(const char *msg, const char *filename, int line_no) {
  MessageBoxA(NULL,
              ("File: "s + filename + "\nLine: "s + std::to_string(line_no) +
               "\n\n"s + msg)
                  .c_str(),
              NULL, MB_OK | MB_ICONINFORMATION);
}

short rect_width(SMALL_RECT &rect) { return rect.Right - rect.Left; }

long rect_width(RECT &rect) { return rect.right - rect.left; }

short rect_height(SMALL_RECT &rect) { return rect.Bottom - rect.Top; }

long rect_height(RECT &rect) { return rect.bottom - rect.top; }

// Adapted from example from MSDN
// (https://learn.microsoft.com/en-us/windows/console/clearing-the-screen#example-2)
void clear(HANDLE h_output) {
  // Get the number of character cells in the current buffer.
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  MB_ASSERT(GetConsoleScreenBufferInfo(h_output, &csbi));

  // Scroll the rectangle of the entire buffer.
  SMALL_RECT scrollRect{0, 0, csbi.dwSize.X, csbi.dwSize.Y};

  // Scroll it upwards off the top of the buffer with a magnitude of the entire
  // height.
  COORD scrollTarget{0, static_cast<short>(0 - csbi.dwSize.Y)};

  // Fill with empty spaces with the buffer's default text attribute.
  CHAR_INFO fill{};
  fill.Char.UnicodeChar = L' ';
  fill.Attributes = csbi.wAttributes;

  // Do the scroll
  MB_ASSERT(ScrollConsoleScreenBufferW(h_output, &scrollRect, NULL,
                                       scrollTarget, &fill));

  MB_ASSERT(SetConsoleCursorPosition(h_output, {0, 0}));
}

ConsoleWriter::ConsoleWriter(const HANDLE h_console) : h_console{h_console} {}

ConsoleWriter &ConsoleWriter::write(const std::wstring &val) {
  WriteConsoleW(h_console, val.c_str(), static_cast<DWORD>(val.size()), nullptr,
                nullptr);
  return *this;
}

ConsoleWriter &ConsoleWriter::write(const wchar_t *val) {
  WriteConsoleW(h_console, val, static_cast<DWORD>(wcslen(val)), nullptr,
                nullptr);
  return *this;
}

ConsoleWriter &ConsoleWriter::write(int val) {
  return write(std::to_wstring(val));
}

ConsoleWriter &ConsoleWriter::write(long val) {
  return write(std::to_wstring(val));
}

ConsoleWriter &ConsoleWriter::write(short val) {
  return write(std::to_wstring(val));
}

ConsoleWriter &ConsoleWriter::write(unsigned int val) {
  return write(std::to_wstring(val));
}

ConsoleWriter &ConsoleWriter::write(unsigned long val) {
  return write(std::to_wstring(val));
}

ConsoleWriter &ConsoleWriter::write(unsigned short val) {
  return write(std::to_wstring(val));
}

ConsoleWriter &ConsoleWriter::write(size_t val) {
  return write(std::to_wstring(val));
}

ConsoleWriter &ConsoleWriter::writeline(const std::wstring &val) {
  return write(val).write(L"\n");
}

ConsoleWriter &ConsoleWriter::writeline(const wchar_t *val) {
  return write(val).write(L"\n");
}

ConsoleWriter &ConsoleWriter::writeline(int val) {
  return write(std::to_wstring(val)).writeline();
}

ConsoleWriter &ConsoleWriter::writeline(long val) {
  return write(std::to_wstring(val)).writeline();
}

ConsoleWriter &ConsoleWriter::writeline(short val) {
  return write(std::to_wstring(val)).writeline();
}

ConsoleWriter &ConsoleWriter::writeline(unsigned int val) {
  return write(std::to_wstring(val)).writeline();
}

ConsoleWriter &ConsoleWriter::writeline(unsigned long val) {
  return write(std::to_wstring(val)).writeline();
}

ConsoleWriter &ConsoleWriter::writeline(unsigned short val) {
  return write(std::to_wstring(val)).writeline();
}

ConsoleWriter &ConsoleWriter::writeline(size_t val) {
  return write(std::to_wstring(val)).writeline();
}

ConsoleWriter &ConsoleWriter::writeline() { return write(L"\n"); }

COORD get_console_dimensions(HANDLE h_output) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  MB_ASSERT(GetConsoleScreenBufferInfo(h_output, &csbi));
  return csbi.dwSize;
}

bool is_alphanumeric_vk_code(unsigned short vk_code) {
  return (vk_code >= '0' && vk_code <= '9') ||
         (vk_code >= 'A' && vk_code <= 'Z') ||
         (vk_code >= VK_NUMPAD0 && vk_code <= VK_NUMPAD9) ||
         vk_code == VK_SPACE;
}
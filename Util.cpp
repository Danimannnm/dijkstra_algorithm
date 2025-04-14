#include "Util.h"
#include "WinUtil.h"
#include <codecvt>
#include <fstream>
#include <ios>
#include <memory>

// Intended to convert ASCII to Unicode
std::wstring reinterpret_as_wstring(const std::string &str) {
  auto result = L""s;
  for (auto ch : str) {
    result += ch;
  }
  return result;
}

// Copies a wchar_t string into the specified location in the provided buffer,
// discarding the null terminator of the string.
void insert_wstr_in_buffer(wchar_t *dest, const wchar_t *src) {
  auto dest_len = wcslen(dest);
  auto src_len = wcslen(src);

  MB_ASSERT(dest_len >= src_len);

  for (size_t i = 0; i < src_len; ++i) {
    dest[i] = src[i];
  }
}

void insert_wstr_in_buffer(wchar_t *dest, const std::wstring &src) {
  auto dest_len = wcslen(dest);
  auto src_len = src.size();

  MB_ASSERT(dest_len >= src_len);

  for (size_t i = 0; i < src_len; ++i) {
    dest[i] = src[i];
  }
}

std::wstring pad_right(std::wstring str, size_t width, wchar_t padding_char) {
  if (str.size() > width) {
    str = str.substr(0, width - 3) + L"...";
  } else {
    str.resize(width, padding_char);
  }
  return str;
}

std::wstring pad_left(std::wstring str, size_t width, wchar_t padding_char) {
  if (str.size() > width) {
    str = str.substr(0, width - 3) + L"...";
  } else {
    str.insert(0, width - str.size(), padding_char);
  }
  return str;
}

// Ignores any newlines
// Ignores BOM if present
// If number of characters read is less than size, the destination is padded
// with spaces
void load_char_buffer(wchar_t *dest, size_t size, const char *src) {
  std::wifstream file{src, std::ios::binary};

  MB_ASSERT(file.is_open());

  // Set the locale to read UTF-16LE encoded file and consume BOM if present
  file.imbue(std::locale(
      file.getloc(),
      new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));

  wchar_t ch;
  size_t i{0};
  while (i < size && file.get(ch)) {
    if (ch != L'\n') { // Ignore newline characters
      dest[i++] = ch;
    }
  }

  while (i < size) {
    dest[i++] = L' ';
  }
}

void load_color_buffer(WORD *dest, size_t size, const char *src) {
  std::ifstream file{src, std::ios::binary};
  MB_ASSERT(file.is_open());

  std::unique_ptr<char[]> temp{new char[size]};
  file.read(temp.get(), size);

  for (size_t i{0}; i < size; ++i) {
    dest[i] = static_cast<WORD>(temp[i]);
  }
}

// Round string containing number to 1dp
std::wstring round_to_1dp(const std::wstring &str) {
  const auto point = str.find(L'.');
  if (point != std::wstring::npos && str.length() > point + 2) {
    return str.substr(0, point + 2);
  }
  return str;
}

unsigned char random_byte() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);
  return static_cast<unsigned char>(dis(gen));
}

// Gets the absolute difference between two size_t's
size_t abs_diff(size_t a, size_t b) {
  if (a > b) {
    return a - b;
  }
  return b - a;
}

// Check if the specified file exists
bool file_exists(const std::string &filename) {
  std::ifstream file{filename};
  return file.good();
}

// Creates the specified file if it does not exist
void create_file_if_not_exists(const std::string &filename) {
  if (!file_exists(filename)) {
    std::ofstream file{filename};
    MB_ASSERT(file.good());
  }
}

long double tolerant_floor(long double n, long double threshold) {
  if (n >= ceill(n) - threshold) {
    return ceill(n);
  } else {
    return floorl(n);
  }
}

size_t discretize(long double n) {
  return static_cast<size_t>(tolerant_floor(n));
}
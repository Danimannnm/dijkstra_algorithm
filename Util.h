#pragma once

#include <fstream>
#include <random>
#include <string>

using namespace std::string_literals;

// Intended to convert ASCII to Unicode
std::wstring reinterpret_as_wstring(const std::string &str);

// Copies a wchar_t string into the specified location in the provided buffer.
// Discards the null terminator.
void insert_wstr_in_buffer(wchar_t *dest, const wchar_t *src);

void insert_wstr_in_buffer(wchar_t *dest, const std::wstring &src);

std::wstring pad_right(std::wstring str, size_t width,
                       wchar_t padding_char = L' ');

std::wstring pad_left(std::wstring str, size_t width,
                      wchar_t padding_char = L' ');

void load_char_buffer(wchar_t *dest, size_t size, const char *src);

void load_color_buffer(unsigned short *dest, size_t size, const char *src);

// Fills a rectangular region of the buffer with the specified character
// Accommodates partially overlapping rectangles
template <typename T>
void fill_rect(T *dest, size_t size, size_t stride, size_t start_idx, size_t w,
               size_t h, T ch) {
  if (size == 0 || w == 0 || h == 0) {
    // Handle invalid input or empty buffer
    return;
  }

  for (size_t i = 0; i < h; ++i) {
    for (size_t j = 0; j < w; ++j) {
      size_t row = start_idx / stride + i;
      size_t col = start_idx % stride + j;

      if (row < size / stride && col < stride) {
        dest[row * stride + col] = ch;
      }
    }
  }
}

// Fills a rectangular region of the buffer with data from another buffer
// Accommodates partially overlapping rectangles
template <typename T>
void insert_rect(T *dest, size_t size, size_t stride, size_t start_idx,
                 size_t w, size_t h, const T *src) {
  if (size == 0 || w == 0 || h == 0) {
    // Handle invalid input or empty buffer
    return;
  }

  for (size_t i = 0; i < h; ++i) {
    for (size_t j = 0; j < w; ++j) {
      size_t row = start_idx / stride + i;
      size_t col = start_idx % stride + j;

      if (row < size / stride && col < stride) {
        dest[row * stride + col] = src[i * w + j];
      }
    }
  }
}

std::wstring round_to_1dp(const std::wstring &str);

unsigned char random_byte();

// Gets the absolute difference between two size_t's
size_t abs_diff(size_t a, size_t b);

bool file_exists(const std::string &filename);

void create_file_if_not_exists(const std::string &filename);

long double tolerant_floor(long double n, long double threshold = 0.00000001);

size_t discretize(long double n);

// Write bytes to file
template <typename T>
void write_bytes(const std::string &filename, const T *data, size_t len = 1) {
  std::ofstream file{filename, std::ios::binary};
  MB_ASSERT(file.is_open());
  file.write(reinterpret_cast<const char *>(data), sizeof(T) * len);
}

// Append bytes to file
template <typename T>
void append_bytes(const std::string &filename, const T *data, size_t len = 1) {
  create_file_if_not_exists(filename);
  std::ofstream file{filename, std::ios::binary | std::ios::app};
  MB_ASSERT(file.is_open());
  file.write(reinterpret_cast<const char *>(data), sizeof(T) * len);
}

// Read bytes from file
// Returns the number of T's actually read
template <typename T>
size_t read_bytes(const std::string &filename, T *data, size_t len = 1) {
  std::ifstream file{filename, std::ios::binary};
  MB_ASSERT(file.is_open());
  file.read(reinterpret_cast<char *>(data), sizeof(T) * len);
  return file.gcount() / sizeof(T);
}
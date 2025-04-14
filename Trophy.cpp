#pragma once

#include "Trophy.h"

// High four bites represent prefix, low four bits represent suffix
std::wstring get_trophy_string(unsigned char trophy) {
  auto prefix = trophy >> 4;
  auto suffix = trophy & 0x0F;

  return trophy_prefixes[prefix] + L" " + trophy_suffixes[suffix];
}

void add_trophy() {
  std::ofstream file{trophy_filename, std::ios::app | std::ios::binary};
  MB_ASSERT(file.is_open());
  file.put(random_byte());
  file.close();
};

DoublyLinkedList<std::wstring> get_trophies() {
  auto result = DoublyLinkedList<std::wstring>{};

  std::ifstream file{trophy_filename, std::ios::binary};
  MB_ASSERT(file.is_open());

  size_t i{0};
  while (file.peek() != EOF && i < 15) {
    auto trophy = file.get();
    result.push_back(get_trophy_string(static_cast<unsigned char>(trophy)));
    ++i;
  }

  file.close();

  return result;
}
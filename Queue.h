#pragma once

#include "DoublyLinkedList.h"
#include "WinUtil.h"

// NOTE: Due to limitations of the C++ language, we cannot separate the
// declaration of a template class from its implementation. Hence this class
// has to be completely placed in this header file
template <typename T> class Queue {
  DoublyLinkedList<T> dll{};

public:
  Queue() = default;
  Queue(const Queue<T> &other) : dll{other.dll} {}
  Queue(Queue<T> &&other) : dll{std::move(other.dll)} {}

  Queue(std::initializer_list<T> init_list) : dll{init_list} {}

  void enqueue(T data) { dll.push_back(data); }

  T dequeue() {
    MB_ASSERT(dll.size() > 0);
    return dll.pop_front();
  }

  T front() {
    MB_ASSERT(dll.size() > 0);
    return dll.get(0);
  }

  T back() {
    MB_ASSERT(dll.size() > 0);
    return dll.get(dll.size() - 1);
  }

  size_t size() const { return dll.size(); }

  bool empty() const { return dll.empty(); }

  void clear() { dll.clear(); }

  // Gets the underlying doubly linked list
  DoublyLinkedList<T> &list() { return dll; }
};
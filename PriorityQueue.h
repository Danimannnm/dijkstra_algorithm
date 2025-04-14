#pragma once

#include "DoublyLinkedList.h"
#include "WinUtil.h"

template <typename T> class PriorityQueue {
  struct Element {
    T data;
    int priority;
  };

  DoublyLinkedList<Element> dll{};

public:
  PriorityQueue() = default;
  PriorityQueue(const PriorityQueue<T> &other) : dll{other.dll} {}
  PriorityQueue(PriorityQueue<T> &&other) : dll{std::move(other.dll)} {}

  PriorityQueue(std::initializer_list<T> init_list) {
    for (auto &data : init_list) {
      enqueue(data, 0);
    }
  }

  void enqueue(T data, int priority) {
    Element e{data, priority};
    if (dll.empty()) {
      dll.push_back(e);
    } else {
      size_t i = 0;
      for (; i < dll.size(); i++) {
        if (dll.get(i).priority < priority) {
          dll.insert(e, i);
          break;
        }
      }
      if (i == dll.size()) {
        dll.push_back(e);
      }
    }
  }

  T dequeue() {
    MB_ASSERT(dll.size() > 0);
    return dll.pop_front().data;
  }

  T front() {
    MB_ASSERT(dll.size() > 0);
    return dll.get(0).data;
  }

  T back() {
    MB_ASSERT(dll.size() > 0);
    return dll.get(dll.size() - 1).data;
  }

  size_t size() const { return dll.size(); }

  bool empty() const { return dll.empty(); }

  void clear() { dll.clear(); }

  // Gets the underlying doubly linked list
  DoublyLinkedList<Element> &list() { return dll; }
};

#pragma once

#include "WinUtil.h"
#include <initializer_list>

// NOTE: Due to limitations of the C++ language, we cannot separate the
// declaration of a template class from its implementation. Hence this class
// has to be completely placed in this header file
template <typename T> class DoublyLinkedList {
  struct Node {
    T data;
    Node *next{nullptr};
    Node *prev{nullptr};
  };

  Node *head{nullptr};
  Node *tail{nullptr};
  size_t m_size = 0;

public:
  DoublyLinkedList() = default;
  DoublyLinkedList(const DoublyLinkedList<T> &other) {
    Node *current{other.head};
    while (current != nullptr) {
      push_back(current->data);
      current = current->next;
    }
  }
  DoublyLinkedList(DoublyLinkedList<T> &&other) {
    head = other.head;
    tail = other.tail;
    m_size = other.m_size;
    other.head = nullptr;
    other.tail = nullptr;
    other.m_size = 0;
  }

  DoublyLinkedList(std::initializer_list<T> init_list) {
    for (auto &item : init_list) {
      push_back(item);
    }
  }

  // Copy assignment operator
  DoublyLinkedList<T> &operator=(const DoublyLinkedList<T> &other) {
    if (this != &other) {
      clear();
      for (auto &item : other) {
        push_back(item);
      }
    }
    return *this;
  }

  void push_back(T data) {
    if (head == nullptr) {
      head = tail = new Node{data};
    } else {
      tail->next = new Node{data};
      tail->next->prev = tail;
      tail = tail->next;
    }
    ++m_size;
  }

  void push_front(T data) {
    if (head == nullptr) {
      head = tail = new Node{data};
    } else {
      head->prev = new Node{data};
      head->prev->next = head;
      head = head->prev;
    }
    ++m_size;
  }

  T pop_back() {
    MB_ASSERT(head != nullptr && tail != nullptr);

    T data = tail->data;
    if (head == tail) {
      delete tail;
      head = tail = nullptr;
    } else {
      tail = tail->prev;
      delete tail->next;
      tail->next = nullptr;
    }

    --m_size;

    return data;
  }

  T pop_front() {
    MB_ASSERT(head != nullptr && tail != nullptr);

    T data = head->data;
    if (head == tail) {
      delete head;
      head = tail = nullptr;
    } else {
      head = head->next;
      delete head->prev;
      head->prev = nullptr;
    }

    --m_size;

    return data;
  }

  void insert(T data, size_t index) {
    MB_ASSERT(index <= m_size);

    if (index == 0) {
      push_front(data);
    } else if (index == m_size) {
      push_back(data);
    } else {
      Node *current{head};
      for (size_t i{0}; i < index; i++) {
        current = current->next;
      }
      Node *new_node = new Node{data};
      new_node->next = current;
      new_node->prev = current->prev;
      current->prev->next = new_node;
      current->prev = new_node;
      ++m_size;
    }
  }

  void remove(size_t index) {
    MB_ASSERT(index < m_size);

    if (index == 0) {
      pop_front();
    } else if (index == m_size - 1) {
      pop_back();
    } else {
      Node *current{head};
      for (size_t i{0}; i < index; i++) {
        current = current->next;
      }
      current->prev->next = current->next;
      current->next->prev = current->prev;
      delete current;
      --m_size;
    }
  }

  void clear() {
    Node *current{head};
    while (current != nullptr) {
      Node *temp{current};
      current = current->next;
      delete temp;
    }
    head = tail = nullptr;
    m_size = 0;
  }

  T &get(size_t index) const {
    MB_ASSERT(index < m_size);

    Node *current{head};
    for (size_t i{0}; i < index; i++) {
      current = current->next;
    }
    return current->data;
  }

  size_t size() const { return m_size; }

  bool empty() const { return m_size == 0; }

  // Set-like operations
  bool contains(const T &data) const {
    Node *current{head};
    while (current != nullptr) {
      if (current->data == data) {
        return true;
      }
      current = current->next;
    }
    return false;
  }

  ~DoublyLinkedList() { clear(); }

  // Iterator class for DoublyLinkedList
  // This will allow us to iterate over our lists using C++'s range-based
  // for-loop
  class Iterator {
  private:
    Node *current;

  public:
    Iterator(Node *node) : current(node) {}

    T &operator*() { return current->data; }

    Iterator &operator++() {
      current = current->next;
      return *this;
    }

    bool operator!=(const Iterator &other) { return current != other.current; }
  };

  // Create and return an iterator pointing to the beginning of the list
  Iterator begin() { return Iterator(head); }

  // Create and return an iterator indicating the end of the list
  Iterator end() { return Iterator(nullptr); }

  // Create and return a const iterator pointing to the beginning of the list
  // (for const instances)
  const Iterator begin() const { return Iterator(head); }

  // Create and return a const iterator indicating the end of the list (for
  // const instances)
  const Iterator end() const { return Iterator(nullptr); }
};
#pragma once

template <typename T> void swap(T &a, T &b) {
  if (&a == &b)
    return;
  T temp{a};
  a = b;
  b = temp;
}

template <typename T>
void insertion_sort(T *arr, const size_t sz, const size_t cursor = 1) {
  if (sz == 0 || cursor == sz)
    return;

  // Keep swapping the insertee with the element before it until it reaches the
  // subarray [0..cursor] is sorted
  for (size_t i{cursor}; i > 0; --i) {
    if (arr[i - 1] > arr[i]) {
      swap(arr[i], arr[i - 1]);
    } else {
      break;
    }
  }
  return insertion_sort(arr, sz, cursor + 1);
}

template <typename T> void reverse(T *arr, const size_t sz) {
  if (sz == 0)
    return;

  for (size_t i{0}; i < sz / 2; ++i) {
    swap(arr[i], arr[sz - i - 1]);
  }
}
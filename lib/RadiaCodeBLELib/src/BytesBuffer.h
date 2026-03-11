#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>  // for memcpy and size_t

class BytesBuffer {
 private:
  size_t start;
  size_t end;
  uint8_t* data;
  size_t capacity;

 public:
  BytesBuffer(size_t capacity);
  ~BytesBuffer();

  void clear();
  size_t size();
  bool empty();
  uint8_t at(size_t place);
  void pop_back();  // for hacky thing
  void fill(const uint8_t* new_data, size_t len);
  void copy(BytesBuffer& other);
  void drain(uint8_t* output, size_t len);
  void print();
  void printAll();

  template <typename T>
  T consume() {
    T res;
    if (capacity - start >= sizeof(T)) {
      memcpy(&res, data + start, sizeof(T));
      start += sizeof(T);
      return res;
    } else {
      // if start is at the edge
      // copy the first bits
      size_t first_bits = capacity - start;
      memcpy(&res, data + start, first_bits);
      start = 0;  // wrap around to the beginning
      // then the last bits
      memcpy(((uint8_t*)&res) + first_bits, data + start,
             sizeof(T) - first_bits);
      start += sizeof(T) - first_bits;
      return res;
    }
  }
};

#endif
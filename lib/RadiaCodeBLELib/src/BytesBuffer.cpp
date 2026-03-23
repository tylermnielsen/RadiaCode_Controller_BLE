#include "BytesBuffer.h"

#include <Arduino.h>  // for print and printAll

#ifndef BytesBuffer_printf
#define BytesBuffer_printf(...) Serial.printf(__VA_ARGS__)
#endif

BytesBuffer::BytesBuffer(size_t capacity) {
  this->start = 0;
  this->end = 0;
  this->capacity = capacity;
  this->data = new uint8_t[capacity];
}

BytesBuffer::~BytesBuffer() { delete[] data; }

void BytesBuffer::clear() {
  start = 0;
  end = 0;
}

size_t BytesBuffer::size() {
  if (start <= end)
    return end - start;
  else
    return capacity - start + end;
}

bool BytesBuffer::empty() { return start == end; }

uint8_t BytesBuffer::at(size_t place) {
  size_t it = start + place;
  if (it >= capacity) {
    return data[it - capacity];
  } else {
    return data[it];
  }
}

void BytesBuffer::pop_back() {
  if (end == 0) {
    end = capacity - 1;
  } else {
    end--;
  }
}

void BytesBuffer::fill(const uint8_t* new_data, size_t len) {
  if (this->size() + len >= this->capacity) {
    BytesBuffer_printf("ERROR: Buffer overflow\n");
    return;
  }
  // fill from start
  if (capacity - end >= len) {
    memcpy(data + end, new_data, len);
    end += len;
  } else {
    // fill across boundary
    // copy the first bits
    size_t first_part = capacity - end;
    memcpy(data + end, new_data, first_part);
    end = 0;  // wrap around to the beginning
    // then the last bits
    memcpy(data + end, new_data + first_part, len - first_part);
    end += len - first_part;
  }
  // for(int i = 0; i < capacity; i++){
  //   printf("%02x", this->data[i]);
  // }
  // printf("\n");
}

void BytesBuffer::drain(uint8_t* output, size_t len) {
  if (capacity - start >= len) {
    if (output != nullptr) memcpy(output, data + start, len);
    start += len;
  } else {
    // if start is at the edge
    // copy the first bits
    size_t first_bits = capacity - start;
    if (output != nullptr) memcpy(output, data + start, first_bits);
    start = 0;  // wrap around to the beginning
    // then the last bits
    if (output != nullptr)
      memcpy(output + first_bits, data + start, len - first_bits);
    start += len - first_bits;
  }
}

void BytesBuffer::copy(BytesBuffer& other) {
  // capacity must be the same for this
  this->start = other.start;
  this->end = other.end;
  memcpy(this->data, other.data, this->capacity);
}

void BytesBuffer::print() {
  BytesBuffer_printf("start: %u end: %u capacity: %u\n", start, end, capacity);
  int i = start;
  while (i != end) {
    BytesBuffer_printf("%02x ", this->data[i]);
    i++;
    if (i >= capacity) i = 0;
  }
}

void BytesBuffer::printAll() {
  for (int i = 0; i < capacity; i++) {
    BytesBuffer_printf("%02x ", this->data[i]);
  }
}
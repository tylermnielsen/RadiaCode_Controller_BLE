#include "BytesBuffer.h"
#include <stdio.h>

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

void BytesBuffer::fill(uint8_t* new_data, size_t len) {
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

#include "BytesBuffer.h"

BytesBuffer::BytesBuffer(size_t capacity){
  this->start = 0; 
  this->end = 0; 
  this->capacity = capacity;
  this->data = new uint8_t[capacity]; 
}

BytesBuffer::~BytesBuffer(){
  delete[] this->data; 
}

void BytesBuffer::clear(){
  this->start = 0; 
  this->end = 0; 
}

size_t BytesBuffer::size(){
  if(this->start <= this->end) return this->end - this->start; 
  else return this->capacity - this->start + this->end; 
}

void BytesBuffer::fill(uint8_t* new_data, size_t len){
  // replace whatever is in the buffer 
  this->start = 0; 
  memcpy(data, new_data, len); 
  this->end = len; 
}

template <typename T> 
T BytesBuffer::consume(){
  T res;
  if(capacity - start >= sizeof(T)){
    memcpy(&res, start, sizeof(T)); 
    start += sizeof(T); 
    return res; 
  } else {
    // if start is at the edge 
    // copy the first bits
    memcpy(&res, start, capacity - start);
    start = 0; // wrap around to the beginning  
    // then the last bits
    memcpy(&res + (capacity - start), start, sizeof(T) - (capacity - start)); 
    start += sizeof(T) - (capacity - start); 
    return res; 
  }
}
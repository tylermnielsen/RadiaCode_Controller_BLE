#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <string.h> // for memcpy and size_t 

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
    void fill(uint8_t* new_data, size_t len); 

    template <typename T> 
    T consume(); 
};

#endif 
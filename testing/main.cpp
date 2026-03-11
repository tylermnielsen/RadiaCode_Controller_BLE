#include <stdint.h>

#include <iostream>

#include "BytesBuffer.h"

using namespace std;

int main() {
  BytesBuffer bb(13);

  // simple bytes
  uint8_t b1[] = {1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10};
  for (size_t j = 0; j < 10; j++) {
    bb.fill(b1, sizeof(b1));
    for (size_t i = 0; i < sizeof(b1); i++) {
      uint8_t res = bb.consume<uint8_t>();
      if (res != b1[i])
        cout << "failed " << to_string(res) << " != " << to_string(b1[i])
             << endl;
    }
  }

  cout << bb.size() << endl;

  for (int i = 0; i < 5; i++) {
    uint8_t b2[10];
    uint32_t x = 33;
    uint16_t y = 11;
    float z = 44.3;
    memcpy(b2, &x, sizeof(uint32_t));
    memcpy(b2 + sizeof(uint32_t), &y, sizeof(uint16_t));
    memcpy(b2 + sizeof(uint32_t) + sizeof(uint16_t), &z, sizeof(float));

    bb.fill(b2, 10);
    uint32_t res_x = bb.consume<uint32_t>();
    uint16_t res_y = bb.consume<uint16_t>();
    float res_z = bb.consume<float>();

    cout << x << " ?= " << res_x << endl;
    cout << y << " ?= " << res_y << endl;
    cout << z << " ?= " << res_z << endl;
  }

  cout << "Done." << endl;

  return 0;
}
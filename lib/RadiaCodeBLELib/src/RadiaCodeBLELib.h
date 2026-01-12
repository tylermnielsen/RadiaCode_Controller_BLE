#ifndef RADIACODE_BLE_LIB_H
#define RADIACODE_BLE_LIB_H

#include <Arduino.h>
#include <BTstackLib.h>
#include <stdint.h>

#include "BLEDriver.h"

class RadiaCodeBLEController {
 private:
  String target_mac;

  // scanning utility
  static void advertisementCallback(BLEAdvertisement* bleAd);

 public:
  RadiaCodeBLEController(String target_mac);
  void begin();
  void loop();

  // scanning utility
  static bool _only_named_devices;
  static void deviceScanUtility();
};

#endif  // RADIACODE_BLE_LIB_H
#include "RadiaCodeBLELib.h"

/**
 * @brief Setting for device scanner to only print devices that provide a name
 * in their advertisement
 *
 */
bool RadiaCodeBLEController::_only_named_devices = true;

/**
 * @brief Construct a new RadiaCode BLE Controller for the given MAC address
 *
 * @param target_mac MAC address of the target RadiaCode device
 */
RadiaCodeBLEController::RadiaCodeBLEController(String target_mac) {
  this->target_mac = target_mac;
  this->device = BD_ADDR(target_mac.c_str());
}

/**
 * @brief Device Scan Utility - scans for any BLE devices and prints their mac
 * addresses and info - NOT INTENDED FOR USE IN FINAL DESIGN, FOR FINDING MAC
 * ADDRESSES FOR LATER USE ONLY
 *
 * @param only_named_devices If true, only devices with names will be printed
 */
void RadiaCodeBLEController::deviceScanUtility() {
  BTstack.setBLEAdvertisementCallback(advertisementCallback);
  BTstack.setup();
  BTstack.bleStartScanning();

  while (1) {
    BTstack.loop();  // Process events
  }
}

void RadiaCodeBLEController::advertisementCallback(BLEAdvertisement* bleAd) {
  if (!(bleAd->isIBeacon())) {
    String device_name = "";

    const uint8_t* adv_data = bleAd->getAdvData();
    uint8_t len = adv_data[0];
    uint8_t type = adv_data[1];
    if (type == 0x09 || type == 0x08) {  // if data has a name field
      for (uint8_t i = 0; i < len - 1; i++) {
        device_name += (char)adv_data[i + 2];
      }
    } else {
      if (RadiaCodeBLEController::_only_named_devices) {
        return;  // skip unnamed devices if only_named_devices is true
      }
    }

    Serial.printf("Device discovered: %s - %s\n",
                  bleAd->getBdAddr()->getAddressString(), device_name.c_str());
    // const uint8_t * data = bleAd->getAdvData();
    // Serial.print("Advertisement data: ");
    // for (int i = 0; i < 41; i++) {
    //   Serial.printf("%c ", data[i]);
    // }
  }
}
#include <Arduino.h>

#include <BTstackLib.h>
#include <SPI.h>

/**
 * 1. Install the appropriate callbacks using the BluetoothHIDMaster::onXXX
 * methods
 * 2. Start the Bluetooth processing with BluetoothHIDMaster::begin() or
 * BluetoothHIDMaster::beginBLE()
 * 3. Connect to the first device found with BluetoothHIDMaster::connectXXX()
 * and start receiving callbacks.
 * 4. Callbacks will come at interrupt time and set global state variables,
 * which the main loop() will process
 *
 */

int counter = 0;
BD_ADDR device = BD_ADDR("52:43:06:60:17:dd");

void advertisementCallback(BLEAdvertisement *bleAd);

void setup() {
  while (!Serial)
    ;

  Serial.begin(115200);

  Serial.println("Starting Bluetooth HID Master");

  BTstack.setBLEAdvertisementCallback(advertisementCallback);
  BTstack.setup();
  BTstack.bleStartScanning();

  Serial.println("Setup Done.");
}

void loop() {
  BTstack.loop(); 
}

void advertisementCallback(BLEAdvertisement *bleAd) {
  if (!(bleAd->isIBeacon())) {
    // Serial.print("Device discovered: ");
    // Serial.println(bleAd->getBdAddr()->getAddressString());

    if (memcmp(bleAd->getBdAddr()->getAddress(), device.getAddress(),
               sizeof(device)) == 0) {
      counter++;
      Serial.printf("Found device %s, has been found %d times.\n",
                    device.getAddressString(), counter);
    }
  }
}
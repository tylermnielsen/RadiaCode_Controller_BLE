#include <Arduino.h>
#include <RadiaCodeBLELib.h>

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

String target_mac = "52:43:06:60:17:dd";
RadiaCodeBLEController rc(target_mac);

void advertisementCallback(BLEAdvertisement* bleAd);

void setup() {
  while (!Serial);

  Serial.begin(115200);

  Serial.println("Starting Bluetooth HID Master");

  rc.begin();

  Serial.println("Setup Done.");
}

void loop() { rc.loop(); }

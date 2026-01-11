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

int counter = 0;
BD_ADDR device = BD_ADDR("52:43:06:60:17:dd");

void advertisementCallback(BLEAdvertisement* bleAd);

void setup() {
  while (!Serial);

  Serial.begin(115200);

  Serial.println("Starting Bluetooth HID Master");

  RadiaCodeBLEController::_only_named_devices = true;
  RadiaCodeBLEController::deviceScanUtility();

  Serial.println("Setup Done.");
}

void loop() { BTstack.loop(); }

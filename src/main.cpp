#include <Arduino.h>
// #include <RadiaCodeBLELib.h>

#include <BLE.h>
#include "btstack.h"

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

#define RADIACODE_SERVICE_UUID "e63215e5-7003-49d8-96b0-b024798fb901"
#define RADIACODE_WRITE_FD_UUID "e63215e6-7003-49d8-96b0-b024798fb901"
#define RADIACODE_NOTIFY_FD_UUID "e63215e7-7003-49d8-96b0-b024798fb901"


String target_mac = "52:43:06:60:17:DD";
// RadiaCodeBLEController rc(target_mac);
// BLEAddress rc_addr(target_mac); 


BLEUUID rc_service_uuid(RADIACODE_SERVICE_UUID);
BLEUUID rc_write_uuid(RADIACODE_WRITE_FD_UUID);
BLEUUID rc_notify_uuid(RADIACODE_NOTIFY_FD_UUID);

BLERemoteCharacteristic* rc_write_char; 
BLERemoteCharacteristic*rc_notify_char; 


void notify(BLERemoteCharacteristic * c, const uint8_t * data, uint32_t len){
  (void) c; 

  Serial.println("Notify:");
  Serial.write(data, len); 
  Serial.println(); 
}

void setup() {
  Serial.begin(115200);

  while (!Serial);

  Serial.println("Starting BLE Client");

  BLE.begin(); 

  Serial.println("Scanning..."); 

  BLEScanReport* res = BLE.scan();

  for(BLEAdvertising item : *res){
    if(item.getAddress().toString() == target_mac){
      Serial.println("Found device, connecting...");
      if(BLE.client()->connect(item) == false){
        Serial.println("Connected."); 
      } 
      break;
    }
  }

  Serial.println("Done."); 

  Serial.println("Exploring service..."); 
  BLERemoteService* service = BLE.client()->service(rc_service_uuid); 

  if(service){

    rc_notify_char = service->characteristic(rc_notify_uuid);
    if(rc_notify_char){
      Serial.println("Found notify char"); 
      rc_notify_char->onNotify(notify); 
      rc_notify_char->enableNotifications(); 

    }
    
    rc_write_char = service->characteristic(rc_write_uuid); 
    if(rc_write_char){
      Serial.println("Found write char"); 
    }

  } else {
    Serial.println("Error on service"); 
  }

  Serial.println("Setup Done.");
}

void loop() {

}

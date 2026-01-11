#include <Arduino.h>

#include <BluetoothHCI.h>

/**
 * 1. Install the appropriate callbacks using the BluetoothHIDMaster::onXXX methods 
 * 2. Start the Bluetooth processing with BluetoothHIDMaster::begin() or BluetoothHIDMaster::beginBLE() 
 * 3. Connect to the first device found with BluetoothHIDMaster::connectXXX() and start receiving callbacks. 
 * 4. Callbacks will come at interrupt time and set global state variables, which the main loop() will process
 * 
 */

BluetoothHCI hci; 

void setup(){
  while(!Serial); 

  Serial.begin(115200);

  Serial.println("Starting Bluetooth HID Master"); 

  l2cap_init();
  gatt_client_init();
  sm_init();
  sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
  gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
  hci_set_master_slave_policy(HCI_ROLE_MASTER);
  hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);

  hci.setBLEName("Pico BLE Scanner");
  hci.install();
  hci.begin();


  Serial.println("Setup Done."); 
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.printf("BEGIN BLE SCAN @%lu ...", millis());
  auto l = hci.scanBLE(BluetoothHCI::any_cod);
  Serial.printf("END BLE SCAN @%lu\n\n", millis());
  Serial.printf("%-8s | %-17s | %-4s | %s\n", "Class", "Address", "RSSI", "Name");
  Serial.printf("%-8s | %-17s | %-4s | %s\n", "--------", "-----------------", "----", "----------------");
  for (auto e : l) {
    Serial.printf("%08lx | %17s | %4d | %s\n", e.deviceClass(), e.addressString(), e.rssi(), e.name());
  }
  Serial.printf("\n\n\n");
}

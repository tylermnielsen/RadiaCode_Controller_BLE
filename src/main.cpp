#include <Arduino.h>
// #include <RadiaCodeBLELib.h>
#include <stdint.h>
#include <BLE.h>
#include "btstack.h"
#include <vector>
#include "Config.h"

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

// day, month, year, 0, second, minute, hour, 0
uint8_t datetime[] = {9, 2, 26, 0, 0, 45, 1, 0};

String target_mac = "52:43:06:60:17:DD";
// RadiaCodeBLEController rc(target_mac);
// BLEAddress rc_addr(target_mac); 


BLEUUID rc_service_uuid(RADIACODE_SERVICE_UUID);
BLEUUID rc_write_uuid(RADIACODE_WRITE_FD_UUID);
BLEUUID rc_notify_uuid(RADIACODE_NOTIFY_FD_UUID);

BLERemoteCharacteristic* rc_write_char; 
BLERemoteCharacteristic*rc_notify_char; 

std::vector<uint8_t> _resp_buffer; 
std::vector<uint8_t> _response; 
size_t _resp_size = 0; 
void notify(BLERemoteCharacteristic * c, const uint8_t * data, uint32_t len){
  if(c == rc_notify_char){
    Serial.println("rc_notify_char notify");
    for(int i = 0; i < len; i++){
      Serial.printf("%02x ", data[i]);
    }
    Serial.println(); 
  }

  if(c == rc_write_char){
    Serial.println("rc_write_char notify"); 
  }

  if(_resp_size == 0){
    // read first 4 bytes as signed integer
    int size_buf;
    memcpy(&size_buf, data, sizeof(int));
    _resp_size = 4 + size_buf;
    // read in the rest of the bytes as data
    _resp_buffer = std::vector<uint8_t>(data+4, data+len);
  } else {
    // read in the bytes as data
    _resp_buffer.insert(_resp_buffer.end(), data, data+len);
  }

  // reduce size 
  _resp_size -= len;

  
  if(_resp_size == 0){
    // copied entire message
    Serial.println("message ended"); 
    // send to _response
    _response.insert(_response.end(), _resp_buffer.begin(), _resp_buffer.end());
    // wipe _resp_buffer
    _resp_buffer.clear();
  }
}

std::vector<uint8_t> ble_execute(uint8_t* data, size_t len) {
  for(int i = 0; i < len; i += 18){
    // write write_fd
    if(i * 18 + 18 < len){
      rc_write_char->setValue(data + i, 18); 
    } else {
      rc_write_char->setValue(data + i, len % 18); 
    }
  } 

  uint32_t start_time = millis(); 
  while (_response.empty()) {
    if(millis() - start_time > 5000){
      Serial.println("timeout"); 
      return _response;  
    }
  }

  std::vector<uint8_t> res(_response);
  _response.clear(); 

  return res; 
}

struct __attribute__((packed)) BLERequestHeader {
  uint32_t length; 
  uint16_t req_type; 
  uint8_t zero = 0; 
  uint8_t req_seq_no; 
};

static uint16_t seq = 0; 
std::vector<uint8_t> execute(uint16_t req_type, uint8_t* args, size_t len){
  Serial.println("execute"); 
  uint8_t req_seq_no = 0x80 + seq; 
  seq = (seq + 1) % 32; 

  BLERequestHeader header; 
  header.length = len + sizeof(BLERequestHeader) - 4; // length does not include itself 
  header.req_type = req_type; 
  header.zero = 0; 
  header.req_seq_no = req_seq_no; 

  //                 header                data
  uint8_t buffer[sizeof(BLERequestHeader) + len]; 
  memcpy(buffer, &header, sizeof(BLERequestHeader)); 
  if(args != NULL){
    memcpy(buffer + sizeof(BLERequestHeader), args, len); 
  }

  Serial.println("Sending buffer:"); 
  for(auto c : buffer){
    Serial.printf("%02x ", c);
  }
  Serial.println(); 

  std::vector<uint8_t> response = ble_execute(buffer, len+sizeof(BLERequestHeader)); 

  return response; 
}

void write_request(int command_id, uint8_t* data, size_t len){

  uint8_t buf[4 + len]; 
  memcpy(buf, &command_id, sizeof(int)); 
  memcpy(buf+sizeof(int), data, len); 

  std::vector<uint8_t> r = execute(Command::WR_VIRT_SFR, buf, (4 + len));

  if(r.empty()){
    Serial.println("No r, likely timeout"); 
    return; 
  }

  // read last 4 bytes into retcode 
  // match BytesBuffer behavior 
  uint32_t retcode = 0; 
  memcpy(&retcode, r.data() + r.size()-4, sizeof(uint32_t));

  if(retcode != 1){
    Serial.printf("Bad retcode %u\n", retcode);
  }
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

  uint8_t data[] = {0x01, 0xff, 0x12, 0xff}; 
  execute(Command::SET_EXCHANGE, data, sizeof(data));

  // set local time 
  execute(Command::SET_TIME, datetime, sizeof(datetime)); 

  // set device time 
  uint8_t time_setting[] = {0x00, 0x00, 0x00, 0x00};
  write_request(VSFR::DEVICE_TIME, time_setting, sizeof(time_setting));

  Serial.println("Setup Done.");
}

void loop() {

}

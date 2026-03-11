#include <Arduino.h>
// #include <RadiaCodeBLELib.h>
#include <stdint.h>
#include <BLE.h>
#include "btstack.h"
// #include <vector>
#include "Config.h"
#include <string> 

#include "BytesBuffer.h"

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
BLERemoteCharacteristic* rc_notify_char; 

// globally defined vectors with preallocation to avoid heap reallocation, will have 
// reserved capacity large enough for their use case
// 4000 >>> ~3200 from get config 
// static std::vector<uint8_t> _resp_buffer; // isr only 
static BytesBuffer _resp_buffer(4000); 
// static std::vector<uint8_t> _response; // transfer 
static BytesBuffer _response(4000); 
static mutex_t _response_mutex; 
volatile bool _response_ready = false; 
// static std::vector<uint8_t> res_ret; // main safe 
static BytesBuffer res_ret(4000); 
// static std::vector<int> spectrum; 

static void reserve_buffers(){
  mutex_init(&_response_mutex); 
  // accumulator for _response - max seen = 890 - per transfer max seen size = 20 
  // _resp_buffer.reserve(890 * 2);
  // max seen size = 890
  // _response.reserve(890 * 2);
  // max seen size = 890
  // res_ret.reserve(890 * 2);
  // max seen size = 1024
  // this is for spectrums which should always be 1024
  // spectrum.reserve(1024 + 200); 
}

static size_t _resp_size = 0; 
void notify(BLERemoteCharacteristic * c, const uint8_t * data, uint32_t len){
  // if(c == rc_notify_char){
  //   Serial.println("rc_notify_char notify");
  //   for(int i = 0; i < len; i++){
  //     Serial.printf("%02x ", data[i]);
  //   }
  //   Serial.println(); 
  // }

  // if(c == rc_write_char){
  //   Serial.println("rc_write_char notify"); 
  // }

  if(_resp_size == 0){
    // read first 4 bytes as signed integer
    int size_buf;
    memcpy(&size_buf, data, sizeof(int));
    _resp_size = 4 + size_buf;
    // read in the rest of the bytes as data
    // _resp_buffer = std::vector<uint8_t>(data+4, data+len);
    _resp_buffer.clear();
    // _resp_buffer.insert(_resp_buffer.end(), data+4, data+len); 
    _resp_buffer.fill(data+4, len-4); 
  } else {
    // read in the bytes as data
    // _resp_buffer.insert(_resp_buffer.end(), data, data+len);
    _resp_buffer.fill(data, len);
  }

  // reduce size 
  _resp_size -= len;

  
  if(_resp_size == 0){
    // copied entire message
    // Serial.println("message ended"); 
    // send to _response
    mutex_enter_blocking(&_response_mutex); 
    // _response.insert(_response.end(), _resp_buffer.begin(), _resp_buffer.end());
    _response.copy(_resp_buffer); 
    mutex_exit(&_response_mutex); 
    _response_ready = true; 

    // wipe _resp_buffer
    _resp_buffer.clear();
  }
}

BytesBuffer* ble_execute(uint8_t* data, size_t len) {
  for(int i = 0; i < len; i += 18){
    // write write_fd
    if(i * 18 + 18 < len){
      rc_write_char->setValue(data + i, 18); 
    } else {
      rc_write_char->setValue(data + i, len % 18); 
    }
  } 

  uint32_t start_time = millis(); 
  while (_response_ready == false) {
    if(millis() - start_time > 5000){
      Serial.println("timeout"); 
      return nullptr;  
    }
  }

  // std::vector<uint8_t> res(_response);
  res_ret.clear(); // reuse res_ret

  mutex_enter_blocking(&_response_mutex); 
  // res_ret.insert(res_ret.end(), _response.begin(), _response.end());
  res_ret.copy(_response); 
  _response.clear(); 
  mutex_exit(&_response_mutex); 
  _response_ready = false; 

  return &res_ret; 
}

struct __attribute__((packed)) BLERequestHeader {
  uint32_t length; 
  uint16_t req_type; 
  uint8_t zero = 0; 
  uint8_t req_seq_no; 
};

static uint16_t seq = 0; 
BytesBuffer* execute(uint16_t req_type, uint8_t* args, size_t len){
  // Serial.println("execute"); 
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

  // Serial.println("Sending buffer:"); 
  // for(auto c : buffer){
  //   Serial.printf("%02x ", c);
  // }
  // Serial.println(); 

  BytesBuffer* response = ble_execute(buffer, len+sizeof(BLERequestHeader)); 

  // check header 
  // get header
  uint16_t received_req_type = response->consume<uint16_t>(); 
  uint8_t received_zero = response->consume<uint8_t>(); 
  uint8_t received_req_seq_no = response->consume<uint8_t>(); 
  if(received_req_type != header.req_type || received_zero != header.zero || received_req_seq_no != header.req_seq_no){ //memcmp(response->data(), &(header.req_type), 4) != 0){
    Serial.println("Response header does not match request header!"); 
    Serial.printf("(expected|received) req_type: %u|%u zero: %u|%u req_seq_no: %u|%u\n", header.req_type, received_req_type, header.zero, received_zero, header.req_seq_no, received_req_seq_no);

    return nullptr; 
  }

  // remove compared header fields  
  // response->erase(response->begin(), response->begin() + 4);

  return response; 
}

void write_request(int command_id, uint8_t* data, size_t len){

  uint8_t buf[4 + len]; 
  memcpy(buf, &command_id, sizeof(int)); 
  memcpy(buf+sizeof(int), data, len); 

  BytesBuffer* r = execute(Command::WR_VIRT_SFR, buf, (4 + len));

  if(r->empty()){
    Serial.println("No r, likely timeout"); 
    return; 
  }

  // idk about this --> read last 4 bytes into retcode 
  // match BytesBuffer behavior 
  uint32_t retcode = r->consume<uint32_t>(); //0; 
  // memcpy(&retcode, r->data() + r->size()-4, sizeof(uint32_t));

  if(retcode != 1){
    Serial.printf("Bad retcode %u\n", retcode);
  }
}

BytesBuffer* read_request(uint32_t command_id){
  // Serial.println("read_request");
  BytesBuffer* r = execute(Command::RD_VIRT_STRING, (uint8_t*)&command_id, sizeof(int));
  
  if(r == nullptr) return nullptr; 

  uint32_t retcode = r->consume<uint32_t>(); 
  uint32_t flen = r->consume<uint32_t>(); 
  // memcpy(&retcode, r->data(), sizeof(int)); 
  // memcpy(&flen, r->data() + sizeof(int), sizeof(int));
  // r->erase(r->begin(), r->begin() + 8); // remove retcode and flen from buffer

  if(retcode != 1){
    Serial.printf("Bad retcode %x\n", retcode);
    Serial.printf("flen: %x\n", flen);
    for(int i = 0; i < r->size(); i++){
      Serial.printf("%02x ", r->at(i));
    }
    return nullptr; 
  }

  // hack?
  if(r->size() == flen + 1 && r->at(r->size()-1) == 0x00){
    r->pop_back(); 
  }
  // end

  if(r->size() != flen) Serial.printf("Mismatch between buffer (%d) and header (%d)\n", r->size(), flen); 

  return r; 
}

String decode_cp1251(BytesBuffer* data){
  String res; 

  while(data->empty() == false){
    char c = data->consume<char>(); 
    if(c < 0x80) res += c;
    // probably don't need the cyrillic character - maybe later 
    else if(c >= 0xC0 && c <= 0xDF) res += '_';//(char)(0x0410 + (c - 0xC0)); 
    else if(c >= 0xE0 && c <= 0xFF) res += '_';//(char)(0x0430 + (c - 0xE0)); 
    else if(c == 0xA8) res += '_'; //(char)0x0401; 
    else if(c == 0xB8) res += '_'; //(char)0x0451; 
    else res += c; // unknown character
  }

  return res; 
}


uint8_t decode_spectrum(BytesBuffer* data, int* ret, float& a0, float& a1, float& a2, uint32_t& ts){
  // ret is assumed to be an array of len 1024
  
  // read first bytes 
  ts = data->consume<uint32_t>(); 
  a0 = data->consume<float>(); 
  a1 = data->consume<float>(); 
  a2 = data->consume<float>(); 

  size_t ret_it = 0; 
  int last = 0; 
  int v = 0; 
  while(data->empty() == false){  //place < data->data() + data->size()){
    uint16_t u16 = data->consume<uint16_t>(); 
    // memcpy(&u16, place, sizeof(uint16_t)); 
    // place += sizeof(uint16_t); 
    uint16_t cnt = (u16 >> 4) & 0x0FFF; 
    uint16_t vlen = u16 & 0x0F; 
    // Serial.printf("\nu16: %u vlen: %u cnt: %u \n", u16, vlen, cnt); 
    // if(cnt > 0) Serial.print("\t"); 
    
    for(int i = 0; i < cnt; i++){
      if(vlen == 0){
        v = 0; 
      } else if(vlen == 1){
        // unpack('<B')
        uint8_t b = data->consume<uint8_t>(); 
        // memcpy(&b, place, sizeof(uint8_t)); 
        // place += sizeof(uint8_t); 

        v = b;  
      } else if(vlen == 2){
        // last + unpack('<b')
        int8_t b = data->consume<int8_t>(); 
        // memcpy(&b, place, sizeof(int8_t)); 
        // place += sizeof(int8_t); 

        v = last + b; 
      } else if(vlen == 3){
        // last + unpack('<h')
        int16_t h = data->consume<int16_t>(); 
        // memcpy(&h, place, sizeof(int16_t)); 
        // place += sizeof(int16_t); 

        v = last + h; 
      } else if(vlen == 4){
        // a, b, c = unpack('<BBb')
        uint8_t a = data->consume<uint8_t>(); 
        uint8_t b = data->consume<uint8_t>(); 
        int8_t c = data->consume<int8_t>(); 

        // memcpy(&a, place, sizeof(uint8_t)); 
        // place += sizeof(uint8_t); 

        // memcpy(&b, place, sizeof(uint8_t)); 
        // place += sizeof(uint8_t); 

        // memcpy(&c, place, sizeof(int8_t)); 
        // place += sizeof(int8_t); 

        v = last + ((c << 16) | (b << 8) | a); 
      } else if(vlen == 5){
        // last + unpack('<i')
        int32_t i = data->consume<int32_t>();
        // memcpy(&i, place, sizeof(int32_t));
        // place += sizeof(int32_t);

        v = last + i; 
      } else {
        Serial.printf("Unsupported vlen %u\n", vlen);
        return 1; 
      }

      // Serial.printf("v: %d, ", v);
      last = v; 
      ret[ret_it++] = v;
    }
  }

  return 0; 
}

void printSpectrum(){
  BytesBuffer* r = read_request(VS::SPECTRUM);

  float a0, a1, a2;
  uint32_t ts; 
  int spectrum[1024];  
  decode_spectrum(r, spectrum, a0, a1, a2, ts);
  Serial.printf("a0: %f, a1: %f, a2: %f, ts: %u\n", a0, a1, a2, ts);
  Serial.println("Spectrum:");
  for(int i = 0; i < 1024; i++){
    Serial.printf("%d, ", spectrum[i]);
  }
  Serial.println(); 
}

uint8_t consume_data_buf(BytesBuffer* r){
  if(r->size() < 7) return -1; // too short? 

  uint8_t seq = r->consume<uint8_t>(); 
  uint8_t eid = r->consume<uint8_t>(); 
  uint8_t gid = r->consume<uint8_t>(); 
  int32_t ts_offset = r->consume<int32_t>(); 

  

  return 0; 
}

void setup() {
  // setup allocations 
  reserve_buffers();

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

  // init? 
  uint8_t data[] = {0x01, 0xff, 0x12, 0xff}; 
  execute(Command::SET_EXCHANGE, data, sizeof(data));

  // set local time 
  // Serial.println("Setting local time...");
  // execute(Command::SET_TIME, datetime, sizeof(datetime)); 

  // set device time 
  // uint8_t time_setting[] = {0x00, 0x00, 0x00, 0x00};
  // write_request(VSFR::DEVICE_TIME, time_setting, sizeof(time_setting));

  // read configuration 
  // Serial.println("Reading config..."); 
  // BytesBuffer* r = read_request(VS::CONFIGURATION); 
  // if(r != nullptr) Serial.println(decode_cp1251(r)); 
  // else Serial.println("ERROR"); 

  Serial.println("Setup Done.");
}

void loop() {

  // printSpectrum(); 

  // get event data 
  BytesBuffer* r = read_request(VS::DATA_BUF); 

  while(r->size() > 0){
    consume_data_buf(r); 
  }
  
  delay(5000); 
}

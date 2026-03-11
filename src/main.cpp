#include <Arduino.h>
// #include <RadiaCodeBLELib.h>
#include <stdint.h>
#include <BLE.h>
#include "btstack.h"
// #include <vector>
#include "Config.h"
#include <string> 
#include "Events.h"

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
    _resp_buffer.clear();
    _resp_buffer.fill(data+4, len-4); 
  } else {
    // read in the bytes as data
    _resp_buffer.fill(data, len);
  }

  // reduce size 
  _resp_size -= len;

  
  if(_resp_size == 0){
    // copied entire message
    // Serial.println("message ended"); 
    // send to _response
    mutex_enter_blocking(&_response_mutex); 
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

  res_ret.clear(); // reuse res_ret

  mutex_enter_blocking(&_response_mutex); 
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

BytesBuffer* execute(uint16_t req_type, uint8_t* args, size_t len){
  static uint16_t seq = 0; 
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
  if(received_req_type != header.req_type || received_zero != header.zero || received_req_seq_no != header.req_seq_no){
    Serial.println("Response header does not match request header!"); 
    Serial.printf("(expected|received) req_type: %u|%u zero: %u|%u req_seq_no: %u|%u\n", header.req_type, received_req_type, header.zero, received_zero, header.req_seq_no, received_req_seq_no);

    return nullptr; 
  }

  // remove compared header fields  

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

  // match BytesBuffer behavior 
  uint32_t retcode = r->consume<uint32_t>(); 

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
    uint16_t cnt = (u16 >> 4) & 0x0FFF; 
    uint16_t vlen = u16 & 0x0F; 
    
    for(int i = 0; i < cnt; i++){
      if(vlen == 0){
        v = 0; 
      } else if(vlen == 1){
        // unpack('<B')
        uint8_t b = data->consume<uint8_t>();  

        v = b;  
      } else if(vlen == 2){
        // last + unpack('<b')
        int8_t b = data->consume<int8_t>(); 

        v = last + b; 
      } else if(vlen == 3){
        // last + unpack('<h')
        int16_t h = data->consume<int16_t>(); 

        v = last + h; 
      } else if(vlen == 4){
        // a, b, c = unpack('<BBb')
        uint8_t a = data->consume<uint8_t>(); 
        uint8_t b = data->consume<uint8_t>(); 
        int8_t c = data->consume<int8_t>(); 

        v = last + ((c << 16) | (b << 8) | a); 
      } else if(vlen == 5){
        // last + unpack('<i')
        int32_t i = data->consume<int32_t>();

        v = last + i; 
      } else {
        Serial.printf("Unsupported vlen %u\n", vlen);
        return 1; 
      }

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

DataPoint consume_data_buf(BytesBuffer* r){
  if(r->size() < 7) {
    // too short?
    return Error(); 
  }

  uint8_t seq = r->consume<uint8_t>(); 
  uint8_t eid = r->consume<uint8_t>(); 
  uint8_t gid = r->consume<uint8_t>(); 
  int32_t ts_offset = r->consume<int32_t>(); 

  dt_t dt = ts_offset * 10; 
  
  String line = ""; 
  if(eid == 0){
    if(gid == 0){
      // RealTimeData
      return RealTimeData {
        dt, 
        r->consume<float>(), 
        r->consume<float>(), 
        r->consume<uint16_t>(),
        r->consume<uint16_t>(), 
        r->consume<uint16_t>(),
        r->consume<uint8_t>()
      };
    }
    else if(gid == 1){
      // RawData
      return RawData {
        dt, 
        r->consume<float>(), 
        r->consume<float>()
      };
    }
    else if(gid == 2){
      // DoseRateDB
      return DoseRateDB {
        dt, 
        r->consume<uint32_t>(), 
        r->consume<float>(),
        r->consume<float>(), 
        r->consume<uint16_t>(), 
        r->consume<uint16_t>()
      };
    }
    else if(gid == 3){
      // RareData
      return RareData {
        dt, 
        r->consume<uint32_t>(),
        r->consume<float>(), 
        r->consume<uint16_t>(), 
        r->consume<uint16_t>(), 
        r->consume<uint16_t>()
      };
    }
    else if(gid == 4){
      // UserData - ?
      r->consume<uint32_t>(); 
      r->consume<float>(); 
      r->consume<float>(); 
      r->consume<uint16_t>(); 
      r->consume<uint16_t>(); 
      return None {eid, gid}; 
    }
    else if(gid == 5){
      // ScheduleData - ?
      r->consume<uint32_t>(); 
      r->consume<float>(); 
      r->consume<float>(); 
      r->consume<uint16_t>(); 
      r->consume<uint16_t>(); 
      return None {eid, gid}; 
    }
    else if(gid == 6){
      // AccelData - ?
      r->consume<uint16_t>();
      r->consume<uint16_t>();
      r->consume<uint16_t>();
      return None {eid, gid}; 
    }
    else if(gid == 7){
      // Event 
      return Event {
        dt, 
        r->consume<uint8_t>(), 
        r->consume<uint8_t>(), 
        r->consume<uint16_t>()
      };
    }
    else if(gid == 8){
      // RawCountRate
      return RawCountRate {
        dt,
        r->consume<float>(),
        r->consume<uint16_t>()
      };
    }
    else if(gid == 9){
      // RawDoseRate
      return RawDoseRate {
        dt,
        r->consume<float>(),
        r->consume<uint16_t>()
      };
    } else {
      Serial.printf("Unknown eid: %d gid: %d\n", eid, gid); 
    }
  } else if(eid == 1){
    if(gid == 1){
      // ???
      uint16_t samples_num = r->consume<uint16_t>(); 
      uint32_t smpl_time_ms = r->consume<uint32_t>(); 
      r->drain(nullptr, 8*samples_num); 
      return None {eid, gid}; 
    } else if(gid == 2){
      // ???
      uint16_t samples_num = r->consume<uint16_t>(); 
      uint32_t smpl_time_ms = r->consume<uint32_t>(); 
      r->drain(nullptr, 16*samples_num); 
      return None {eid, gid}; 
    } else if(gid == 3){
      // ???
      uint16_t samples_num = r->consume<uint16_t>(); 
      uint32_t smpl_time_ms = r->consume<uint32_t>(); 
      r->drain(nullptr, 14*samples_num); 
      return None {eid, gid}; 
    }
    else {
      Serial.printf("Unknown eid: %d gid: %d\n", eid, gid); 
    }
  } else {
    Serial.printf("Unknown eid: %d gid: %d\n", eid, gid); 
  }

  return Error();
}

void radiacode_ble_init(bool verbose){
  mutex_init(&_response_mutex); 

  if(verbose) Serial.println("Starting BLE Client");

  BLE.begin(); 

  if(verbose) Serial.println("Scanning..."); 

  BLEScanReport* res = BLE.scan();

  for(BLEAdvertising item : *res){
    if(item.getAddress().toString() == target_mac){
      if(verbose) Serial.println("Found device, connecting...");
      if(BLE.client()->connect(item) == false){
        if(verbose) Serial.println("Connected."); 
      } 
      break;
    }
  }

  if(verbose) Serial.println("Done."); 

  if(verbose) Serial.println("Exploring service..."); 
  BLERemoteService* service = BLE.client()->service(rc_service_uuid); 

  if(service){

    rc_notify_char = service->characteristic(rc_notify_uuid);
    if(rc_notify_char){
      if(verbose) Serial.println("Found notify char"); 
      rc_notify_char->onNotify(notify); 
      rc_notify_char->enableNotifications(); 
    }
    
    rc_write_char = service->characteristic(rc_write_uuid); 
    if(rc_write_char){
      if(verbose) Serial.println("Found write char"); 
    }

  } else {
    if(verbose) Serial.println("Error on service"); 
  }

  // init? 
  uint8_t data[] = {0x01, 0xff, 0x12, 0xff}; 
  execute(Command::SET_EXCHANGE, data, sizeof(data));

}

int last_spectrum = 0; 
void setup() {
  Serial.begin(115200);

  while (!Serial);

  radiacode_ble_init(true);

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

  printSpectrum(); 
  last_spectrum = millis(); 

  Serial.println("Setup Done.");
}

void loop() {

  // printSpectrum(); 

  // get event data 
  BytesBuffer* r = read_request(VS::DATA_BUF); 

  while(r->size() >= 7){
    DataPoint d = consume_data_buf(r); 

    std::visit([](const auto& v){
      char str[500];
      v.to_string(str, 500); 
      Serial.printf("%u,", millis());
      Serial.println(str);
    }, d);
  }

  if(millis() - last_spectrum > 5000){
    last_spectrum = millis(); 
    printSpectrum(); 
  }
  
  delay(1000); 
}

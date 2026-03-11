#include "RadiacodeBLE.h"

#include <BLE.h>

#define RADIACODE_SERVICE_UUID "e63215e5-7003-49d8-96b0-b024798fb901"
#define RADIACODE_WRITE_FD_UUID "e63215e6-7003-49d8-96b0-b024798fb901"
#define RADIACODE_NOTIFY_FD_UUID "e63215e7-7003-49d8-96b0-b024798fb901"

BLEUUID rc_service_uuid(RADIACODE_SERVICE_UUID);
BLEUUID rc_write_uuid(RADIACODE_WRITE_FD_UUID);
BLEUUID rc_notify_uuid(RADIACODE_NOTIFY_FD_UUID);

BLERemoteCharacteristic* rc_write_char; 
BLERemoteCharacteristic* rc_notify_char; 

#define BLE_BUFFER_SIZE 4000
static BytesBuffer _resp_buffer(BLE_BUFFER_SIZE); 
static BytesBuffer _response(BLE_BUFFER_SIZE); 
static mutex_t _response_mutex; 
volatile bool _response_ready = false; 
static BytesBuffer res_ret(BLE_BUFFER_SIZE); 


#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include <Arduino.h>
#include <BTstackLib.h>

#define RADIACODE_SERVICE_UUID "e63215e5-7003-49d8-96b0-b024798fb901"
#define RADIACODE_WRITE_FD_UUID "e63215e6-7003-49d8-96b0-b024798fb901"
#define RADIACODE_NOTIFY_FD_UUID "e63215e7-7003-49d8-96b0-b024798fb901"

class BLEDriver {
 private:
  UUID _service_UUID = UUID(RADIACODE_SERVICE_UUID);
  UUID _write_fd_UUID = UUID(RADIACODE_WRITE_FD_UUID);
  UUID _notify_fd_UUID = UUID(RADIACODE_NOTIFY_FD_UUID);

  String mac;
  BD_ADDR device;

  int _stage = 0;
  bool _flag = false;

  String _resp_buffer;
  size_t _resp_size;

  String _response;
  bool _closing = false;

  // _conn_handle
  hci_con_handle_t
      _conn_handle;  // connection handle determined in connection callback

  BLEDevice _device;

  // has both start and end handles
  BLEService _service_handle;
  // _start_handle
  // BLEService _start_handle;
  // // _end_handle
  // BLEService _end_handle;

  // _write_fd_handle
  BLECharacteristic _write_fd_handle;
  // _notify_fd_handle
  BLECharacteristic _notify_fd_handle;

  void deviceConnectedCallback(BLEStatus status, BLEDevice* device);
  void serviceDiscoveredCallback(BLEStatus status, BLEDevice* device,
                                 BLEService* bleService);
  void characteristicDiscoveredCallback(BLEStatus status, BLEDevice* device,
                                        BLECharacteristic* characteristic);
  void characteristicNotificationCallback(BLEDevice* device,
                                          uint16_t value_handle, uint8_t* value,
                                          uint16_t length);

  BLEDriver() {}  // private constructor for singleton pattern

 public:
  BLEDriver(const BLEDriver&) = delete;  // delete copy constructor
  BLEDriver& operator=(const BLEDriver&) =
      delete;  // delete assignment operator

  static BLEDriver& instance();

  void init(String mac);

  // blocking write (target_handle, data)

  // handleNotification (data, size)

  // uint8_t* execute(uint8_t* req);

  // void close();
};

#endif  // BLE_H
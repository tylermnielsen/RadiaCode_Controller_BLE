#include "BLEDriver.h"

#define RADIACODE_CONNECT_TIMEOUT_MS 10000

#define DEBUG(x)                 \
  if (1) {                       \
    Serial.print("BLEDriver: "); \
    Serial.println(x);           \
  }

BLEDriver& BLEDriver::instance() {
  static BLEDriver instance;
  return instance;
}

void BLEDriver::init(String mac) {
  // setup BD_ADDR
  this->mac = mac;
  this->device = BD_ADDR(mac.c_str());

  // set up for connection sequence
  this->_flag = false;
  BTstack.setBLEDeviceConnectedCallback(
      [](BLEStatus status, BLEDevice* device) {
        BLEDriver::instance().deviceConnectedCallback(status, device);
      });
  BTstack.setGATTServiceDiscoveredCallback(
      [](BLEStatus status, BLEDevice* device, BLEService* bleService) {
        BLEDriver::instance().serviceDiscoveredCallback(status, device,
                                                        bleService);
      });
  BTstack.setGATTCharacteristicDiscoveredCallback(
      [](BLEStatus status, BLEDevice* device,
         BLECharacteristic* characteristic) {
        BLEDriver::instance().characteristicDiscoveredCallback(status, device,
                                                               characteristic);
      });
  BTstack.setGATTCharacteristicNotificationCallback(
      [](BLEDevice* device, uint16_t value_handle, uint8_t* value,
         uint16_t length) {
        BLEDriver::instance().characteristicNotificationCallback(
            device, value_handle, value, length);
      });

  // connect to device
  BTstack.bleConnect(BD_ADDR_TYPE::PUBLIC_ADDRESS, this->device.getAddress(),
                     RADIACODE_CONNECT_TIMEOUT_MS);

  // busy wait for connection
  unsigned long start = millis();
  while (!this->_flag) {
    if (millis() - start > RADIACODE_CONNECT_TIMEOUT_MS) {
      DEBUG("Connection timed out on stage: " + String(this->_stage));
      return;
    }
    BTstack.loop();
  }

  DEBUG("Device connection set up.\n");
}

void BLEDriver::deviceConnectedCallback(BLEStatus status, BLEDevice* device) {
  if (status == BLE_STATUS_OK) {
    DEBUG("Device connected successfully.");

    this->_device = *device;
    this->_conn_handle = device->getHandle();

    // now find services
    this->_stage = 1;
    BTstack.discoverGATTServices(device);
  } else {
    DEBUG("Device connection failed.");
  }
}

void BLEDriver::serviceDiscoveredCallback(BLEStatus status, BLEDevice* device,
                                          BLEService* bleService) {
  if (status == BLE_STATUS_OK) {
    DEBUG("Service discovered successfully.");

    // check if service matches our target service UUID
    if (bleService->matches(&_service_UUID)) {
      DEBUG("Target service found.");

      // store start and end handles
      this->_service_handle = *bleService;
      DEBUG(this->_service_handle.getUUID()->getUuidString());
      DEBUG(this->_service_handle.getService()->start_group_handle);
      DEBUG(this->_service_handle.getService()->end_group_handle);

      this->_stage = 2;
    } else {
      DEBUG("Discovered service does not match target UUID (" +
            String(bleService->getUUID()->getUuidString()) + ").");
    }
  } else if (status == BLE_STATUS_DONE) {
    DEBUG("Service discovery done.");
    // now discover characteristics for this service
    if (this->_stage != 2) {
      DEBUG("Target service not found during discovery.");
      return;
    }

    BTstack.discoverCharacteristicsForService(&this->_device,
                                              &this->_service_handle);
  } else {
    DEBUG("Service discovery failed.");
  }
}

void BLEDriver::characteristicDiscoveredCallback(
    BLEStatus status, BLEDevice* device, BLECharacteristic* characteristic) {
  Serial.printf("char callback: status=%d device=%p handle=%u\n", status,
                (void*)device, device ? device->getHandle() : 0);

  if (status == BLE_STATUS_OK) {
    DEBUG("Characteristic discovered successfully.");

    // check if characteristic matches write fd UUID
    if (characteristic->matches(&_write_fd_UUID)) {
      DEBUG("Write FD characteristic found.");
      this->_write_fd_handle = *characteristic;
      this->_stage++;
    }

    // check if characteristic matches notify fd UUID
    if (characteristic->matches(&_notify_fd_UUID)) {
      DEBUG("Notify FD characteristic found.");
      this->_notify_fd_handle = *characteristic;
      this->_stage++;

      // subscribe for notifications
      this->_flag = true;  // signal that connection sequence is done
    }
  } else if (status == BLE_STATUS_DONE) {
    DEBUG("Characteristic discovery done.");

    if (this->_stage < 4) {
      DEBUG("Not all target characteristics found.");
      return;
    }

    BTstack.subscribeForNotifications(device, characteristic);

  } else {
    DEBUG("Characteristic discovery failed - " + String(status));
  }
}

void BLEDriver::characteristicNotificationCallback(BLEDevice* device,
                                                   uint16_t value_handle,
                                                   uint8_t* value,
                                                   uint16_t length) {
  DEBUG("Notification received.");

  // make sure it's from the correct device
  if (device->getHandle() != this->_conn_handle) {
    DEBUG("Notification from unknown device, ignoring.");
    return;
  }

  if (this->_notify_fd_handle.isValueHandle(value_handle)) {
    DEBUG("Nofity handle");
  }

  Serial.print("Notification data: ");
  Serial.write(value, length);
  Serial.println();
}
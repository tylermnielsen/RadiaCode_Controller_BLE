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
  __lockBluetooth();
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
  BTstack.setup();

  // connect to device
  BTstack.bleConnect(BD_ADDR_TYPE::PUBLIC_ADDRESS, this->device.getAddress(),
                     RADIACODE_CONNECT_TIMEOUT_MS);
  __unlockBluetooth();

  // busy wait for connection
  unsigned long start = millis();
  while (!this->_flag) {
    if (millis() - start > RADIACODE_CONNECT_TIMEOUT_MS) {
      DEBUG("Connection timed out on stage: " + String(this->_stage));
      return;
    }
    // BTstack.loop();
    delay(100);
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

    // __lockBluetooth();
    // if (BTstack.discoverGATTServices(device) != ERROR_CODE_SUCCESS)
    //   DEBUG("Error starting service discovery.");
    // __unlockBluetooth();
    device->discoverGATTServices();
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

    // device->discoverCharacteristicsForService(&this->_service_handle);

    gatt_client_service_t hci_service = *(this->_service_handle.getService());
    hci_service.start_group_handle = 1;
    hci_service.end_group_handle = 0xFFFF;

    BLEService service_override(hci_service);

    DEBUG(service_override.getService()->start_group_handle);
    DEBUG(service_override.getService()->end_group_handle);

    __lockBluetooth();
    if (BTstack.discoverCharacteristicsForService(
            &this->_device, &service_override) != ERROR_CODE_SUCCESS) {
      DEBUG("Error starting characteristic discovery.");
    }
    __unlockBluetooth();

  } else {
    DEBUG("Service discovery failed.");
  }
}

void BLEDriver::characteristicDiscoveredCallback(
    BLEStatus status, BLEDevice* device, BLECharacteristic* characteristic) {
  if (status == BLE_STATUS_OK) {
    // DEBUG("Characteristic discovered successfully.");

    Serial.print("Characteristic Discovered: ");
    Serial.print(characteristic->getUUID()->getUuidString());
    Serial.print(", handle 0x");
    Serial.println(characteristic->getCharacteristic()->value_handle, HEX);

    // check if characteristic matches write fd UUID
    if (characteristic->matches(&_write_fd_UUID)) {
      DEBUG("Write FD characteristic found.");
      this->_write_fd_handle = *characteristic;
      this->_stage++;
    }

    // check if characteristic matches notify fd UUID
    else if (characteristic->matches(&_notify_fd_UUID)) {
      DEBUG("Notify FD characteristic found.");
      this->_notify_fd_handle = *characteristic;
      this->_stage++;

      // subscribe for notifications
      this->_flag = true;  // signal that connection sequence is done
    } else {
      DEBUG("Discovered characteristic does not match target UUID (" +
            String(characteristic->getUUID()->getUuid128String()) + ").");
    }
  } else if (status == BLE_STATUS_DONE) {
    DEBUG("Characteristic discovery done.");

    if (this->_stage < 4) {
      DEBUG("Not all target characteristics found.");
      return;
    }

    // __lockBluetooth();
    // if (BTstack.subscribeForNotifications(device, &this->_notify_fd_handle)
    // !=
    //     ERROR_CODE_SUCCESS) {
    //   DEBUG("Error subscribing for notifications.");
    // }
    // __unlockBluetooth();
    device->subscribeForNotifications(&this->_notify_fd_handle);

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
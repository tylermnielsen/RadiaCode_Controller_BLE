#ifndef UTIL_H
#define UTIL_H

#include <Arduino.h>
#include <stdint.h>
#include "btstack.h"

hci_con_handle_t connection_handle;

enum gatt_action_t {
  GATT_ACTION_NONE = 0,
  GATT_ACTION_SUBSCRIBE,
} gatt_action;

gatt_client_notification_t notification_listener;
typedef struct advertising_report {
  uint8_t type;
  uint8_t event_type;
  uint8_t address_type;
  bd_addr_t address;
  uint8_t rssi;
  uint8_t length;
  const uint8_t* data;
} advertising_report_t;

// Handles connect, disconnect, and advertising report events,
// starts the GATT client, and sends the first query.
void handle_hci_event(uint8_t packet_type, uint16_t channel,
                             uint8_t* packet, uint16_t size);

// Handles GATT client query results, sends queries and the
// GAP disconnect command when the querying is done.
void handle_gatt_client_event(uint8_t packet_type, uint16_t channel,
                                     uint8_t* packet, uint16_t size);

void printUUID(uint8_t* uuid128, uint16_t uuid16) {
  if (uuid16) {
    Serial.printf("%04x", uuid16);
  } else {
    Serial.printf("%s", uuid128_to_str(uuid128));
  }
}

void dump_advertising_report(advertising_report_t* e) {
  Serial.printf(
      "    * adv. event: evt-type %u, addr-type %u, addr %s, rssi %u, length "
      "adv %u, data: ",
      e->event_type, e->address_type, bd_addr_to_str(e->address), e->rssi,
      e->length);
  Serial.write(e->data, e->length);
}

void dump_characteristic(gatt_client_characteristic_t* characteristic) {
  Serial.printf(
      "    * characteristic: [0x%04x-0x%04x-0x%04x], properties 0x%02x, uuid ",
      characteristic->start_handle, characteristic->value_handle,
      characteristic->end_handle, characteristic->properties);
  printUUID(characteristic->uuid128, characteristic->uuid16);
  Serial.printf("\n");
}

void dump_service(gatt_client_service_t* service) {
  Serial.printf("    * service: [0x%04x-0x%04x], uuid ",
                service->start_group_handle, service->end_group_handle);
  printUUID(service->uuid128, service->uuid16);
  Serial.printf("\n");
}

void fill_advertising_report_from_packet(advertising_report_t* report,
                                                uint8_t* packet) {
  gap_event_advertising_report_get_address(packet, report->address);
  report->event_type =
      gap_event_advertising_report_get_advertising_event_type(packet);
  report->address_type = gap_event_advertising_report_get_address_type(packet);
  report->rssi = gap_event_advertising_report_get_rssi(packet);
  report->length = gap_event_advertising_report_get_data_length(packet);
  report->data = gap_event_advertising_report_get_data(packet);
}

int btstack_main(int argc, const char* argv[]);

bool _writing = false; 
static void btstack_packet_handler_write_with_response(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    Serial.println("response"); 
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    uint8_t event_type = hci_event_packet_get_type(packet);
    if (event_type == GATT_EVENT_QUERY_COMPLETE) {
        uint16_t conn_handle = gatt_event_query_complete_get_handle(packet);
        uint16_t status = gatt_event_query_complete_get_att_status(packet);
        Serial.printf("  --> gatt query write complete conn_handle=%d status=%d\n", conn_handle, status);
        _writing = false; 
    }
}

void blocking_write(uint16_t value_handle, uint16_t value_len, uint8_t* value){
  _writing = true; 
  uint8_t res = gatt_client_write_value_of_characteristic(&btstack_packet_handler_write_with_response, connection_handle, value_handle, value_len, value);
  Serial.printf("Res = %u\n", res); 
  if(res == 0){
    while(_writing){
      delay(100); 
    }
  } else {
    _writing = false; 
  }
}

#endif
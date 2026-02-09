
#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btstack.h"

#include "util.h"

// #define RADIACODE_SERVICE_UUID "e63215e5-7003-49d8-96b0-b024798fb901"
// #define RADIACODE_WRITE_FD_UUID "e63215e6-7003-49d8-96b0-b024798fb901"
// #define RADIACODE_NOTIFY_FD_UUID "e63215e7-7003-49d8-96b0-b024798fb901"

#define RADIACODE_SERVICE_UUID "E63215E5-7003-49D8-96B0-B024798FB901"
#define RADIACODE_WRITE_FD_UUID "E63215E6-7003-49D8-96B0-B024798FB901"
#define RADIACODE_NOTIFY_FD_UUID "E63215E7-7003-49D8-96B0-B024798FB901"

static gatt_client_characteristic_t radiacode_write_char;
static gatt_client_characteristic_t radiacode_notify_char;


static bd_addr_t cmdline_addr;
static int cmdline_addr_found = 0;

static gatt_client_service_t services[40];
static int service_count = 0;
static int service_index = 0;

static btstack_packet_callback_registration_t hci_event_callback_registration;

static volatile bool subscribed = false; 

/* @section GATT client setup
 *
 * @text In the setup phase, a GATT client must register the HCI and GATT client
 * packet handlers, as shown in Listing GATTClientSetup.
 * Additionally, the security manager can be setup, if signed writes, or
 * encrypted, or authenticated connection are required, to access the
 * characteristics, as explained in Section on
 * [SMP](../protocols/#sec:smpProtocols).
 */

/* LISTING_START(GATTClientSetup): Setting up GATT client */


static void gatt_client_setup(void) {
  // Initialize L2CAP and register HCI event handler
  l2cap_init();

  // Initialize GATT client
  gatt_client_init();

  // Optinoally, Setup security manager
  sm_init();
  sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

  // register for HCI events
  hci_event_callback_registration.callback = &handle_hci_event;
  hci_add_event_handler(&hci_event_callback_registration);
}
/* LISTING_END */

/* LISTING_START(GATTBrowserHCIPacketHandler): Connecting and disconnecting from
 * the GATT client */
void handle_hci_event(uint8_t packet_type, uint16_t channel,
                             uint8_t* packet, uint16_t size) {
  UNUSED(channel);
  UNUSED(size);

  if (packet_type != HCI_EVENT_PACKET) return;
  advertising_report_t report;

  bd_addr_t target = {0x52, 0x43, 0x06, 0x60, 0x17, 0xDD};

  uint8_t event = hci_event_packet_get_type(packet);
  switch (event) {
    case BTSTACK_EVENT_STATE:
      // BTstack activated, get started
      if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) break;
      // if (cmdline_addr_found){
      //     Serial.printf("Trying to connect to %s\n",
      //     bd_addr_to_str(cmdline_addr));

      //     gap_connect(cmdline_addr, bd_addr_type_t::BD_ADDR_TYPE_LE_PUBLIC);
      //     break;
      // }
      Serial.printf("BTstack activated, start scanning!\n");
      gap_set_scan_parameters(0, 0x0030, 0x0030);
      gap_start_scan();
      break;
    case GAP_EVENT_ADVERTISING_REPORT:
      fill_advertising_report_from_packet(&report, packet);
      // dump_advertising_report(&report);

      // if(report.address )
      // for(int i = 0; i < 6; i++){
      //     Serial.print(report.address[i], HEX);
      //     Serial.print(" ");
      // }
      // Serial.println();
      // break;
      if (memcmp(report.address, target, 6) != 0) {
        // Serial.println("Not target device, continue scanning...");
        break;
      }
      Serial.printf("Found target device:\n");
      dump_advertising_report(&report);

      // stop scanning, and connect to the device
      gap_stop_scan();
      gap_connect(report.address, (bd_addr_type_t)(report.address_type));
      break;
    case HCI_EVENT_META_GAP:
      // wait for connection complete
      if (hci_event_gap_meta_get_subevent_code(packet) !=
          GAP_SUBEVENT_LE_CONNECTION_COMPLETE)
        break;
      Serial.printf("\nGATT browser - CONNECTED\n");
      connection_handle =
          gap_subevent_le_connection_complete_get_connection_handle(packet);
      // query primary services
      gatt_client_discover_primary_services(handle_gatt_client_event,
                                            connection_handle);
      break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
      Serial.printf("\nGATT browser - DISCONNECTED\n");
      break;
    default:
      Serial.printf("Unkown hci event %d\n", event); 
      break;
  }
}
/* LISTING_END */

/* LISTING_START(GATTBrowserQueryHandler): Handling of the GATT client queries
 */

void handle_gatt_client_event(uint8_t packet_type, uint16_t channel,
                                     uint8_t* packet, uint16_t size) {
  UNUSED(packet_type);
  UNUSED(channel);
  UNUSED(size);

  gatt_client_service_t service;
  gatt_client_characteristic_t characteristic;
  switch (hci_event_packet_get_type(packet)) {
    case GATT_EVENT_SERVICE_QUERY_RESULT:
      gatt_event_service_query_result_get_service(packet, &service);
      dump_service(&service);
      services[service_count++] = service;
      break;

    case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
      gatt_event_characteristic_query_result_get_characteristic(
          packet, &characteristic);

      if (strcmp(uuid128_to_str(characteristic.uuid128),
                 RADIACODE_NOTIFY_FD_UUID) == 0) {
        Serial.printf("Found RadiaCode Notify Characteristic!\n");
        radiacode_notify_char = characteristic;

        Serial.println("Subscribing to notifications...");
        gatt_action = GATT_ACTION_SUBSCRIBE;
        gatt_client_listen_for_characteristic_value_updates(
            &notification_listener, handle_gatt_client_event, connection_handle,
            &radiacode_notify_char);

        // gatt_client_write_client_characteristic_configuration(
        //     handle_gatt_client_event, connection_handle, &radiacode_notify_char,
        //     GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
        // gatt_client_write_value_of_characteristic(handle_gatt_client_event, 
        //                                           connection_handle, 
        //                                           radiacode_notify_char.value_handle + 1, 
        //                                           2, 
        //                                           (uint8_t*)"\x01\x00");
        blocking_write(radiacode_notify_char.value_handle + 1, 2, (uint8_t*)"\x01\x00"); 

        dump_characteristic(&characteristic);
        subscribed = true; 
      } else if (strcmp(uuid128_to_str(characteristic.uuid128),
                        RADIACODE_WRITE_FD_UUID) == 0) {
        Serial.printf("Found RadiaCode Write Characteristic!\n");
        radiacode_write_char = characteristic;

        dump_characteristic(&characteristic);
      } else {
        Serial.println("unexpected characteristic:");
        dump_characteristic(&characteristic);
      }

      break;
    case GATT_EVENT_QUERY_COMPLETE:

      if (gatt_action == GATT_ACTION_SUBSCRIBE) {
        Serial.println("Subscribed to notifications.");
        break;
      }

      // GATT_EVENT_QUERY_COMPLETE of search characteristics
      if (service_index < service_count) {
        service = services[service_index++];
        Serial.printf(
            "\nGATT browser - CHARACTERISTIC for SERVICE %s, [0x%04x-0x%04x]\n",
            uuid128_to_str(service.uuid128), service.start_group_handle,
            service.end_group_handle);

        gatt_client_discover_characteristics_for_service(
            handle_gatt_client_event, connection_handle, &service);
        break;
      }
      service_index = 0;
      break;

    case GATT_EVENT_NOTIFICATION:
      Serial.println("Received Notification:");
      {
        uint16_t value_handle =
            gatt_event_notification_get_value_handle(packet);
        uint16_t value_length =
            gatt_event_notification_get_value_length(packet);
        const uint8_t* value = gatt_event_notification_get_value(packet);
        Serial.printf("  * handle 0x%04x, length %u, value: ", value_handle,
                      value_length);
        for (int i = 0; i < value_length; i++) {
          Serial.printf("%c", value[i]);
        }
        Serial.printf("\n");
      }
      break;

    default:
      Serial.printf("Unknown case %u\n", hci_event_packet_get_type(packet));
      break;
  }
}
/* LISTING_END */

int btstack_main(int argc, const char* argv[]) {
  (void)argc;
  (void)argv;

  while (!Serial);
  Serial.println("Start");

  // setup GATT client
  gatt_client_setup();

  // turn on!
  hci_power_control(HCI_POWER_ON);

  return 0;
}

void execute(){

}

/* EXAMPLE_END */

void setup() {
  Serial.begin(115200);
  btstack_main(0, NULL);

  while(subscribed == false){
    delay(100); 
  }

  
}

void loop() {}
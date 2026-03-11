#include <Arduino.h>
#include <BLE.h>
#include <stdint.h>

#include <string>

#include "BytesBuffer.h"
#include "Events.h"
#include "RadiacodeBLE.h"
#include "btstack.h"

String target_mac = "52:43:06:60:17:DD";

int last_spectrum = 0;
void setup() {
  Serial.begin(115200);

  while (!Serial);

  radiacode_ble_init(target_mac, true);

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
  // get event data
  BytesBuffer* r = read_request(VS::DATA_BUF);

  while (r->size() >= 7) {
    DataPoint d = consume_data_buf(r);

    std::visit(
        [](const auto& v) {
          char str[500];
          v.to_string(str, 500);
          Serial.printf("%u,", millis());
          Serial.println(str);
        },
        d);
  }

  if (millis() - last_spectrum > 5000) {
    last_spectrum = millis();
    printSpectrum();
  }

  delay(1000);
}

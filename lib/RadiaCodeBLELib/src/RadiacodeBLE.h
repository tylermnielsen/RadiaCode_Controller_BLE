#ifndef RADIACODE_BLE_H
#define RADIACODE_BLE_H

#include <Arduino.h>

#include "BytesBuffer.h"
#include "Events.h"

void radiacode_ble_init();
uint8_t radiacode_ble_connect(String target_mac, bool verbose);
void radiacode_ble_disconnect(); 

uint8_t write_request(int command_id, uint8_t* data, size_t len);
BytesBuffer* read_request(uint32_t command_id);
String decode_cp1251(BytesBuffer* data);
uint8_t decode_spectrum(BytesBuffer* data, int* ret, float& a0, float& a1,
                        float& a2, uint32_t& ts);
BytesBuffer* readSpectrumData();
void printSpectrum();
DataPoint consume_data_buf(BytesBuffer* r);

enum Command {
  GET_STATUS = 0x0005,
  SET_EXCHANGE = 0x0007,
  GET_VERSION = 0x000A,
  GET_SERIAL = 0x000B,
  FW_IMAGE_GET_INFO = 0x0012,
  FW_SIGNATURE = 0x0101,
  RD_HW_CONFIG = 0x0807,
  RD_VIRT_SFR = 0x0824,
  WR_VIRT_SFR = 0x0825,
  RD_VIRT_STRING = 0x0826,
  WR_VIRT_STRING = 0x0827,
  RD_VIRT_SFR_BATCH = 0x082A,
  WR_VIRT_SFR_BATCH = 0x082B,
  RD_FLASH = 0x081C,
  SET_TIME = 0x0A04
};

enum VSFR {
  DEVICE_CTRL = 0x0500,
  DEVICE_LANG = 0x0502,
  DEVICE_ON = 0x0503,
  DEVICE_TIME = 0x0504,

  DISP_CTRL = 0x0510,
  DISP_BRT = 0x0511,
  DISP_CONTR = 0x0512,
  DISP_OFF_TIME = 0x0513,
  DISP_ON = 0x0514,
  DISP_DIR = 0x0515,
  DISP_BACKLT_ON = 0x0516,

  SOUND_CTRL = 0x0520,
  SOUND_VOL = 0x0521,
  SOUND_ON = 0x0522,
  SOUND_BUTTON = 0x0523,

  VIBRO_CTRL = 0x0530,
  VIBRO_ON = 0x0531,

  LEDS_CTRL = 0x0540,
  LED0_BRT = 0x0541,
  LED1_BRT = 0x0542,
  LED2_BRT = 0x0543,
  LED3_BRT = 0x0544,
  LEDS_ON = 0x0545,

  ALARM_MODE = 0x05E0,
  PLAY_SIGNAL = 0x05E1,

  MS_CTRL = 0x0600,
  MS_MODE = 0x0601,
  MS_SUB_MODE = 0x0602,
  MS_RUN = 0x0603,

  BLE_TX_PWR = 0x0700,

  DR_LEV1_uR_h = 0x8000,
  DR_LEV2_uR_h = 0x8001,
  DS_LEV1_100uR = 0x8002,
  DS_LEV2_100uR = 0x8003,
  DS_UNITS = 0x8004,
  CPS_FILTER = 0x8005,
  RAW_FILTER = 0x8006,
  DOSE_RESET = 0x8007,
  CR_LEV1_cp10s = 0x8008,
  CR_LEV2_cp10s = 0x8009,

  USE_nSv_h = 0x800C,

  CHN_TO_keV_A0 = 0x8010,
  CHN_TO_keV_A1 = 0x8011,
  CHN_TO_keV_A2 = 0x8012,
  CR_UNITS = 0x8013,
  DS_LEV1_uR = 0x8014,
  DS_LEV2_uR = 0x8015,

  CPS = 0x8020,
  DR_uR_h = 0x8021,
  DS_uR = 0x8022,

  TEMP_degC = 0x8024,
  ACC_X = 0x8025,
  ACC_Y = 0x8026,
  ACC_Z = 0x8027,
  OPT = 0x8028,

  RAW_TEMP_degC = 0x8033,
  TEMP_UP_degC = 0x8034,
  TEMP_DN_degC = 0x8035,

  VBIAS_mV = 0xC000,
  COMP_LEV = 0xC001,
  CALIB_MODE = 0xC002,
  DPOT_RDAC = 0xC004,
  DPOT_RDAC_EEPROM = 0xC005,
  DPOT_TOLER = 0xC006,

  SYS_MCU_ID0 = 0xFFFF0000,
  SYS_MCU_ID1 = 0xFFFF0001,
  SYS_MCU_ID2 = 0xFFFF0002,

  SYS_DEVICE_ID = 0xFFFF0005,
  SYS_SIGNATURE = 0xFFFF0006,
  SYS_RX_SIZE = 0xFFFF0007,
  SYS_TX_SIZE = 0xFFFF0008,
  SYS_BOOT_VERSION = 0xFFFF0009,
  SYS_TARGET_VERSION = 0xFFFF000A,
  SYS_STATUS = 0xFFFF000B,
  SYS_MCU_VREF = 0xFFFF000C,
  SYS_MCU_TEMP = 0xFFFF000D,
  SYS_FW_VER_BT = 0xFFFF010
};

enum VS {
  CONFIGURATION = 2,
  FW_DESCRIPTOR = 3,
  SERIAL_NUMBER = 8,
  // UNKNOWN_13 = 0xd,
  TEXT_MESSAGE = 0xF,
  MEM_SNAPSHOT = 0xE0,
  // UNKNOWN_240 = 0xf0,
  DATA_BUF = 0x100,
  SFR_FILE = 0x101,
  SPECTRUM = 0x200,
  ENERGY_CALIB = 0x202,
  SPEC_ACCUM = 0x205,
  SPEC_DIFF = 0x206,  // TODO: what's that? Can be decoded by spectrum decoder
  SPEC_RESET = 0x207  // TODO: looks like spectrum, but our spectrum decoder
                      // fails with `vlen == 7 unsupported`
};

#endif
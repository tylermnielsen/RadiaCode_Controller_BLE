#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>

typedef uint32_t dt_t;

struct RealTimeData {
  dt_t dt; 
  float count_rate; 
  float dose_rate; 
  uint16_t count_rate_err; 
  uint16_t dose_rate_err; 
  uint16_t flags; 
  uint8_t rt_flags; 
};

struct RawData {
  dt_t dt; 
  float count_rate;
  float dose_rate; 
}; 

struct DoseRateDB {
  dt_t dt; 
  uint32_t count; 
  float count_rate; 
  float dose_rate; 
  uint16_t dose_rate_err; 
  uint16_t flags; 
};

struct RareData {
  dt_t dt; 
  int duration; 
  float dose; 
  uint16_t temperature; 
  uint16_t charge_level; 
  uint16_t flags; 
};

struct Event {
  dt_t dt; 
  uint8_t event; 
  uint8_t event_param1; 
  uint16_t flags; 
};

#endif 
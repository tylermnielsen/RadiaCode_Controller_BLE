#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>
#include <stdio.h>

#include <variant>

typedef uint32_t dt_t;

struct RealTimeData {
  dt_t dt;
  float count_rate;
  float dose_rate;
  uint16_t count_rate_err;
  uint16_t dose_rate_err;
  uint16_t flags;
  uint8_t rt_flags;
  int to_string(char* str, size_t len) const {
    // 5 decimal place floats
    return snprintf(str, len, "Real,%u,%.5f,%u,%.5f,%u,%u,%u", dt, count_rate,
                    count_rate_err, dose_rate, dose_rate_err, flags, rt_flags);
  }
  int to_store(char* str, size_t len) const {
    // more known max size
    return snprintf(str, len, "Real,%x,%x,%x,%x,%x,%x,%x", dt, count_rate,
                    count_rate_err, dose_rate, dose_rate_err, flags, rt_flags);
  }
};

struct RawData {
  dt_t dt;
  float count_rate;
  float dose_rate;
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "Raw,%u,%.5f,%.5f", dt, count_rate, dose_rate);
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "Raw,%x,%x,%x", dt, count_rate, dose_rate);
  }
};

struct DoseRateDB {
  dt_t dt;
  uint32_t count;
  float count_rate;
  float dose_rate;
  uint16_t dose_rate_err;
  uint16_t flags;
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "Dose,%u,%u,%.5f,%.5f,%u,%u", dt, count,
                    count_rate, dose_rate, dose_rate_err, flags);
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "Dose,%u,%u,%.5f,%.5f,%u,%u", dt, count,
                    count_rate, dose_rate, dose_rate_err, flags);
  }
};

struct RareData {
  dt_t dt;
  uint32_t duration;
  float dose;
  uint16_t temperature;
  uint16_t charge_level;
  uint16_t flags;
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "Rare,%u,%d,%.5f,%u,%u,%u", dt, duration, dose,
                    temperature, charge_level, flags);
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "Rare,%x,%x,%x,%x,%x,%x", dt, duration, dose,
                    temperature, charge_level, flags);
  }
};

struct Event {
  dt_t dt;
  uint8_t event;
  uint8_t event_param1;
  uint16_t flags;
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "Eve,%u,%u,%u,%u", dt, event, event_param1,
                    flags);
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "Eve,%x,%x,%x,%x", dt, event, event_param1,
                    flags);
  }
};

// less confident in these types:
struct RawCountRate {
  dt_t dt;
  float count_rate;
  uint16_t flags;
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "%u,%.5f,%u", dt, count_rate, flags);
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "%x,%x,%x", dt, count_rate, flags);
  }
};

struct RawDoseRate {
  dt_t dt;
  float dose_rate;
  uint16_t flags;
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "%u,%.5f,%u", dt, dose_rate, flags);
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "%x,%x,%x", dt, dose_rate, flags);
  }
};
// end

// to be returned for the unknown types
struct RCNone {
  uint8_t eid;
  uint8_t gid;
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "[RCNone %u|%u]", eid, gid);
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "[RCNone %x|%x]", eid, gid);
  }
};

struct RCError {
  int to_string(char* str, size_t len) const {
    return snprintf(str, len, "[RCError]");
  }
  int to_store(char* str, size_t len) const {
    return snprintf(str, len, "[RCError]");
  }
};

// now combine into variant set
using DataPoint = std::variant<RealTimeData, RawData, DoseRateDB, RareData,
                               Event, RawDoseRate, RawCountRate, RCError, RCNone>;

#endif
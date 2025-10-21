#pragma once

#include <stddef.h>
#include <stdint.h>

enum DeviceRole {
  DEVICE_ROLE__TESTER = 0,
  DEVICE_ROLE__SUT = 1,
};

struct DeviceInfo {
  enum DeviceRole role;
  size_t device_id_length;
  uint8_t device_id[100];
};

enum ResponseType {
  RESPONSE_TYPE__DEVICE_INFO = 0,
};

struct Response {
  int32_t result_code;
  enum ResponseType payload_type;
  union {
    struct DeviceInfo device_info;
  } payload;
};
#pragma once

#include <stddef.h>
#include <stdint.h>

extern char logs[5000];

enum DeviceRole {
  DEVICE_ROLE__TESTER = 0,
  DEVICE_ROLE__SUT = 1,
};

struct DeviceInfo {
  enum DeviceRole role;
  size_t device_id_length;
  uint8_t device_id[100];
};

struct GpioInfo {
  size_t errors_length;
  char **errors;
};

enum ResponseType {
  RESPONSE_TYPE__DEVICE_INFO = 0,
  RESPONSE_TYPE__GPIO = 1,
};

struct Response {
  int32_t result_code;
  enum ResponseType payload_type;
  union {
    struct DeviceInfo device_info;
    struct GpioInfo gpio_response;
  } payload;
};

enum RequestType {
  REQUEST_TYPE__GET_DEVICE_INFO = 0,
  REQUEST_TYPE__SETUP_GPIO_TEST = 1,
  REQUEST_TYPE__EXECUTE_GPIO_TEST = 2,
  REQUEST_TYPE__STOP_GPIO_TEST = 3,
};

struct Request {
  enum RequestType type;
};

int handle_device_info_request(const struct Request *request,
                               struct Response *response);
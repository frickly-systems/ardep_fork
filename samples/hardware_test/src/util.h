#pragma once

#include <stddef.h>
#include <stdint.h>

extern char logs[5000];

enum DeviceRole {
  DEVICE_ROLE__TESTER = 0,
  DEVICE_ROLE__SUT = 1,
};

struct DeviceInfo {
  size_t device_id_length;
  uint8_t device_id[100];
};

struct GpioInfo {};

struct UartInfo {
  char name[50];
  size_t name_len;
  char receive_buffer[50];
  size_t receive_len;
  char send_buffer[50];
  size_t send_len;
  char last_error[200];
  size_t last_error_len;
};

enum ResponseType {
  RESPONSE_TYPE__DEVICE_INFO = 0,
  RESPONSE_TYPE__GPIO = 1,
  RESPONSE_TYPE__UART = 2,
};

struct Response {
  int32_t result_code;
  enum DeviceRole role;
  enum ResponseType payload_type;
  union {
    struct DeviceInfo device_info;
    struct GpioInfo gpio_response;
    struct UartInfo uart_response;
  } payload;
};

enum RequestType {
  REQUEST_TYPE__GET_DEVICE_INFO = 0,
  REQUEST_TYPE__SETUP_GPIO_TEST = 1,
  REQUEST_TYPE__EXECUTE_GPIO_TEST = 2,
  REQUEST_TYPE__STOP_GPIO_TEST = 3,
  REQUEST_TYPE__SETUP_UART_TEST = 4,
  REQUEST_TYPE__EXECUTE_UART_TEST = 5,
  REQUEST_TYPE__STOP_UART_TEST = 6,
};

struct Request {
  enum RequestType type;
};

int handle_device_info_request(const struct Request* request,
                               struct Response* response);
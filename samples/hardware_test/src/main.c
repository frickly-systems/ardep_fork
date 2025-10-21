/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protobuf_helper.h"
#include "serial_communication.h"
#include "uart_rx.h"
#include "uart_tx.h"
#include "util.h"
#include "zephyr/devicetree.h"
#include "zephyr/drivers/hwinfo.h"
#include "zephyr/logging/log.h"

#include <stdint.h>

#include <zephyr/kernel.h>

#define LOG_MODULE_NAME sut
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

const struct device *test_uart_dev =
    DEVICE_DT_GET(DT_CHOSEN(hw_test_command_uart));

int handle_device_info_request(const struct Request *request,
                               struct Response *response) {
  uint8_t device_id[100];
  ssize_t device_id_len = hwinfo_get_device_id(device_id, sizeof(device_id));
  if (device_id_len < 0) {
    LOG_ERR("Failed to get device ID: %d", device_id_len);
    response->result_code = device_id_len;
    response->payload_type = RESPONSE_TYPE__DEVICE_INFO;
    response->payload.device_info.device_id_length = 0;
    response->payload.device_info.role = DEVICE_ROLE__SUT;
    return 0;
  }

  char hex_string[device_id_len * 2 + 1];
  for (int i = 0; i < device_id_len; i++) {
    sprintf(&hex_string[i * 2], "%02X", device_id[i]);
  }
  hex_string[device_id_len * 2] = '\0';

  LOG_INF("Device ID: %s", hex_string);

  response->result_code = 0;
  response->payload_type = RESPONSE_TYPE__DEVICE_INFO;
  response->payload.device_info.role = DEVICE_ROLE__SUT;
  response->payload.device_info.device_id_length = device_id_len;
  memcpy(response->payload.device_info.device_id, device_id, device_id_len);

  return 0;
}

int request_response(const struct Request *request, struct Response *response) {
  if (request->type == REQUEST_TYPE__GET_DEVICE_INFO) {
    return handle_device_info_request(request, response);
  }

  LOG_ERR("Unknown request type: %d", request->type);
  return -EINVAL;
}

int main() {
  LOG_INF("Starting Hardware Test Firmware");

  int ret = serial_communication_init(test_uart_dev, request_response);
  if (ret != 0) {
    LOG_ERR("Failed to initialize serial communication: %d", ret);
    return ret;
  }

  ret = serial_communication_start();
  if (ret != 0) {
    LOG_ERR("Failed to start serial communication: %d", ret);
    return ret;
  }

  LOG_INF("Serial communication started successfully");

  return 0;
}
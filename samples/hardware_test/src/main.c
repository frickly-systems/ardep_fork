/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "deps.h"
#include "protobuf_helper.h"
#include "serial_communication.h"
#include "util.h"
#include "zephyr/devicetree.h"
#include "zephyr/logging/log.h"

#include <stdint.h>

#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

char logs[5000];

const struct device* test_uart_dev =
    DEVICE_DT_GET(DT_CHOSEN(hw_test_command_uart));

int request_response(const struct Request* request, struct Response* response) {
  switch (request->type) {
    case REQUEST_TYPE__GET_DEVICE_INFO:
      response->payload_type = RESPONSE_TYPE__DEVICE_INFO;
      return handle_device_info_request(request, response);

    case REQUEST_TYPE__SETUP_UART_TEST:
      response->payload_type = RESPONSE_TYPE__GPIO;
      return setup_uart_test(request, response);
    case REQUEST_TYPE__EXECUTE_UART_TEST:
      response->payload_type = RESPONSE_TYPE__GPIO;
      return execute_uart_test(request, response);
    case REQUEST_TYPE__STOP_UART_TEST:
      response->payload_type = RESPONSE_TYPE__GPIO;
      return stop_uart_test(request, response);

    case REQUEST_TYPE__SETUP_GPIO_TEST:
    case REQUEST_TYPE__EXECUTE_GPIO_TEST:
    case REQUEST_TYPE__STOP_GPIO_TEST:
    default:
      LOG_ERR("Unknown request type: %d", request->type);
      return -EINVAL;
  }
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
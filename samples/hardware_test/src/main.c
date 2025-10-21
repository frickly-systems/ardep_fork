/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protobuf_helper.h"
#include "uart_tx.h"
#include "util.h"
#include "zephyr/devicetree.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/drivers/hwinfo.h"
#include "zephyr/drivers/uart.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <stdint.h>
#include <string.h>

#include <zephyr/console/console.h>
#include <zephyr/kernel.h>

#define LOG_MODULE_NAME sut
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

void gpio_test(void);
void log_hardware_info(void);
void uart_test(void);
void can_test(void);
void lin_test(void);

int main() {
  LOG_INF("Starting Tester Firmware");
  // console_getline_init();
  // LOG_INF("Tester Firmware started");

  // while (true) {
  //   char *s = console_getline();

  //   if (strcmp(s, "hwInfo start") == 0) {
  //     LOG_INF("hwInfo start");
  //     log_hardware_info();
  //     LOG_INF("hwInfo stop");
  //   } else if (strcmp(s, "gpio start") == 0) {
  //     LOG_INF("gpio start");
  //     gpio_test();
  //     LOG_INF("gpio stop");
  //   } else if (strcmp(s, "can start") == 0) {
  //     LOG_INF("can start");
  //     can_test();
  //     LOG_INF("can stop");
  //   } else if (strcmp(s, "lin start") == 0) {
  //     LOG_INF("lin start");
  //     lin_test();
  //     LOG_INF("lin stop");
  //   } else if (strcmp(s, "uart start") == 0) {
  //     LOG_INF("uart start");
  //     uart_test();
  //     LOG_INF("uart stop");
  //   } else {
  //     LOG_ERR("Unknown command: %s", s);
  //   }
  // }
}

const struct device *test_uart_dev =
    DEVICE_DT_GET(DT_CHOSEN(hw_test_command_uart));

int encode_counter(const void *data,
                   uint8_t *buffer,
                   size_t buffer_size,
                   size_t *message_length) {
  uint8_t d = *(uint8_t *)data;
  buffer[0] = d;
  *message_length = 1;

  return 0;
}

void entry(void *p1, void *p2, void *p3) {
  int ret = uart_tx_init(test_uart_dev);
  if (ret != 0) {
    LOG_ERR("UART device not ready");
    return;
  }

  LOG_INF("UART device ready");

  uint8_t device_id[100];
  ssize_t device_id_len = hwinfo_get_device_id(device_id, sizeof(device_id));

  char hex_string[device_id_len * 2 + 1];
  for (int i = 0; i < device_id_len; i++) {
    sprintf(&hex_string[i * 2], "%02X", device_id[i]);
  }
  hex_string[device_id_len * 2] = '\0';

  LOG_INF("Device ID: %s", hex_string);

  while (1) {
    k_sleep(K_SECONDS(1));
    struct Response response = {
      .payload_type = RESPONSE_TYPE__DEVICE_INFO,
    };

    if (ret < 0) {
      LOG_ERR("Failed to get device ID");
      response.result_code = device_id_len;
      response.payload.device_info.device_id_length = 0;
      response.payload.device_info.role = DEVICE_ROLE__SUT;
    } else {
      response.result_code = 0;
      response.payload.device_info.device_id_length = device_id_len;
      response.payload.device_info.role = DEVICE_ROLE__SUT;
      memcpy(response.payload.device_info.device_id, device_id, device_id_len);
    }

    ret = encode_and_transmit(&response, response_to_proto,
                              "Failed to encode Response");

    LOG_INF("Message send");
  }
}

K_THREAD_DEFINE(asdf, 1024, entry, NULL, NULL, NULL, 7, 0, 0);
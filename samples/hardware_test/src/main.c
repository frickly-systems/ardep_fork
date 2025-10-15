/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/drivers/uart.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <stdint.h>

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
  console_getline_init();
  LOG_INF("Tester Firmware started");

  while (true) {
    char *s = console_getline();

    if (strcmp(s, "hwInfo start") == 0) {
      LOG_INF("hwInfo start");
      log_hardware_info();
      LOG_INF("hwInfo stop");
    } else if (strcmp(s, "gpio start") == 0) {
      LOG_INF("gpio start");
      gpio_test();
      LOG_INF("gpio stop");
    } else if (strcmp(s, "can start") == 0) {
      LOG_INF("can start");
      can_test();
      LOG_INF("can stop");
    } else if (strcmp(s, "lin start") == 0) {
      LOG_INF("lin start");
      lin_test();
      LOG_INF("lin stop");
    } else if (strcmp(s, "uart start") == 0) {
      LOG_INF("uart start");
      uart_test();
      LOG_INF("uart stop");
    } else {
      LOG_ERR("Unknown command: %s", s);
    }
  }
}

const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(hw_test_command_uart));

void entry(void *p1, void *p2, void *p3) {
  if (!device_is_ready(uart_dev)) {
    LOG_ERR("UART device not ready");
    return;
  }

  LOG_INF("UART device ready");
  uint8_t counter = 0;

  while (1) {
    k_sleep(K_SECONDS(1));

    uart_poll_out(uart_dev, counter);
    LOG_INF("Counter: %d\n", counter);

    counter = counter == UINT8_MAX ? 0 : counter + 1;
  }
}

K_THREAD_DEFINE(asdf, 1024, entry, NULL, NULL, NULL, 7, 0, 0);
/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../deps.h"
#include "../util.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <zephyr/console/console.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define DEVICE_DT_BY_PROP_IDX(node_id, prop, idx) \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct device* const uart_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, uarts, DEVICE_DT_BY_PROP_IDX, (, ))};

struct Response* uart_response = NULL;

#define UART_CREATE_FUNCTIONS_BY_IDX(node_id, prop, idx)                 \
  static void uart_interrupt_callback_##idx(const struct device* dev,    \
                                            void* user_data) {           \
    uint8_t byte;                                                        \
                                                                         \
    if (!uart_irq_update(dev)) {                                         \
      return;                                                            \
    }                                                                    \
                                                                         \
    if (!uart_irq_rx_ready(dev)) {                                       \
      return;                                                            \
    }                                                                    \
                                                                         \
    while (uart_fifo_read(dev, &byte, 1) == 1) {                         \
      LOG_INF("%s: received %c", dev->name, byte);                       \
      uart_poll_out(dev, byte);                                          \
    }                                                                    \
  }                                                                      \
                                                                         \
  static int setup_uart_##idx() {                                        \
    const struct device* dev = uart_devices[idx];                        \
    if (!device_is_ready(dev)) {                                         \
      LOG_ERR("%s: UART device not ready", dev->name);                   \
      return -ENODEV;                                                    \
    }                                                                    \
                                                                         \
    char byte;                                                           \
    while (uart_fifo_read(dev, &byte, 1) == 1) {                         \
    }                                                                    \
                                                                         \
    int ret = uart_irq_callback_user_data_set(                           \
        dev, uart_interrupt_callback_##idx, NULL);                       \
                                                                         \
    if (ret < 0) {                                                       \
      if (ret == -ENOTSUP) {                                             \
        LOG_ERR("%s: Interrupt-driven UART API support not enabled",     \
                dev->name);                                              \
      } else if (ret == -ENOSYS) {                                       \
        LOG_ERR("%s: UART device does not support interrupt-driven API", \
                dev->name);                                              \
      } else {                                                           \
        LOG_ERR("%s: Error setting UART callback: %d", dev->name, ret);  \
      }                                                                  \
      return ret;                                                        \
    }                                                                    \
    uart_irq_rx_enable(dev);                                             \
                                                                         \
    return 0;                                                            \
  }                                                                      \
                                                                         \
  static void disable_uart_irq_##idx() {                                 \
    const struct device* dev = uart_devices[idx];                        \
    uart_irq_rx_disable(dev);                                            \
  }

static void disable_uart_irq(int idx) {
  const struct device* dev = uart_devices[idx];
  uart_irq_rx_disable(dev);
}

static void uart_interrupt_callback(const struct device* dev, void* user_data) {
  uint8_t byte;

  if (!uart_irq_update(dev)) {
    return;
  }

  if (!uart_irq_rx_ready(dev)) {
    return;
  }

  while (uart_fifo_read(dev, &byte, 1) == 1) {
    LOG_INF("%s: received %c", dev->name, byte);
    uart_poll_out(dev, byte);
  }
}

static void setup_uart(int idx) {
  const struct device* dev = uart_devices[idx];
  if (!device_is_ready(dev)) {
    LOG_ERR("%s: UART device not ready", dev->name);
    uart_response->result_code = -ENODEV;
    return;
  }

  char byte;
  while (uart_fifo_read(dev, &byte, 1) == 1) {
  }

  int ret = uart_irq_callback_user_data_set(dev, uart_interrupt_callback, NULL);

  if (ret < 0) {
    if (ret == -ENOTSUP) {
      LOG_ERR("%s: Interrupt-driven UART API support not enabled", dev->name);
    } else if (ret == -ENOSYS) {
      LOG_ERR("%s: UART device does not support interrupt-driven API",
              dev->name);
    } else {
      LOG_ERR("%s: Error setting UART callback: %d", dev->name, ret);
    }

    uart_response->result_code = ret;
    return;
  }
  uart_irq_rx_enable(dev);
}

static void setup_uarts() {
  for (int i = 0; i < ARRAY_SIZE(uart_devices); i++) {
    setup_uart(i);
  }
}

static void disable_uart_interrupts() {
  for (int i = 0; i < ARRAY_SIZE(uart_devices); i++) {
    disable_uart_irq(i);
  }
}

// TODO: See that we can actually return an error code here
int setup_uart_test(const struct Request* request, struct Response* response) {
  uart_response = response;

  setup_uarts();

  uart_response = NULL;

  return 0;
}

int execute_uart_test(const struct Request* request,
                      struct Response* response) {
  return 0;
}

// TODO: See that we can actually return an error code here
int stop_uart_test(const struct Request* request, struct Response* response) {
  uart_response = response;

  disable_uart_interrupts();

  uart_response = NULL;

  return 0;
}
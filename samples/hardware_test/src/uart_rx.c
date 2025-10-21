/*
 * Copyright (c) Frickly Systems GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_rx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_rx, CONFIG_APP_LOG_LEVEL);

#include "pb.h"
#include "pb_decode.h"
#include "protobuf_helper.h"
#include "src/data.pb.h"
#include "uart_tx.h"
#include "util.h"

#include <string.h>

#include <zephyr/data/cobs.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#define PROTOBUF_BUF_SIZE 512
#define PROTOBUF_BUF_COUNT 12

NET_BUF_POOL_DEFINE(
    uart_rx_net_buf_pool, PROTOBUF_BUF_COUNT, PROTOBUF_BUF_SIZE, 0, NULL);

#define UART_RX_THREAD_STACK_SIZE 10240
#define UART_RX_THREAD_PRIORITY 5

static const struct device *uart_device;

static struct net_buf *uart_rx_buffer;
static struct net_buf *uart_rx_decode_buffer;

static K_THREAD_STACK_DEFINE(uart_rx_thread_stack, UART_RX_THREAD_STACK_SIZE);
static struct k_thread uart_rx_thread_data;
static bool thread_running = false;

static rx_callback_fn_t *uart_rx_callback = NULL;

/**
 * @brief UART RX thread function for receiving data
 */
static void uart_rx_thread_func(void *arg1, void *arg2, void *arg3) {
  uint8_t c;

  LOG_INF("UART RX thread started, listening for COBS frames");

  while (thread_running) {
    if (uart_poll_in(uart_device, &c) == 0) {
      if (c != COBS_DEFAULT_DELIMITER) {
        // Add byte to buffer if there's space
        if (net_buf_tailroom(uart_rx_buffer) > 0) {
          net_buf_add_u8(uart_rx_buffer, c);
        } else {
          LOG_HEXDUMP_WRN(uart_rx_buffer->data, uart_rx_buffer->len,
                          "UART RX buffer overflow, resetting");
          net_buf_reset(uart_rx_buffer);
        }
      } else {
        // Complete COBS frame received
        if (uart_rx_buffer->len > 0) {
          LOG_HEXDUMP_DBG(uart_rx_buffer->data, uart_rx_buffer->len,
                          "COBS encoded frame received");

          int rc = cobs_decode(uart_rx_buffer, uart_rx_decode_buffer,
                               COBS_DEFAULT_DELIMITER);

          if (!uart_rx_callback) {
            LOG_WRN("No UART RX callback registered");
          }

          rc = uart_rx_callback(uart_rx_decode_buffer->data,
                                uart_rx_decode_buffer->len);

          if (rc != 0) {
            LOG_ERR("UART RX callback failed with error %d", rc);
          }

          net_buf_reset(uart_rx_decode_buffer);
        }

        // Reset buffer for next frame
        net_buf_reset(uart_rx_buffer);
      }
    } else {
      // No data available, sleep briefly to avoid busy waiting
      k_sleep(K_USEC(100));
    }
  }

  LOG_INF("UART RX thread stopped");
}

int uart_rx_init(const struct device *uart_dev, rx_callback_fn_t *callback) {
  if (!uart_dev) {
    LOG_ERR("Invalid parameters");
    return -EINVAL;
  }

  if (!device_is_ready(uart_dev)) {
    LOG_ERR("UART device not ready");
    return -ENODEV;
  }

  uart_rx_callback = callback;
  uart_device = uart_dev;

  // Allocate RX buffers
  uart_rx_buffer = net_buf_alloc(&uart_rx_net_buf_pool, K_SECONDS(1));
  if (!uart_rx_buffer) {
    LOG_ERR("Failed to allocate UART RX buffer");
    return -ENOMEM;
  }

  uart_rx_decode_buffer = net_buf_alloc(&uart_rx_net_buf_pool, K_SECONDS(1));
  if (!uart_rx_decode_buffer) {
    LOG_ERR("Failed to allocate decoded buffer");
    net_buf_unref(uart_rx_buffer);
    return -ENOMEM;
  }

  // Initialize the buffers
  net_buf_reset(uart_rx_buffer);
  net_buf_reset(uart_rx_decode_buffer);

  LOG_INF("UART RX initialized successfully");
  return 0;
}

int uart_rx_start_thread(void) {
  if (thread_running) {
    LOG_WRN("UART RX thread is already running");
    return -EALREADY;
  }

  if (!uart_device) {
    LOG_ERR("UART RX not initialized");
    return -ENODEV;
  }

  thread_running = true;

  // Start the UART RX thread
  k_thread_create(&uart_rx_thread_data, uart_rx_thread_stack,
                  K_THREAD_STACK_SIZEOF(uart_rx_thread_stack),
                  uart_rx_thread_func, NULL, NULL, NULL,
                  UART_RX_THREAD_PRIORITY, 0, K_NO_WAIT);

  k_thread_name_set(&uart_rx_thread_data, "uart_rx");
  LOG_INF("UART RX thread started");

  return 0;
}

int uart_rx_stop_thread(void) {
  if (!thread_running) {
    LOG_WRN("UART RX thread is not running");
    return -EALREADY;
  }

  thread_running = false;

  // Wait for thread to stop
  k_thread_join(&uart_rx_thread_data, K_FOREVER);

  // Clean up buffers
  if (uart_rx_buffer) {
    net_buf_unref(uart_rx_buffer);
    uart_rx_buffer = NULL;
  }

  if (uart_rx_decode_buffer) {
    net_buf_unref(uart_rx_decode_buffer);
    uart_rx_decode_buffer = NULL;
  }

  return 0;
}
/*
 * Copyright (c) Frickly Systems GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "deps.h"
#include "uart_tx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#include "protobuf_helper.h"

#include <zephyr/data/cobs.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#define UART_TX_NET_BUF_SIZE 512
#define UART_TX_NET_BUF_COUNT 24

NET_BUF_POOL_DEFINE(
    uart_tx_net_buf_pool, UART_TX_NET_BUF_COUNT, UART_TX_NET_BUF_SIZE, 0, NULL);

// Message structure for UART TX queue
struct uart_tx_msg {
  struct net_buf *buf;
};

// Message queue for UART transmission
K_MSGQ_DEFINE(uart_tx_msgq, sizeof(struct uart_tx_msg), 16, 4);

// Thread stack and priority
#define UART_TX_THREAD_STACK_SIZE 1024 * 4
#define UART_TX_THREAD_PRIORITY 5

K_THREAD_STACK_DEFINE(uart_tx_thread_stack, UART_TX_THREAD_STACK_SIZE);
static struct k_thread uart_tx_thread_data;
static k_tid_t uart_tx_thread_id;

static const struct device *uart_device;

/**
 * @brief UART TX thread entry point
 *
 * Continuously waits for buffers from the message queue and transmits them
 * via UART using polling mode.
 */
static void uart_tx_thread(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  LOG_INF("UART TX thread started");

  struct uart_tx_msg msg;

  while (1) {
    // Wait for a message from the queue
    if (k_msgq_get(&uart_tx_msgq, &msg, K_FOREVER) == 0) {
      if (msg.buf && uart_device) {
        LOG_HEXDUMP_INF(msg.buf->data, msg.buf->len, "COBS Encoded Data");
        // Transmit the COBS encoded data
        for (int i = 0; i < msg.buf->len; i++) {
          uart_poll_out(uart_device, msg.buf->data[i]);
        }

        // Send the delimiter to properly terminate the COBS frame
        uart_poll_out(uart_device, COBS_DEFAULT_DELIMITER);

        // Release the buffer back to the pool
        net_buf_unref(msg.buf);
      }
    }
  }
}

int uart_tx_init(const struct device *uart_dev) {
  if (!uart_dev) {
    LOG_ERR("Invalid parameters");
    return -EINVAL;
  }

  if (!device_is_ready(uart_dev)) {
    LOG_ERR("UART device not ready");
    return -ENODEV;
  }

  uart_device = uart_dev;

  // Start the UART TX thread
  uart_tx_thread_id = k_thread_create(
      &uart_tx_thread_data, uart_tx_thread_stack,
      K_THREAD_STACK_SIZEOF(uart_tx_thread_stack), uart_tx_thread, NULL, NULL,
      NULL, UART_TX_THREAD_PRIORITY, 0, K_NO_WAIT);

  k_thread_name_set(uart_tx_thread_id, "uart_tx");

  LOG_INF("UART TX initialized successfully");
  return 0;
}

/**
 * @brief Generic function to encode data to protobuf and transmit via UART
 *
 * This function encapsulates the common pattern of:
 * 1. Allocating buffers
 * 2. Encoding data to protobuf
 * 3. COBS encoding and transmitting
 * 4. Handling cleanup on errors
 *
 * @param data Pointer to the data structure to encode
 * @param encode_fn Callback function to encode the data to protobuf
 * @param error_msg Error message to log if encoding fails
 * @return int 0 on success, negative error code on failure
 */
int encode_and_transmit(const void *data,
                        proto_encode_fn_t encode_fn,
                        const char *error_msg) {
  uint32_t message_length;
  struct net_buf *raw_data_buf;
  struct net_buf *cobs_encoded_buf;

  raw_data_buf = net_buf_alloc(&uart_tx_net_buf_pool, K_MSEC(5000));
  if (!raw_data_buf) {
    LOG_ERR("Failed to allocate raw data buffer (timeout after 5s)");
    return -ENOMEM;
  }

  cobs_encoded_buf = net_buf_alloc(&uart_tx_net_buf_pool, K_MSEC(5000));
  if (!cobs_encoded_buf) {
    LOG_ERR("Failed to allocate COBS encoded buffer (timeout after 5s)");
    net_buf_unref(raw_data_buf);
    return -ENOMEM;
  }

  int rc = encode_fn(data, raw_data_buf->data, net_buf_tailroom(raw_data_buf),
                     &message_length);
  if (rc < 0) {
    LOG_ERR("%s", error_msg);
    net_buf_unref(raw_data_buf);
    net_buf_unref(cobs_encoded_buf);
    return rc;
  }

  net_buf_add(raw_data_buf, message_length);

  rc = cobs_encode(raw_data_buf, cobs_encoded_buf, COBS_DEFAULT_DELIMITER);
  if (rc != 0) {
    LOG_ERR("COBS encoding failed with error %d", rc);
    net_buf_unref(raw_data_buf);
    net_buf_unref(cobs_encoded_buf);
    return rc;
  }

  // Submit the encoded buffer to the TX thread via message queue
  struct uart_tx_msg msg = {.buf = cobs_encoded_buf};

  rc = k_msgq_put(&uart_tx_msgq, &msg, K_MSEC(1000));
  if (rc != 0) {
    LOG_ERR("Failed to submit buffer to TX queue (timeout after 1s): %d", rc);
    // Clean up both buffers since transmission won't happen
    net_buf_unref(raw_data_buf);
    net_buf_unref(cobs_encoded_buf);
    return rc;
  }

  // Raw data buffer no longer needed after encoding
  net_buf_unref(raw_data_buf);

  // Don't unref cobs_encoded_buf here - the TX thread will do it after
  // transmission

  return 0;
}

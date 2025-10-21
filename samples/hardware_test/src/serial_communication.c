
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(serial_comm, CONFIG_APP_LOG_LEVEL);

#include "pb_decode.h"
#include "protobuf_helper.h"
#include "serial_communication.h"
#include "src/data.pb.h"
#include "uart_rx.h"
#include "uart_tx.h"
#include "util.h"

#include <string.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <sys/types.h>

static request_response_fn_t *serial_communication_request_handler = NULL;

int rx_callback(const uint8_t *data, size_t length) {
  LOG_HEXDUMP_DBG(data, length, "Decoded protobuf data");

  // Decode as Request message
  Request request_proto = Request_init_zero;
  struct Request request_struct = {0};

  pb_istream_t stream = pb_istream_from_buffer(data, length);

  bool decode_successful = pb_decode(&stream, Request_fields, &request_proto);

  if (!decode_successful) {
    LOG_ERR("Failed to decode Request protobuf");
    LOG_ERR("Stream bytes_left: %zu", stream.bytes_left);
    LOG_ERR("Stream errmsg: %s", PB_GET_ERROR(&stream));
    return -EINVAL;
  }

  int decode_ret = request_from_proto(&request_proto, &request_struct);
  if (decode_ret != 0) {
    LOG_ERR("Failed to convert request struct: %d", decode_ret);
    return decode_ret;
  }

  LOG_INF("Successfully decoded Request message");
  LOG_INF("Request type: %d", request_struct.type);

  struct Response response;
  memset(&response, 0, sizeof(response));

  if (!serial_communication_request_handler) {
    LOG_ERR("No request handler registered");
    return -EINVAL;
  }

  int ret = serial_communication_request_handler(&request_struct, &response);
  if (ret != 0) {
    LOG_ERR("Request handler failed with error %d", ret);
    return ret;
  }
  ret = encode_and_transmit(&response, response_to_proto,
                            "Failed to encode Response");

  if (ret != 0) {
    LOG_ERR("Failed to send response: %d", ret);
  }

  return 0;
}

int serial_communication_init(const struct device *uart,
                              request_response_fn_t *handler) {
  serial_communication_request_handler = handler;

  int ret = uart_rx_init(uart, rx_callback);
  if (ret != 0) {
    LOG_ERR("Failed to initialize UART RX: %d", ret);
    return ret;
  }

  ret = uart_tx_init(uart);
  if (ret != 0) {
    LOG_ERR("Failed to initialize UART TX: %d", ret);
    return ret;
  }

  LOG_INF("Serial communication initialized successfully");

  return 0;
}

int serial_communication_start(void) {
  int ret = uart_rx_start_thread();
  if (ret != 0) {
    LOG_ERR("Failed to start UART RX thread: %d", ret);
    return ret;
  }

  return 0;
}

int serial_communication_stop(void) {
  int ret = uart_rx_stop_thread();
  if (ret != 0) {
    LOG_ERR("Failed to stop UART RX thread: %d", ret);
    return ret;
  }

  return 0;
}
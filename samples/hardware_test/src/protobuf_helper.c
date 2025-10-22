/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "deps.h"
#include "src/data.pb.h"
#include "util.h"

#include <stddef.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "protobuf_helper.h"

#include <zephyr/drivers/sensor.h>

bool encode_string(pb_ostream_t* stream,
                   const pb_field_t* field,
                   void* const* arg) {
  if (!pb_encode_tag_for_field(stream, field)) {
    return false;
  }

  const char* str = (const char*)(*arg);
  return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

struct BytesData {
  const uint8_t* data;
  size_t length;
};

struct StringArray {
  const char* const* strings;
  size_t count;
};

bool encode_bytes(pb_ostream_t* stream,
                  const pb_field_t* field,
                  void* const* arg) {
  if (!pb_encode_tag_for_field(stream, field)) {
    return false;
  }

  const struct BytesData* bytes_data = (struct BytesData*)(*arg);
  return pb_encode_string(stream, bytes_data->data, bytes_data->length);
}

int request_from_proto(Request* proto, struct Request* request) {
  if (!proto || !request) {
    LOG_ERR("Invalid parameters for request_from_proto");
    return -EINVAL;
  }

  switch (proto->type) {
    case RequestType_GET_DEVICE_INFO:
      request->type = REQUEST_TYPE__GET_DEVICE_INFO;
      break;
    case RequestType_SETUP_GPIO_TEST:
      request->type = REQUEST_TYPE__SETUP_GPIO_TEST;
      break;
    case RequestType_EXECUTE_GPIO_TEST:
      request->type = REQUEST_TYPE__EXECUTE_GPIO_TEST;
      break;
    case RequestType_STOP_GPIO_TEST:
      request->type = REQUEST_TYPE__STOP_GPIO_TEST;
      break;
    case RequestType_SETUP_UART_TEST:
      request->type = REQUEST_TYPE__SETUP_UART_TEST;
      break;
    case RequestType_EXECUTE_UART_TEST:
      request->type = REQUEST_TYPE__EXECUTE_UART_TEST;
      break;
    case RequestType_STOP_UART_TEST:
      request->type = REQUEST_TYPE__STOP_UART_TEST;
      break;
    default:
      LOG_ERR("Unknown RequestType: %d", proto->type);
      return -EINVAL;
  }

  return 0;
}

static int device_role_to_proto(enum DeviceRole role, DeviceRole* proto_role) {
  switch (role) {
    case DEVICE_ROLE__TESTER:
      *proto_role = DeviceRole_TESTER;
      return 0;
    case DEVICE_ROLE__SUT:
      *proto_role = DeviceRole_SUT;
      return 0;
    default:
      LOG_ERR("Unknown DeviceRole: %d", role);
      return -EINVAL;
  }
}

static int response_device_info_to_proto(const struct Response* resp,
                                         uint8_t* buffer,
                                         size_t buffer_size,
                                         size_t* message_length) {
  Response prot_response = Response_init_zero;

  device_role_to_proto(resp->role, &prot_response.role);

  prot_response.result_code = resp->result_code;

  // BytesData must remain valid during pb_encode call
  // So we declare it outside the inner scope
  struct BytesData device_id_data = {
    .data = resp->payload.device_info.device_id,
    .length = resp->payload.device_info.device_id_length,
  };

  // Set which oneof field is active
  prot_response.which_payload = Response_device_info_tag;

  prot_response.payload.device_info.device_id.funcs.encode = encode_bytes;
  prot_response.payload.device_info.device_id.arg = &device_id_data;

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

  if (!pb_encode(&stream, Response_fields, &prot_response)) {
    LOG_ERR("Failed to encode DeviceInfo response");
    return -EINVAL;
  }

  *message_length = stream.bytes_written;
  return 0;
}

static int response_gpio_to_proto(const struct Response* resp,
                                  uint8_t* buffer,
                                  size_t buffer_size,
                                  size_t* message_length) {
  Response prot_response = Response_init_zero;

  prot_response.result_code = resp->result_code;
  prot_response.role = DeviceRole_TESTER;
  prot_response.which_payload = Response_gpio_response_tag;

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

  if (!pb_encode(&stream, Response_fields, &prot_response)) {
    LOG_ERR("Failed to encode GPIO response");
    return -EINVAL;
  }

  *message_length = stream.bytes_written;
  return 0;
}

bool encode_uart_list(pb_ostream_t* stream,
                      const pb_field_t* field,
                      void* const* arg) {
  const struct UartInfo* uart_info = (const struct UartInfo*)(*arg);

  // Encode a single UART message
  UART uart_msg = UART_init_zero;

  // Set up device name callback
  uart_msg.device.funcs.encode = encode_string;
  uart_msg.device.arg = (void*)uart_info->name;

  // Set up receive_data callback
  struct BytesData receive_data = {
    .data = (const uint8_t*)uart_info->receive_buffer,
    .length = uart_info->receive_len,
  };
  uart_msg.receive_data.funcs.encode = encode_bytes;
  uart_msg.receive_data.arg = &receive_data;

  // Set up send_data callback
  struct BytesData send_data = {
    .data = (const uint8_t*)uart_info->send_buffer,
    .length = uart_info->send_len,
  };
  uart_msg.send_data.funcs.encode = encode_bytes;
  uart_msg.send_data.arg = &send_data;

  // Encode the UART submessage
  if (!pb_encode_tag_for_field(stream, field)) {
    return false;
  }

  return pb_encode_submessage(stream, UART_fields, &uart_msg);
}

static int response_uart_to_proto(const struct Response* resp,
                                  uint8_t* buffer,
                                  size_t buffer_size,
                                  size_t* message_length) {
  Response prot_response = Response_init_zero;

  device_role_to_proto(resp->role, &prot_response.role);
  prot_response.result_code = resp->result_code;
  prot_response.which_payload = Response_uart_response_tag;

  // Set up the uarts callback to encode the UART data
  prot_response.payload.uart_response.uarts.funcs.encode = encode_uart_list;
  prot_response.payload.uart_response.uarts.arg =
      (void*)&resp->payload.uart_response;

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

  if (!pb_encode(&stream, Response_fields, &prot_response)) {
    LOG_ERR("Failed to encode UART response");
    return -EINVAL;
  }

  *message_length = stream.bytes_written;
  return 0;
}

int response_to_proto(const void* response,
                      uint8_t* buffer,
                      size_t buffer_size,
                      size_t* message_length) {
  const struct Response* resp = (const struct Response*)response;

  switch (resp->payload_type) {
    case RESPONSE_TYPE__DEVICE_INFO:
      return response_device_info_to_proto(resp, buffer, buffer_size,
                                           message_length);
    case RESPONSE_TYPE__GPIO:
      return response_gpio_to_proto(resp, buffer, buffer_size, message_length);
    case RESPONSE_TYPE__UART:
      return response_uart_to_proto(resp, buffer, buffer_size, message_length);
    default:
      LOG_ERR("Unknown response payload type: %d", resp->payload_type);
      return -EINVAL;
  }
}

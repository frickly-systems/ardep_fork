/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "src/data.pb.h"
#include "util.h"

#include <stddef.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME sut
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "protobuf_helper.h"

#include <zephyr/drivers/sensor.h>

bool encode_string(pb_ostream_t *stream,
                   const pb_field_t *field,
                   void *const *arg) {
  if (!pb_encode_tag_for_field(stream, field)) {
    return false;
  }

  const char *str = (const char *)(*arg);
  return pb_encode_string(stream, (uint8_t *)str, strlen(str));
}

struct BytesData {
  const uint8_t *data;
  size_t length;
};

struct StringArray {
  const char *const *strings;
  size_t count;
};

bool encode_bytes(pb_ostream_t *stream,
                  const pb_field_t *field,
                  void *const *arg) {
  if (!pb_encode_tag_for_field(stream, field)) {
    return false;
  }

  const struct BytesData *bytes_data = (struct BytesData *)(*arg);
  return pb_encode_string(stream, bytes_data->data, bytes_data->length);
}

static bool encode_string_list(pb_ostream_t *stream,
                               const pb_field_t *field,
                               void *const *arg) {
  const struct StringArray *string_array = (const struct StringArray *)(*arg);

  for (size_t i = 0; i < string_array->count; ++i) {
    const char *entry = string_array->strings[i];

    if (!entry) {
      continue;
    }

    if (!pb_encode_tag_for_field(stream, field)) {
      return false;
    }

    if (!pb_encode_string(stream, (const uint8_t *)entry, strlen(entry))) {
      return false;
    }
  }

  return true;
}

int request_from_proto(Request *proto, struct Request *request) {
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
    default:
      LOG_ERR("Unknown RequestType: %d", proto->type);
      return -EINVAL;
  }

  return 0;
}

static int response_device_info_to_proto(const struct Response *resp,
                                         uint8_t *buffer,
                                         size_t buffer_size,
                                         size_t *message_length) {
  Response prot_response = Response_init_zero;

  prot_response.result_code = resp->result_code;
  prot_response.role = DeviceRole_TESTER;

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

static int response_gpio_to_proto(const struct Response *resp,
                                  uint8_t *buffer,
                                  size_t buffer_size,
                                  size_t *message_length) {
  Response prot_response = Response_init_zero;

  prot_response.result_code = resp->result_code;
  prot_response.role = DeviceRole_TESTER;
  prot_response.which_payload = Response_gpio_response_tag;

  struct StringArray errors = {
    .strings = (const char *const *)resp->payload.gpio_response.errors,
    .count = resp->payload.gpio_response.errors_length,
  };

  prot_response.payload.gpio_response.errors.funcs.encode = encode_string_list;
  prot_response.payload.gpio_response.errors.arg = &errors;

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

  if (!pb_encode(&stream, Response_fields, &prot_response)) {
    LOG_ERR("Failed to encode GPIO response");
    return -EINVAL;
  }

  *message_length = stream.bytes_written;
  return 0;
}

int response_to_proto(const void *response,
                      uint8_t *buffer,
                      size_t buffer_size,
                      size_t *message_length) {
  const struct Response *resp = (const struct Response *)response;

  switch (resp->payload_type) {
    case RESPONSE_TYPE__DEVICE_INFO:
      return response_device_info_to_proto(resp, buffer, buffer_size,
                                           message_length);
    case RESPONSE_TYPE__GPIO:
      return response_gpio_to_proto(resp, buffer, buffer_size, message_length);
    default:
      LOG_ERR("Unknown response payload type: %d", resp->payload_type);
      return -EINVAL;
  }
}

// #ifdef CONFIG_CES2026_BMM350

// int bmm350_data_to_proto(const struct bmm350_data *data, uint8_t *buffer,
//                          size_t buffer_size, size_t *message_length) {
//   // Start from a clean SensorMessage instance
//   SensorMessage message = SensorMessage_init_zero;

//   // Set that we have sensor data
//   message.has_data = true;

//   // Set that we have BMM350 data
//   message.data.has_bmm350 = true;

//   // Set magnetometer data
//   if (data->has_magnetic_flux_density) {
//     message.data.bmm350.has_mag = true;
//     message.data.bmm350.mag.x =
//         sensor_value_to_double(&data->magnetic_flux_density.x) *
//         100.0; // Convert [G] to [uT]
//     message.data.bmm350.mag.y =
//         sensor_value_to_double(&data->magnetic_flux_density.y) *
//         100.0; // Convert [G] to [uT]
//     message.data.bmm350.mag.z =
//         sensor_value_to_double(&data->magnetic_flux_density.z) *
//         100.0; // Convert [G] to [uT]
//   }

//   // Set temperature data
//   if (data->has_temperature) {
//     message.data.bmm350.has_temperature = true;
//     message.data.bmm350.temperature.temperature =
//         sensor_value_to_double(&data->temperature);
//   }

//   pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

//   if (!pb_encode(&stream, SensorMessage_fields, &message)) {
//     LOG_ERR("Failed to encode SensorMessage with BMM350 data");
//     return -EINVAL;
//   }

//   *message_length = stream.bytes_written;
//   return 0;
// }

// int bmm350_config_to_proto(const struct bmm350_manager_config *config,
//                            uint8_t *buffer, size_t buffer_size,
//                            size_t *message_length) {
//   // Start from a clean SensorMessage instance
//   SensorMessage message = SensorMessage_init_zero;

//   // Set that we have sensor config
//   message.has_config = true;

//   // Set that we have BMM350 config
//   message.config.has_bmm350_config = true;

//   if (config->has_power_mode) {
//     LOG_INF("Encoding BMM350 power mode config");
//     message.config.bmm350_config.has_power_mode = true;
//     message.config.bmm350_config.power_mode.power_mode = config->power_mode;
//   }

//   if (config->has_odr_performance) {
//     LOG_INF("Encoding BMM350 ODR performance config");
//     message.config.bmm350_config.has_odr_performance = true;
//     message.config.bmm350_config.odr_performance.odr =
//     config->output_data_rate;
//     message.config.bmm350_config.odr_performance.performance =
//         config->measurement_averages;
//   }

//   if (config->has_enable_axes) {
//     LOG_INF("Encoding BMM350 enable axes config");
//     message.config.bmm350_config.has_enable_axes = true;
//     message.config.bmm350_config.enable_axes.x = config->enable_x_axis;
//     message.config.bmm350_config.enable_axes.y = config->enable_y_axis;
//     message.config.bmm350_config.enable_axes.z = config->enable_z_axis;
//   }

//   pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

//   if (!pb_encode(&stream, SensorMessage_fields, &message)) {
//     LOG_ERR("Failed to encode SensorMessage with BMM350 config");
//     return -EINVAL;
//   }

//   *message_length = stream.bytes_written;
//   return 0;
// }

// int bmm350_config_from_proto(const Bmm350Config *proto,
//                              struct bmm350_manager_config *config) {
//   if (proto->has_power_mode) {
//     config->has_power_mode = true;
//     config->power_mode = proto->power_mode.power_mode;
//   }

//   if (proto->has_odr_performance) {
//     config->has_odr_performance = true;
//     config->output_data_rate = proto->odr_performance.odr;
//     config->measurement_averages = proto->odr_performance.performance;
//   }

//   if (proto->has_enable_axes) {
//     config->has_enable_axes = true;
//     config->enable_x_axis = proto->enable_axes.x;
//     config->enable_y_axis = proto->enable_axes.y;
//     config->enable_z_axis = proto->enable_axes.z;
//   }

//   return 0;
// }

// #endif // CONFIG_CES2026_BMM350

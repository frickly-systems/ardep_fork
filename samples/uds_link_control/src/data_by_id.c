/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_link_ctrl_sample, LOG_LEVEL_DBG);

#include "uds.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

const uint16_t primitive_type_id = 0x50;
uint16_t primitive_type = 5;
uint16_t primitive_type_size = sizeof(primitive_type);

UDSErr_t read_data_by_id_check(const struct uds_context *const context,
                               bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_data_by_id_action(struct uds_context *const context,
                                bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;

  LOG_INF("Reading data id: 0x%02X", args->dataId);

  uint8_t temp[5] = {0};

  if (args->dataId == primitive_type_id) {
    uint16_t t = sys_cpu_to_be16(
        *(uint16_t *)context->registration->data_identifier.data);
    memcpy(temp, &t, sizeof(t));
  }

  *consume_event = true;

  return args->copy(
      context->server, temp,
      *(uint16_t *)context->registration->data_identifier.user_context);
}

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        primitive_type_id,
                                        &primitive_type,
                                        read_data_by_id_check,
                                        read_data_by_id_action,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &primitive_type_size);

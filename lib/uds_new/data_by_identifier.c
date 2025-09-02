/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_new.h"
#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "data_by_identifier.h"

#include <zephyr/kernel.h>

UDSErr_t uds_new_handle_read_data_by_identifier(
    struct uds_new_instance_t* instance, UDSEvent_t event, void* arg) {
  bool found_at_least_one_match = false;

  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    struct uds_new_context context = {.instance = instance,
                                      .registration = reg,
                                      .event = event,
                                      .arg = arg,
                                      .additional_param = NULL};

    bool apply_action = false;
    UDSErr_t ret = reg->data_identifier.read.check(&context, &apply_action);
    if (ret != UDS_OK) {
      LOG_WRN(
          "Check failed for Read Data Identifier 0x%04X. Registration addr: "
          "%p. Err: %d",
          reg->data_identifier.data_id, (void*)reg, ret);
      return ret;
    }

    if (!apply_action) {
      continue;
    }

    found_at_least_one_match = true;
    bool consume_event = true;
    ret = reg->data_identifier.read.action(&context, &consume_event);
    if (ret != UDS_OK) {
      LOG_WRN(
          "Action failed for Read Data Identifier 0x%04X. Registration addr: "
          "%p. Err: %d",
          reg->data_identifier.data_id, (void*)reg, ret);
      return ret;
    }

    if (consume_event) {
      return ret;
    }
  }

  if (!found_at_least_one_match) {
    return UDS_NRC_RequestOutOfRange;
  }

  return UDS_PositiveResponse;
}

UDSErr_t uds_new_handle_write_data_by_identifier(
    struct uds_new_instance_t* instance, UDSEvent_t event, void* arg) {
  bool found_at_least_one_match = false;

  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    struct uds_new_context context = {.instance = instance,
                                      .registration = reg,
                                      .event = event,
                                      .arg = arg,
                                      .additional_param = NULL};

    bool apply_action = false;
    UDSErr_t ret = reg->data_identifier.write.check(&context, &apply_action);
    if (ret != UDS_OK) {
      LOG_WRN(
          "Check failed for Write Data Identifier 0x%04X. Registration addr: "
          "%p. Err: %d",
          reg->data_identifier.data_id, (void*)reg, ret);
      return ret;
    }

    if (!apply_action) {
      continue;
    }

    found_at_least_one_match = true;
    bool consume_event = true;
    ret = reg->data_identifier.write.action(&context, &consume_event);
    if (ret != UDS_OK) {
      LOG_WRN(
          "Action failed for Write Data Identifier 0x%04X. Registration addr: "
          "%p. Err: %d",
          reg->data_identifier.data_id, (void*)reg, ret);
      return ret;
    }

    if (consume_event) {
      return UDS_PositiveResponse;
    }
  }

  if (!found_at_least_one_match) {
    return UDS_NRC_RequestOutOfRange;
  }

  return UDS_PositiveResponse;
}

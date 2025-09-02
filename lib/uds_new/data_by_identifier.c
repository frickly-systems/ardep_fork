/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_new.h"
#include "iso14229.h"
#include "uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "data_by_identifier.h"

#include <zephyr/kernel.h>

UDSErr_t uds_new_handle_read_data_by_identifier(
    struct uds_new_instance_t* instance, UDSEvent_t event, void* arg) {
  bool found_at_least_one_match = false;

  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    bool consume_event = true;
    int ret = _uds_new_check_and_act_on_event(
        instance, reg, reg->data_identifier.read.check,
        reg->data_identifier.read.action, event, arg, &found_at_least_one_match,
        &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }
  }

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
  struct uds_new_registration_t* reg = instance->dynamic_registrations;
  while (reg != NULL) {
    bool consume_event = false;
    int ret = _uds_new_check_and_act_on_event(
        instance, reg, reg->data_identifier.read.check,
        reg->data_identifier.read.action, event, arg, &found_at_least_one_match,
        &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }

    reg = reg->next;
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

  if (!found_at_least_one_match) {
    return UDS_NRC_RequestOutOfRange;
  }

  return UDS_PositiveResponse;
}

UDSErr_t uds_new_handle_write_data_by_identifier(
    struct uds_new_instance_t* instance, UDSEvent_t event, void* arg) {
  bool found_at_least_one_match = false;

  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    bool consume_event = true;
    int ret = _uds_new_check_and_act_on_event(
        instance, reg, reg->data_identifier.write.check,
        reg->data_identifier.write.action, event, arg,
        &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }
  }

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
  struct uds_new_registration_t* reg = instance->dynamic_registrations;
  while (reg != NULL) {
    bool consume_event = false;
    int ret = _uds_new_check_and_act_on_event(
        instance, reg, reg->data_identifier.write.check,
        reg->data_identifier.write.action, event, arg,
        &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }

    reg = reg->next;
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

  if (!found_at_least_one_match) {
    return UDS_NRC_RequestOutOfRange;
  }

  return UDS_PositiveResponse;
}

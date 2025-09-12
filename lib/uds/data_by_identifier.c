/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "data_by_identifier.h"

bool uds_new_filter_for_data_by_id_event(UDSEvent_t event) {
  return event == UDS_EVT_ReadDataByIdent || event == UDS_EVT_WriteDataByIdent;
}

uds_new_check_fn uds_new_get_check_for_read_data_by_identifier(
    const struct uds_new_registration_t* const reg) {
  return reg->data_identifier.read.check;
}
uds_new_action_fn uds_new_get_action_for_read_data_by_identifier(
    const struct uds_new_registration_t* const reg) {
  return reg->data_identifier.read.action;
}

uds_new_check_fn uds_new_get_check_for_write_data_by_identifier(
    const struct uds_new_registration_t* const reg) {
  return reg->data_identifier.write.check;
}
uds_new_action_fn uds_new_get_action_for_write_data_by_identifier(
    const struct uds_new_registration_t* const reg) {
  return reg->data_identifier.write.action;
}
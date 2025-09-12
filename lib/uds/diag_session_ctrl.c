/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "diag_session_ctrl.h"

bool uds_filter_for_diag_session_ctrl_event(UDSEvent_t event) {
  return event == UDS_EVT_DiagSessCtrl || event == UDS_EVT_SessionTimeout;
}

uds_check_fn uds_get_check_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.read.check;
}
uds_action_fn uds_get_action_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.read.action;
}

uds_check_fn uds_get_check_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.write.check;
}
uds_action_fn uds_get_action_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.write.action;
}
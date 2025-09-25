/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

uds_check_fn uds_get_check_for_link_control(
    const struct uds_registration_t* const reg) {
  return reg->link_control.actor.check;
}

uds_action_fn uds_get_action_for_link_control(
    const struct uds_registration_t* const reg) {
  return reg->link_control.actor.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_link_control_) = {
  .event = UDS_EVT_LinkControl,
  .registration_type = UDS_REGISTRATION_TYPE__LINK_CONTROL,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .get_check = uds_get_check_for_link_control,
  .get_action = uds_get_action_for_link_control,
};

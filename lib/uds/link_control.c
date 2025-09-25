/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_UDS_USE_LINK_CONTROL

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

#include <zephyr/drivers/can.h>

uds_check_fn uds_get_check_for_link_control(
    const struct uds_registration_t *const reg) {
  return reg->link_control.actor.check;
}

uds_action_fn uds_get_action_for_link_control(
    const struct uds_registration_t *const reg) {
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

static uint8_t uds_default_link_control_handler_modifier = 0;
static bool uds_default_link_control_handler_modifier_set = false;

UDSErr_t uds_check_default_link_control(const struct uds_context *const context,
                                        bool *apply_action) {
  UDSLinkCtrlArgs_t *args = context->arg;

  switch (args->type) {
    case UDS_LINK_CONTROL__VERIFY_MODE_TRANSITION_WITH_FIXED_PARAMETER: {
      uint8_t modifier = *(uint8_t *)args->data;

      if (modifier != UDS_LINK_CONTROL_MODIFIER__CAN_125000_BAUD &&
          modifier != UDS_LINK_CONTROL_MODIFIER__CAN_250000_BAUD &&
          modifier != UDS_LINK_CONTROL_MODIFIER__CAN_500000_BAUD) {
        LOG_WRN("Link control: Unsupported link modifier 0x%02X", modifier);
        return UDS_NRC_RequestOutOfRange;
      }

      *apply_action = true;
      return UDS_OK;
    }
    case UDS_LINK_CONTROL__TRANSITION_MODE:
      if (!uds_default_link_control_handler_modifier_set) {
        *apply_action = false;
        return UDS_NRC_RequestSequenceError;
      }
      return UDS_OK;
    default:
      return UDS_NRC_SubFunctionNotSupported;
  }
}

UDSErr_t uds_action_default_link_control(struct uds_context *const context,
                                         bool *consume_event) {
  UDSLinkCtrlArgs_t *args = context->arg;
  uint8_t modifier = *(uint8_t *)args->data;

  switch (args->type) {
    case UDS_LINK_CONTROL__VERIFY_MODE_TRANSITION_WITH_FIXED_PARAMETER: {
      uds_default_link_control_handler_modifier = modifier;
      uds_default_link_control_handler_modifier_set = true;

      *consume_event = true;
      return UDS_OK;
    }
    case UDS_LINK_CONTROL__TRANSITION_MODE:
      return uds_set_can_bitrate(
          context->instance->can_dev,
          uds_link_control_modifier_to_baudrate(modifier));

    default:
      return UDS_NRC_SubFunctionNotSupported;
  }
}

#endif  // CONFIG_UDS_USE_LINK_CONTROL
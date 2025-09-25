/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_UDS_H
#define ARDEP_LIB_UDS_UDS_H

#include <ardep/iso14229.h>
#include <ardep/uds.h>
#include <iso14229.h>

/**
 * @brief Associated events with other data required to handle them
 */
struct uds_event_handler_data {
  UDSEvent_t event;
  uds_get_check_fn get_check;
  uds_get_action_fn get_action;
  UDSErr_t default_nrc;
  enum uds_registration_type_t registration_type;
};

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

/**
 * @brief internal variant of `uds_register_event_handler`
 *
 * returns the heap allocated registration object to allow modifications
 * after the registration if required.
 */
int uds_register_event_handler(struct uds_instance_t* inst,
                               struct uds_registration_t registration,
                               uint32_t* dynamic_id,
                               struct uds_registration_t** registration_out);

/**
 * @brief Event handler callback registered with the iso14229 lib
 *
 * This is the main entry point for UDS events to be handled
 */
UDSErr_t uds_event_callback(struct iso14229_zephyr_instance* inst,
                            UDSEvent_t event,
                            void* arg,
                            void* user_context);

#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION

/**
 * @brief Transitions the CAN controller to the specified baudrate
 *
 * @param can_dev the CAN device
 * @param baud_rate the target baudrate in bit/s
 * @return UDS_OK on success, otherwise an appropriate negative error code
 */
UDSErr_t transition_can_modes(const struct device* can_dev, uint32_t baud_rate);

#endif  // ARDEP_LIB_UDS_UDS_H
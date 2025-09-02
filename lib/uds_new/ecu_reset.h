/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_NEW_ECU_RESET_H
#define ARDEP_LIB_UDS_NEW_ECU_RESET_H

#include <ardep/uds_new.h>

uds_new_check_fn uds_new_get_check_for_ecu_reset(
    const struct uds_new_registration_t* const reg);
uds_new_action_fn uds_new_get_action_for_ecu_reset(
    const struct uds_new_registration_t* const reg);

uds_new_check_fn uds_new_get_check_for_execute_scheduled_reset(
    const struct uds_new_registration_t* const reg);
uds_new_action_fn uds_new_get_action_for_execute_scheduled_reset(
    const struct uds_new_registration_t* const reg);

UDSErr_t uds_new_check_ecu_hard_reset(
    const struct uds_new_context* const context, bool* apply_action);
UDSErr_t uds_new_action_ecu_hard_reset(struct uds_new_context* const context,
                                       bool* consume_event);

UDSErr_t uds_new_check_execute_scheduled_reset(
    const struct uds_new_context* const context, bool* apply_action);
UDSErr_t uds_new_action_execute_scheduled_reset(
    struct uds_new_context* const context, bool* consume_event);

#endif  // ARDEP_LIB_UDS_NEW_ECU_RESET_H
/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_NEW_UDS_H
#define ARDEP_LIB_UDS_NEW_UDS_H

#include <ardep/iso14229.h>
#include <ardep/uds_new.h>
#include <iso14229.h>

UDSErr_t _uds_new_check_and_act_on_event(struct uds_new_instance_t* instance,
                                         struct uds_new_registration_t* reg,
                                         uds_new_check_fn check,
                                         uds_new_action_fn action,
                                         UDSEvent_t event,
                                         void* arg,
                                         bool* found_at_least_one_match,
                                         bool* consume_event);
#endif  // ARDEP_LIB_UDS_NEW_UDS_H
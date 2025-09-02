/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ARDEP_LIB_UDS_NEW_DATA_BY_IDENTIFIER_H
#define ARDEP_LIB_UDS_NEW_DATA_BY_IDENTIFIER_H

#include "iso14229.h"

#include <ardep/iso14229.h>
#include <ardep/uds_new.h>

#pragma once

UDSErr_t uds_new_handle_read_data_by_identifier(
    struct uds_new_instance_t* instance, UDSEvent_t event, void* arg);

UDSErr_t uds_new_handle_write_data_by_identifier(
    struct uds_new_instance_t* instance, UDSEvent_t event, void* arg);

#endif  // ARDEP_LIB_UDS_NEW_DATA_BY_IDENTIFIER_H
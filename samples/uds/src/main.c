/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

// Override the default user context function to provide our authentication
// data structure
void uds_default_instance_user_context(void **user_context) {
  *user_context = &auth_data;
}

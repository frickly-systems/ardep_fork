/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_INCLUDE_CAN_LOG_H_
#define ARDEP_INCLUDE_CAN_LOG_H_

#include <stdint.h>

#ifdef CONFIG_CAN_LOG_ADDRESS_PROVIDER_EXTERNAL
/**
 * @brief Get the CAN ID that should be used for CAN logging
 * @note Must be implemented by the user if CAN_LOG_ADDRESS_PROVIDER_EXTERNAL is
 *       enabled
 * @return uint16_t CAN ID
 */
uint16_t can_log_get_id();
#endif  // CONFIG_CAN_LOG_ADDRESS_PROVIDER_EXTERNAL

#endif  // ARDEP_INCLUDE_CAN_LOG_H_

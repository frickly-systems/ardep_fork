/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) 2025 MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <ardep/can_log.h>
#include <ardep/drivers/binary_encoded_gpio.h>

LOG_MODULE_REGISTER(gearshift_can_log_address_provider,
                    CONFIG_GEARSHIFT_ADDRESS_PROVIDERS_LOG_LEVEL);

static const struct device* gearshift_dev =
    DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gearshift));

uint16_t can_log_get_id() {
  int ret = binary_encoded_gpios_get_value(gearshift_dev);
  if (ret < 0) {
    LOG_ERR("Failed to read gearshift position: %d", ret);
    return CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER_BASE_ID;
  }

  return CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER_BASE_ID + ret;
}

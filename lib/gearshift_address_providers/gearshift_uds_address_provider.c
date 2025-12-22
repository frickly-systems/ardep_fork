/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) 2025 MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <ardep/drivers/binary_encoded_gpio.h>
#include <ardep/uds.h>

LOG_MODULE_REGISTER(gearshift_uds_address_provider,
                    CONFIG_GEARSHIFT_ADDRESS_PROVIDERS_LOG_LEVEL);

static const struct device* gearshift_dev =
    DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gearshift));

UDSISOTpCConfig_t uds_default_instance_get_addresses() {
  UDSISOTpCConfig_t cfg = {
    // Hardware Addresses
    .source_addr = CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER_BASE_PHYS_SA,
    .target_addr = CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER_BASE_PHYS_TA,

    // Functional Addresses
    .source_addr_func = UDS_TP_NOOP_ADDR,
    .target_addr_func = UDS_TP_NOOP_ADDR,
  };

  int ret = binary_encoded_gpios_get_value(gearshift_dev);
  if (ret < 0) {
    LOG_ERR("Failed to read gearshift position: %d", ret);
    return cfg;
  }

  cfg.source_addr += ret;
  cfg.target_addr += ret;

  return cfg;
}

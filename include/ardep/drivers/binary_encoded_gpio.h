/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_INCLUDE_DRIVERS_BINARY_ENCODED_GPIO_H_
#define ARDEP_INCLUDE_DRIVERS_BINARY_ENCODED_GPIO_H_

#include <zephyr/device.h>

__subsystem struct binary_encoded_gpio_driver_api {
  /**
   * @brief Get the current binary encoded GPIO value
   *
   * @param device Pointer to the binary encoded GPIO device
   * @return int Current value or negative error code
   */
  int (*get_value)(const struct device *device);
};

/**
 * @brief Get the current binary encoded GPIO value
 *
 * @param device Pointer to the binary encoded GPIO device
 * @return int Current value or negative error code
 */
__syscall int binary_encoded_gpios_get_value(const struct device *device);

static inline int z_impl_binary_encoded_gpios_get_value(
    const struct device *device) {
  const struct binary_encoded_gpio_driver_api *api = device->api;
  return api->get_value(device);
}

#include <syscalls/binary_encoded_gpio.h>

#endif  // ARDEP_INCLUDE_DRIVERS_BINARY_ENCODED_GPIO_H_

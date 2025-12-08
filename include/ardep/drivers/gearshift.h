/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_INCLUDE_DRIVERS_GEARSHIFT_H_
#define ARDEP_INCLUDE_DRIVERS_GEARSHIFT_H_

#include <zephyr/device.h>

__subsystem struct gearshift_driver_api {
  /**
   * @brief Get the current gearshift position
   *
   * @param device Pointer to the gearshift device
   * @return int Current gearshift position or negative error code
   */
  int (*get_position)(const struct device *device);
};

__syscall int gearshift_get_position(const struct device *device);

static inline int z_impl_gearshift_get_position(const struct device *device) {
  const struct gearshift_driver_api *api = device->api;
  return api->get_position(device);
}

#include <syscalls/gearshift.h>

#endif  // ARDEP_INCLUDE_DRIVERS_GEARSHIFT_H_

/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/binary_encoded_gpio.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static const struct device* gearshift_dev =
    DEVICE_DT_GET(DT_NODELABEL(gearshift));

int main(void) {
  if (!device_is_ready(gearshift_dev)) {
    LOG_ERR("Gearshift device not ready");
    return -ENODEV;
  }

  LOG_INF("Starting Gearshift sample application");

  while (1) {
    int position = binary_encoded_gpios_get_value(gearshift_dev);
    if (position < 0) {
      LOG_ERR("Failed to read gearshift position: %d", position);
    } else {
      LOG_INF("Current gearshift position: %d", position);
    }

    k_sleep(K_SECONDS(1));
  }

  return 0;
}

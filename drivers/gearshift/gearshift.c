/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) 2025 MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ardep_gearshift

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/gearshift.h>

LOG_MODULE_REGISTER(gearshift, CONFIG_GEARSHIFT_LOG_LEVEL);

#define GEARSHIFT_PIN_COUNT 3

struct gearshift_config {
  struct gpio_dt_spec gear_pins[GEARSHIFT_PIN_COUNT];
};

static int get_position(const struct device *device) {
  const struct gearshift_config *config = device->config;
  int position = 0;

  for (int i = 0; i < GEARSHIFT_PIN_COUNT; i++) {
    int ret = gpio_pin_get_dt(&config->gear_pins[i]);
    if (ret < 0) {
      LOG_ERR("Failed to read gear pin %d: %d", i, ret);
      return ret;
    }

    position |= (ret << i);
  }

  LOG_DBG("Current gearshift position: %d", position);

  return position;
}

static int gearshift_init(const struct device *dev) {
  const struct gearshift_config *config = dev->config;

  LOG_DBG("Initializing gearshift driver");

  for (int i = 0; i < GEARSHIFT_PIN_COUNT; i++) {
    if (!device_is_ready(config->gear_pins[i].port)) {
      return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&config->gear_pins[i], GPIO_INPUT);
    if (ret < 0) {
      return ret;
    }
  }

  LOG_DBG("Gearshift driver initialized successfully");

  return 0;
}

DEVICE_API(gearshift, gearshift_api) = {
  .get_position = get_position,
};

BUILD_ASSERT(CONFIG_GEARSHIFT_INIT_PRIORITY > CONFIG_GPIO_INIT_PRIORITY,
             "GEARSHIFT_INIT_PRIORITY must be higher than GPIO_INIT_PRIORITY");

#define GEARSHIFT_INIT(inst)                                                   \
  static const struct gearshift_config gearshift_config_##inst = {             \
    .gear_pins =                                                               \
        {                                                                      \
          DT_INST_FOREACH_PROP_ELEM_SEP(inst, gearshift_gpios,                 \
                                        GPIO_DT_SPEC_GET_BY_IDX, (, )),        \
        },                                                                     \
  };                                                                           \
  BUILD_ASSERT(DT_INST_PROP_LEN(inst, gearshift_gpios) == GEARSHIFT_PIN_COUNT, \
               "Gearshift driver requires exactly " STRINGIFY(GEARSHIFT_PIN_COUNT) " gearshift_gpios");                   \
  DEVICE_DT_INST_DEFINE(inst, gearshift_init, NULL, NULL,                      \
                        &gearshift_config_##inst, POST_KERNEL,                 \
                        CONFIG_GEARSHIFT_INIT_PRIORITY, &gearshift_api);

DT_INST_FOREACH_STATUS_OKAY(GEARSHIFT_INIT);

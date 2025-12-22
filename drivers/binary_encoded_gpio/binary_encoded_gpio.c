/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) 2025 MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_binary_encoded_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/binary_encoded_gpio.h>

// number of bits for an integer minus sign bit
#define MAXIMUM_SUPPORTED_PIN_COUNT (sizeof(int) * 8 - 1)

LOG_MODULE_REGISTER(binary_encoded_gpio, CONFIG_BINARY_ENCODED_GPIO_LOG_LEVEL);

struct binary_encoded_gpio_config {
  const struct gpio_dt_spec *input_pins;
  size_t pin_count;
};

static int get_value(const struct device *device) {
  const struct binary_encoded_gpio_config *config = device->config;
  int value = 0;

  for (int i = 0; i < config->pin_count; i++) {
    int ret = gpio_pin_get_dt(&config->input_pins[i]);
    if (ret < 0) {
      LOG_ERR("Failed to read input pin %d: %d", i, ret);
      return ret;
    }

    value |= (ret << i);
  }

  LOG_DBG("Current binary encoded GPIO value: %d", value);

  return value;
}

static int binary_encoded_gpio_init(const struct device *dev) {
  const struct binary_encoded_gpio_config *config = dev->config;

  LOG_DBG("Initializing binary encoded GPIO driver");

  for (int i = 0; i < config->pin_count; i++) {
    if (!device_is_ready(config->input_pins[i].port)) {
      return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&config->input_pins[i], GPIO_INPUT);
    if (ret < 0) {
      return ret;
    }
  }

  LOG_DBG("Binary encoded GPIO driver initialized successfully");

  return 0;
}

DEVICE_API(binary_encoded_gpio, binary_encoded_gpio_api) = {
  .get_value = get_value,
};

BUILD_ASSERT(
    CONFIG_BINARY_ENCODED_GPIO_INIT_PRIORITY > CONFIG_GPIO_INIT_PRIORITY,
    "BINARY_ENCODED_GPIO_INIT_PRIORITY must be higher than GPIO_INIT_PRIORITY");

#define BINARY_ENCODED_GPIO_INIT(inst)                                        \
  static const struct gpio_dt_spec input_pins_##inst[] = {                    \
    DT_INST_FOREACH_PROP_ELEM_SEP(inst, input_gpios, GPIO_DT_SPEC_GET_BY_IDX, \
                                  (, )),                                      \
  };                                                                          \
  static const struct binary_encoded_gpio_config                              \
      binary_encoded_gpio_config_##inst = {                                   \
        .input_pins = input_pins_##inst,                                      \
        .pin_count = DT_INST_PROP_LEN(inst, input_gpios),                     \
  };                                                                          \
  BUILD_ASSERT(                                                               \
      DT_INST_PROP_LEN(inst, input_gpios) <= MAXIMUM_SUPPORTED_PIN_COUNT,     \
      "Binary encoded GPIO driver can use at "                                \
      "most " STRINGIFY(MAXIMUM_SUPPORTED_PIN_COUNT) " input_gpios");         \
  DEVICE_DT_INST_DEFINE(inst, binary_encoded_gpio_init, NULL, NULL,           \
                        &binary_encoded_gpio_config_##inst, POST_KERNEL,      \
                        CONFIG_BINARY_ENCODED_GPIO_INIT_PRIORITY,             \
                        &binary_encoded_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(BINARY_ENCODED_GPIO_INIT);

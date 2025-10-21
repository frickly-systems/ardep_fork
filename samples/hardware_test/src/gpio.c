/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "util.h"
#include "zephyr/devicetree.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <zephyr/kernel.h>

LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec gpios[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, gpios, GPIO_DT_SPEC_GET_BY_IDX, (, ))};

static int gpio_ready_check(uint32_t *log_offset) {
  LOG_INF("gpio ready check");
  bool failed = false;
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    if (!gpio_is_ready_dt(&gpios[i])) {
      LOG_ERR("gpio device not ready: %s", gpios[i].port->name);
      int len = snprintk(&logs[*log_offset], sizeof(logs) - *log_offset,
                         "gpio device not ready: %s\n", gpios[i].port->name);
      if (len < 0) {
        LOG_ERR("snprintk failed while logging gpio error");
        return -EIO;
      }

      *log_offset += len;
    }
  }
  LOG_INF("gpio ready check finished");
  return failed ? -ENODEV : 0;
}

static int gpio_initialize(uint32_t *log_offset) {
  LOG_INF("gpio configure");
  bool failed = false;
  int rc = 0;
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    int ret = gpio_pin_configure_dt(&gpios[i], GPIO_OUTPUT_INACTIVE);

    if (ret < 0) {
      LOG_ERR("gpio configure %s pin %d with error %d", gpios[i].port->name,
              gpios[i].pin, ret);

      int len = snprintk(&logs[*log_offset], sizeof(logs) - *log_offset,
                         "gpio config error (%d): port %s, pin %d\n", ret,
                         gpios[i].port->name, gpios[i].pin);
      if (len < 0) {
        LOG_ERR("snprintk failed while logging gpio error");
        return -EIO;
      }

      *log_offset += len;

      failed = true;
      rc = ret;
    }
  }

  LOG_INF("gpio configure finished");
  return failed ? rc : 0;
}

static void gpio_toggle() {
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    LOG_INF("%s pin %d on", gpios[i].port->name, gpios[i].pin);
    gpio_pin_set_dt(&gpios[i], 1);
    k_msleep(10);
    gpio_pin_set_dt(&gpios[i], 0);
    k_msleep(10);
  }
}

static void disable_gpios() {
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    gpio_pin_set_dt(&gpios[i], 0);
  }
}

int setup_gpio_test(const struct Request *request, struct Response *response) {
  uint32_t log_offset = 0;

  response->payload_type = RESPONSE_TYPE__GPIO;
  response->payload.gpio_response.errors = (char **)&logs;
  int ret = gpio_ready_check(&log_offset);
  if (ret != 0) {
    response->payload.gpio_response.errors_length = log_offset;
    return ret;
  }

  ret = gpio_initialize(&log_offset);
  if (ret != 0) {
    response->payload.gpio_response.errors_length = log_offset;
    return ret;
  }

  response->payload.gpio_response.errors_length = log_offset;
  return 0;
}

int execute_gpio_test(const struct Request *request,
                      struct Response *response) {
  gpio_toggle();
  return 0;
}

int stop_gpio_test(const struct Request *request, struct Response *response) {
  disable_gpios();
  return 0;
}
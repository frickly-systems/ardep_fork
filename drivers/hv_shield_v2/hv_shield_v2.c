#define DT_DRV_COMPAT hv_shield_v2_custom

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/hv_shield.h>

LOG_MODULE_REGISTER(hv_shield, CONFIG_HV_SHIELD_V2_LOG_LEVEL);

struct hv_shield_v2_config {
  struct i2c_dt_spec i2c;
  struct gpio_dt_spec int_gpios[2];
  uint8_t int_gpio_count;
};

struct hv_shield_v2_data {
  struct {
    uint8_t iocon;
  } reg_cache;
};

static int write_iocon(const struct hv_shield_v2_config* config,
                       uint8_t value) {
  uint8_t buf[3] = {0x0A, value, value};
  int ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
  if (ret) {
    LOG_ERR("Failed to write IOCON register 0x0A: %d", ret);
  }
  return ret;
}

static int hv_shield_v2_init(const struct device* dev) {
  const struct hv_shield_v2_config* config = dev->config;

  if (!device_is_ready(config->i2c.bus)) {
    LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
    return -ENODEV;
  }

  for (int i = 0; i < config->int_gpio_count; i++) {
    if (!device_is_ready(config->int_gpios[i].port)) {
      LOG_ERR("INT GPIO %s not ready", config->int_gpios[i].port->name);
      return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&config->int_gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Failed to configure INT GPIO pin %d: %d",
              config->int_gpios[i].pin, ret);
      return ret;
    }

    // todo: register interrupt
  }

  // Note, that this driver uses IOCON.BANK=0, which is the default state after
  // reset.

  // Set INT pins as open drain output (ODR=1)
  if (write_iocon(config, 1 << 2) !=
      0) {  // todo: check if other bits should be set
    return -EIO;
  }

  LOG_INF("HV Shield v2 initialized on I2C address 0x%02x", config->i2c.addr);

  return 0;
}

DEVICE_API(gpio, hv_shield_api) = {};

#define HV_SHIELD_V2_INIT(x)                                               \
  BUILD_ASSERT(DT_INST_PROP_LEN(x, int_gpios) <= 2);                       \
  static const struct hv_shield_v2_config hv_shield_##x##_config = {       \
    .i2c = I2C_DT_SPEC_INST_GET(x),                                        \
    .int_gpios =                                                           \
        {                                                                  \
          GPIO_DT_SPEC_INST_GET_BY_IDX_OR(x, int_gpios, 0, {0}),           \
          GPIO_DT_SPEC_INST_GET_BY_IDX_OR(x, int_gpios, 1, {0}),           \
        },                                                                 \
    .int_gpio_count = DT_INST_PROP_LEN(x, int_gpios),                      \
  };                                                                       \
  static struct hv_shield_v2_data hv_shield_##x##_data = {                 \
    .reg_cache =                                                           \
        {                                                                  \
          .iocon = 0,                                                      \
        },                                                                 \
  };                                                                       \
  DEVICE_DT_INST_DEFINE(x, hv_shield_v2_init, NULL, &hv_shield_##x##_data, \
                        &hv_shield_##x##_config, POST_KERNEL,              \
                        CONFIG_HV_SHIELD_V2_INIT_PRIORITY, &hv_shield_api)

DT_INST_FOREACH_STATUS_OKAY(HV_SHIELD_V2_INIT)

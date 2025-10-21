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
};

struct hv_shield_v2_data {};

static int hv_shield_v2_init(const struct device* dev) {
  const struct hv_shield_v2_config* config = dev->config;

  if (!device_is_ready(config->i2c.bus)) {
    LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
    return -ENODEV;
  }

  LOG_INF("HV Shield v2 initialized on I2C address 0x%02x", config->i2c.addr);

  return 0;
}

DEVICE_API(gpio, hv_shield_api) = {};

#define HV_SHIELD_V2_INIT(x)                                               \
  static const struct hv_shield_v2_config hv_shield_##x##_config = {       \
    .i2c = I2C_DT_SPEC_INST_GET(x),                                        \
  };                                                                       \
  static struct hv_shield_v2_data hv_shield_##x##_data = {};               \
  DEVICE_DT_INST_DEFINE(x, hv_shield_v2_init, NULL, &hv_shield_##x##_data, \
                        &hv_shield_##x##_config, POST_KERNEL,              \
                        CONFIG_HV_SHIELD_V2_INIT_PRIORITY, &hv_shield_api)

DT_INST_FOREACH_STATUS_OKAY(HV_SHIELD_V2_INIT)
#define DT_DRV_COMPAT hv_shield_v2_custom

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/hv_shield.h>
#include <ardep/dt-bindings/hv-shield-v2.h>

LOG_MODULE_REGISTER(hv_shield, CONFIG_HV_SHIELD_V2_LOG_LEVEL);

struct hv_shield_v2_config {
  struct gpio_driver_config common;
  struct i2c_dt_spec i2c;
  struct gpio_dt_spec int_gpios[2];
  uint8_t int_gpio_count;
};

struct hv_shield_v2_data {
  struct gpio_driver_data common;
  struct {
    uint8_t gpintena;
    uint8_t gpintenb;
    // defval
    uint8_t intcona;  // =0
    uint8_t intconb;
    uint16_t gpio;

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

static int write_u16_reg(const struct hv_shield_v2_config* config,
                         uint8_t reg_addr,
                         uint16_t value) {
  uint8_t buf[3] = {reg_addr, (uint8_t)(value & 0xFF),
                    (uint8_t)((value >> 8) & 0xFF)};
  int ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
  if (ret) {
    LOG_ERR("Failed to write register 0x%02x: %d", reg_addr, ret);
  }
  return ret;
}

static int read_u16_reg(const struct hv_shield_v2_config* config,
                        uint8_t reg_addr,
                        uint16_t* value) {
  uint8_t buf[2];
  int ret = i2c_write_read_dt(&config->i2c, &reg_addr, 1, buf, sizeof(buf));
  if (ret) {
    LOG_ERR("Failed to read register 0x%02x: %d", reg_addr, ret);
    return ret;
  }
  *value = buf[0] | (buf[1] << 8);
  return 0;
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

  uint8_t write_buffer[3];

  // Write IODIR registers
  write_buffer[0] = 0x00;        // IODIRA register
  write_buffer[1] = 0b00100000;  // 0-5 as outputs, 6 as input and 7 as output
                                 // because of requirements
  write_buffer[2] =
      0b01111111;  // 0-6 as inputs, 7 as output because of requirements

  if (i2c_write_dt(&config->i2c, write_buffer, sizeof(write_buffer)) != 0) {
    LOG_ERR("Failed to write IODIR registers");
    return -EIO;
  }

  LOG_INF("HV Shield v2 initialized on I2C address 0x%02x", config->i2c.addr);

  return 0;
}

static int hv_shield_v2_pin_to_bit(uint8_t pin) {
  uint8_t pin_base = pin & HV_SHIELD_V2_BASE_MASK;
  uint8_t pin_index = pin & ~HV_SHIELD_V2_BASE_MASK;

  switch (pin_base) {
    case HV_SHIELD_V2_INPUT_BASE:
      if (pin_index >= 6) {
        LOG_ERR("Invalid input pin index: %d", pin_index);
        break;
      }
      return pin_index + 8;  // Inputs are on port b 0-5

    case HV_SHIELD_V2_OUTPUT_BASE:
      if (pin_index >= 6) {
        LOG_ERR("Invalid output pin index: %d", pin_index);
        break;
      }
      return pin_index;  // Outputs are on port a 0-5

    case HV_SHIELD_V2_FAULT_BASE:  // Fault (treated as input)
      switch (pin_index) {
        case 0:
          return 6;
        case 1:
          return 14;
        case 2:
          return 15;
        default:
          LOG_ERR("Invalid fault pin index: %d", pin_index);
          break;
      }

    default:
      break;
  }

  return -ENOTSUP;
}

static int hv_shield_v2_port_get_raw(const struct device* port,
                                     gpio_port_value_t* value) {
  const struct hv_shield_v2_config* config = port->config;
  struct hv_shield_v2_data* data = port->data;

  uint16_t reg_value;
  if (read_u16_reg(config, 0x12, &reg_value) != 0) {
    return -EIO;
  }

  data->reg_cache.gpio = reg_value;

  // todo: map to logical pin layout
  *value = reg_value;

  return 0;
}

static int hv_shield_v2_gpio_set_masked_raw(const struct device* port,
                                            gpio_port_pins_t mask,
                                            gpio_port_value_t value) {
  const struct hv_shield_v2_config* config = port->config;
  struct hv_shield_v2_data* data = port->data;

  // extract output bits from mask and value
  const uint8_t output_mask = (mask >> HV_SHIELD_V2_OUTPUT_BASE) & 0x003F;
  const uint8_t output_value = (value >> HV_SHIELD_V2_OUTPUT_BASE) & 0x003F;
  // shift to correct mcp pin bits
  const uint16_t mapped_mask = (output_mask << 8);  // Outputs are on pins 8-13
  const uint16_t mapped_value = (output_value << 8);

  const uint16_t new_gpio =
      (data->reg_cache.gpio & ~mapped_mask) | (mapped_value & mapped_mask);

  if (write_u16_reg(config, 0x12, new_gpio) != 0) {
    return -EIO;
  }

  data->reg_cache.gpio = new_gpio;

  return 0;
}

// Note that configuring does nothing on this device
static int hv_shield_v2_pin_configure(const struct device* dev,
                                      gpio_pin_t pin,
                                      gpio_flags_t flags) {
  const struct hv_shield_v2_config* config = dev->config;
  struct hv_shield_v2_data* data = dev->data;

  uint8_t bit = hv_shield_v2_pin_to_bit(pin);
  if (bit < 0) {
    return bit;
  }

  uint8_t pin_base = pin & HV_SHIELD_V2_BASE_MASK;
  uint8_t pin_index = pin & ~HV_SHIELD_V2_BASE_MASK;

  switch (pin_base) {
    case HV_SHIELD_V2_INPUT_BASE:
      if (flags & GPIO_OUTPUT) {
        LOG_ERR("Cannot configure input pin %d as output", pin_index);
        return -ENOTSUP;
      }
      if ((flags & GPIO_PULL_DOWN) | (flags & GPIO_PULL_UP)) {
        LOG_ERR("Pull up/down not supported on input pin %d", pin_index);
        return -ENOTSUP;
      }
      break;

    case HV_SHIELD_V2_OUTPUT_BASE:
      if (flags & GPIO_INPUT) {
        LOG_ERR("Cannot configure output pin %d as input", pin_index);
        return -ENOTSUP;
      }
      if ((flags & GPIO_PULL_DOWN) | (flags & GPIO_PULL_UP)) {
        LOG_ERR("Pull up/down not supported on output pin %d", pin_index);
        return -ENOTSUP;
      }
      if (flags & GPIO_SINGLE_ENDED) {
        LOG_ERR("Open drain/source not supported on output pin %d", pin_index);
        return -ENOTSUP;
      }
      // todo: check if this is the right flag to test
      if (flags & GPIO_OUTPUT_INIT_HIGH) {
        data->reg_cache.gpio |= (1 << bit);
      } else {
        data->reg_cache.gpio &= ~(1 << bit);
      }

      if (write_u16_reg(config, 0x12, data->reg_cache.gpio) != 0) {
        return -EIO;
      }
      break;

    // todo: do fault pins have to be something else?
    case HV_SHIELD_V2_FAULT_BASE:  // Fault (treated as input)
      if (flags & GPIO_OUTPUT) {
        LOG_ERR("Cannot configure fault pin %d as output", pin_index);
        return -ENOTSUP;
      }
      if ((flags & GPIO_PULL_DOWN) | (flags & GPIO_PULL_UP)) {
        LOG_ERR("Pull up/down not supported on fault pin %d", pin_index);
        return -ENOTSUP;
      }
      break;
    default:
      return -ENOTSUP;
  }

  return 0;
}

DEVICE_API(gpio, hv_shield_api) = {
  .pin_configure = hv_shield_v2_pin_configure,
  .port_get_raw = hv_shield_v2_port_get_raw,
  .port_set_masked_raw = hv_shield_v2_gpio_set_masked_raw,
};

// todo: .common init
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
          .gpintena = 0,                                                   \
          .gpintenb = 0,                                                   \
          .intcona = 0,                                                    \
          .intconb = 0,                                                    \
          .gpio = 0,                                                       \
        },                                                                 \
  };                                                                       \
  DEVICE_DT_INST_DEFINE(x, hv_shield_v2_init, NULL, &hv_shield_##x##_data, \
                        &hv_shield_##x##_config, POST_KERNEL,              \
                        CONFIG_HV_SHIELD_V2_INIT_PRIORITY, &hv_shield_api)

DT_INST_FOREACH_STATUS_OKAY(HV_SHIELD_V2_INIT)

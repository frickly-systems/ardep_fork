#define DT_DRV_COMPAT hv_shield_v2_custom

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/hv_shield.h>
#include <ardep/dt-bindings/hv-shield-v2.h>

LOG_MODULE_REGISTER(hv_shield_v2, CONFIG_HV_SHIELD_V2_LOG_LEVEL);

#define REG_IODIRA 0x00
#define REG_IOCONA 0x0A

#define REG_INTCONA 0x08
#define REG_DEFVALA 0x06
#define REG_GPINTENA 0x04
#define REG_GPIOA 0x12

struct hv_shield_v2_config {
  struct gpio_driver_config common;
  struct i2c_dt_spec i2c;
  struct gpio_dt_spec int_gpios[2];
  uint8_t int_gpio_count;
};

struct hv_shield_v2_data {
  struct gpio_driver_data common;
  const struct device*
      device;  // back reference to device struct (used in work handler)
  sys_slist_t interrupt_callbacks;
  struct gpio_callback interrupt_gpio_cb;

  struct {
    uint16_t gpinten;
    uint16_t intcon;
    uint16_t defval;
    uint16_t gpio;
  } reg_cache;

  uint16_t int_trigger_rising;
  uint16_t int_trigger_falling;

  struct k_work on_interrupt_work;
};

static int write_iocon(const struct hv_shield_v2_config* config,
                       uint8_t value) {
  uint8_t buf[3] = {REG_IOCONA, value, value};
  int ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
  if (ret) {
    LOG_ERR("Failed to write IOCON register " STRINGIFY(REG_IOCONA)": %d", ret);
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

static void hv_shield_v2_int_gpio_handler(const struct device* port,
                                          struct gpio_callback* cb,
                                          gpio_port_pins_t pins) {
  struct hv_shield_v2_data* data =
      CONTAINER_OF(cb, struct hv_shield_v2_data, interrupt_gpio_cb);

  k_work_submit(&data->on_interrupt_work);
}

static inline uint32_t hv_shield_v2_internal_pins_to_zephyr_bits(
    uint16_t hv_shield_pins) {
  const uint32_t input_values =
      ((hv_shield_pins >> 8) & 0x3F);  // Inputs are on port b 0-5
  const uint32_t output_values =
      (hv_shield_pins & 0x3F);  // Outputs are on port a 0-5
  const uint32_t fault_values =
      ((hv_shield_pins >> 6) & 0x1) | (((hv_shield_pins >> 14) & 0x1) << 1) |
      (((hv_shield_pins >> 15) & 0x1)
       << 2);  // Faults on bits 6,14,15 mapped to 0..2

  return (input_values << HV_SHIELD_V2_INPUT_BASE) |
         (output_values << HV_SHIELD_V2_OUTPUT_BASE) |
         (fault_values << HV_SHIELD_V2_FAULT_BASE);
}

static void hv_shield_v2_interrupt_work_handler(struct k_work* work) {
  struct hv_shield_v2_data* data =
      CONTAINER_OF(work, struct hv_shield_v2_data, on_interrupt_work);
  const struct device* dev = data->device;
  const struct hv_shield_v2_config* config = dev->config;

  // todo: read registers
  uint16_t intf = 0;
  int err = read_u16_reg(config, 0x0E, &intf);
  if (err) {
    LOG_ERR("Error handling interrupt; could not read register: %d", err);
    return;
  }

  if (!intf) {
    LOG_DBG("Interrupt was not for this IC");
    return;
  }

  uint16_t intcap = 0;
  err = read_u16_reg(config, 0x10, &intcap);
  if (err) {
    LOG_ERR("Error handling interrupt; could not read INTCAP register: %d",
            err);
    return;
  }

  // todo: optimize interrupt detection

  // note that these are ANDed with intf and gpinten later
  const uint16_t level_interrupts = data->reg_cache.intcon;
  const uint16_t rising_edge_interrupts = intcap & data->int_trigger_rising;
  const uint16_t falling_edge_interrupts =
      (~intcap) & data->int_trigger_falling;

  const uint16_t ints =
      data->reg_cache.gpinten & intf &
      (level_interrupts | rising_edge_interrupts | falling_edge_interrupts);

  gpio_fire_callbacks(&data->interrupt_callbacks, dev,
                      hv_shield_v2_internal_pins_to_zephyr_bits(ints));

  // todo: test
  // if any level interrupt is active, resubmit work to check if the level is
  // still active. If yes the callbacks are fired again, looping until the level
  // goes inactive
  if (level_interrupts & ints) {
    k_work_submit(&data->on_interrupt_work);
  }
}

static int hv_shield_v2_init(const struct device* dev) {
  const struct hv_shield_v2_config* config = dev->config;
  struct hv_shield_v2_data* data = dev->data;

  data->device = dev;

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
  }

  if (config->int_gpio_count > 0) {
    gpio_init_callback(&data->interrupt_gpio_cb, hv_shield_v2_int_gpio_handler,
                       BIT(config->int_gpios[0].pin));
    int ret =
        gpio_add_callback(config->int_gpios[0].port, &data->interrupt_gpio_cb);
    if (ret != 0) {
      LOG_ERR("Failed to add INT GPIO callback: %d", ret);
      return ret;
    }
    ret = gpio_pin_interrupt_configure_dt(&config->int_gpios[0],
                                          GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
      LOG_ERR("Failed to configure INT GPIO interrupt: %d", ret);
      return ret;
    }
  }

  // Note, that this driver uses IOCON.BANK=0, which is the default state after
  // reset.

  // todo: implement bank-interrupts and remove mirror bit
  // Set INT pins as open drain output (ODR=1 MIRROR=1)
  if (write_iocon(config, (1 << 2) | (1 << 6)) != 0) {
    return -EIO;
  }

  // Write IODIR registers (1 is input, 0 is output)
  if (write_u16_reg(config, REG_IODIRA, 0b0111111101000000) != 0) {
    LOG_ERR("Failed to write IODIR registers");
    return -EIO;
  }

  LOG_INF("HV Shield v2 initialized on I2C address 0x%02x", config->i2c.addr);

  k_work_init(&data->on_interrupt_work, hv_shield_v2_interrupt_work_handler);

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
  if (read_u16_reg(config, REG_GPIOA, &reg_value) != 0) {
    return -EIO;
  }

  data->reg_cache.gpio = reg_value;

  *value = hv_shield_v2_internal_pins_to_zephyr_bits(reg_value);

  return 0;
}

static int hv_shield_v2_gpio_set_masked_raw(const struct device* port,
                                            gpio_port_pins_t mask,
                                            gpio_port_value_t value) {
  const struct hv_shield_v2_config* config = port->config;
  struct hv_shield_v2_data* data = port->data;

  // extract output bits from mask and value (0x3F is the mask for the 6 output
  // bits)
  const uint8_t output_mask = (mask >> HV_SHIELD_V2_OUTPUT_BASE) & 0x003F;
  const uint8_t output_value = (value >> HV_SHIELD_V2_OUTPUT_BASE) & 0x003F;
  // shift to correct mcp pin bits
  const uint16_t mapped_mask = output_mask;  // Outputs are on pins 0-5
  const uint16_t mapped_value = output_value;

  const uint16_t new_gpio =
      (data->reg_cache.gpio & ~mapped_mask) | (mapped_value & mapped_mask);

  if (write_u16_reg(config, REG_GPIOA, new_gpio) != 0) {
    return -EIO;
  }

  data->reg_cache.gpio = new_gpio;

  return 0;
}

static int hv_shield_v2_gpio_set_bits_raw(const struct device* port,
                                          gpio_port_pins_t mask) {
  return hv_shield_v2_gpio_set_masked_raw(port, mask, mask);
}

static int hv_shield_v2_gpio_clear_bits_raw(const struct device* port,
                                            gpio_port_pins_t mask) {
  return hv_shield_v2_gpio_set_masked_raw(port, mask, 0);
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

      if (write_u16_reg(config, REG_GPIOA, data->reg_cache.gpio) != 0) {
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

static int hv_shield_v2_manage_callback(const struct device* dev,
                                        struct gpio_callback* cb,
                                        bool set) {
  struct hv_shield_v2_data* data = dev->data;
  return gpio_manage_callback(&data->interrupt_callbacks, cb, set);
}

static int hv_shield_v2_pin_interrupt_configure(const struct device* port,
                                                gpio_pin_t pin,
                                                enum gpio_int_mode mode,
                                                enum gpio_int_trig trig) {
  struct hv_shield_v2_data* data = port->data;
  if ((pin & HV_SHIELD_V2_BASE_MASK) != HV_SHIELD_V2_INPUT_BASE &&
      (pin & HV_SHIELD_V2_BASE_MASK) != HV_SHIELD_V2_FAULT_BASE) {
    LOG_ERR("Interrupts can only be configured on input and fault pins");
    return -ENOTSUP;
  }
  uint8_t bit = hv_shield_v2_pin_to_bit(pin);

  uint16_t defval = data->reg_cache.defval;
  uint16_t intcon =
      data->reg_cache.intcon;  // 0 = on change, 1 = compare to defval
  uint16_t gpinten = data->reg_cache.gpinten;  // 0 = disabled, 1 = enabled

  switch (mode) {
    case GPIO_INT_MODE_DISABLED:
      gpinten &= ~BIT(bit);
      break;
    case GPIO_INT_MODE_LEVEL:
      gpinten |= BIT(bit);
      intcon |= BIT(bit);
      switch (trig) {
        case GPIO_INT_TRIG_LOW:
          defval &= ~BIT(bit);
          break;
        case GPIO_INT_TRIG_HIGH:
          defval |= BIT(bit);
          break;
        default:
          LOG_ERR("Invalid trigger for level interrupt");
          return -EINVAL;
      }
      break;
    case GPIO_INT_MODE_EDGE:
      gpinten |= BIT(bit);
      intcon &= ~BIT(bit);
      switch (trig) {
        case GPIO_INT_TRIG_HIGH:  // rising edge
          data->int_trigger_rising |= BIT(bit);
          data->int_trigger_falling &= ~BIT(bit);
          break;
        case GPIO_INT_TRIG_LOW:  // falling edge
          data->int_trigger_falling |= BIT(bit);
          data->int_trigger_rising &= ~BIT(bit);
          break;
        case GPIO_INT_TRIG_BOTH:
          data->int_trigger_rising |= BIT(bit);
          data->int_trigger_falling |= BIT(bit);
          break;
        default:
          LOG_ERR("Invalid trigger for edge interrupt");
          return -EINVAL;
      }
      break;
    default:
      LOG_ERR("Invalid interrupt mode");
      return -EINVAL;
  }

  if (write_u16_reg(port->config, REG_DEFVALA, defval) != 0) {
    return -EIO;
  }
  if (write_u16_reg(port->config, REG_INTCONA, intcon) != 0) {
    return -EIO;
  }
  if (write_u16_reg(port->config, REG_GPINTENA, gpinten) != 0) {
    return -EIO;
  }

  data->reg_cache.defval = defval;
  data->reg_cache.intcon = intcon;
  data->reg_cache.gpinten = gpinten;

  return 0;
}

DEVICE_API(gpio, hv_shield_api) = {
  .pin_configure = hv_shield_v2_pin_configure,
  .port_get_raw = hv_shield_v2_port_get_raw,
  .port_set_masked_raw = hv_shield_v2_gpio_set_masked_raw,
  .port_set_bits_raw = hv_shield_v2_gpio_set_bits_raw,
  .port_clear_bits_raw = hv_shield_v2_gpio_clear_bits_raw,
  .manage_callback = hv_shield_v2_manage_callback,
  .pin_interrupt_configure = hv_shield_v2_pin_interrupt_configure,
};

// todo: .common init; todo: interrupt pin stuff
#define HV_SHIELD_V2_INIT(x)                                               \
  BUILD_ASSERT(DT_INST_PROP_LEN_OR(x, int_gpios, 0) <= 2);                 \
  static const struct hv_shield_v2_config hv_shield_##x##_config = {       \
    .common = {.port_pin_mask = 0x73F3F},                                  \
    .i2c = I2C_DT_SPEC_INST_GET(x),                                        \
    .int_gpios =                                                           \
        {                                                                  \
          GPIO_DT_SPEC_INST_GET_BY_IDX_OR(x, int_gpios, 0, {0}),           \
          GPIO_DT_SPEC_INST_GET_BY_IDX_OR(x, int_gpios, 1, {0}),           \
        },                                                                 \
    .int_gpio_count = DT_INST_PROP_LEN_OR(x, int_gpios, 0),                \
  };                                                                       \
  static struct hv_shield_v2_data hv_shield_##x##_data = {                 \
    .reg_cache =                                                           \
        {                                                                  \
          .gpinten = 0,                                                    \
          .intcon = 0,                                                     \
          .defval = 0,                                                     \
          .gpio = 0,                                                       \
        },                                                                 \
  };                                                                       \
  DEVICE_DT_INST_DEFINE(x, hv_shield_v2_init, NULL, &hv_shield_##x##_data, \
                        &hv_shield_##x##_config, POST_KERNEL,              \
                        CONFIG_HV_SHIELD_V2_INIT_PRIORITY, &hv_shield_api)

DT_INST_FOREACH_STATUS_OKAY(HV_SHIELD_V2_INIT)

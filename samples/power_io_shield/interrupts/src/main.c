#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/dt-bindings/power-io-shield.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device* power_io_shield =
    DEVICE_DT_GET(DT_NODELABEL(power_io_shield0));

#define DEFINE_GPIO_ARRAY(node, prop)                                    \
  static const struct gpio_dt_spec prop[] = {                            \
    DT_FOREACH_PROP_ELEM_SEP(node, prop, GPIO_DT_SPEC_GET_BY_IDX, (, )), \
  };

DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), output_gpios);
DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), input_gpios);
DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), fault_gpios);

struct gpio_callback input_interrupt_cb;

void input_interrupt_handler(const struct device* dev,
                             struct gpio_callback* cb,
                             uint32_t pins) {
  LOG_INF("Input GPIO interrupt on pins: 0x%08x", pins);
}

int main() {
  if (!device_is_ready(power_io_shield)) {
    LOG_ERR("HV Shield device not ready");
    return 1;
  }

  gpio_init_callback(&input_interrupt_cb, input_interrupt_handler, 0xffff);

  LOG_INF("Initializing input GPIOs...");
  for (size_t i = 0; i < ARRAY_SIZE(input_gpios); i++) {
    int ret = gpio_pin_configure_dt(&input_gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Failed to configure input GPIO pin %d: %d", input_gpios[i].pin,
              ret);
      return 1;
    }
  }

  gpio_add_callback(power_io_shield, &input_interrupt_cb);
  int ret =
      gpio_pin_interrupt_configure_dt(&input_gpios[1], GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    LOG_ERR("Failed to configure interrupt for input GPIO pin %d: %d",
            input_gpios[1].pin, ret);
    return 1;
  }
  ret = gpio_pin_interrupt_configure_dt(&input_gpios[2],
                                        GPIO_INT_EDGE_TO_INACTIVE);
  if (ret != 0) {
    LOG_ERR("Failed to configure interrupt for input GPIO pin %d: %d",
            input_gpios[2].pin, ret);
    return 1;
  }

  ret = gpio_pin_interrupt_configure_dt(&input_gpios[3], GPIO_INT_LEVEL_ACTIVE);
  if (ret != 0) {
    LOG_ERR("Failed to configure interrupt for input GPIO pin %d: %d",
            input_gpios[3].pin, ret);
    return 1;
  }

  LOG_INF("Initializing output GPIOs...");
  for (size_t i = 0; i < ARRAY_SIZE(output_gpios); i++) {
    int ret = gpio_pin_configure_dt(&output_gpios[i], GPIO_OUTPUT_LOW);
    if (ret != 0) {
      LOG_ERR("Failed to configure output GPIO pin %d: %d", output_gpios[i].pin,
              ret);
      return 1;
    }
  }

  LOG_INF("Initializing fault GPIOs...");
  for (size_t i = 0; i < ARRAY_SIZE(fault_gpios); i++) {
    int ret = gpio_pin_configure_dt(&fault_gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Failed to configure fault GPIO pin %d: %d", fault_gpios[i].pin,
              ret);
      return 1;
    }
  }

  LOG_INF("GPIOs initialized successfully.");

  LOG_INF("Entering main loop, toggling output GPIOs and logging inputs");

  uint8_t output_value = 0;
  for (;;) {
    for (size_t i = 0; i < ARRAY_SIZE(output_gpios); i++) {
      int ret = gpio_pin_set_dt(&output_gpios[i], (output_value >> i) & 1);
      if (ret != 0) {
        LOG_ERR("Failed to set output GPIO pin %d: %d", output_gpios[i].pin,
                ret);
      }
    }
    output_value++;

    printk("Input GPIO states: ");
    for (size_t i = 0; i < ARRAY_SIZE(input_gpios); i++) {
      int ret = gpio_pin_get_dt(&input_gpios[i]);
      if (ret < 0) {
        LOG_ERR("Failed to read input GPIO pin %d: %d", input_gpios[i].pin,
                ret);
        continue;
      }
      printk("%d ", ret);
    }
    printk("\n");

    printk("Fault GPIO states: ");
    for (size_t i = 0; i < ARRAY_SIZE(fault_gpios); i++) {
      int ret = gpio_pin_get_dt(&fault_gpios[i]);
      if (ret < 0) {
        LOG_ERR("Failed to read fault GPIO pin %d: %d", fault_gpios[i].pin,
                ret);
        continue;
      }
      printk("%d ", ret);
    }
    printk("\n");

    printk("\n");

    k_msleep(1000);
  }
}

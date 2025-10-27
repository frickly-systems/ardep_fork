#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device* mcp23017 = DEVICE_DT_GET(DT_NODELABEL(mcp23017));

#define DEFINE_GPIO_ARRAY(node, prop)                                    \
  static const struct gpio_dt_spec prop[] = {                            \
    DT_FOREACH_PROP_ELEM_SEP(node, prop, GPIO_DT_SPEC_GET_BY_IDX, (, )), \
  };

DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), output_gpios);
DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), input_gpios);
DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), fault_gpios);

int main() {
  if (!device_is_ready(mcp23017)) {
    LOG_ERR("MCP23017 device not ready");
    return 1;
  }

  LOG_INF("Initializing input GPIOs...");
  for (size_t i = 0; i < ARRAY_SIZE(input_gpios); i++) {
    int ret = gpio_pin_configure_dt(&input_gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Failed to configure input GPIO pin %d: %d", input_gpios[i].pin,
              ret);
      return 1;
    }
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

  LOG_INF("Initializing mirror GPIO...");
  int ret = gpio_pin_configure_dt(&mirror_gpio, GPIO_INPUT);
  if (ret != 0) {
    LOG_ERR("Failed to configure mirror GPIO pin %d: %d", mirror_gpio.pin, ret);
    return 1;
  }

  LOG_INF("GPIOs initialized successfully.");

  LOG_INF("Entering main loop, toggling output GPIOs and logging inputs");

  bool value = false;
  for (;;) {
    int ret = gpio_pin_set_dt(&output_gpios[0], value);
    if (ret != 0) {
      LOG_ERR("Failed to set output GPIO pin %d: %d", output_gpios[0].pin, ret);
    }
    value = !value;
  }
}

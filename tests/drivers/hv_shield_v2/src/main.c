#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/drivers/emul/hv_shield_v2.h>
#include <ardep/drivers/i2c_fake.h>
#include <ardep/dt-bindings/hv-shield-v2.h>

DEFINE_FFF_GLOBALS;

static const struct device* hv_shield = DEVICE_DT_GET(DT_NODELABEL(hv_shield0));
static const struct emul* hv_shield_emul =
    EMUL_DT_GET(DT_NODELABEL(hv_shield0));

static const struct device* gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

ZTEST_SUITE(mcp_driver, NULL, NULL, NULL, NULL, NULL);

ZTEST(mcp_driver, test_iodir_and_iocon_init) {
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x00), 0x7f40);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x0A), 0x4444);
}

ZTEST(mcp_driver, test_set_output_0) {
  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(0), 1), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x12),
                0x0001);  // Bank A: 0x01, Bank B: 0x00

  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(0), 0), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x12), 0x0000);
}

ZTEST(mcp_driver, test_set_output_5) {
  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(5), 1), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x12),
                0x0020);  // Bank A: 0x20, Bank B: 0x00

  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(5), 0), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x12), 0x0000);
}

ZTEST(mcp_driver, test_read_inputs_and_faults) {
  gpio_port_value_t value;

  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x12, 0x40);  // fault 0 set
  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x13,
                            0x92);  // input 1, 4, fault 2 set

  zassert_equal(gpio_port_get(hv_shield, &value), 0);
  zassert_equal((value >> HV_SHIELD_V2_INPUT_BASE) & 0x3F, 0b10010);
  zassert_equal((value >> HV_SHIELD_V2_FAULT_BASE) & 0x7, 0b101);

  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x12, 0x00);
  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x13, 0xc0);  // Fault 1,2 is high
  zassert_equal(gpio_port_get(hv_shield, &value), 0);
  zassert_equal((value >> HV_SHIELD_V2_FAULT_BASE), 0b110);

  // reset
  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x12, 0x00);
  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x13, 0x00);
  zassert_equal(gpio_port_get(hv_shield, &value), 0);
  zassert_equal(value, 0);
}

ZTEST(mcp_driver, test_configure_outputs) {
  // configure with active flag (meaning active on configure)
  zassert_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0), GPIO_OUTPUT_ACTIVE),
      0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x12), 0x0001);

  // reconfigure without ACTIVE flag
  zassert_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0), GPIO_OUTPUT), 0);

  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x12), 0x0000);

  // try to configure with invalid flags
  zassert_not_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0), GPIO_INPUT), 0);
  zassert_not_equal(gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0),
                                       GPIO_OUTPUT | GPIO_OPEN_DRAIN),
                    0);
  zassert_not_equal(gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0),
                                       GPIO_OUTPUT | GPIO_PULL_UP),
                    0);
  zassert_not_equal(gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0),
                                       GPIO_OUTPUT | GPIO_PULL_DOWN),
                    0);
}

ZTEST(mcp_driver, test_configure_inputs) {
  zassert_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_INPUT(0), GPIO_INPUT), 0);
  zassert_not_equal(gpio_pin_configure(hv_shield, HV_SHIELD_V2_INPUT(0),
                                       GPIO_INPUT | GPIO_PULL_DOWN),
                    0);
  zassert_not_equal(gpio_pin_configure(hv_shield, HV_SHIELD_V2_INPUT(0),
                                       GPIO_INPUT | GPIO_PULL_UP),
                    0);
  zassert_not_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_INPUT(0), GPIO_OUTPUT), 0);
}

FAKE_VOID_FUNC(gpio_demo_interrupt,
               const struct device*,
               struct gpio_callback*,
               gpio_port_pins_t);

ZTEST(mcp_driver, test_rising_edge_interrupt) {
  struct gpio_callback callback;
  gpio_init_callback(&callback, gpio_demo_interrupt,
                     BIT(HV_SHIELD_V2_INPUT(0)));
  zassert_equal(gpio_add_callback(hv_shield, &callback), 0);

  zassert_equal(gpio_pin_interrupt_configure(hv_shield, HV_SHIELD_V2_INPUT(0),
                                             GPIO_INT_EDGE_TO_ACTIVE),
                0);

  // Interrupt should be configured for the chip
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x04),
                0x0100);  // gpinten
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x08),
                0x0000);  // intcon
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, 0x06),
                0x0000);  // defval

  // nothing should happen when not INTF is set
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_msleep(1);
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);
  zassert_equal(gpio_demo_interrupt_fake.call_count, 0);

  // set INTF and INTCAP to simulate rising edge interrupt

  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x11, 0x01);  // INTF on input pin 0
  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x0F,
                            0x01);  // INTCAP on input pin 0

  // set interrupt gpio to 1
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_msleep(1);
  zassert_equal(gpio_demo_interrupt_fake.call_count, 1);
  k_msleep(10);
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);
  k_msleep(1);
  zassert_equal(gpio_demo_interrupt_fake.call_count, 1);

  // reset
  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x11, 0x00);
  hv_shield_v2_emul_set_reg(hv_shield_emul, 0x0F, 0x00);

  zassert_equal(gpio_remove_callback(hv_shield, &callback), 0);
  zassert_equal(gpio_pin_interrupt_configure(hv_shield, HV_SHIELD_V2_INPUT(0),
                                             GPIO_INT_DISABLE),
                0);
}

// todo: test level interrupts

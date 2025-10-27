#include "devices.h"
#include "regs.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/drivers/emul/hv_shield_v2.h>
#include <ardep/drivers/i2c_fake.h>
#include <ardep/dt-bindings/hv-shield-v2.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(mcp_driver, NULL, NULL, NULL, NULL, NULL);

ZTEST(mcp_driver, test_iodir_and_iocon_init) {
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_IODIRA),
                0x7f40);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_IOCONA),
                0x4444);
}

ZTEST(mcp_driver, test_set_output_0) {
  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(0), 1), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_GPIOA),
                0x0001);  // Bank A: 0x01, Bank B: 0x00

  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(0), 0), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_GPIOA),
                0x0000);
}

ZTEST(mcp_driver, test_set_output_5) {
  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(5), 1), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_GPIOA),
                0x0020);  // Bank A: 0x20, Bank B: 0x00

  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(5), 0), 0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_GPIOA),
                0x0000);
}

ZTEST(mcp_driver, test_read_inputs_and_faults) {
  gpio_port_value_t value;

  hv_shield_v2_emul_set_u16_reg(hv_shield_emul, REG_GPIOA,
                                0x9240);  // set input 1 and 4, fault 0 and 2

  zassert_equal(gpio_port_get(hv_shield, &value), 0);
  zassert_equal((value >> HV_SHIELD_V2_INPUT_BASE) & 0x3F, 0b10010);
  zassert_equal((value >> HV_SHIELD_V2_FAULT_BASE) & 0x7, 0b101);

  hv_shield_v2_emul_set_u16_reg(hv_shield_emul, REG_GPIOA, 0xc000);
  zassert_equal(gpio_port_get(hv_shield, &value), 0);
  zassert_equal((value >> HV_SHIELD_V2_FAULT_BASE), 0b110);

  // reset
  hv_shield_v2_emul_set_u16_reg(hv_shield_emul, REG_GPIOA, 0x0000);
  zassert_equal(gpio_port_get(hv_shield, &value), 0);
  zassert_equal(value, 0);
}

ZTEST(mcp_driver, test_configure_outputs) {
  // configure with active flag (meaning active on configure)
  zassert_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0), GPIO_OUTPUT_ACTIVE),
      0);
  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_GPIOA),
                0x0001);

  // reconfigure without ACTIVE flag
  zassert_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0), GPIO_OUTPUT), 0);

  zassert_equal(hv_shield_v2_emul_get_u16_reg(hv_shield_emul, REG_GPIOA),
                0x0000);

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

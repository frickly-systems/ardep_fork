#include <zephyr/drivers/gpio.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/drivers/i2c_fake.h>
#include <ardep/dt-bindings/hv-shield-v2.h>

DEFINE_FFF_GLOBALS;

static const struct device* i2c_fake = DEVICE_DT_GET(DT_NODELABEL(test_i2c));
static const struct device* hv_shield = DEVICE_DT_GET(DT_NODELABEL(hv_shield0));

ZTEST_SUITE(mcp_driver, NULL, NULL, NULL, NULL, NULL);

ZTEST(mcp_driver, before_all_device_init) {
  zassert_equal(fake_i2c_transfer_fake.call_count, 0);
  zassert_equal(device_init(hv_shield), 0);
  zassert_equal(fake_i2c_transfer_fake.call_count, 2);
  zassert_equal(fake_i2c_transfer_fake.arg0_val, i2c_fake);

  struct i2c_fake_data* data = fake_i2c_get_fake_data(i2c_fake);
  zassert_equal(data->transfer_msg_history[1].len, 3);  // IOCON write
  zassert_equal(data->transfer_msg_history[1].buf[0], 0x0A);

  zassert_equal(data->transfer_msg_history[0].len, 3);  // IODIR write
  zassert_equal(data->transfer_msg_history[0].buf[0], 0x00);
}

ZTEST(mcp_driver, test_set_output_0) {
  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(0), 1), 0);
  zassert_equal(fake_i2c_transfer_fake.call_count, 1);
  zassert_equal(fake_i2c_transfer_fake.arg0_val, i2c_fake);
  struct i2c_fake_data* data = fake_i2c_get_fake_data(i2c_fake);
  zassert_equal(data->transfer_msg_history[0].len, 3);
  zassert_equal(data->transfer_msg_history[0].buf[0], 0x12);  // GPIO register
  zassert_equal(data->transfer_msg_history[0].buf[1], 0x01);
  zassert_equal(data->transfer_msg_history[0].buf[2], 0x00);

  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(0), 0), 0);
  zassert_equal(fake_i2c_transfer_fake.call_count, 2);
  zassert_equal(fake_i2c_transfer_fake.arg0_val, i2c_fake);
  data = fake_i2c_get_fake_data(i2c_fake);
  zassert_equal(data->transfer_msg_history[0].len, 3);
  zassert_equal(data->transfer_msg_history[0].buf[0], 0x12);  // GPIO register
  zassert_equal(data->transfer_msg_history[0].buf[1], 0x00);
  zassert_equal(data->transfer_msg_history[0].buf[2], 0x00);
}

ZTEST(mcp_driver, test_set_output_5) {
  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(5), 1), 0);
  zassert_equal(fake_i2c_transfer_fake.call_count, 1);
  zassert_equal(fake_i2c_transfer_fake.arg0_val, i2c_fake);
  struct i2c_fake_data* data = fake_i2c_get_fake_data(i2c_fake);
  zassert_equal(data->transfer_msg_history[0].len, 3);
  zassert_equal(data->transfer_msg_history[0].buf[0], 0x12);  // GPIO register
  zassert_equal(data->transfer_msg_history[0].buf[1], 0x20);
  zassert_equal(data->transfer_msg_history[0].buf[2], 0x00);

  zassert_equal(gpio_pin_set(hv_shield, HV_SHIELD_V2_OUTPUT(5), 0), 0);
  zassert_equal(fake_i2c_transfer_fake.call_count, 2);
  zassert_equal(fake_i2c_transfer_fake.arg0_val, i2c_fake);
  data = fake_i2c_get_fake_data(i2c_fake);
  zassert_equal(data->transfer_msg_history[0].len, 3);
  zassert_equal(data->transfer_msg_history[0].buf[0], 0x12);  // GPIO register
  zassert_equal(data->transfer_msg_history[0].buf[1], 0x00);
  zassert_equal(data->transfer_msg_history[0].buf[2], 0x00);
}

ZTEST(mcp_driver, test_read_inputs_and_faults) {
  uint8_t read_data[2] = {0x40,
                          0x92};  // Input 1, 4 is high; fault 0, 2 is high
  fake_i2c_set_next_read_data(i2c_fake, read_data, sizeof(read_data));

  gpio_port_value_t value;
  zassert_equal(gpio_port_get(hv_shield, &value), 0);

  zassert_equal((value >> HV_SHIELD_V2_INPUT_BASE) & 0x3F, 0b10010);
  zassert_equal((value >> HV_SHIELD_V2_FAULT_BASE) & 0x7, 0b101);

  // reset
  read_data[0] = 0x00;
  read_data[1] = 0x00;
  fake_i2c_set_next_read_data(i2c_fake, read_data, sizeof(read_data));
  zassert_equal(gpio_port_get(hv_shield, &value), 0);
  zassert_equal(value, 0);
}

ZTEST(mcp_driver, test_configure_outputs) {
  zassert_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0), GPIO_OUTPUT_ACTIVE),
      0);

  zassert_equal(fake_i2c_transfer_fake.call_count, 1);
  zassert_equal(fake_i2c_transfer_fake.arg0_val, i2c_fake);
  struct i2c_fake_data* data = fake_i2c_get_fake_data(i2c_fake);
  zassert_equal(data->transfer_msg_history[0].len, 3);
  zassert_equal(data->transfer_msg_history[0].buf[0], 0x12);  // GPIO register
  zassert_equal(data->transfer_msg_history[0].buf[1], 0x01);
  zassert_equal(data->transfer_msg_history[0].buf[2], 0x00);

  // reconfigure without ACTIVE flag
  zassert_equal(
      gpio_pin_configure(hv_shield, HV_SHIELD_V2_OUTPUT(0), GPIO_OUTPUT), 0);

  zassert_equal(fake_i2c_transfer_fake.call_count, 2);
  zassert_equal(fake_i2c_transfer_fake.arg0_val, i2c_fake);
  data = fake_i2c_get_fake_data(i2c_fake);
  zassert_equal(data->transfer_msg_history[0].len, 3);
  zassert_equal(data->transfer_msg_history[0].buf[0], 0x12);  // GPIO register
  zassert_equal(data->transfer_msg_history[0].buf[1], 0x00);
  zassert_equal(data->transfer_msg_history[0].buf[2], 0x00);

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

  // should not actually configure anything
  zassert_equal(fake_i2c_transfer_fake.call_count, 0);
}

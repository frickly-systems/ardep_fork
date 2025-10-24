#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <ardep/drivers/i2c_fake.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT i2c_fake

struct i2c_fake_config {};

struct i2c_fake_data* fake_i2c_get_fake_data(const struct device* dev) {
  return (struct i2c_fake_data*)dev->data;
}

static int fake_i2c_transfer_custom(const struct device* dev,
                                    struct i2c_msg* msg,
                                    uint8_t num_msgs,
                                    uint16_t flags) {
  struct i2c_fake_data* data = (struct i2c_fake_data*)dev->data;
  // shift transfer_msg_history by num_msgs to make room for new msgs (shift to
  // higher numbers so that 0 is the last msg)
  if (num_msgs > 16) {
    num_msgs = 16;
  }

  // deallocate buffers from old msgs that are being overwritten
#if CONFIG_HEAP_MEM_POOL_SIZE > 0
  for (int i = 0; i < num_msgs && i < data->transfer_msg_count; i++) {
    k_free(data->transfer_msg_history[15 - i].buf);
  }
#endif

  data->transfer_msg_count += num_msgs;
  if (data->transfer_msg_count > 16) {
    data->transfer_msg_count = 16;
  }

  for (int i = 15; i >= num_msgs; i--) {
    data->transfer_msg_history[i] = data->transfer_msg_history[i - num_msgs];
  }

  // allocate heap buffers
  for (int i = 0; i < num_msgs; i++) {
    data->transfer_msg_history[i].len = msg[i].len;
    data->transfer_msg_history[i].flags = msg[i].flags;
    if (msg[i].flags & I2C_MSG_READ) {
      if (msg[i].len != data->read_buf_lens[data->read_buf_populated_len - 1]) {
        return -EIO;  // requested read length does not match prepared data
                      // length
      }
      memcpy(msg[i].buf, data->read_bufs[data->read_buf_populated_len - 1],
             data->read_buf_lens[data->read_buf_populated_len - 1]);
      // shift read buffers down
      data->read_buf_populated_len--;
      for (size_t j = 0; j < data->read_buf_populated_len; j++) {
        data->read_bufs[j] = data->read_bufs[j + 1];
        data->read_buf_lens[j] = data->read_buf_lens[j + 1];
      }
      data->transfer_msg_history[i].buf = NULL;
    } else {
#if CONFIG_HEAP_MEM_POOL_SIZE > 0
      data->transfer_msg_history[i].buf = k_malloc(msg[i].len);
      if (data->transfer_msg_history[i].buf == NULL) {
        return -ENOMEM;
      }
      memcpy(data->transfer_msg_history[i].buf, msg[i].buf, msg[i].len);
#else
      data->transfer_msg_history[i].buf = NULL;
#endif
    }
  }

  return 0;
}

static int i2c_fake_init(const struct device* dev) {
  struct i2c_fake_data* data = fake_i2c_get_fake_data(dev);

  data->transfer_msg_count = 0;

  fake_i2c_transfer_fake.custom_fake = fake_i2c_transfer_custom;

  return 0;
}

DEFINE_FAKE_VALUE_FUNC(int, fake_i2c_configure, const struct device*, uint32_t);

DEFINE_FAKE_VALUE_FUNC(int,
                       fake_i2c_get_config,
                       const struct device*,
                       uint32_t*);

DEFINE_FAKE_VALUE_FUNC(int,
                       fake_i2c_transfer,
                       const struct device*,
                       struct i2c_msg*,
                       uint8_t,
                       uint16_t);

DEFINE_FAKE_VALUE_FUNC(int,
                       fake_i2c_target_register,
                       const struct device*,
                       struct i2c_target_config*);

DEFINE_FAKE_VALUE_FUNC(int,
                       fake_i2c_target_unregister,
                       const struct device*,
                       struct i2c_target_config*);

#ifdef CONFIG_I2C_CALLBACK
DEFINE_FAKE_VALUE_FUNC(int,
                       fake_i2c_transfer_cb,
                       const struct device*,
                       struct i2c_msg*,
                       uint8_t,
                       uint16_t,
                       i2c_callback_t,
                       void*);
#endif /* CONFIG_I2C_CALLBACK */

#if defined(CONFIG_I2C_RTIO)
DEFINE_FAKE_VOID_FUNC(fake_i2c_iodev_submit,
                      const struct device*,
                      struct rtio_iodev_sqe*);
#endif /* CONFIG_I2C_RTIO */

DEFINE_FAKE_VALUE_FUNC(int, fake_i2c_recover_bus, const struct device*);

#ifdef CONFIG_ZTEST
static void fake_i2c_reset_rule_before(const struct ztest_unit_test* test,
                                       void* fixture) {
  ARG_UNUSED(test);
  ARG_UNUSED(fixture);

  RESET_FAKE(fake_i2c_configure);
  RESET_FAKE(fake_i2c_get_config);
  RESET_FAKE(fake_i2c_transfer);
  RESET_FAKE(fake_i2c_target_register);
  RESET_FAKE(fake_i2c_target_unregister);
#ifdef CONFIG_I2C_CALLBACK
  RESET_FAKE(fake_i2c_transfer_cb);
#endif /* CONFIG_I2C_CALLBACK */
#if defined(CONFIG_I2C_RTIO)
  RESET_FAKE(fake_i2c_iodev_submit);
#endif /* CONFIG_I2C_RTIO */
  RESET_FAKE(fake_i2c_recover_bus);

  // Re-install default custom_fake
  fake_i2c_transfer_fake.custom_fake = fake_i2c_transfer_custom;
}

ZTEST_RULE(fake_i2c_reset_rule, fake_i2c_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

void fake_i2c_set_next_read_data(const struct device* dev,
                                 const uint8_t* data,
                                 size_t len) {
  struct i2c_fake_data* fake_data = fake_i2c_get_fake_data(dev);

  if (fake_data->read_buf_populated_len >= ARRAY_SIZE(fake_data->read_bufs)) {
    return;
  }

  fake_data->read_bufs[fake_data->read_buf_populated_len] = data;
  fake_data->read_buf_lens[fake_data->read_buf_populated_len] = len;
  fake_data->read_buf_populated_len++;
}

DEVICE_API(i2c, i2c_fake_driver_api) = {
  .configure = fake_i2c_configure,
  .get_config = fake_i2c_get_config,
  .transfer = fake_i2c_transfer,
  .target_register = fake_i2c_target_register,
  .target_unregister = fake_i2c_target_unregister,
#ifdef CONFIG_I2C_CALLBACK
  .transfer_cb = fake_i2c_transfer_cb,
#endif
#ifdef CONFIG_I2C_RTIO
  .iodev_submit = fake_i2c_iodev_subm,
#endif
  .recover_bus = fake_i2c_recover_bus,
};

#define I2C_FAKE_INIT(inst)                                                   \
  static const struct i2c_fake_config i2c_fake_config_##inst = {};            \
  static struct i2c_fake_data i2c_fake_data_##inst;                           \
  I2C_DEVICE_DT_INST_DEFINE(inst, i2c_fake_init, NULL, &i2c_fake_data_##inst, \
                            &i2c_fake_config_##inst, POST_KERNEL,             \
                            CONFIG_I2C_INIT_PRIORITY, &i2c_fake_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_FAKE_INIT)

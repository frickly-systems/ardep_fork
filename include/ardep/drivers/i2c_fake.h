#ifndef ARDEP_INCLUDE_DRIVERS_I2C_FAKE_H_
#define ARDEP_INCLUDE_DRIVERS_I2C_FAKE_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int,
                        fake_i2c_configure,
                        const struct device*,
                        uint32_t);
DECLARE_FAKE_VALUE_FUNC(int,
                        fake_i2c_get_config,
                        const struct device*,
                        uint32_t*);
DECLARE_FAKE_VALUE_FUNC(int,
                        fake_i2c_transfer,
                        const struct device*,
                        struct i2c_msg*,
                        uint8_t,
                        uint16_t);
DECLARE_FAKE_VALUE_FUNC(int,
                        fake_i2c_target_register,
                        const struct device*,
                        struct i2c_target_config*);
DECLARE_FAKE_VALUE_FUNC(int,
                        fake_i2c_target_unregister,
                        const struct device*,
                        struct i2c_target_config*);

#ifdef CONFIG_I2C_CALLBACK
DECLARE_FAKE_VALUE_FUNC(int,
                        fake_i2c_transfer_cb,
                        const struct device*,
                        struct i2c_msg*,
                        uint8_t,
                        uint16_t,
                        i2c_callback_t,
                        void*);
#endif
#if defined(CONFIG_I2C_RTIO)
DECLARE_FAKE_VOID_FUNC(fake_i2c_iodev_submit,
                       const struct device*,
                       struct rtio_iodev_sqe*);
#endif

DECLARE_FAKE_VALUE_FUNC(int, fake_i2c_recover_bus, const struct device*);

struct i2c_fake_data {
  struct i2c_msg transfer_msg_history[16];
  size_t transfer_msg_count;

  const uint8_t* read_bufs[16];
  size_t read_buf_lens[16];
  size_t read_buf_populated_len;
};

struct i2c_fake_data* fake_i2c_get_fake_data(const struct device* dev);

// Note that data buffer is not copied, so it must remain valid until the next
// call to transfer
void fake_i2c_set_next_read_data(const struct device* dev,
                                 const uint8_t* data,
                                 size_t len);

#ifdef __cplusplus
}
#endif

#endif

#ifndef APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_
#define APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_

#include "ardep/uds_new.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#include <ardep/iso14229.h>
#include <iso14229.h>

DECLARE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

DECLARE_FAKE_VALUE_FUNC(UDSErr_t,
                        data_id_check_fn,
                        struct uds_new_context *const,
                        bool *);

DECLARE_FAKE_VALUE_FUNC(UDSErr_t,
                        data_id_action_fn,
                        struct uds_new_context *const,
                        bool *);

extern const uint16_t data_id_r;
extern uint8_t data_id_r_data[4];

extern const uint16_t data_id_rw;
extern uint8_t data_id_rw_data[4];

extern const uint16_t data_id_rw_duplicated;
extern uint8_t data_id_rw_duplicated_data[4];

enum callback_type {
  DATA_ID_CHECK,
  DATA_ID_ACTION,
};

struct fake_function_args {
  enum callback_type type;
  union {
    struct {
      const struct uds_new_context *const ctx;
      bool *result;
    } data_id_check;
    struct {
      struct uds_new_context *const ctx;
      bool *result;
    } data_id_action;
  };
};

struct lib_uds_new_fixture {
  UDSISOTpCConfig_t cfg;
  struct uds_new_instance_t *instance;
  const struct device *can_dev;

  struct fake_function_args fff_args[10];
  size_t fff_args_count;
};

/**
 * @brief Receive an event from iso14229
 */
UDSErr_t receive_event(struct uds_new_instance_t *inst,
                       UDSEvent_t event,
                       void *args);

/**
 * @brief Assert that the copied data matches the expected data.
 *
 * Beware that the data is in big endian!
 */
void assert_copy_data(const uint8_t *data, uint32_t len);

#endif  // APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_

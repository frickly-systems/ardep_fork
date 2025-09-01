/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

FAKE_VOID_FUNC(ecu_reset_work_handler, struct k_work *);

ZTEST_F(lib_uds_new, test_0x11_ecu_reset) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSECUResetArgs_t args = {.type = ECU_RESET_HARD};

  int ret = receive_event(instance, UDS_EVT_EcuReset, &args);
  zassert_ok(ret);

  // Wait for the scheduled worker to finish
  k_msleep(2000);
  zassert_equal(ecu_reset_work_handler_fake.call_count, 1);
}

ZTEST_F(lib_uds_new, test_0x11_ecu_reset_fails_when_subtype_not_implemented) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSECUResetArgs_t args = {.type = ECU_RESET_KEY_OFF_ON};

  int ret = receive_event(instance, UDS_EVT_EcuReset, &args);
  zassert_equal(ret, UDS_NRC_SubFunctionNotSupported);
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_single_element) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data1_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(by_id_data1));

  uint8_t expected[2] = {
    0x00,
    0x05,
  };
  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_fails_when_id_unknown) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data_unknown_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_array) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data2_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(by_id_data2));

  uint8_t expected[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_custom) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data_custom_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(by_id_data_custom));

  assert_copy_data((uint8_t *)&by_id_data_custom_default,
                   sizeof(by_id_data_custom_default));
}

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
ZTEST_F(lib_uds_new, test_0x22_read_by_id_dynamic_array) {
  struct uds_new_instance_t *instance = fixture->instance;

  instance->state.diag_session_type = 1;

  uint16_t id = 0x9988;
  __unused uint32_t data[4] = {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};
  uint32_t context = 0x12345678;

  struct uds_new_state_requirements state_req = {
    .session_type = 1,
    .session_type_level = UDS_NEW_STATE_LEVEL_LESS_OR_EQUAL,
  };

  int ret = instance->register_data_by_identifier(
      instance, id, data_id_custom_read_fn, data_id_custom_write_fn, state_req,
      &context);
  zassert_ok(ret);

  UDSRDBIArgs_t args = {
    .dataId = id,
    .copy = copy,
  };

  ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(data_id_custom_write_fn_fake.call_count, 0);

  zassert_equal(data_id_custom_read_fn_fake.call_count, 1);
  zassert_equal(data_id_custom_read_fn_fake.arg0_val, id);
  zassert_equal(data_id_custom_read_fn_fake.arg1_val.session_type,
                state_req.session_type);
  zassert_equal(data_id_custom_read_fn_fake.arg1_val.session_type_level,
                state_req.session_type_level);
  zassert_equal_ptr(data_id_custom_read_fn_fake.arg5_val, &context);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);

  uint8_t expected[4] = {0x78, 0x56, 0x34, 0x12};
  zassert_equal(copy_fake.arg2_val, sizeof(expected));

  assert_copy_data(expected, sizeof(expected));
}
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

ZTEST_F(lib_uds_new,
        test_0x22_read_by_id_fails_when_state_requirements_do_not_match) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data3_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_equal(ret, UDS_NRC_ConditionsNotCorrect);

  zassert_equal(copy_fake.call_count, 0);
}

ZTEST_F(lib_uds_new, test_0x2E_write_by_id_fails_when_id_unknown) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint8_t data[2] = {0xBE, 0xEF};
  UDSWDBIArgs_t args = {
    .dataId = by_id_data_unknown_id,
    .data = data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x2E_write_by_id_static_single_element) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint8_t data[2] = {0xBE, 0xEF};
  UDSWDBIArgs_t args = {
    .dataId = by_id_data1_id,
    .data = data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &args);
  zassert_ok(ret);

  uint16_t expected = 0xBEEF;
  zassert_equal(expected, by_id_data1, "Expected 0x%04X, but was: 0x%04X",
                expected, by_id_data1);
}

ZTEST_F(lib_uds_new, test_0x2E_write_by_id_static_array) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint8_t data[6] = {0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
  UDSWDBIArgs_t args = {
    .dataId = by_id_data2_id,
    .data = data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &args);
  zassert_ok(ret);

  uint16_t expected[3] = {0xBEEF, 0xCAFE, 0xBABE};
  zassert_mem_equal(expected, by_id_data2, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x2E_write_by_id_static_custom) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint8_t data[6] = {0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
  UDSWDBIArgs_t args = {
    .dataId = by_id_data_custom_id,
    .data = data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(data_id_custom_write_fn_fake.call_count, 1);
  zassert_equal(data_id_custom_write_fn_fake.arg0_val, by_id_data_custom_id);
  zassert_equal_ptr(data_id_custom_write_fn_fake.arg3_val, data);
  zassert_equal(data_id_custom_write_fn_fake.arg4_val, sizeof(data));
}

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
ZTEST_F(lib_uds_new, test_0x2E_write_by_id_dynamic_array) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint16_t id = 0x8899;
  uint32_t context = 0x12345678;
  struct uds_new_state_requirements state_req = {
    .session_type = 1,
    .session_type_level = UDS_NEW_STATE_LEVEL_LESS_OR_EQUAL,
  };

  instance->register_data_by_identifier(instance, id, data_id_custom_read_fn,
                                        data_id_custom_write_fn, state_req,
                                        &context);

  uint8_t data_to_write[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                               0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};

  UDSWDBIArgs_t args = {
    .dataId = id,
    .data = (uint8_t *)data_to_write,
    .len = sizeof(data_to_write),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal_ptr(data_id_custom_write_fn_fake.arg3_val, &data_to_write[0]);
  zassert_equal_ptr(data_id_custom_write_fn_fake.arg4_val,
                    sizeof(data_to_write));
  zassert_equal_ptr(data_id_custom_write_fn_fake.arg5_val, &context);
}
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

ZTEST_F(lib_uds_new, test_0x2E_write_by_id_fails_when_write_not_allowed) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint8_t data_to_write[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

  UDSWDBIArgs_t args = {
    .dataId = by_id_data_no_rw_id,
    .data = data_to_write,
    .len = sizeof(data_to_write),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new,
        test_0x2E_write_by_id_fails_when_state_requirements_do_not_match) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint8_t data[6] = {0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
  UDSWDBIArgs_t args = {
    .dataId = by_id_data3_id,
    .data = data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &args);
  zassert_equal(ret, UDS_NRC_ConditionsNotCorrect);
}
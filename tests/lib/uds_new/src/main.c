/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_single_element) {
  struct uds_new_instance_t *instance = &fixture->instance;

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

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_array) {
  struct uds_new_instance_t *instance = &fixture->instance;

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
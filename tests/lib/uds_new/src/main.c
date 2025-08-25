/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>
#include <iso14229/server.h>
#include <iso14229/tp/isotp_c.h>
#include <iso14229/uds.h>

FAKE_VALUE_FUNC(UDSErr_t,
                test_ecu_reset_callback,
                struct iso14229_zephyr_instance *,
                enum ecu_reset_type,
                void *);

ZTEST_F(lib_uds_new, test_0x11_ecu_reset) {
  RESET_FAKE(test_ecu_reset_callback);
  zassert_equal(test_ecu_reset_callback_fake.call_count, 0);

  test_ecu_reset_callback_fake.return_val = UDS_OK;

  struct iso14229_zephyr_instance *instance = &fixture->instance;
  int ret = set_ecu_reset_callback(test_ecu_reset_callback);
  assert(ret == 0);

  uint8_t request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x11,  // SID (ECU Reset)
    0x01,  // LEV_RT  (Hard Reset)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_ecu_reset_callback_fake.call_count, 1);
  zassert_equal(test_ecu_reset_callback_fake.arg1_val, ECU_RESET_HARD);
}
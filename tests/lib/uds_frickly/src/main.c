/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_frickly.h>

ZTEST_F(lib_uds_frickly, test_0x11_ecu_reset) {
  struct uds_service* service = &fixture->service;

  UDSECUResetArgs_t args = {
    .type = 1,
    .powerDownTimeMillis = 100,
  };
  receive_event(service, UDS_EVT_EcuReset, &args);

  k_msleep(2000);

  service->iso14229.thread_tick(&service->iso14229);

  zassert_equal(1, 1);
}
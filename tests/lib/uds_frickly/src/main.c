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
  struct uds_service* service = fixture->service;

  UDSDiagSessCtrlArgs_t args = {
    .type = 0x01,
    .p2_ms = 100,
    .p2_star_ms = 5000,
  };
  int ret = receive_event(service, UDS_EVT_DiagSessCtrl, &args);
  zassert_ok(ret);

  zassert_equal(service->state.session_type, 0x01);
}
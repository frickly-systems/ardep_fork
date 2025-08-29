/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_frickly.h>

ZTEST_F(lib_uds_frickly, test_0x11_ecu_reset) { zassert_equal(1, 1); }
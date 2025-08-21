/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "iso14229_common.h"

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/ztest.h>

#include <iso14229/server.h>
#include <iso14229/tp.h>
#include <iso14229/tp/isotp_c.h>
#include <iso14229_common.h>
#include <uds_new.h>

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(UDSErr_t,
                       test_uds_callback,
                       struct iso14229_zephyr_instance *,
                       UDSEvent_t,
                       void *,
                       void *);

static const UDSISOTpCConfig_t cfg = {
  // Hardwarea Addresses
  .source_addr = 0x7E8,  // Can ID Client
  .target_addr = 0x7E0,  // Can ID Server (us)

  // Functional Addresses
  .source_addr_func = 0x7DF,             // ID Client
  .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Server (us)
};

// Variables to capture callback and test data
static can_rx_callback_t captured_rx_callback_phys = NULL;
static void *captured_user_data_phy = NULL;
static can_rx_callback_t captured_rx_callback_func = NULL;
static void *captured_user_data_func = NULL;

struct can_tx_default_cb_ctx {
  struct k_sem done;
  int status;
};

int can_send_t_fake_impl(const struct device *dev,
                         const struct can_frame *frame,
                         k_timeout_t timeout,
                         can_tx_callback_t callback,
                         void *user_data) {
  struct can_tx_default_cb_ctx *ctx = user_data;

  k_sem_give(&ctx->done);
  ctx->status = 0;  // Success
  return 0;
}

// Custom fake to capture the RX filter callback
static int capture_rx_filter_fake(const struct device *dev,
                                  can_rx_callback_t callback,
                                  void *user_data,
                                  const struct can_filter *filter) {
  ARG_UNUSED(dev);
  ARG_UNUSED(filter);

  if (filter->id == cfg.source_addr) {
    captured_rx_callback_phys = callback;
    captured_user_data_phy = user_data;
  } else if (filter->id == cfg.source_addr_func) {
    captured_rx_callback_func = callback;
    captured_user_data_func = user_data;
  } else {
    // If the filter ID does not match, we do not capture it
    return -EINVAL;  // Return error for unsupported filter ID
  }

  return 0;
}

void send_phys_can_frame(const struct lib_uds_minimal_fixture *fixture,
                         uint8_t *data,
                         uint8_t data_len) {
  const struct device *dev = fixture->can_dev;

  struct can_frame frame = {
    .id = fixture->cfg.target_addr,  // 0x7E0 - message TO the server
    .dlc = data_len,                 // data_len == dlc for Can CC
    .flags = 0,
  };
  memcpy(frame.data, data, data_len);

  captured_rx_callback_phys(dev, &frame, captured_user_data_phy);
}

void assert_send_phy_can_frame(const struct lib_uds_minimal_fixture *fixture,
                               uint8_t *data,
                               uint8_t data_len) {
  struct can_frame expected_frame = {
    .id = fixture->cfg.source_addr,  // 0x7E0 - message TO the server
    .dlc = data_len,                 // data_len == dlc for Can CC
    .flags = 0,
  };
  memcpy(expected_frame.data, data, data_len);

  struct can_frame actual_frame = *fake_can_send_fake.arg1_val;

  zassert_equal(actual_frame.id, expected_frame.id);  // response address
  zassert_equal(actual_frame.dlc,
                expected_frame.dlc);  // data_len == dlc for Can CC
  zassert_mem_equal(actual_frame.data, expected_frame.data, data_len);
}

static void *uds_minimal_setup(void) {
  static struct lib_uds_minimal_fixture fixture = {
    .cfg = cfg,
    .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
  };

  return &fixture;
}

static void uds_minimal_before(void *f) {
  struct lib_uds_minimal_fixture *fixture = f;
  const struct device *dev = fixture->can_dev;
  struct iso14229_zephyr_instance *uds_instance = &fixture->instance;

  RESET_FAKE(test_uds_callback);
  FFF_RESET_HISTORY();

  // Set up the fake to capture RX filter callbacks
  fake_can_send_fake.custom_fake = can_send_t_fake_impl;
  fake_can_add_rx_filter_fake.custom_fake = capture_rx_filter_fake;

  // Variables to capture CAN RX filter setup
  captured_rx_callback_phys = NULL;
  captured_user_data_phy = NULL;

  // Configure UDS TP settings
  UDSISOTpCConfig_t tp_config = {
    .source_addr = fixture->cfg.source_addr,            // 0x7E8 (client)
    .target_addr = fixture->cfg.target_addr,            // 0x7E0 (server - us)
    .source_addr_func = fixture->cfg.source_addr_func,  // 0x7DF
    .target_addr_func = fixture->cfg.target_addr_func,  // UDS_TP_NOOP_ADDR
  };

  int ret = iso14229_zephyr_init(uds_instance, &tp_config, dev, NULL);

  assert(ret == 0);
  // we add 2 can filters in iso14229_zephyr_init()
  assert(fake_can_add_rx_filter_fake.call_count == 2);
  assert(uds_instance->server.fn);

  // Set the unified callback
  iso14229_zephyr_set_callback(uds_instance, test_uds_callback);
}

ZTEST_SUITE(
    lib_uds_minimal, NULL, uds_minimal_setup, uds_minimal_before, NULL, NULL);
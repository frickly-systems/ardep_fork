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
                       uds_diag_sess_ctrl_callback,
                       struct UDSServer *,
                       const UDSDiagSessCtrlArgs_t *,
                       void *);

DEFINE_FAKE_VALUE_FUNC(UDSErr_t,
                       uds_read_mem_by_addr_callback,
                       struct UDSServer *,
                       const UDSReadMemByAddrArgs_t *,
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

static const uint8_t memory_data[255] = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
  0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
  0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
  0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,
  0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
  0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
  0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82,
  0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C,
  0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
  0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
  0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3,
  0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
  0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD,
  0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
  0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF

};

// fake_can_send
static UDSErr_t inject_test_data(struct UDSServer *srv,
                                 const UDSReadMemByAddrArgs_t *read_args,
                                 void *user_data) {
  return read_args->copy(srv,
                         &memory_data[(uint32_t)(uintptr_t)read_args->memAddr],
                         read_args->memSize);
}

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
  struct can_frame frame = {
    .id = fixture->cfg.source_addr,  // 0x7E0 - message TO the server
    .dlc = data_len,                 // data_len == dlc for Can CC
    .flags = 0,
  };
  memcpy(frame.data, data, data_len);

  zassert_equal(frame.id, fixture->cfg.source_addr);  // response address
  zassert_equal(frame.dlc, data_len);  // data_len == dlc for Can CC
  zassert_mem_equal(frame.data, data, data_len);
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

  RESET_FAKE(uds_read_mem_by_addr_callback);
  RESET_FAKE(uds_diag_sess_ctrl_callback);
  FFF_RESET_HISTORY();

  uds_read_mem_by_addr_callback_fake.custom_fake = inject_test_data;
  fake_can_send_fake.custom_fake = can_send_t_fake_impl;
  // Variables to capture CAN RX filter setup
  captured_rx_callback_phys = NULL;
  captured_user_data_phy = NULL;

  // Set up the fake to capture RX filter callbacks
  fake_can_add_rx_filter_fake.custom_fake = capture_rx_filter_fake;

  // Configure UDS TP settings
  UDSISOTpCConfig_t tp_config = {
    .source_addr = fixture->cfg.source_addr,            // 0x7E8 (client)
    .target_addr = fixture->cfg.target_addr,            // 0x7E0 (server - us)
    .source_addr_func = fixture->cfg.source_addr_func,  // 0x7DF
    .target_addr_func = fixture->cfg.target_addr_func,  // UDS_TP_NOOP_ADDR
  };

  // Setup callbacks
  struct uds_callbacks callbacks = {
    .uds_read_mem_by_addr_fn = uds_read_mem_by_addr_callback,
    .uds_uds_diag_sess_ctrl_fn = uds_diag_sess_ctrl_callback,
  };

  int ret =
      iso14229_zephyr_init(uds_instance, &tp_config, dev, callbacks, NULL);

  assert(ret == 0);
  // we add 2 can filters in iso14229_zephyr_init()
  assert(fake_can_add_rx_filter_fake.call_count == 2);
  assert(uds_instance->server.fn);
}

ZTEST_SUITE(
    lib_uds_minimal, NULL, uds_minimal_setup, uds_minimal_before, NULL, NULL);
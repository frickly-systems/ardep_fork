/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

// Include UDS minimal library headers
#include <iso14229/server.h>
#include <iso14229/tp/isotp_c.h>
#include <iso14229/uds.h>
#include <iso14229_common.h>
#include <uds_new.h>

ZTEST_F(lib_uds_minimal, test_read_memory) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  // Create a UDS Read Memory by Address request frame (single frame ISO-TP)
  // Service ID 0x23 = ReadMemoryByAddress
  // Format: [PCI] [SID] [ALFID] [Address] [Size]
  // PCI = 0x07 (single frame, 7 bytes of data)
  // SID = 0x23 (ReadMemoryByAddress)
  // ALFID = 0x14 (size length = 4 bytes, address length = 1 byte) (this is
  //   inverted to the position in data!!)
  // Address = 0x00000001 (4 byte address)
  // Size = 0x04 (1 byte size)
  uint8_t request_data[] = {0x07, 0x23, 0x14, 0x00, 0x00, 0x00, 0x01, 0x04};

  send_phys_can_frame_array(fixture, request_data);
  iso14229_zephyr_thread_tick(instance);

  // Verify the callback was called correctly
  zassert_equal(uds_read_mem_by_addr_callback_fake.call_count, 1);
  zassert_equal(
      (uint32_t)(uintptr_t)uds_read_mem_by_addr_callback_fake.arg1_val->memAddr,
      1);

  zassert_equal(uds_read_mem_by_addr_callback_fake.arg1_val->memSize, 4);

  // Sleep long enough to elapse the timeout for the response to be send
  k_msleep(1000);
  iso14229_zephyr_thread_tick(instance);

  // Verify that a CAN frame was sent in response
  zassert_equal(fake_can_send_fake.call_count, 1);

  // Expected: [PCI=0x05] [SID=0x63] [Data=1,2,3,4]
  // PCI    = 0x05 (single frame, 5 bytes of data: SID + 4 data bytes)
  // RMBAPR = 0x63 (positive response to 0x23)
  // DREC   = data read from memory
  uint8_t response_data[] = {
    0x05, 0x63, 0x01, 0x02, 0x03, 0x04,
  };
  assert_send_phy_can_frame_array(fixture, response_data);
}

ZTEST_F(lib_uds_minimal, test_diag_session_ctrl) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  // Create a UDS Diagnostic Session Control request frame (single frame ISO-TP)
  // Service ID 0x10 = DiagnosticSessionControl
  // Format: [PCI] [DSC] [DS]
  // PCI = 0x02 (single frame, 2 bytes of data)
  // SID = 0x10 (DiagnosticSessionControl)
  // DS  = 0x02 (Programming Session)
  uint8_t request_data[] = {0x02, 0x10, 0x02};

  send_phys_can_frame_array(fixture, request_data);
  k_msleep(1000);
  iso14229_zephyr_thread_tick(instance);

  zassert_equal(uds_diag_sess_ctrl_callback_fake.call_count, 1);
  zassert_equal(uds_diag_sess_ctrl_callback_fake.arg1_val->type, 02);

  // Sleep long enough to elapse the timeout for the response to be send
  k_msleep(1000);
  iso14229_zephyr_thread_tick(instance);

  // Verify that a CAN frame was sent in response
  zassert_equal(fake_can_send_fake.call_count, 1);

  // Expected: [PCI=0x05] [SID=0x63] [Data=1,2,3,4]
  // PCI   = 0x05 (single frame, 5 bytes of data: SID + 4 data bytes)
  // DSCPR = 0x50 (positive response to 0x10)
  // DS    = 0x2 (programming session)
  // SPREC = session parameter (p2_ms and p2*_ms encoded)
  uint8_t response_data[] = {
    0x05,
    0x63,
    0x02,
    UDS_CLIENT_DEFAULT_P2_MS >> 8,
    UDS_CLIENT_DEFAULT_P2_MS & 0xFF,
    (uint8_t)((UDS_CLIENT_DEFAULT_P2_STAR_MS / 10) >> 8),
    (uint8_t)(UDS_CLIENT_DEFAULT_P2_STAR_MS / 10),
  };
  assert_send_phy_can_frame_array(fixture, response_data);
}

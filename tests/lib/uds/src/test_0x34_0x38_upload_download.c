/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/ztest.h>

#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash_controller))
#define SCRATCH_PARTITION DT_NODELABEL(scratch_partition)
#define SCRATCH_PARTITION_OFFSET DT_REG_ADDR(SCRATCH_PARTITION)
#define SCRATCH_PARTITION_SIZE DT_REG_SIZE(SCRATCH_PARTITION)

#define SCRATCH_BASE_ADDRESS (FLASH_BASE_ADDRESS + SCRATCH_PARTITION_OFFSET)

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_fail_on_size_0) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
    .addr = (void*)SCRATCH_BASE_ADDRESS,
    .size = 0,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_fail_on_format_identifier_not_0) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
    .addr = (void*)SCRATCH_BASE_ADDRESS,
    .size = 128,
    .dataFormatIdentifier = 0x01,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_success) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
    .addr = (void*)SCRATCH_BASE_ADDRESS,
    .size = SCRATCH_PARTITION_SIZE,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_OK);
}

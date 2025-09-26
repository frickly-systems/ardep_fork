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

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_fail_on_size_0) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
    .addr = (void*)0x20000000,
    .size = 0,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_fail_on_format_identifier_not_0) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
    .addr = (void*)0x20000000,
    .size = 128,
    .dataFormatIdentifier = 0x01,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

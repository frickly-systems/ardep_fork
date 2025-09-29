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

#include <zephyr/drivers/flash.h>

#include <zephyr/ztest.h>

#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash_controller))
#define SCRATCH_PARTITION DT_NODELABEL(scratch_partition)
#define SCRATCH_PARTITION_OFFSET DT_REG_ADDR(SCRATCH_PARTITION)
#define SCRATCH_PARTITION_SIZE DT_REG_SIZE(SCRATCH_PARTITION)

#define SCRATCH_BASE_ADDRESS (FLASH_BASE_ADDRESS + SCRATCH_PARTITION_OFFSET)

const struct device *const flash_controller =
  DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

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

static void clear_scratch_partition(void) {
  int ret = flash_erase(flash_controller, SCRATCH_PARTITION_OFFSET, SCRATCH_PARTITION_SIZE);
  zassert_equal(ret, 0);
}

static void fill_scratch_with_test_pattern(void) {
  clear_scratch_partition();

  uint8_t buf[SCRATCH_PARTITION_SIZE];

  for (size_t i = 0; i < sizeof(buf); i++) {
    buf[i] = (uint8_t)i; // some test pattern
  }

  int ret = flash_write(flash_controller, SCRATCH_PARTITION_OFFSET, buf, sizeof(buf));
  zassert_equal(ret, 0);
}

static void assert_scratch_is_erased(void) {
  uint8_t buf[SCRATCH_PARTITION_SIZE];

  int ret = flash_read(flash_controller, SCRATCH_PARTITION_OFFSET, buf, sizeof(buf));
  zassert_equal(ret, 0);

  uint8_t erased_pattern = 0xFF;
  for (size_t i = 0; i < sizeof(buf); i++) {
    zassert_equal(buf[i], erased_pattern);
  }
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_success) {
  fill_scratch_with_test_pattern();

  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
    .addr = (void*)SCRATCH_BASE_ADDRESS,
    .size = SCRATCH_PARTITION_SIZE,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_OK);

  assert_scratch_is_erased();
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_data) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t download_args = {
    .addr = (void*)SCRATCH_BASE_ADDRESS,
    .size = SCRATCH_PARTITION_SIZE,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &download_args);
  zassert_equal(ret, UDS_OK);

  assert_scratch_is_erased();

  UDSTransferDataArgs_t transfer_args_1 = {
    .data = (const uint8_t[]){0xDE, 0xAD, 0xBE, 0xEF},
    .len = 4,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args_1);
  zassert_equal(ret, UDS_OK);

  uint8_t buf[4];
  ret = flash_read(flash_controller, SCRATCH_PARTITION_OFFSET, buf, sizeof(buf));
  zassert_equal(ret, 0);
  zassert_mem_equal(buf, transfer_args_1.data, sizeof(buf));

  UDSTransferDataArgs_t transfer_args_2 = {
    .data = (const uint8_t[]){0xCA, 0xFE, 0xBA, 0xBE},
    .len = 4,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args_2);
  zassert_equal(ret, UDS_OK);

  ret = flash_read(flash_controller, SCRATCH_PARTITION_OFFSET + 4, buf, sizeof(buf));
  zassert_equal(ret, 0);
  zassert_mem_equal(buf, transfer_args_2.data, sizeof(buf));
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_data_prevent_overflow) {
  struct uds_instance_t *instance = fixture->instance;

  const size_t download_size = SCRATCH_PARTITION_SIZE;
  zassert_true(download_size > 0);

  UDSRequestDownloadArgs_t download_args = {
    .addr = (void*)SCRATCH_BASE_ADDRESS,
    .size = download_size,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &download_args);
  zassert_equal(ret, UDS_OK);

  assert_scratch_is_erased();

  uint8_t guard_before;
  ret = flash_read(flash_controller,
                   SCRATCH_PARTITION_OFFSET + download_size,
                   &guard_before,
                   sizeof(guard_before));
  zassert_equal(ret, 0);

  uint8_t chunk[256];
  size_t bytes_sent = 0;

  while (bytes_sent < download_size) {
    size_t remaining = download_size - bytes_sent;
    size_t chunk_len = remaining < sizeof(chunk) ? remaining : sizeof(chunk);

    for (size_t i = 0; i < chunk_len; i++) {
      chunk[i] = (uint8_t)((bytes_sent + i) & 0xFF);
    }

    UDSTransferDataArgs_t transfer_chunk = {
      .data = chunk,
      .len = chunk_len,
    };

    ret = receive_event(instance, UDS_EVT_TransferData, &transfer_chunk);
    zassert_equal(ret, UDS_OK);

    bytes_sent += chunk_len;
  }

  uint8_t last_written_byte;
  ret = flash_read(flash_controller,
                   SCRATCH_PARTITION_OFFSET + download_size - 1,
                   &last_written_byte,
                   sizeof(last_written_byte));
  zassert_equal(ret, 0);
  zassert_equal(last_written_byte, (uint8_t)((download_size - 1) & 0xFF));

  const uint8_t overflow_byte = 0xAA;
  UDSTransferDataArgs_t overflow_args = {
    .data = &overflow_byte,
    .len = 1,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &overflow_args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);

  uint8_t guard_after;
  ret = flash_read(flash_controller,
                   SCRATCH_PARTITION_OFFSET + download_size,
                   &guard_after,
                   sizeof(guard_after));
  zassert_equal(ret, 0);
  zassert_equal(guard_after, guard_before);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_exit_success) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t download_args = {
    .addr = (void*)SCRATCH_BASE_ADDRESS,
    .size = SCRATCH_PARTITION_SIZE,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &download_args);
  zassert_equal(ret, UDS_OK);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_OK);

  const uint8_t dummy_data = 0xFF;
  UDSTransferDataArgs_t transfer_args = {
    .data = &dummy_data,
    .len = 1,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_exit_out_of_sequence) {
  struct uds_instance_t *instance = fixture->instance;

  int ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  if (ret == UDS_OK) {
    // try again to make sure we are out of sequence
    ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  }

  zassert_equal(ret, UDS_NRC_RequestSequenceError);
}



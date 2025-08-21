/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Use scripts/uds_iso14229_demo_script.py to test

#include "iso14229_common.h"
#include "write_memory_by_addr_impl.h"

#include <errno.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <iso14229/server.h>
#include <iso14229/tp/isotp_c.h>
#include <iso14229/util.h>
#include <uds_new.h>

LOG_MODULE_REGISTER(iso14229_testing, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

uint8_t dummy_memory[512] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x66, 0x7, 0x8};

struct iso14229_zephyr_instance inst;

struct uds_new_instance_t instance;

uint16_t variable = 5;
char variable2[] = "Hello world";
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(dings, &instance, 0x1234, variable);
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(
    dings2, &instance, 0x1235, variable2, sizeof(variable2));

// todo: tickets für nötige msgs

UDSErr_t read_mem_by_addr_impl(struct UDSServer *srv,
                               const UDSReadMemByAddrArgs_t *read_args,
                               void *user_context) {
  uint32_t addr = (uintptr_t)read_args->memAddr;

  LOG_INF("Read Memory By Address: addr=0x%08X size=%u", addr,
          read_args->memSize);

  return read_args->copy(srv,
                         &dummy_memory[(uint32_t)(uintptr_t)read_args->memAddr],
                         read_args->memSize);
}

// Unified callback function for handling UDS events
UDSErr_t uds_event_callback(struct iso14229_zephyr_instance *inst,
                            UDSEvent_t event,
                            void *arg,
                            void *user_context) {
  switch (event) {
    case UDS_EVT_ReadMemByAddr: {
      UDSReadMemByAddrArgs_t *read_args = (UDSReadMemByAddrArgs_t *)arg;
      return read_mem_by_addr_impl(&inst->server, read_args, user_context);
    }
    case UDS_EVT_DiagSessCtrl: {
      LOG_INF("Diagnostic Session Control event");
      return UDS_OK;
    }
    default:
      LOG_DBG("Unhandled UDS event: %s", UDSEventToStr(event));
      return UDS_OK;
  }
}

int main(void) {
  UDSISOTpCConfig_t cfg = {
    // Hardwarea Addresses
    .source_addr = 0x7E8,  // Can ID Client
    .target_addr = 0x7E0,  // Can ID Server (us)

    // Functional Addresses
    .source_addr_func = 0x7DF,             // ID Client
    .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Server (us)
  };

  iso14229_zephyr_init(&inst, &cfg, can_dev, NULL);

  // Set the unified callback
  iso14229_zephyr_set_callback(&inst, uds_event_callback);

  int err;
  if (!device_is_ready(can_dev)) {
    printk("CAN device not ready\n");
    return -ENODEV;
  }

  err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    printk("Failed to set CAN mode: %d\n", err);
    return err;
  }

  err = can_start(can_dev);
  if (err) {
    printk("Failed to start CAN device: %d\n", err);
    return err;
  }
  printk("CAN device started\n");

  while (1) {
    iso14229_zephyr_thread(&inst);
  }
}

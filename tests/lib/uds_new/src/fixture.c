/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_new.h"
#include "fixture.h"
#include "iso14229/uds.h"
#include "zephyr/sys/util.h"

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/ztest.h>

#include <iso14229/server.h>
#include <iso14229/tp.h>
#include <iso14229/tp/isotp_c.h>

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(operate_on_id1,
                                        NULL,
                                        by_id_data1_id,
                                        by_id_data1);

UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(operate_on_id2,
                                              NULL,
                                              by_id_data2_id,
                                              by_id_data2);

static const UDSISOTpCConfig_t cfg = {
  // Hardware Addresses
  .source_addr = 0x7E8,  // Can ID Server (us)
  .target_addr = 0x7E0,  // Can ID Client (them)

  // Functional Addresses
  .source_addr_func = 0x7DF,             // ID Server (us)
  .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Client (them)
};

static uint8_t copied_data[32];
static uint32_t copied_len;

void assert_copy_data(uint8_t *data, uint32_t len) {
  zassert_equal(copied_len, len);
  zassert_mem_equal(copied_data, data, len);
}

static uint8_t custom_copy(UDSServer_t *server,
                           const void *data,
                           uint16_t len) {
  copied_len = len;
  memcpy(copied_data, data, len);

  return 0;
}

static void *uds_new_setup(void) {
  static struct lib_uds_new_fixture fixture = {
    .cfg = cfg,
    .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
  };

  return &fixture;
}

static void uds_new_before(void *f) {
  struct lib_uds_new_fixture *fixture = f;
  const struct device *dev = fixture->can_dev;
  struct uds_new_instance_t *uds_instance = &fixture->instance;

  RESET_FAKE(copy);
  FFF_RESET_HISTORY();

  copy_fake.custom_fake = custom_copy;

  int ret = uds_new_init(uds_instance, &cfg, dev, NULL);
  assert(ret == 0);

  operate_on_id1.instance = uds_instance;
  by_id_data1 = by_id_data1_default;

  operate_on_id2.instance = uds_instance;
  memcpy(by_id_data2, by_id_data2_default, sizeof(by_id_data2_default));

  memset(copied_data, 0, sizeof(copied_data));
  copied_len = 0;
}

typedef UDSErr_t (*uds_callback)(struct iso14229_zephyr_instance *inst,
                                 UDSEvent_t event,
                                 void *arg,
                                 void *user_context);

UDSErr_t receive_event(struct uds_new_instance_t *inst,
                       UDSEvent_t event,
                       void *args) {
  return inst->iso14229.event_callback(&inst->iso14229, event, args, inst);
}

ZTEST_SUITE(lib_uds_new, NULL, uds_new_setup, uds_new_before, NULL, NULL);
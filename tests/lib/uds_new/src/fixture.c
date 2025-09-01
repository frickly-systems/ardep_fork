/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_new.h"
#include "fixture.h"
#include "zephyr/sys/util.h"

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/ztest.h>

#include <iso14229.h>

const uint16_t by_id_data1_default = 5;
uint16_t by_id_data1;
const uint16_t by_id_data1_id = 0x1234;

const uint16_t by_id_data2_default[3] = {0x1234, 0x5678, 0x9ABC};
uint16_t by_id_data2[3];
const uint16_t by_id_data2_id = 0x2468;

__attribute__((unused)) uint16_t by_id_data_no_rw[4] = {0xB1, 0x6B, 0x00, 0xB5};
const uint16_t by_id_data_no_rw_id = 0xBAAD;

const uint16_t by_id_data_unknown_id = 0xDEAD;

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

struct uds_new_instance_t fixture_uds_instance;

const uint16_t by_id_data1_default = 5;
uint16_t by_id_data1;
const uint16_t by_id_data1_id = 0x1234;

UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(
    &fixture_uds_instance, by_id_data1_id, by_id_data1, true, true);

const uint16_t by_id_data2_default[3] = {0x1234, 0x5678, 0x9ABC};
uint16_t by_id_data2[3];
const uint16_t by_id_data2_id = 0x2468;
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(
    &fixture_uds_instance, by_id_data2_id, by_id_data2, true, true);

UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(&fixture_uds_instance,
                                              by_id_data_no_rw_id,
                                              by_id_data_no_rw,
                                              false);

static const UDSISOTpCConfig_t cfg = {
  // Hardware Addresses
  .source_addr = 0x7E8,  // Can ID Server (us)
  .target_addr = 0x7E0,  // Can ID Client (them)

  // Functional Addresses
  .source_addr_func = 0x7DF,             // ID Server (us)
  .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Client (them)
};

static uint8_t copied_data[4096];
static uint32_t copied_len;

void assert_copy_data(const uint8_t *data, uint32_t len) {
  zassert_equal(copied_len, len, "Expected length %u, but got %u", len,
                copied_len);
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
  memset(&fixture_uds_instance, 0, sizeof(fixture_uds_instance));

  static struct lib_uds_new_fixture fixture = {
    .cfg = cfg,
    .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
    .instance = &fixture_uds_instance,
  };

  printk("Using CAN device %s\n", fixture.can_dev->name);

  return &fixture;
}

static void uds_new_before(void *f) {
  struct lib_uds_new_fixture *fixture = f;
  const struct device *dev = fixture->can_dev;
  struct uds_new_instance_t *uds_instance = fixture->instance;

  RESET_FAKE(copy);
  FFF_RESET_HISTORY();

  copy_fake.custom_fake = custom_copy;

  int ret = uds_new_init(uds_instance, &cfg, dev, NULL);
  assert(ret == 0);

  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    if (reg->data_identifier.data_id == by_id_data1_id) {
      memcpy(reg->user_data, &by_id_data1_default, sizeof(by_id_data1_default));
    }

    if (reg->data_identifier.data_id == by_id_data2_id) {
      memcpy(reg->user_data, &by_id_data2_default, sizeof(by_id_data2_default));
    }
  }

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
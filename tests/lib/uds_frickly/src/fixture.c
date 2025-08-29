/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/ztest.h>

#include <ardep/iso14229.h>

DEFINE_FFF_GLOBALS;

static const UDSISOTpCConfig_t cfg = {
  // Hardware Addresses
  .source_addr = 0x7E8,  // Can ID Server (us)
  .target_addr = 0x7E0,  // Can ID Client (them)

  // Functional Addresses
  .source_addr_func = 0x7DF,             // ID Server (us)
  .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Client (them)
};

static void *uds_frickly_setup(void) {
  static struct lib_uds_frickly_fixture fixture = {
    .cfg = cfg,
    .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
  };

  return &fixture;
}

ARDEP_UDS_SERVICE_DEFINE(
    test_service,
    ARDEP_UDS_SERVICE_CAN_DEFINE(DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
                                 CAN_MODE_NORMAL,
                                 cfg));

#ARDEP_UPDS_SERVICE_STATIC_DATA_BY_ID_DEFINE(id)

static void uds_frickly_before(void *f) {
  struct lib_uds_frickly_fixture *fixture = f;
  fixture->service = ardep_uds_service_get_by_id("test_service");
  struct uds_service *service = fixture->service;

  FFF_RESET_HISTORY();

  int ret = ardep_uds_service_init(service, f);
  assert(ret == 0);

  ret = ardep_uds_service_start(service);
  assert(ret == 0);
}

typedef UDSErr_t (*uds_callback)(struct iso14229_zephyr_instance *inst,
                                 UDSEvent_t event,
                                 void *arg,
                                 void *user_context);

UDSErr_t receive_event(struct uds_service *service,
                       UDSEvent_t event,
                       void *args) {
  return service->iso14229.event_callback(&service->iso14229, event, args,
                                          service);
}

ZTEST_SUITE(
    lib_uds_frickly, NULL, uds_frickly_setup, uds_frickly_before, NULL, NULL);
#ifndef APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_
#define APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_

#include "ardep/uds_frickly.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#include <ardep/iso14229.h>
#include <iso14229.h>

struct lib_uds_frickly_fixture {
  struct uds_service service;

  UDSISOTpCConfig_t cfg;
  const struct device *can_dev;
};

/**
 * @brief Receive an event from iso14229
 */
UDSErr_t receive_event(struct uds_service *service,
                       UDSEvent_t event,
                       void *args);

#endif  // APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_

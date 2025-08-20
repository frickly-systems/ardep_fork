#ifndef APP_TESTS_LIB_UDS_MINIMAL_SRC_FIXTURE_H_
#define APP_TESTS_LIB_UDS_MINIMAL_SRC_FIXTURE_H_

#include "iso14229_common.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#include <iso14229/server.h>
#include <iso14229/tp.h>
#include <iso14229/tp/isotp_c.h>
#include <iso14229_common.h>

DECLARE_FAKE_VALUE_FUNC(UDSErr_t,
                        uds_diag_sess_ctrl_callback,
                        struct UDSServer *,
                        const UDSDiagSessCtrlArgs_t *,
                        void *);

DECLARE_FAKE_VALUE_FUNC(UDSErr_t,
                        uds_read_mem_by_addr_callback,
                        struct UDSServer *,
                        const UDSReadMemByAddrArgs_t *,
                        void *);

struct lib_uds_minimal_fixture {
  UDSISOTpCConfig_t cfg;

  struct iso14229_zephyr_instance instance;

  const struct device *can_dev;
};

void send_phys_can_frame(const struct lib_uds_minimal_fixture *fixture,
                         uint8_t *data,
                         uint8_t data_len);

#define send_phys_can_frame_array(fixture, data_array) \
  send_phys_can_frame(fixture, data_array, ARRAY_SIZE(data_array))

void assert_send_phy_can_frame(const struct lib_uds_minimal_fixture *fixture,
                               uint8_t *data,
                               uint8_t data_len);

#define assert_send_phy_can_frame_array(fixture, data_array) \
  assert_send_phy_can_frame(fixture, data_array, ARRAY_SIZE(data_array))

#endif  // APP_TESTS_LIB_UDS_MINIMAL_SRC_FIXTURE_H_
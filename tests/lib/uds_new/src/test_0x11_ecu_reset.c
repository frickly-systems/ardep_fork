#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

ZTEST_F(lib_uds_new, test_0x11_ecu_reset_return_subfunc_not_sup) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSECUResetArgs_t arg = {
    .type = ECU_RESET_HARD,
    .powerDownTimeMillis = 500,
  };

  int ret = receive_event(instance, UDS_EVT_EcuReset, &arg);
  zassert_equal(ret, UDS_NRC_SubFunctionNotSupported);
}

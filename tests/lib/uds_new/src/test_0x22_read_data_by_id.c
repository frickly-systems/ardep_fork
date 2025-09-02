#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

UDSErr_t data_id_check_fn_custom(const struct uds_new_context *const context,
                                 bool *apply_action) {}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_apply_action_when_check_succeeds) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_r,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_ok(ret);
}
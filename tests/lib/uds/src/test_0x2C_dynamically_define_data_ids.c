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

UDSErr_t dynamic_def_data_id_0x2C_check_fn(
    const struct uds_context *const context, bool *apply_action) {
  //   zassert_equal(context->event, UDS_EVT_RoutineCtrl);
  //   zassert_not_null(context->arg);

  //   UDSRoutineCtrlArgs_t *args = context->arg;

  //   zassert_equal(args->ctrlType, UDS_ROUTINE_CONTROL__START_ROUTINE);
  //   zassert_equal(args->id, routine_id);
  //   zassert_equal(args->len, 2);
  //   uint8_t option_record[2] = {0x12, 0x34};
  //   zassert_mem_equal(args->optionRecord, option_record,
  //   sizeof(option_record));

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t dynamic_def_data_id_0x2C__action_fn(struct uds_context *const context,
                                             bool *consume_event) {
  //   UDSRoutineCtrlArgs_t *args = context->arg;

  //   uint8_t routine_status_record[] = {0x11, 0x22};
  //   return args->copyStatusRecord(context->server, routine_status_record,
  //                                 sizeof(routine_status_record));
  *consume_event = true;
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__data_by_id) {
  struct uds_instance_t *instance = fixture->instance;

  UDSDDDI_DBIArgs_t sources[] = {
    {
      .sourceDataId = data_id_r,
      .position = 1,
      .size = 2,
    },
    {

      .sourceDataId = data_id_rw,
      .position = 2,
      .size = 1,
    },
  };

  UDSDDDIArgs_t args = {
    .type = 0x01,  // define by data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineById = {.len = 2, .sources = sources},
  };

  int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  UDSRDBIArgs_t read_arg = {
    .dataId = 0xFEDC,
    .copy = copy,
  };

  ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &read_arg);
  zassert_ok(ret);
}
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

#include <wchar.h>

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

UDSErr_t custom_check_for_0x2C_dynamically_define_data(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    UDSRDBIArgs_t *args = context->arg;
    if ((context->registration->data_identifier.data_id == data_id_r &&
         args->dataId == data_id_r) ||
        (context->registration->data_identifier.data_id == data_id_rw &&
         args->dataId == data_id_rw)) {
      *apply_action = true;
    }
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_0x2C_dynamically_define_data(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    UDSRDBIArgs_t *args = context->arg;
    if (context->registration->data_identifier.data_id == data_id_r) {
      *consume_event = true;
      uint8_t data[] = {0x11, 0x22, 0x33, 0x44};
      args->copy(context->server, data, sizeof(data));
    }
    if (context->registration->data_identifier.data_id == data_id_rw) {
      *consume_event = true;
      uint8_t data[] = {0x55, 0x66, 0x77, 0x88};
      args->copy(context->server, data, sizeof(data));
    }
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__data_by_id) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2C_dynamically_define_data;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2C_dynamically_define_data;

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

  uint8_t expected_data[] = {0x22, 0x33, 0x77};

  assert_copy_data(expected_data, sizeof(expected_data));

  UDSDDDIArgs_t remove_args = {
    .type = 0x03,  // clear dynamic data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
  };

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);
}

ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__data_by_id_multiple) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2C_dynamically_define_data;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2C_dynamically_define_data;

  UDSDDDI_DBIArgs_t sources1[] = {
    {
      .sourceDataId = data_id_r,
      .position = 1,
      .size = 2,
    },
  };

  UDSDDDIArgs_t args = {
    .type = 0x01,  // define by data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineById = {.len = 2, .sources = sources1},
  };

  int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  UDSDDDI_DBIArgs_t sources2[] = {
    {
      .sourceDataId = data_id_rw,
      .position = 2,
      .size = 1,
    },
  };

  args.subFuncArgs.defineById.sources = sources2;
  args.subFuncArgs.defineById.len = 1;

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  UDSRDBIArgs_t read_arg = {
    .dataId = 0xFEDC,
    .copy = copy,
  };

  ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &read_arg);
  zassert_ok(ret);

  uint8_t expected_data[] = {0x22, 0x33, 0x77};

  assert_copy_data(expected_data, sizeof(expected_data));

  UDSDDDIArgs_t remove_args = {
    .type = 0x03,  // clear dynamic data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
  };

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);
}

// ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__data_by_mem) {
//   struct uds_instance_t *instance = fixture->instance;

//   data_id_check_fn_fake.custom_fake =
//       custom_check_for_0x2C_dynamically_define_data;
//   data_id_action_fn_fake.custom_fake =
//       custom_action_for_0x2C_dynamically_define_data;

//   UDSDDDI_DBMArgs_t sources[] = {
//     {
//       .memAddr = (void *)0x1000,
//       .memSize = 3,
//     },
//     {

//       .memAddr = (void *)0x2000,
//       .memSize = 1,
//     },
//   };

//   UDSDDDIArgs_t args = {
//     .type = 0x02,  // define by memory address
//     .allDataIds = false,
//     .dynamicDataId = 0xFEDC,
//     .subFuncArgs.defineByMemAddress = {.len = 2, .sources = sources},
//   };

//   int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
//   zassert_ok(ret);

//   UDSRDBIArgs_t read_arg = {
//     .dataId = 0xFEDC,
//     .copy = copy,
//   };

//   ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &read_arg);
//   zassert_ok(ret);

//   uint8_t expected_data[] = {0x11, 0x22, 0x55};
//   assert_copy_data(expected_data, sizeof(expected_data));
// }
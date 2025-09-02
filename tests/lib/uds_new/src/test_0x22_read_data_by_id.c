#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

ZTEST_F(lib_uds_new, test_0x22_read_by_id_fails_when_no_action_applies) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_r,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 0);
}

//////////////////////7

UDSErr_t custom_check_for_applies_action_when_check_succeeds(
    struct uds_new_context *const context, bool *apply_action) {
  if (context->registration->type ==
          UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == data_id_r &&
      context->event == UDS_EVT_ReadDataByIdent) {
    *apply_action = true;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_applies_action_when_check_succeeds) {
  struct uds_new_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_applies_action_when_check_succeeds;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_r,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

//////////////////////7

UDSErr_t custom_check_for_consume_event_by_default_on_action(
    struct uds_new_context *const context, bool *apply_action) {
  if (context->registration->type ==
          UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_ReadDataByIdent) {
    *apply_action = true;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_consume_event_by_default_on_action) {
  struct uds_new_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_consume_event_by_default_on_action;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_r,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

//////////////////////7

UDSErr_t custom_check_for_both_actions_are_executed(
    struct uds_new_context *const context, bool *apply_action) {
  if (context->registration->type ==
          UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_ReadDataByIdent) {
    *apply_action = true;
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_both_actions_are_executed(
    struct uds_new_context *const context, bool *consume_event) {
  if (context->registration->type ==
          UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_ReadDataByIdent) {
    *consume_event = false;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_both_actions_are_executed) {
  struct uds_new_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_both_actions_are_executed;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_both_actions_are_executed;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_r,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 2);
}

//////////////////////7

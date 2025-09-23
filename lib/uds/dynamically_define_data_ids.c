/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/kernel.h"
#include "zephyr/sys/sflist.h"
#include "zephyr/sys/slist.h"

#include <stdint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

// requires dynamic registration

uint8_t _buf[256];

uds_check_fn uds_get_check_for_dynamically_define_data_ids(
    const struct uds_registration_t* const reg) {
  return reg->dynamically_define_data_ids.actor.check;
}
uds_action_fn uds_get_action_for_dynamically_define_data_ids(
    const struct uds_registration_t* const reg) {
  return reg->dynamically_define_data_ids.actor.action;
}

STRUCT_SECTION_ITERABLE(
    uds_event_handler_data,
    __uds_event_handler_data_dynamically_define_data_ids_) = {
  .event = UDS_EVT_DynamicDefineDataId,
  .get_check = uds_get_check_for_dynamically_define_data_ids,
  .get_action = uds_get_action_for_dynamically_define_data_ids,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS,
};

UDSErr_t uds_check_default_dynamically_define_data_ids(
    const struct uds_context* const context, bool* apply_action) {
  UDSDDDIArgs_t* args = context->arg;
  if (args->type == UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_DATA_ID ||
      args->type ==
          UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_MEMORY_ADDRESS ||
      args->type == UDS_DYNAMICALLY_DEFINED_DATA_IDS__CLEAR) {
    *apply_action = true;
  }
  return UDS_OK;
}

static UDSErr_t uds_dynamic_data_by_id_read_data_by_id_check(
    const struct uds_context* const context, bool* apply_action) {
  // TODO: Do I have to check something here?
  *apply_action = true;
  return UDS_OK;
}

static UDSErr_t uds_dynamic_data_by_id_read_data_by_id_action(
    struct uds_context* const context, bool* consume_event) {
  UDSRDBIArgs_t* args
      // TODO
      * consume_event = true;
  return UDS_OK;
}

UDSErr_t uds_action_default_dynamically_define_data_ids(
    struct uds_context* const context, bool* consume_event) {
  UDSDDDIArgs_t* args = context->arg;
  switch (args->type) {
    case UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_DATA_ID: {
      // TODO: Check that identifier exists
      // TODO: Handle append to existing instead of new list

      sys_slist_t* list = k_malloc(sizeof(sys_slist_t));
      sys_slist_init(list);

      struct uds_registration_t reg = {
        .type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
        .instance = context->instance,
        .data_identifier = {
          .data = list,
          .data_id = args->dynamicDataId,
          .read =
              {
                .check = uds_dynamic_data_by_id_read_data_by_id_check,
                .action = uds_dynamic_data_by_id_read_data_by_id_action,
              },
        }};

      for (uint32_t i = 0; i < args->subFuncArgs.defineById.len; i++) {
        uint16_t id = args->subFuncArgs.defineById.sources[i].sourceDataId;
        uint8_t position = args->subFuncArgs.defineById.sources[i].position;
        uint8_t size = args->subFuncArgs.defineById.sources[i].size;

        struct uds_dynamically_defined_data* data =
            k_malloc(sizeof(struct uds_dynamically_defined_data));
        if (!data) {
          // TODO: undo all previous allocations and return error
          // TODO: free sys_slist_t list
        }
        data->type = UDS_DYNAMICALLY_DEFINED_DATA_TYPE__ID;
        data->id.id = id;
        data->id.position = position;
        data->id.size = size;
        data->node = (sys_snode_t){0};

        sys_slist_append(list, &data->node);
      }

      for (uint32_t i = 0; i < UINT32_MAX; i++) {
        int ret = context->instance->register_event_handler(context->instance,
                                                            reg, i);
        if (ret == 0) {
          break;
        }
      }

      *consume_event = true;
      return UDS_OK;
    };
    case UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_MEMORY_ADDRESS: {
    };
    case UDS_DYNAMICALLY_DEFINED_DATA_IDS__CLEAR: {
      // TODO: Just unregister the dynamically defined ID and use the unregister
      // callback to free all list elements
      // TODO: Just unregister the dynamically defined ID and use the unregister
      // callback to free all list elements
    };
    default:
      return UDS_NRC_SubFunctionNotSupported;
  }
  return UDS_OK;
}
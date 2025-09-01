/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_new.h"
#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "ardep/iso14229.h"
#include "data_by_identifier.h"
#include "zephyr/sys/byteorder.h"

#include <zephyr/kernel.h>

#define DATA_LEN_IN_BYTES(reg) \
  (reg->data_identifier.num_of_elem * reg->data_identifier.len_elem)

static bool uds_new_requirement_is_met(
    uint8_t current,
    uint8_t expected,
    enum uds_new_state_requirement_type level) {
  switch (level) {
    case UDS_NEW_STATE_REQ_EQUAL:
      return current == expected;
    case UDS_NEW_STATE_REQ_LESS_OR_EQUAL:
      return current <= expected;
    case UDS_NEW_STATE_REQ_GREATER_OR_EQUAL:
      return current >= expected;
  }

  LOG_WRN("Invalid level to check requirements: %d", level);
  return false;
}

static UDSErr_t uds_new_check_data_id_state_requirements(
    const struct uds_new_state_requirements* const req,
    const struct uds_new_state* const state) {
  if (!uds_new_requirement_is_met(state->diag_session_type, req->session_type,
                                  req->session_type_req)) {
    LOG_WRN("Requirements not met: Diag Session type");
    return UDS_NRC_ConditionsNotCorrect;
  }

  return UDS_OK;
}

UDSErr_t _uds_new_data_identifier_static_read(
    void* data,
    size_t* len,
    struct uds_new_registration_t* reg,
    const struct uds_new_state* const state) {
  if (*len < DATA_LEN_IN_BYTES(reg)) {
    LOG_WRN("Buffer too small to read Data Identifier 0x%04X",
            reg->data_identifier.data_id);
    return UDS_NRC_ConditionsNotCorrect;
  }

  *len = DATA_LEN_IN_BYTES(reg);
  memcpy(data, reg->user_data, *len);

  if (reg->data_identifier.len_elem == 1) {
    return UDS_OK;
  }

  // Data is send MSB first, so we convert every element of the array to BE
  for (uint32_t i = 0; i < *len; i += reg->data_identifier.len_elem) {
    sys_cpu_to_be(((uint8_t*)data) + i, reg->data_identifier.len_elem);
  }

  return UDS_OK;
}

UDSErr_t _uds_new_data_identifier_static_write(
    const void* data,
    size_t len,
    struct uds_new_registration_t* reg,
    const struct uds_new_state* const state) {
  if (len != DATA_LEN_IN_BYTES(reg)) {
    LOG_WRN("Wrong length to write Data to Identifier 0x%04X",
            reg->data_identifier.data_id);
    return UDS_NRC_ConditionsNotCorrect;
  }

  // Use separate buffer to convert to system byte order to minimize access time
  // to user_data
  uint8_t write_buf[DATA_LEN_IN_BYTES(reg)];
  memcpy(write_buf, data, DATA_LEN_IN_BYTES(reg));

  // Data is send MSB first, so we convert every element of the array back to
  // system byteorder
  if (reg->data_identifier.len_elem > 1) {
    for (uint32_t i = 0; i < DATA_LEN_IN_BYTES(reg);
         i += reg->data_identifier.len_elem) {
      sys_be_to_cpu(write_buf + i, reg->data_identifier.len_elem);
    }
  }

  memmove(reg->user_data, write_buf, DATA_LEN_IN_BYTES(reg));
  return UDS_OK;
}

static UDSErr_t uds_new_try_read_from_identifier(
    struct uds_new_instance_t* instance,
    UDSRDBIArgs_t* args,
    struct uds_new_registration_t* reg) {
  if (reg->instance != instance) {
    return UDS_FAIL;
  }

  if (reg->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      reg->type != UDS_NEW_REGISTRATION_TYPE__CUSTOM) {
    return UDS_FAIL;
  }

  if (reg->data_identifier.data_id != args->dataId) {
    return UDS_FAIL;
  }

  if (reg->type == UDS_NEW_REGISTRATION_TYPE__CUSTOM) {
    uint8_t buf[128];
    size_t len = sizeof(buf);
    UDSErr_t ret =
        reg->custom.read(reg->custom.data_id, reg->custom.state_requirements,
                         instance->state, buf, &len, reg->user_data);
    if (ret != UDS_OK) {
      LOG_WRN("Failed to read custom data identifier 0x%04X",
              reg->custom.data_id);
      return ret;
    }

    ret = args->copy(&instance->iso14229.server, buf, len);
    if (ret != UDS_OK) {
      LOG_WRN("Failed to copy custom data for identifier 0x%04X",
              reg->custom.data_id);
      return UDS_NRC_ConditionsNotCorrect;
    }

    return UDS_OK;
  } else if (reg->type == UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    UDSErr_t ret = uds_new_check_data_id_state_requirements(
        &reg->data_identifier.state_requirements, &instance->state);
    if (ret != UDS_OK) {
      LOG_WRN("State requirements not met to read Data Identifier 0x%04X",
              reg->data_identifier.data_id);
      return ret;
    }

    uint8_t read_buf[DATA_LEN_IN_BYTES(reg)];
    size_t len = sizeof(read_buf);
    reg->data_identifier.read(read_buf, &len, reg, &instance->state);
    return args->copy(&instance->iso14229.server, read_buf, len);
  }

  LOG_WRN("Unhandled read request for Data Identifier 0x%04X", args->dataId);
  return UDS_FAIL;
}

UDSErr_t uds_new_handle_read_data_by_identifier(
    struct uds_new_instance_t* instance, UDSRDBIArgs_t* args) {
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    int ret = uds_new_try_read_from_identifier(instance, args, reg);
    if (ret != UDS_FAIL) {
      return ret;
    }
    if (ret == UDS_OK) {
      return UDS_PositiveResponse;
    }
  }

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  struct uds_new_registration_t* current = instance->dynamic_registrations;
  while (current != NULL) {
    int ret = uds_new_try_read_from_identifier(instance, args, current);
    if (ret == UDS_OK) {
      return UDS_PositiveResponse;
    }
    if (ret != UDS_FAIL) {
      current = current->next;
      return ret;
    }
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  return UDS_NRC_RequestOutOfRange;
}

static UDSErr_t uds_new_try_write_to_identifier(
    struct uds_new_instance_t* instance,
    UDSWDBIArgs_t* args,
    struct uds_new_registration_t* reg) {
  if (reg->instance != instance) {
    return UDS_FAIL;
  }

  if (reg->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      reg->type != UDS_NEW_REGISTRATION_TYPE__CUSTOM) {
    return UDS_FAIL;
  }

  if (reg->data_identifier.data_id != args->dataId) {
    return UDS_FAIL;
  }

  if (reg->type == UDS_NEW_REGISTRATION_TYPE__CUSTOM) {
    return reg->custom.write(reg->custom.data_id,
                             reg->custom.state_requirements, instance->state,
                             args->data, args->len, reg->user_data);
  } else if (reg->type == UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    if (!reg->data_identifier.write) {
      LOG_WRN("Data Identifier 0x%04X cannot be written", args->dataId);
      return UDS_NRC_RequestOutOfRange;
    }

    UDSErr_t ret = uds_new_check_data_id_state_requirements(
        &reg->data_identifier.state_requirements, &instance->state);
    if (ret != UDS_OK) {
      LOG_WRN("State requirements not met to write Data Identifier 0x%04X",
              reg->data_identifier.data_id);
      return ret;
    }

    return reg->data_identifier.write(args->data, args->len, reg,
                                      &instance->state);
  }

  LOG_WRN("Unhandled write request for Data Identifier 0x%04X", args->dataId);
  return UDS_FAIL;
}

UDSErr_t uds_new_handle_write_data_by_identifier(
    struct uds_new_instance_t* instance, UDSWDBIArgs_t* args) {
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    int ret = uds_new_try_write_to_identifier(instance, args, reg);
    if (ret != UDS_FAIL) {
      return ret;
    }
    if (ret == UDS_OK) {
      return UDS_PositiveResponse;
    }
  }

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  struct uds_new_registration_t* current = instance->dynamic_registrations;
  while (current != NULL) {
    int ret = uds_new_try_write_to_identifier(instance, args, current);
    if (ret == UDS_OK) {
      return UDS_PositiveResponse;
    }
    if (ret != UDS_FAIL) {
      current = current->next;
      return ret;
    }
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

  return UDS_NRC_RequestOutOfRange;
}

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
int uds_new_register_runtime_data_identifier(
    struct uds_new_instance_t* inst,
    uint16_t data_id,
    uds_new_data_id_custom_read_fn read,
    uds_new_data_id_custom_write_fn write,
    struct uds_new_state_requirements state_requirements,
    void* user_data) {
  struct uds_new_registration_t* reg =
      k_malloc(sizeof(struct uds_new_registration_t));

  if (!read) {
    LOG_ERR("Read callback must be provided");
    return -EINVAL;
  }

  reg->type = UDS_NEW_REGISTRATION_TYPE__CUSTOM;

  reg->instance = inst;
  reg->user_data = user_data;
  reg->next = NULL;
  reg->custom.data_id = data_id;
  reg->custom.read = read;
  reg->custom.write = write;
  reg->custom.state_requirements = state_requirements;

  // Append reg to the singly linked list at inst->dynamic_registrations
  if (inst->dynamic_registrations == NULL) {
    inst->dynamic_registrations = reg;
  } else {
    struct uds_new_registration_t* current = inst->dynamic_registrations;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = reg;
  }

  return 0;
}

#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
#include "ardep/uds_minimal.h"
#include "read_data_by_identifier.h"
#include "zephyr/sys/byteorder.h"

UDSErr_t _uds_new_data_identifier_static_read(
    void* data, size_t* len, struct uds_new_registration_t* reg) {
  if (*len < reg->data_identifier.len) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;  // todo: better error
  }

  *len = reg->data_identifier.len * reg->data_identifier.len_elem;
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
    const void* data, size_t len, struct uds_new_registration_t* reg) {
  if (len < reg->data_identifier.len) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;  // todo: better error
  }

  memcpy(reg->user_data, data, len);
  return UDS_OK;
}

UDSErr_t handle_data_read_by_identifier(struct uds_new_instance_t* instance,
                                        UDSRDBIArgs_t* args) {
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    if (reg->instance != instance) {
      continue;
    }

    if (reg->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
      continue;
    }
    if (reg->data_identifier.data_id != args->dataId) {
      continue;
    }

    uint8_t read_buf[32];
    size_t len = sizeof(read_buf);
    reg->data_identifier.read(read_buf, &len, reg);
    return args->copy(&instance->iso14229.server, read_buf, len);
  }
  return UDS_NRC_RequestOutOfRange;
}

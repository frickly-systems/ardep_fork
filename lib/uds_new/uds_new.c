#include "uds_new.h"

UDSErr_t _uds_new_data_identifier_static_read(
    void* data, size_t* len, struct uds_new_registration_t* reg) {
  if (*len < reg->data_identifier.len) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;  // todo: better error
  }

  *len = reg->data_identifier.len;
  memcpy(data, reg->user_data, *len);
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

UDSErr_t handle_data_read_by_identifier(UDSServer_t* srv, UDSRDBIArgs_t* args) {
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    // todo: check if instance is correct
    if (reg->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
      continue;
    }
    if (reg->data_identifier.data_id != args->dataId) {
      continue;
    }

    uint8_t read_buf[32];
    size_t len = sizeof(read_buf);
    reg->data_identifier.read(read_buf, &len, reg);
    args->copy(srv, read_buf, len);
    return UDS_OK;
  }
  return UDS_NRC_RequestOutOfRange;
}

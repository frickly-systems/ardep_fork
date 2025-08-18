#include "iso14229_common.h"

#pragma once

struct uds_new_registration_t;  // defined later

struct uds_new_instance_t {
  struct iso14229_zephyr_instance iso14229;

  struct uds_new_registration_t* static_registrations;
  struct uds_new_registration_t* dynamic_registrations;

  struct {
  } callbacks;
};

enum uds_new_registration_type_t {
  UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,
};

struct uds_new_registration_t {
  struct uds_new_instance_t* instance;

  enum uds_new_registration_type_t type;

  void* user_data;

  union {
    struct {
      uint16_t data_id;
      size_t len;
      UDSErr_t (*read)(void* data,
                       size_t* len,
                       struct uds_new_registration_t*
                           reg);  // where data is the output and len
                                  // is the maximum size of the output
                                  // and must be written to be the real
                                  // length; return value is an error
                                  // or UDS_OK
      UDSErr_t (*write)(const void* data,
                        size_t len,
                        struct uds_new_registration_t*
                            reg);  // where data is the new data, len is
                                   // the length of the written data and
                                   // return value is an error or UDS_OK
    } data_identifier;
  };

  struct uds_new_registration_t* next;  // only used for dynamic registrations
};

UDSErr_t _uds_new_data_identifier_static_read(
    void* data, size_t* len, struct uds_new_registration_t* reg);
UDSErr_t _uds_new_data_identifier_static_write(
    const void* data, size_t len, struct uds_new_registration_t* reg);

UDSErr_t handle_data_read_by_identifier(UDSServer_t* srv, UDSRDBIArgs_t* args);

#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(name, _instance, _data_id, \
                                                variable)                  \
  STRUCT_SECTION_ITERABLE(uds_new_registration_t, name) = {                \
    .instance = _instance,                                                 \
    .type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,                    \
    .user_data = &variable,                                                \
    .data_identifier =                                                     \
        {                                                                  \
          .data_id = _data_id,                                             \
          .len = sizeof(variable),                                         \
          .read = _uds_new_data_identifier_static_read,                    \
          .write = _uds_new_data_identifier_static_write,                  \
        },                                                                 \
  };

#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(      \
    name, _instance, _data_id, variable, size)              \
  STRUCT_SECTION_ITERABLE(uds_new_registration_t, name) = { \
    .instance = _instance,                                  \
    .type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,     \
    .user_data = variable,                                  \
    .data_identifier =                                      \
        {                                                   \
          .data_id = _data_id,                              \
          .len = size,                                      \
          .read = _uds_new_data_identifier_static_read,     \
          .write = _uds_new_data_identifier_static_write,   \
        },                                                  \
  };

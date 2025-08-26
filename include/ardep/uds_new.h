#ifndef ARDEP_UDS_NEW_H
#define ARDEP_UDS_NEW_H

// Forward declaration to avoid include dependency issues
#include "ardep/uds_minimal.h"
#include "iso14229/uds.h"

struct uds_new_instance_t {
  struct iso14229_zephyr_instance iso14229;

  struct uds_new_registration_t* static_registrations;
  struct uds_new_registration_t* dynamic_registrations;
};

int uds_new_init(struct uds_new_instance_t* inst,
                 const UDSISOTpCConfig_t* iso_tp_config,
                 const struct device* can_dev,
                 void* user_context);

#ifdef CONFIG_UDS_NEW_ENABLE_RESET

enum ecu_reset_type {
  ECU_RESET_HARD = 1,
  ECU_RESET_KEY_OFF_ON = 2,
  ECU_RESET_ENABLE_RAPID_POWER_SHUT_DOWN = 4,
  ECU_RESET_DISABLE_RAPID_POWER_SHUT_DOWN = 5,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_START = 0x40,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_END = 0x5F,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_START = 0x60,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_END = 0x7E,
};

/**
 * @brief Callback type for ECU reset events
 *
 * @param inst Pointer to the UDS server instance
 * @param reset_type Type of reset to perform
 * @param user_context User-defined context pointer as passed to \ref
 * uds_new_init()
 */
typedef UDSErr_t (*ecu_reset_callback_t)(struct iso14229_zephyr_instance* inst,
                                         enum ecu_reset_type reset_type,
                                         void* user_context);

/**
 * @brief Set a custom callback for ECU reset events
 *
 * The default event handling is disabled when a callback is set
 *
 * @param callback Pointer to the custom callback function
 *
 * @retval 0 on success
 * @retval Negative error code on failure
 *
 */
int set_ecu_reset_callback(ecu_reset_callback_t callback);

/**
 * @brief Schedule a delayed ECU reset after p2 timeout
 *
 * @param server Pointer to the UDS server instance
 * @param reset_type Type of reset to perform
 *
 * @retval UDS_OK on success
 * @retval UDS_NRC on failure
 */
UDSErr_t handle_ecu_reset_event(struct iso14229_zephyr_instance* inst,
                                enum ecu_reset_type reset_type);

#endif  // CONFIG_UDS_NEW_ENABLE_RESET

/**
 * @brief opaque data. used internally
 */
struct uds_new_registration_t;

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

// clang-format off
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

// clang-format on

#endif  // ARDEP_UDS_NEW_H
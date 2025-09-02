#ifndef ARDEP_UDS_NEW_H
#define ARDEP_UDS_NEW_H

// Forward declaration to avoid include dependency issues
#include "ardep/iso14229.h"

#include <iso14229.h>

struct uds_new_instance_t;
struct uds_new_state_requirements;
struct uds_new_state;
struct uds_new_registration_t;

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
typedef UDSErr_t (*ecu_reset_callback_t)(struct uds_new_instance_t* inst,
                                         enum ecu_reset_type reset_type,
                                         void* user_context);

/**
 * Set the ECU reset callback function for custom callbacks
 *
 * @param inst Pointer to the UDS server instance
 * @param callback Pointer to the callback function to set
 * @return 0 on success, negative error code on failure
 */
typedef int (*set_ecu_reset_callback_fn)(struct uds_new_instance_t* inst,
                                         ecu_reset_callback_t callback);

struct uds_new_context {
  struct uds_new_instance_t* const instance;
  struct uds_new_registration_t* const registration;
  UDSEvent_t event;
  void* arg;
  void* additional_param;
};

/**
 * @brief Callback to check whether the associated `uds_new_action_fn`
 * should be executed on this event.
 *
 * @param[in] context The context of this UDS Event
 * @param[out] apply_action set to `true` when an associated action should be
 *                          applied to this event
 * @returns UDS_PositiveResponse on success
 * @returns UDS_NRC_* on failure. This NRC is returned to the UDS client
 */
typedef UDSErr_t (*uds_new_check_fn)(const struct uds_new_context* context,
                                     bool* apply_action);

/**
 * @brief Callback to act on an matching UDS Event
 *
 * When this callback is called, assume that the relevant conditions are met and
 * checked with an associated `uds_new_check_fn` beforehand.
 *
 * @param[in,out] context The context of this UDS Event
 * @param[out] consume_event Set to `false` if the event should not be consumed
 *                           by this action
 * @returns UDS_PositiveResponse on success
 * @returns UDS_NRC_* on failure. This NRC is returned to the UDS client
 */
typedef UDSErr_t (*uds_new_action_fn)(struct uds_new_context* const context,
                                      bool* consume_event);

/**
 * Associates a check with an action
 */
struct uds_new_actor {
  uds_new_check_fn check;
  uds_new_action_fn action;
};

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

/**
 * @brief Function to register a new data identifier at runtime
 *
 * @param inst Pointer to the UDS server instance
 * @param data_id ID of the element to register
 * @param read Custom read function
 * @param write Custom write function
 * @param state_requirements State requirements to read/write the data
 * @param user_data Custom user data passed to the read/write functions
 *
 */
typedef int (*register_data_by_identifier_fn)(struct uds_new_instance_t* inst,
                                              uint16_t data_id,
                                              struct uds_new_actor read,
                                              struct uds_new_actor write);
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

struct uds_new_state {
  uint8_t diag_session_type;
};

struct uds_new_instance_t {
  struct iso14229_zephyr_instance iso14229;
  struct uds_new_registration_t* static_registrations;
  void* user_context;

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  struct uds_new_registration_t* dynamic_registrations;
  register_data_by_identifier_fn register_data_by_identifier;
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
};

int uds_new_init(struct uds_new_instance_t* inst,
                 const UDSISOTpCConfig_t* iso_tp_config,
                 const struct device* can_dev,
                 void* user_context);

enum uds_new_state_requirement_type {
  UDS_NEW_STATE_REQ_EQUAL,
  UDS_NEW_STATE_REQ_LESS_OR_EQUAL,
  UDS_NEW_STATE_REQ_GREATER_OR_EQUAL,
};

struct uds_new_state_requirements {
  enum uds_new_state_requirement_type session_type_req;
  uint8_t session_type;
};

/**
 * @brief opaque data. used internally
 */
struct uds_new_registration_t;

enum uds_new_registration_type_t {
  UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,
  UDS_NEW_REGISTRATION_TYPE__CUSTOM,
  UDS_NEW_REGISTRATION_TYPE__DID,
};

struct uds_new_registration_t {
  struct uds_new_instance_t* instance;

  enum uds_new_registration_type_t type;

  void* user_data;

  union {
    struct {
      uint16_t data_id;
      struct uds_new_actor read;
      struct uds_new_actor write;
    } data_identifier;
  };

  struct uds_new_registration_t* next;  // only used for dynamic registrations
};

// clang-format off
// 
#define UDS_NEW_UNPACK_CONTEXT(context) \
  const struct uds_new_registration_t *registration = context->registration; \
  const struct uds_new_instance_t *instance = context->instance; \
  UDSEvent_t event = context->event; \
  /* Create an 'args' variable with the correct type of the arguments depending on the event */



#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(                      \
  _instance,                                                          \
  _data_id,                                                           \
  data_ptr,                                                           \
  _read_check,                                                        \
  _read,                                                              \
  _write_check,                                                       \
  _write                                                              \
)                                                                     \
  STRUCT_SECTION_ITERABLE(uds_new_registration_t, id##_data_id) = {   \
    .instance = _instance,                                            \
    .type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,               \
    .user_data = data_ptr,                                            \
    .data_identifier = {                                              \
      .data_id = _data_id,                                            \
      .read = {                                                       \
        .check = _read_check,                                         \
        .action = _read,                                              \
      },                                                              \
      .write = {                                                      \
        .check = _write_check,                                        \
        .action = _write,                                             \
      },                                                              \
    },                                                                \
  };

// clang-format on

#endif  // ARDEP_UDS_NEW_H
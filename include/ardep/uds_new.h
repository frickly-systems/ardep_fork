#ifndef ARDEP_UDS_NEW_H
#define ARDEP_UDS_NEW_H

// Forward declaration to avoid include dependency issues
#include "ardep/iso14229.h"

#include <iso14229.h>

struct uds_new_instance_t;
struct uds_new_state_requirements;
struct uds_new_state;

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

/**
 * @brief Callback for custom data read for the Read Data By ID command.
 *
 * @param data_id ID of the element to read
 * @param state_requirements State requirements
 * @param state Current UDS state
 * @param read_buf Buffer containing the data to read. Beware the data should be
 *                 MSB first encoded.
 * @param read_buf_len Holds the length of the read buffer. Set it to the actual
 *                     length of the read data.
 * @param user_data user-data pointer
 */
typedef UDSErr_t (*uds_new_data_id_custom_read_fn)(
    uint16_t data_id,
    const struct uds_new_state_requirements state_requirements,
    const struct uds_new_state state,
    void* read_buf,
    size_t* read_buf_len,
    void* user_data);  // where read_buf is the output and read_buf_len

/**
 * @brief Callback for custom data writes for the Write Data By ID command.
 *
 * @param data_id ID of the element to write
 * @param state_requirements State requirements to perform the write
 * @param state Current UDS state
 * @param write_buf Buffer containing the data to write
 * @param write_buf_len Length of the data in bytes
 * @param user_data user-data pointer
 */
typedef UDSErr_t (*uds_new_data_id_custom_write_fn)(
    uint16_t data_id,
    const struct uds_new_state_requirements state_requirements,
    const struct uds_new_state state,
    const void* const write_buf,
    size_t write_buf_len,
    void* user_data);  // where write_buf is the new data, write_buf_len is

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

typedef int (*register_data_by_identifier_fn)(
    struct uds_new_instance_t* inst,
    uint16_t data_id,
    uds_new_data_id_custom_read_fn read,
    uds_new_data_id_custom_write_fn write,
    struct uds_new_state_requirements state_requirements,
    void* user_data);
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

struct uds_new_state {
  uint8_t diag_session_type;
};

struct uds_new_instance_t {
  struct iso14229_zephyr_instance iso14229;
  struct uds_new_registration_t* static_registrations;
  struct uds_new_state state;
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

#ifdef CONFIG_UDS_NEW_ENABLE_RESET

#endif  // CONFIG_UDS_NEW_ENABLE_RESET

enum uds_new_state_level {
  UDS_NEW_STATE_LEVEL_EQUAL,
  UDS_NEW_STATE_LEVEL_LESS_OR_EQUAL,
  UDS_NEW_STATE_LEVEL_GREATER_OR_EQUAL,
};

struct uds_new_state_requirements {
  enum uds_new_state_level session_type_level;
  uint8_t session_type;
};

/**
 * @brief opaque data. used internally
 */
struct uds_new_registration_t;

enum uds_new_registration_type_t {
  UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,
  UDS_NEW_REGISTRATION_TYPE__CUSTOM,
};

struct uds_new_registration_t {
  struct uds_new_instance_t* instance;

  enum uds_new_registration_type_t type;

  void* user_data;

  union {
    struct {
      uint16_t data_id;
      size_t num_of_elem;
      size_t len_elem;
      struct uds_new_state_requirements state_requirements;
      UDSErr_t (*read)(void* data,
                       size_t* len,
                       struct uds_new_registration_t* reg,
                       const struct uds_new_state* const
                           state);  // where data is the output and len
                                    // is the maximum size of the output
                                    // and must be written to be the real
                                    // length; return value is an error
                                    // or UDS_OK
      UDSErr_t (*write)(const void* data,
                        size_t len,
                        struct uds_new_registration_t* reg,
                        const struct uds_new_state* const
                            state);  // where data is the new data, len is
                                     // the length of the written data and
                                     // return value is an error or UDS_OK
    } data_identifier;

    struct {
      uint16_t data_id;
      struct uds_new_state_requirements state_requirements;
      uds_new_data_id_custom_read_fn read;  // where read_buf is the output and
                                            // read_buf_len is the maximum size
                                            // of the output and must be written
                                            // to be the real length; return
                                            // value is an error or UDS_OK
      uds_new_data_id_custom_write_fn
          write;  // where write_buf is the new data, write_buf_len is
                  // the length of the written data and
                  // return value is an error or UDS_OK
    } custom;
  };

  struct uds_new_registration_t* next;  // only used for dynamic registrations
};

UDSErr_t _uds_new_data_identifier_static_read(
    void* data,
    size_t* len,
    struct uds_new_registration_t* reg,
    const struct uds_new_state* const state);
UDSErr_t _uds_new_data_identifier_static_write(
    const void* data,
    size_t len,
    struct uds_new_registration_t* reg,
    const struct uds_new_state* const state);

// clang-format off

#define UDS_NEW_STATE_REQUIREMENTS_NONE \
  ((struct uds_new_state_requirements) \
    { \
      .session_type_level = UDS_NEW_STATE_LEVEL_GREATER_OR_EQUAL, \
      .session_type = 0 \
    }\
  )

#define UDS_NEW_STATE_DIAG_SESSION_STATE_REQUIREMENTS(level, type) \
  .session_type_level = (level), \
  .session_type = (type)

  
#define UDS_NEW_STATE_DIAG_SESSION_REQUIREMENTS(...) \
  ((struct uds_new_state_requirements) { __VA_ARGS__ })


/**
 * @brief Register a static data identifier with custom read/write capabilities.
 *
 * This macro registers a static data identifier, associating the given id
 * with the provided user_context and read/write functions.
 * 
 * @param _instance           uds_new instance that owns the reference.
 * @param _data_id            Identifier for the data at @p addr.
 * @param user_data           Context passed to in the read/write functions
 * @param _read               Read callback to call when
 *                             <read_data_by_identifier> is invoked.
 *                             Must not be null
 * @param _write              Write callback to call when
                              <write_data_by_identifier> is invoked.
                              Set to NULL to make data read-only.
 * @param _state_requirements Requirements to read/write the data
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_CUSTOM(             \
  _instance,                                                        \
  _data_id,                                                         \
  user_data,                                                        \
  _read,                                                            \
  _write,                                                           \
  _state_requirements)                                              \
  STRUCT_SECTION_ITERABLE(uds_new_registration_t, id##_data_id) = { \
    .instance = _instance,                                          \
    .type = UDS_NEW_REGISTRATION_TYPE__CUSTOM,                      \
    .user_data = user_data,                                         \
    .custom = {                                                     \
      .data_id = _data_id,                                          \
      .state_requirements = _state_requirements,                    \
      .read = _read,                                                \
      .write = _write,                                              \
    },                                                              \
  };
  

/**
 * @brief Register a static data identifier for the data at @p addr.
 *
 * This macro registers a static data identifier, associating a name and id
 * with a memory address so it can be read by the <read_data_by_identifier> 
 * command.
 * 
 * To not convert the data to big endian before sending, pass 1 as len_elem and
 * the size of the data in bytes to len.
 *
 * @param _instance           uds_new instance that owns the reference.
 * @param _data_id            Identifier for the data at @p addr.
 * @param addr                Memory address where the data is found.
 * @param _num_of_elem        number of elements at @p addr.
 * @param len_elem            Length of each element in bytes ad @p addr.
 *                            These amount of
 * @param _read               Read callback to call when
*                             <read_data_by_identifier> is invoked.
                              Must not be null
 * @param _write              Write callback to call when
                              <write_data_by_identifier> is invoked.
                              Set to NULL to make data read-only.
 * @param _state_requirements Requirements to read/write the data
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM(                \
  _instance,                                                        \
  _data_id,                                                         \
  addr,                                                             \
  _num_of_elem,                                                     \
  _len_elem,                                                        \
  writable,                                                \
  _state_requirements)                                              \
  STRUCT_SECTION_ITERABLE(uds_new_registration_t, id##_data_id) = { \
    .instance = _instance,                                          \
    .type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,             \
    .user_data = addr,                                              \
    .data_identifier = {                                            \
      .data_id = _data_id,                                          \
      .num_of_elem = _num_of_elem,                                  \
      .len_elem = _len_elem,                                        \
      .state_requirements = _state_requirements,                    \
      .read = _uds_new_data_identifier_static_read,                 \
      .write =                                                      \
        writable ? _uds_new_data_identifier_static_write : NULL,    \
    },                                                              \
  };

/**
 * @brief Register a static data identifier for a primitive data type.
 *
 * This macro registers a static data identifier, associating a name and id
 * with a variable so it can be read by the <read_data_by_identifier> command.
 *
 * For other parameters see <UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM>
 *
 * @param variable  Variable to associate with the data identifier.
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(           \
  _instance,                                               \
  _data_id,                                                \
  variable,                                                \
  writable,                                                \
  _state_requirements                                      \
)                                                          \
  UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM(             \
  _instance,                                               \
  _data_id,                                                \
  &variable,                                               \
  1,                                                       \
  sizeof(variable),                                        \
  writable,                                                \
  _state_requirements                                      \
  )


/**
 * @brief Register a static data identifier for an array of data.
 *
 * This macro registers a static data identifier, associating a name and id
 * with an array of data so it can be read by the <read_data_by_identifier>
 * command.
 * 
 * Every element of the array is converted to Big Endian format before transmit
 *
 * For other parameters see <UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM>
 *
 * @param array     Array to associate with the data identifier.
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(     \
  _instance, _data_id, array,                              \
  writable,                                                \
  _state_requirements                                      \
)                                                          \
  UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM(             \
    _instance,                                             \
    _data_id,                                              \
    &array[0],                                             \
    ARRAY_SIZE(array),                                     \
    sizeof(array[0]),                                      \
    writable,                                              \
  _state_requirements                                      \
  )
// clang-format on

#endif  // ARDEP_UDS_NEW_H
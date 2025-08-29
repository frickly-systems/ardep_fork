#ifndef APP_ARDEP_UDS_FRICKLY_H
#define APP_ARDEP_UDS_FRICKLY_H

#include <ardep/iso14229.h>
#include <iso14229.h>

struct uds_service_state;

typedef UDSErr_t (*uds_service_ecu_reset_callback_t)(
    const struct uds_service_state *const state,
    const UDSECUResetArgs_t *args,
    void *user_context);

struct uds_service_custom_callbacks {
  uds_service_ecu_reset_callback_t ecu_reset;
};

enum uds_service_security_level_check_type {
  UDS_SERVICE_SECURITY_LEVEL_CHECK_TYPE_EQUAL,
  UDS_SERVICE_SECURITY_LEVEL_CHECK_TYPE_AT_LEAST,
  UDS_SERVICE_SECURITY_LEVEL_CHECK_TYPE_AT_MOST,
};

struct uds_service_can {
  const struct device *can_bus;
  UDSISOTpCConfig_t config;
  uint8_t mode;
};

typedef UDSErr_t (*uds_service_read_data_by_id_callback_t)(uint16_t data_id,
                                                           uint8_t *data,
                                                           uint16_t *data_len);
typedef UDSErr_t (*uds_service_write_data_by_id_callback_t)(uint16_t data_id,
                                                            const uint8_t *data,
                                                            uint16_t data_len);

struct uds_server_service_requirements {
  bool authentication;
  uint8_t security_level;
  enum uds_service_security_level_check_type security_level_check;
};

struct uds_service_data_by_id {
  uint16_t identifier;
  uds_service_read_data_by_id_callback_t read;
  uds_service_write_data_by_id_callback_t write;
  struct k_mutex *write_lock;
  struct uds_server_service_requirements req;
};

struct uds_service_state {
  uint8_t session_type;
  uint8_t security_access_level;
  bool authenticated;
  bool ecu_reset_scheduled;
};

struct uds_service {
  struct uds_service_can can;
  struct uds_service_state state;
  struct k_mutex event_lock;
  struct uds_service_custom_callbacks callbacks;

  struct iso14229_zephyr_instance iso14229;

  void *user_context;

  struct uds_service_data_by_id ids[];
};

#define ARDEP_UDS_SERVICE_DATA_BY_ID_DEFINE(id, _read, _write, ...)            \
  {                                                                            \
    .identifier = id, .read = _read, .write = _write,                          \
    .write_lock = COND_CODE_1(IS_EMPTY(__VA_ARGS__), (NULL),                   \
                              (GET_ARG_N(1, __VA_ARGS__))),                    \
    .req = COND_CODE_1(IS_EQ(NUM_VA_ARGS_LESS_1(__VA_ARGS__), 0),              \
                       ({.authentication = false,                              \
                         .security_level = 0,                                  \
                         .security_level_check =                               \
                             UDS_SERVICE_SECURITY_LEVEL_CHECK_TYPE_AT_LEAST}), \
                       (GET_ARGS_LESS_N(1, __VA_ARGS__)))                      \
  }

#define ARDEP_UDS_SERVICE_DATA_BY_ID_SERVICES_DEFINE(...) \
  COND_CODE_1(IS_EMPTY(__VA_ARGS__), (), ({__VA_ARGS__}))

#define ARDEP_UDS_SERVICE_CAN_DEFINE(can_bus_node, _mode, _config) \
  { .can_bus = can_bus_node, .mode = _mode, .config = _config }

#define ARDEP_UDS_SERVICE_DEFINE(can_bus, id_list) \
  (struct uds_service) {                           \
    .can = can_bus, .ids = id_list,                \
    .state = {                                     \
      .session_type = 0x00,                        \
      .security_access_level = 0x00,               \
      .authenticated = false,                      \
    },                                             \
  }

int ardep_uds_service_init(struct uds_service *service, void *user_context);

int ardep_uds_service_start(struct uds_service *service);

#endif  // APP_ARDEP_UDS_FRICKLY_H
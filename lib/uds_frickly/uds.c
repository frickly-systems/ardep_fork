/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/kernel.h"

#include <iso14229.h>

enum uds_service_security_level_check_type {
  UDS_SERVICE_SECURITY_LEVEL_CHECK_TYPE_EQUAL,
  UDS_SERVICE_SECURITY_LEVEL_CHECK_TYPE_AT_LEAST,
  UDS_SERVICE_SECURITY_LEVEL_CHECK_TYPE_AT_MOST,
};

struct uds_service_can {
  struct device* can_bus;
  struct UDSISOTpCConfig_t config;
};

typedef UDSErr_t (*uds_service_read_data_by_id_callback_t)();
typedef UDSErr_t (*uds_service_write_data_by_id_callback_t)();

struct uds_server_service_requirements {
  bool authentication;
  uint8_t security_level;
  enum uds_service_security_level_check_type security_level_check;
};

struct uds_service_data_by_id {
  uint16_t identifier;
  uds_service_read_data_by_id_callback_t read;
  uds_service_write_data_by_id_callback_t write;
  struct uds_server_service_requirements req;
};

struct uds_service_state {
  uint8_t session_id;
  uint8_t security_access_level;
  bool authenticated;
};

struct uds_service {
  uds_service_can can;
  uds_service_data_by_id ids[];
  uds_service_state state;

  struct k_mutex lock;
};

// instance name
// struct device* CAN bus
// UDS Can Configuration

#define ARDEP_UDS_SERVICE_DATA_BY_ID_DEFINE(id, _read, _write, ...)        \
  {                                                                        \
    .identifier = id, .read = read, .write = write, .req = { __VA_ARGS__ } \
  }
// uint16_t identifier
// READ Callback
// Write Callback (can be null)

#define ARDEP_UDS_SERVICE_DATA_BY_ID_SERVICES_DEFINE(...) [__VA_ARGS__]

// List of ARDEP_UDS_SERVICE_DATA_BY_ID_DEFINE

#define ARDEP_UDS_SERVICE_CAN_DEFINE(can_bus_node, phys_source_addr,     \
                                     phys_target_addr, func_source_addr, \
                                     func_target_addr)                   \
  {                                                                      \
    .can_bus = can_bus_node, .config = {                                 \
      .source_addr = phys_source_addr,                                   \
      .target_addr = phys_target_addr,                                   \
      .source_addr_func = func_source_addr,                              \
      .target_addr_func = func_target_addr,                              \
    }                                                                    \
  }

#define ARDEP_UDS_SERVICE_DEFINE(instance_name, can_bus, id_list) \
  struct uds_service instance_name = {                            \
    .can = can_bus,                                               \
    .ids = id_list,                                               \
    .state =                                                      \
        {                                                         \
          .session_id = 0x00,                                     \
          .security_access_level = 0x00,                          \
          .authenticated = false,                                 \
        },                                                        \
  }
/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds_frickly, CONFIG_UDS_FRICKLY_LOG_LEVEL);

#include "zephyr/kernel.h"

#include <zephyr/sys/util_macro.h>

#include <ardep/iso14229.h>
#include <ardep/uds_frickly.h>
#include <iso14229.h>

static UDSErr_t _handle_diag_sess_ctrl_event(struct uds_service *service,
                                             UDSDiagSessCtrlArgs_t *args) {
  k_mutex_lock(&service->event_lock, K_FOREVER);
  service->state.session_type = args->type;
  k_mutex_unlock(&service->event_lock);
  return UDS_OK;
}

UDSErr_t uds_service_ecu_reset_callback_default(
    const struct uds_service_state *const state,
    const UDSECUResetArgs_t *args,
    void *user_context) {
  return UDS_FAIL;
}

static UDSErr_t _uds_callback(struct iso14229_zephyr_instance *inst,
                              UDSEvent_t event,
                              void *arg,
                              void *user_context) {
  struct uds_service *service = user_context;

  switch (event) {
    case UDS_EVT_Err:
    case UDS_EVT_DiagSessCtrl:
      return _handle_diag_sess_ctrl_event(service,
                                          (UDSDiagSessCtrlArgs_t *)arg);

    case UDS_EVT_EcuReset:
    case UDS_EVT_ReadMemByAddr:
    case UDS_EVT_CommCtrl:
    case UDS_EVT_SecAccessRequestSeed:
    case UDS_EVT_SecAccessValidateKey:
    case UDS_EVT_WriteDataByIdent:
    case UDS_EVT_RoutineCtrl:
    case UDS_EVT_RequestDownload:
    case UDS_EVT_RequestUpload:
    case UDS_EVT_TransferData:
    case UDS_EVT_RequestTransferExit:
    case UDS_EVT_SessionTimeout:
    case UDS_EVT_DoScheduledReset:
    case UDS_EVT_RequestFileTransfer:
    case UDS_EVT_Custom:
    case UDS_EVT_Poll:
    case UDS_EVT_SendComplete:
    case UDS_EVT_ResponseReceived:
    case UDS_EVT_Idle:
    case UDS_EVT_MAX:
      break;
    case UDS_EVT_ReadDataByIdent:
      break;
  }

  return UDS_FAIL;
}

int ardep_uds_service_init(struct uds_service *service, void *user_context) {
  if (!device_is_ready(service->can.can_bus)) {
    LOG_ERR("CAN device not ready: %s", service->can.can_bus->name);
    return -ENODEV;
  }

  service->user_context = user_context;

  int ret = can_set_mode(service->can.can_bus, service->can.mode);
  if (ret < 0) {
    LOG_ERR("Failed to set CAN mode: %d", ret);
    return ret;
  }

  ret = iso14229_zephyr_init(&service->iso14229, &service->can.config,
                             service->can.can_bus, service);
  if (ret < 0) {
    LOG_ERR("Failed to initialize ISO14229 instance: %d", ret);
    return ret;
  }

  ret = service->iso14229.set_callback(&service->iso14229, _uds_callback);
  if (ret < 0) {
    LOG_ERR("Failed to set event callback on iso14229 instance: %d", ret);
    return ret;
  }

  ret = k_mutex_init(&service->event_lock);
  if (ret != 0) {
    LOG_ERR("Failed to initialize event lock mutex: %d", ret);
    return -ENOLCK;
  }

  return 0;
}

int ardep_uds_service_start(struct uds_service *service) {
  int ret = can_start(service->can.can_bus);
  if (ret) {
    LOG_ERR("Failed to start CAN bus: %d", ret);
  }
  return ret;
}

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
  COND_CODE_1(IS_EMPTY(__VA_ARGS__), ({}), ({__VA_ARGS__}))

#define ARDEP_UDS_SERVICE_CAN_DEFINE(can_bus_node, _mode, phys_source_addr, \
                                     phys_target_addr, func_source_addr,    \
                                     func_target_addr)                      \
  {                                                                         \
    .can_bus = can_bus_node, .mode = _mode, .config = {                     \
      .source_addr = phys_source_addr,                                      \
      .target_addr = phys_target_addr,                                      \
      .source_addr_func = func_source_addr,                                 \
      .target_addr_func = func_target_addr,                                 \
    }                                                                       \
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
    .lock = Z_MUTEX_INITIALIZER(instance_name.lock),              \
  }

/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds_frickly, CONFIG_UDS_FRICKLY_LOG_LEVEL);

#include <zephyr/kernel.h>
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

struct uds_service *ardep_uds_service_get_by_id(char *id) {
  STRUCT_SECTION_FOREACH (uds_service, service) {
    if (id == service->identifier || strcmp(service->identifier, id) == 0) {
      return service;
    }
  }
  return NULL;
}
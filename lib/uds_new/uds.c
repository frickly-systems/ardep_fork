#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "iso14229/uds.h"

#include <ardep/uds_minimal.h>
#include <ardep/uds_new.h>
#include <iso14229/server.h>

UDSErr_t uds_event_callback(struct iso14229_zephyr_instance* inst,
                            UDSEvent_t event,
                            void* arg,
                            void* user_context) {
  //   UDSServer_t* server = &inst->server;

  switch (event) {
    case UDS_EVT_Err:
    case UDS_EVT_DiagSessCtrl:
    case UDS_EVT_EcuReset: {
#ifdef CONFIG_UDS_NEW_ENABLE_RESET
      UDSECUResetArgs_t* args = arg;

      return handle_ecu_reset_event(inst, (enum ecu_reset_type)args->type);
#else
      return UDS_NRC_ServiceNotSupported;
#endif
    }
    case UDS_EVT_ReadDataByIdent:
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
  }

  return UDS_OK;
}

int uds_new_init(struct iso14229_zephyr_instance* inst,
                 const UDSISOTpCConfig_t* iso_tp_config,
                 const struct device* can_dev,
                 void* user_context) {
  int ret = iso14229_zephyr_init(inst, iso_tp_config, can_dev, user_context);
  if (ret < 0) {
    LOG_ERR("Failed to initialize UDS instance");
    return ret;
  }

  ret = iso14229_zephyr_set_callback(inst, uds_event_callback);
  if (ret < 0) {
    LOG_ERR("Failed to set UDS event callback");
    return ret;
  }

  return 0;
}
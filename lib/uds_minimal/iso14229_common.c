// This would be dedicated into a separate library
#include "iso14229_common.h"
#include "uds_new.h"
#include "write_memory_by_addr_impl.h"
#include "zephyr/sys/byteorder.h"

#include <zephyr/logging/log.h>

#include <iso14229/server.h>
#include <iso14229/util.h>

LOG_MODULE_REGISTER(iso14229_common, LOG_LEVEL_DBG);

UDSErr_t uds_cb(struct UDSServer *srv, UDSEvent_t event, void *arg) {
  LOG_DBG("UDS Event: %s", UDSEventToStr(event));
  struct iso14229_zephyr_instance *inst =
      (struct iso14229_zephyr_instance *)srv->fn_data;

  switch (event) {
    case UDS_EVT_Err: {
      UDSErr_t *err = (UDSErr_t *)arg;
      LOG_ERR("UDS Error: %d", *err);
      break;
    }
    case UDS_EVT_DiagSessCtrl: {
      UDSDiagSessCtrlArgs_t *session_args = (UDSDiagSessCtrlArgs_t *)arg;
      LOG_INF("Diagnostic Session Control: %d", session_args->type);

      if (inst->event_callbacks.uds_uds_diag_sess_ctrl_fn) {
        return inst->event_callbacks.uds_uds_diag_sess_ctrl_fn(
            srv, session_args, inst->user_context);
      }
      break;
    }
    case UDS_EVT_EcuReset: {
      uint8_t *reset_type = (uint8_t *)arg;
      LOG_INF("ECU Reset: %d", *reset_type);
      break;
    }
    case UDS_EVT_SessionTimeout: {
      LOG_WRN("Session Timeout");
      srv->sessionType = UDS_LEV_DS_DS;  // reset to default session
      break;
    }
    case UDS_EVT_RoutineCtrl: {
      UDSRoutineCtrlArgs_t *routine = (UDSRoutineCtrlArgs_t *)arg;
      LOG_INF("Routine Control: %d %d", routine->id, routine->ctrlType);
      // as per the standard, basically any data can be returned here
      uint8_t data = 1;
      routine->copyStatusRecord(srv, &data, 1);
      break;
    }
    case UDS_EVT_ReadDataByIdent: {
      UDSRDBIArgs_t *read_args = (UDSRDBIArgs_t *)arg;
      return handle_data_read_by_identifier(srv, read_args);
    }
    case UDS_EVT_RequestDownload: {
      UDSRequestDownloadArgs_t *req = (UDSRequestDownloadArgs_t *)arg;
      LOG_INF("Request Download: addr=%p size=%zu format=%d", req->addr,
              req->size, req->dataFormatIdentifier);
      break;
    }
    case UDS_EVT_TransferData: {  //! note: very import: the first block number
                                  //! must be 1 with this library
      UDSTransferDataArgs_t *transfer_args = (UDSTransferDataArgs_t *)arg;
      LOG_HEXDUMP_INF(transfer_args->data, transfer_args->len, "Transfer Data");
      LOG_INF("Transfer Data: len=%d", transfer_args->len);
      break;
    }
    case UDS_EVT_RequestTransferExit: {
      UDSRequestTransferExitArgs_t *exit_args =
          (UDSRequestTransferExitArgs_t *)arg;
      LOG_INF("Request Transfer Exit: len=%d", exit_args->len);
      break;
    }
    case UDS_EVT_ReadMemByAddr: {
      UDSReadMemByAddrArgs_t *read_args = (UDSReadMemByAddrArgs_t *)arg;
      LOG_INF("Read Memory By Address: addr=%p size=%zu", read_args->memAddr,
              read_args->memSize);

      return inst->event_callbacks.uds_read_mem_by_addr_fn(srv, read_args,
                                                           inst->user_context);
    }
    default:
      UDSCustomArgs_t *custom_args = (UDSCustomArgs_t *)arg;

      if (custom_args->sid == CUSTOMUDS_WriteMemoryByAddr_SID) {
        struct CUSTOMUDS_WriteMemoryByAddr args;
        UDSErr_t e = customuds_decode_write_memory_by_addr(custom_args, &args);
        if (e != UDS_PositiveResponse) {
          return e;
        }

        // TODO TIM: cb
        // This executes the actual write to the memory
        // memcpy(&dummy_memory[args.addr], args.data, args.len);

        LOG_HEXDUMP_INF(args.data, args.len, "Write Memory By Address");
        LOG_INF("Write Memory By Address: addr=0x%08X len=%zu", args.addr,
                args.len);

        return customuds_answer(srv, custom_args, &args);
      }

      return UDS_NRC_ServiceNotSupported;
  }

  return UDS_OK;
}

static void can_rx_cb(const struct device *dev,
                      struct can_frame *frame,
                      void *user_data) {
  LOG_DBG("CAN RX: %03x [%u] %x ...", frame->id, frame->dlc, frame->data[0]);
  k_msgq_put((struct k_msgq *)user_data, frame, K_NO_WAIT);
}

int iso14229_zephyr_init(struct iso14229_zephyr_instance *inst,
                         const UDSISOTpCConfig_t *iso_tp_config,
                         const struct device *can_dev,
                         struct uds_callbacks callbacks,
                         void *user_context) {
  inst->event_callbacks = callbacks;
  inst->user_context = user_context;

  k_msgq_init(&inst->can_phys_msgq, inst->can_phys_buffer,
              sizeof(struct can_frame),
              ARRAY_SIZE(inst->can_phys_buffer) / sizeof(struct can_frame));

  k_msgq_init(&inst->can_func_msgq, inst->can_func_buffer,
              sizeof(struct can_frame),
              ARRAY_SIZE(inst->can_func_buffer) / sizeof(struct can_frame));

  UDSServerInit(&inst->server);
  UDSISOTpCInit(&inst->tp, iso_tp_config);

  inst->server.fn = uds_cb;
  inst->server.fn_data = inst;
  inst->server.tp = &inst->tp.hdl;
  inst->tp.phys_link.user_send_can_arg = (void *)can_dev;
  inst->tp.func_link.user_send_can_arg = (void *)can_dev;

  // Von CAN Nachrichten
  const struct can_filter phys_filter = {
    .id = inst->tp.phys_sa,
    .mask = CAN_STD_ID_MASK,
  };

  // KP woher das kommt?!
  const struct can_filter func_filter = {
    .id = inst->tp.func_sa,
    .mask = CAN_STD_ID_MASK,
  };

  int err =
      can_add_rx_filter(can_dev, can_rx_cb, &inst->can_phys_msgq, &phys_filter);
  if (err < 0) {
    printk("Failed to add RX filter for physical address: %d\n", err);
    return err;
  }
  err =
      can_add_rx_filter(can_dev, can_rx_cb, &inst->can_func_msgq, &func_filter);
  if (err < 0) {
    printk("Failed to add RX filter for functional address: %d\n", err);
    return err;
  }

  return 0;
}

void iso14229_zephyr_thread_tick(struct iso14229_zephyr_instance *inst) {
  struct can_frame frame_phys;
  struct can_frame frame_func;
  int ret_phys = k_msgq_get(&inst->can_phys_msgq, &frame_phys, K_NO_WAIT);
  int ret_func = k_msgq_get(&inst->can_func_msgq, &frame_func, K_NO_WAIT);

  if (ret_phys == 0) {
    isotp_on_can_message(&inst->tp.phys_link, frame_phys.data, frame_phys.dlc);
  }

  if (ret_func == 0) {
    isotp_on_can_message(&inst->tp.func_link, frame_func.data, frame_func.dlc);
  }

  UDSServerPoll(&inst->server);
}

void iso14229_zephyr_thread(struct iso14229_zephyr_instance *inst) {
  while (1) {
    iso14229_zephyr_thread_tick(inst);
    k_sleep(K_MSEC(1));
  }
}
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>
#include <iso14229.h>
#include "uds.h"

static const struct device *const flash_controller =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

enum UploadDownloadState {
  UDS_UPDOWN_IDLE,
  UDS_UPDOWN_DOWNLOAD_IN_PROGRESS,
  UDS_UPDOWN_UPLOAD_IN_PROGRESS,
};

typedef struct {
  enum UploadDownloadState state;
  uintptr_t start_address;
  uintptr_t current_address;
  size_t total_size;
} upload_download_state_t;

upload_download_state_t upload_download_state = {
    .state = UDS_UPDOWN_IDLE,
    
    .start_address = 0,
    .current_address = 0,
    .total_size = 0,
};

static UDSErr_t start_download(
    const struct uds_context* const context) {
  if (upload_download_state.state != UDS_UPDOWN_IDLE) {
    return UDS_NRC_RequestSequenceError;
  }

  if (flash_controller == NULL ||
      !device_is_ready(flash_controller)) {
    return UDS_NRC_UploadDownloadNotAccepted;
  }

  UDSRequestDownloadArgs_t* args =
      (UDSRequestDownloadArgs_t*)context->arg;

  // do not check addr == 0, as this might be correct address for the flash
  if (args->size == 0) {
    return UDS_NRC_RequestOutOfRange;
  }

  upload_download_state.start_address = (uintptr_t)args->addr;
  upload_download_state.current_address = (uintptr_t)args->addr;
  upload_download_state.total_size = args->size;

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
  // prepare flash by erasing necessary sectors
  flash_erase(flash_controller, upload_download_state.start_address, upload_download_state.total_size);
#endif

  upload_download_state.state = UDS_UPDOWN_DOWNLOAD_IN_PROGRESS;

  return UDS_OK;
}

static UDSErr_t start_upload(
    const struct uds_context* const context) {
  if (upload_download_state.state != UDS_UPDOWN_IDLE) {
    return UDS_NRC_RequestSequenceError;
  }

  if (flash_controller == NULL ||
      !device_is_ready(flash_controller)) {
    return UDS_NRC_UploadDownloadNotAccepted;
  }

  UDSRequestUploadArgs_t* args =
      (UDSRequestUploadArgs_t*)context->arg;

  // do not check addr == 0, as this might be correct address for the flash
  if (args->size == 0) {
    return UDS_NRC_RequestOutOfRange;
  }

  upload_download_state.start_address = (uintptr_t)args->addr;
  upload_download_state.current_address = (uintptr_t)args->addr;
  upload_download_state.total_size = args->size;

  // TODO: verify that the address range is valid for reading

  upload_download_state.state = UDS_UPDOWN_UPLOAD_IN_PROGRESS;

  return UDS_OK;
}

static UDSErr_t continue_download(
    const struct uds_context* const context) {
  if (upload_download_state.state != UDS_UPDOWN_DOWNLOAD_IN_PROGRESS) {
    return UDS_NRC_ConditionsNotCorrect;
  }

  UDSTransferDataArgs_t* args =
      (UDSTransferDataArgs_t*)context->arg;

  if (args->len == 0 ||
      upload_download_state.current_address + args->len >
          upload_download_state.start_address + upload_download_state.total_size) {
    return UDS_NRC_RequestOutOfRange;
  }

  int rc = flash_write(flash_controller,
      upload_download_state.current_address,
      args->data,
      args->len);
  if (rc != 0) {
    return UDS_NRC_GeneralProgrammingFailure;
  }

  upload_download_state.current_address += args->len;

  return UDS_OK;
}

static UDSErr_t continue_upload(
    const struct uds_context* const context) {
  if (upload_download_state.state != UDS_UPDOWN_UPLOAD_IN_PROGRESS) {
    return UDS_NRC_RequestSequenceError;
  }

  if (upload_download_state.current_address >=
      upload_download_state.start_address + upload_download_state.total_size) {
    return UDS_NRC_RequestSequenceError;
  }

  UDSTransferDataArgs_t* args =
      (UDSTransferDataArgs_t*)context->arg;

  size_t len_to_copy = MIN(args->maxRespLen,
      upload_download_state.start_address + upload_download_state.total_size -
          upload_download_state.current_address);
  
  int ret = flash_read(flash_controller,
      upload_download_state.current_address,
      (void*)args->data,
      len_to_copy);

  if (ret != 0) {
    return UDS_NRC_GeneralProgrammingFailure;
  }

  if (args->copyResponse == NULL) {
    return UDS_ERR_MISUSE;
  }

  args->copyResponse(&context->instance->iso14229.server,
      args->data, len_to_copy);

  upload_download_state.current_address += len_to_copy;

  return UDS_OK;
}

static UDSErr_t transferExit(
    const struct uds_context* const context) {
  if (upload_download_state.state != UDS_UPDOWN_DOWNLOAD_IN_PROGRESS &&
      upload_download_state.state != UDS_UPDOWN_UPLOAD_IN_PROGRESS) {
    return UDS_NRC_ConditionsNotCorrect;
  }

  // The exit request and response contains some optional user specific parameters
  // which we currently ignore
  // TODO: we could add a checksum verification here

  upload_download_state.state = UDS_UPDOWN_IDLE;
  upload_download_state.start_address = 0;
  upload_download_state.current_address = 0;
  upload_download_state.total_size = 0;

  return UDS_OK;
}

static UDSErr_t uds_action_upload_download(
  struct uds_context* context, bool* consume_event) {
  // there currently should only be one handler for upload/download
  *consume_event = true;

  switch (context->event)
  {
  case UDS_EVT_RequestDownload:
    return start_download(context);
    break;

  case UDS_EVT_RequestUpload:
    return start_upload(context);
    break;

  case UDS_EVT_TransferData:
    if (upload_download_state.state == UDS_UPDOWN_DOWNLOAD_IN_PROGRESS) {
      return continue_download(context);
    } else if (upload_download_state.state == UDS_UPDOWN_UPLOAD_IN_PROGRESS)
    {
      return continue_upload(context);
    } else {
      return UDS_NRC_ConditionsNotCorrect;
    }
    break;

  case UDS_EVT_RequestTransferExit:
    return transferExit(context);

  default:
    return UDS_ERR_MISUSE;
    break;
  }


  return UDS_OK;
}

static UDSErr_t uds_check_always_apply(
    const struct uds_context* const context, bool* apply_action) {
  *apply_action = true;
  return UDS_OK;
}

static uds_check_fn get_check(
    const struct uds_registration_t* const reg) {
  return uds_check_always_apply;
}

static uds_action_fn get_action(
    const struct uds_registration_t* const reg) {
  return uds_action_upload_download;
}

// Handlers for all request event types
// - UDS_EVT_RequestUpload
// - UDS_EVT_RequestDownload
// - UDS_EVT_TransferData
// - UDS_EVT_RequestTransferExit
// - UDS_EVT_RequestFileTransfer

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_upload_download_request_download_) = {
  .event = UDS_EVT_RequestDownload,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
            __uds_event_handler_data_upload_download_request_upload_) = {
  .event = UDS_EVT_RequestUpload,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
            __uds_event_handler_data_upload_download_transfer_data_) = {
  .event = UDS_EVT_TransferData,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
            __uds_event_handler_data_upload_download_request_transfer_exit_) = {
  .event = UDS_EVT_RequestTransferExit,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
            __uds_event_handler_data_upload_download_request_file_transfer_) = {
  .event = UDS_EVT_RequestFileTransfer,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

// Dummy registration to get the upload/download handler registered
STRUCT_SECTION_ITERABLE(uds_registration_t,
        __uds_registration_dummy_upload_download_) = {
  .instance = NULL,
  .type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

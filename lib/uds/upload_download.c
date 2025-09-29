#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/fs/fs.h>

#include <ardep/uds.h>
#include <iso14229.h>
#include "uds.h"

#include <errno.h>
#include <string.h>

static const struct device *const flash_controller =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));
#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash_controller))

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

enum FileTransferMode {
  UDS_FILE_TRANSFER_IDLE,
  UDS_FILE_TRANSFER_WRITE,
  UDS_FILE_TRANSFER_READ,
};

struct file_transfer_state_t {
  enum FileTransferMode mode;
  struct fs_file_t file;
  bool file_open;
  size_t expected_size;
  size_t transferred;
  uint16_t block_length;
} file_transfer_state = {
  .mode = UDS_FILE_TRANSFER_IDLE,
  .file_open = false,
  .expected_size = 0,
  .transferred = 0,
  .block_length = 0,
};

#define UDS_FILE_TRANSFER_MAX_PATH 256U

static uint16_t uds_file_transfer_block_length(uint16_t requested) {
  uint16_t limit = UDS_TP_MTU;

  if (requested > 0 && requested < limit) {
    return requested;
  }

  return limit;
}

static UDSErr_t fs_error_to_nrc(int err) {
  switch (-err) {
  case ENOENT:
    return UDS_NRC_RequestOutOfRange;
  case EPERM:
  case EACCES:
    return UDS_NRC_SecurityAccessDenied;
  default:
    return UDS_NRC_UploadDownloadNotAccepted;
  }
}

static void uds_file_transfer_reset(void) {
  if (file_transfer_state.file_open) {
    fs_close(&file_transfer_state.file);
  }

  file_transfer_state.mode = UDS_FILE_TRANSFER_IDLE;
  file_transfer_state.file_open = false;
  file_transfer_state.expected_size = 0;
  file_transfer_state.transferred = 0;
  file_transfer_state.block_length = 0;
}

static UDSErr_t uds_file_transfer_begin_write(const char *path,
                                              size_t expected_size,
                                              UDSRequestFileTransferArgs_t *args) {
  fs_file_t_init(&file_transfer_state.file);
  int rc = fs_open(&file_transfer_state.file,
                   path,
                   FS_O_CREATE | FS_O_TRUNC | FS_O_RDWR);

  if (rc < 0) {
    return fs_error_to_nrc(rc);
  }

  file_transfer_state.mode = UDS_FILE_TRANSFER_WRITE;
  file_transfer_state.file_open = true;
  file_transfer_state.expected_size = expected_size;
  file_transfer_state.transferred = 0U;
  file_transfer_state.block_length = uds_file_transfer_block_length(args->maxNumberOfBlockLength);

  args->maxNumberOfBlockLength = file_transfer_state.block_length;

  return UDS_OK;
}

static UDSErr_t uds_file_transfer_begin_read(const char *path,
                                             UDSRequestFileTransferArgs_t *args) {
  struct fs_dirent entry;

  fs_file_t_init(&file_transfer_state.file);
  int rc = fs_open(&file_transfer_state.file, path, FS_O_READ);

  if (rc < 0) {
    return fs_error_to_nrc(rc);
  }

  rc = fs_stat(path, &entry);
  if (rc < 0) {
    uds_file_transfer_reset();
    return fs_error_to_nrc(rc);
  }

  if (entry.type != FS_DIR_ENTRY_FILE) {
    uds_file_transfer_reset();
    return UDS_NRC_RequestOutOfRange;
  }

  file_transfer_state.mode = UDS_FILE_TRANSFER_READ;
  file_transfer_state.file_open = true;
  file_transfer_state.expected_size = entry.size;
  file_transfer_state.transferred = 0U;
  file_transfer_state.block_length = uds_file_transfer_block_length(args->maxNumberOfBlockLength);

  args->maxNumberOfBlockLength = file_transfer_state.block_length;

  return UDS_OK;
}

static UDSErr_t uds_file_transfer_delete(const char *path) {
  int rc = fs_unlink(path);

  if (rc < 0) {
    return fs_error_to_nrc(rc);
  }

  return UDS_OK;
}

static UDSErr_t uds_file_transfer_request(struct uds_context *context) {
  if (context == NULL || context->arg == NULL) {
    return UDS_ERR_MISUSE;
  }

  if (upload_download_state.state != UDS_UPDOWN_IDLE ||
      file_transfer_state.mode != UDS_FILE_TRANSFER_IDLE) {
    return UDS_NRC_ConditionsNotCorrect;
  }

  UDSRequestFileTransferArgs_t *args = context->arg;

  if (args->filePath == NULL || args->filePathLen == 0U) {
    return UDS_NRC_RequestOutOfRange;
  }

  if (args->filePathLen >= UDS_FILE_TRANSFER_MAX_PATH) {
    return UDS_NRC_RequestOutOfRange;
  }

  char path[UDS_FILE_TRANSFER_MAX_PATH];
  memcpy(path, args->filePath, args->filePathLen);
  path[args->filePathLen] = '\0';

  switch (args->modeOfOperation) {
  case UDS_MOOP_ADDFILE:
  case UDS_MOOP_REPLFILE:
    return uds_file_transfer_begin_write(path, args->fileSizeCompressed, args);
  case UDS_MOOP_DELFILE:
    args->maxNumberOfBlockLength = 0U;
    return uds_file_transfer_delete(path);
  case UDS_MOOP_RDFILE:
    return uds_file_transfer_begin_read(path, args);
  default:
    return UDS_NRC_RequestOutOfRange;
  }
}

static UDSErr_t uds_file_transfer_write(const UDSTransferDataArgs_t *args) {
  if (args == NULL) {
    return UDS_ERR_MISUSE;
  }

  if (args->data == NULL || args->len == 0U) {
    return UDS_NRC_RequestOutOfRange;
  }

  if (file_transfer_state.expected_size > 0U &&
      file_transfer_state.transferred + args->len > file_transfer_state.expected_size) {
    return UDS_NRC_RequestOutOfRange;
  }

  ssize_t rc = fs_write(&file_transfer_state.file, args->data, args->len);

  if (rc < 0) {
    return fs_error_to_nrc((int)rc);
  }

  if ((size_t)rc != args->len) {
    return UDS_NRC_UploadDownloadNotAccepted;
  }

  file_transfer_state.transferred += args->len;

  return UDS_OK;
}

static UDSErr_t uds_file_transfer_read(struct uds_context *context,
                                       UDSTransferDataArgs_t *args) {
  if (context == NULL || args == NULL) {
    return UDS_ERR_MISUSE;
  }

  if (args->copyResponse == NULL) {
    return UDS_ERR_MISUSE;
  }

  size_t remaining = 0U;
  if (file_transfer_state.expected_size >= file_transfer_state.transferred) {
    remaining = file_transfer_state.expected_size - file_transfer_state.transferred;
  }

  if (remaining == 0U) {
    return UDS_NRC_RequestSequenceError;
  }

  size_t max_len = MIN((size_t)args->maxRespLen, remaining);

  ssize_t rc = fs_read(&file_transfer_state.file,
                       (void *)args->data,
                       max_len);

  if (rc < 0) {
    return fs_error_to_nrc((int)rc);
  }

  if (rc == 0) {
    return UDS_NRC_RequestSequenceError;
  }

  uint8_t copy_status = args->copyResponse(context->server, args->data, (uint16_t)rc);
  if (copy_status != UDS_PositiveResponse) {
    return copy_status;
  }

  file_transfer_state.transferred += (size_t)rc;

  return UDS_OK;
}

static UDSErr_t uds_file_transfer_continue(struct uds_context *context) {
  if (context == NULL || context->arg == NULL) {
    return UDS_ERR_MISUSE;
  }

  if (file_transfer_state.mode == UDS_FILE_TRANSFER_WRITE) {
    return uds_file_transfer_write((UDSTransferDataArgs_t *)context->arg);
  } else if (file_transfer_state.mode == UDS_FILE_TRANSFER_READ) {
    return uds_file_transfer_read(context, (UDSTransferDataArgs_t *)context->arg);
  }

  return UDS_NRC_RequestSequenceError;
}

static UDSErr_t uds_file_transfer_exit(void) {
  enum FileTransferMode mode = file_transfer_state.mode;
  bool complete = true;

  if (mode == UDS_FILE_TRANSFER_IDLE) {
    return UDS_NRC_RequestSequenceError;
  }

  if (mode == UDS_FILE_TRANSFER_WRITE &&
      file_transfer_state.expected_size > 0U &&
      file_transfer_state.transferred != file_transfer_state.expected_size) {
    complete = false;
  }

  uds_file_transfer_reset();

  return complete ? UDS_OK : UDS_NRC_GeneralProgrammingFailure;
}

static UDSErr_t start_download(
    const struct uds_context* const context) {
  /*
   * Here we assume that the upper layer has already checked whether an operation
   * is already in progress. So if we are not in the idle state here thats okay.
   */

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

  // only support plain data (it is the only format defined in the standard)
  if (args->dataFormatIdentifier != 0x00) {
    return UDS_NRC_RequestOutOfRange;
  }

  upload_download_state.start_address = (uintptr_t)(args->addr) - FLASH_BASE_ADDRESS;
  upload_download_state.current_address = (uintptr_t)(args->addr) - FLASH_BASE_ADDRESS;
  upload_download_state.total_size = args->size;

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
  // prepare flash by erasing necessary sectors
  int rc = flash_erase(flash_controller, upload_download_state.start_address, upload_download_state.total_size);
  if (rc != 0) {
    LOG_ERR("Flash erase failed at addr 0x%08lx, size %zu, err %d",
        upload_download_state.start_address,
        upload_download_state.total_size,
        rc);
    return UDS_NRC_GeneralProgrammingFailure;
  }
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
    LOG_ERR("Flash write failed at addr 0x%08lx, size %u, err %d",
        upload_download_state.current_address,
        args->len,
        rc);
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
  ARG_UNUSED(context);

  if (upload_download_state.state == UDS_UPDOWN_DOWNLOAD_IN_PROGRESS ||
      upload_download_state.state == UDS_UPDOWN_UPLOAD_IN_PROGRESS) {
    // The exit request and response contains some optional user specific parameters
    // which we currently ignore
    // TODO: we could add a checksum verification here

    upload_download_state.state = UDS_UPDOWN_IDLE;
    upload_download_state.start_address = 0;
    upload_download_state.current_address = 0;
    upload_download_state.total_size = 0;

    return UDS_OK;
  }

  if (file_transfer_state.mode != UDS_FILE_TRANSFER_IDLE) {
    return uds_file_transfer_exit();
  }

  return UDS_NRC_RequestSequenceError;
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

  case UDS_EVT_RequestFileTransfer:
    return uds_file_transfer_request(context);

  case UDS_EVT_TransferData:
    if (file_transfer_state.mode != UDS_FILE_TRANSFER_IDLE) {
      return uds_file_transfer_continue(context);
    }

    if (upload_download_state.state == UDS_UPDOWN_DOWNLOAD_IN_PROGRESS) {
      return continue_download(context);
    }

    if (upload_download_state.state == UDS_UPDOWN_UPLOAD_IN_PROGRESS) {
      return continue_upload(context);
    }

    return UDS_NRC_RequestSequenceError;

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

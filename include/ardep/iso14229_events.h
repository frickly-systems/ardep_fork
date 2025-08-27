/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_ISO14229_EVENTS_H
#define ARDEP_ISO14229_EVENTS_H

enum uds_event {
  // Common Event ----------------- Argument Type
  UDS_EVENT_ERROR,  // UDSErr_t *

  // Server Event ----------------- Argument Type
  UDS_EVENT_DIAGNOSTIC_SESSION_CONTROL,    // UDSDiagSessCtrlArgs_t *
  UDS_EVENT_ECU_RESET,                     // UDSECUResetArgs_t *
  UDS_EVENT_READ_DATA_BY_IDENTIFIER,       // UDSRDBIArgs_t *
  UDS_EVENT_READ_MEMORY_BY_ADDRESS,        // UDSReadMemByAddrArgs_t *
  UDS_EVENT_COMMUNICATION_CONTROL,         // UDSCommCtrlArgs_t *
  UDS_EVENT_SECURITY_ACCESS_REQUEST_SEED,  // UDSSecAccessRequestSeedArgs_t *
  UDS_EVENT_SECURITY_ACCESS_VALIDATE_KEY,  // UDSSecAccessValidateKeyArgs_t *
  UDS_EVENT_WRITE_DATA_BY_IDENTIFIER,      // UDSWDBIArgs_t *
  UDS_EVENT_ROUTINE_CONTROL,               // UDSRoutineCtrlArgs_t*
  UDS_EVENT_REQUEST_DOWNLOAD,              // UDSRequestDownloadArgs_t*
  UDS_EVENT_REQUEST_UPLOAD,                // UDSRequestUploadArgs_t *
  UDS_EVENT_TRANSFER_DATA,                 // UDSTransferDataArgs_t *
  UDS_EVENT_REQUEST_TRANSFER_EXIT,         // UDSRequestTransferExitArgs_t *
  UDS_EVENT_SESSION_TIMEOUT,               // NULL
  UDS_EVENT_DO_SCHEDULED_RESET,            // uint8_t *
  UDS_EVENT_REQUEST_FILE_TRANSFER,         // UDSRequestFileTransferArgs_t *

  // TODO: change this to hold all other events that we implement but are not in
  // the external iso14229 module
  UDS_EVENT_CUSTOM,  // UDSCustomArgs_t *
  UDS_EVENT_CUSTOM_START,

  UDS_EVENT_MAX = 65535,
};

#endif  // ARDEP_ISO14229_EVENTS_H
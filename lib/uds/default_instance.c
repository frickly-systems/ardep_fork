/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <ardep/uds.h>

LOG_MODULE_DECLARE(uds);

// Note that this file is only compiled if CONFIG_UDS_DEFAULT_INSTANCE is set

struct uds_instance_t uds_default_instance;

static UDSErr_t diag_session_ctrl_check(const struct uds_context* const context,
                                        bool* apply_action) {
  *apply_action = true;

  return UDS_OK;
}

static UDSErr_t diag_session_ctrl_action(struct uds_context* const context,
                                         bool* consume_event) {
  *consume_event = false;

#ifndef CONFIG_UDS_DEFAULT_INSTANCE_DISABLE_SWITCH_TO_FIRMWARE_LOADER
  UDSDiagSessCtrlArgs_t* args = context->arg;
  if (args->type == UDS_DIAG_SESSION__PROGRAMMING) {
    LOG_INF("Switching into firmware loader");
    return uds_switch_to_firmware_loader_with_programming_session();
  }
#endif

  return UDS_PositiveResponse;
}

static UDSErr_t diag_session_timeout_check(
    const struct uds_context* const context, bool* apply_action) {
  *apply_action = true;
  return UDS_OK;
}

static UDSErr_t diag_session_timeout_action(struct uds_context* const context,
                                            bool* consume_event) {
  LOG_INF("Diagnostic Session Timeout");
  *consume_event = false;

  return UDS_PositiveResponse;
}

UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(&uds_default_instance,
                                       diag_session_ctrl_check,
                                       diag_session_ctrl_action,
                                       diag_session_timeout_check,
                                       diag_session_timeout_action,
                                       NULL);

#ifdef CONFIG_UDS_USE_LINK_CONTROL
UDS_REGISTER_LINK_CONTROL_DEFAULT_HANDLER(&uds_default_instance);
#endif  // CONFIG_UDS_USE_LINK_CONTROL

UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(&uds_default_instance);

static const struct device* can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

#ifndef CONFIG_UDS_DEFAULT_INSTANCE_EXTERNAL_ADDRESS_PROVIDER
static UDSISOTpCConfig_t uds_default_instance_get_addresses() {
  UDSISOTpCConfig_t cfg = {
    // Hardware Addresses
    .source_addr = CONFIG_UDS_DEFAULT_INSTANCE_SOURCE_ADDRESS,
    .target_addr = CONFIG_UDS_DEFAULT_INSTANCE_TARGET_ADDRESS,

    // Functional Addresses
    .source_addr_func = CONFIG_UDS_DEFAULT_INSTANCE_FUNCTIONAL_SOURCE_ADDRESS,
    .target_addr_func = CONFIG_UDS_DEFAULT_INSTANCE_FUNCTIONAL_TARGET_ADDRESS,
  };
  return cfg;
}
#endif
__weak void uds_default_instance_user_context(void** user_context) {}

static int uds_default_instance_init() {
  LOG_DBG("UDS Default instance initializing");

  UDSISOTpCConfig_t cfg = uds_default_instance_get_addresses();

  void* user_context = NULL;

  uds_default_instance_user_context(&user_context);

  uds_init(&uds_default_instance, &cfg, can_dev, user_context);

  if (!device_is_ready(can_dev)) {
    LOG_INF("CAN device not ready");
    return -ENODEV;
  }

  int err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    LOG_ERR("Failed to set CAN mode: %d", err);
    return err;
  }

  err = can_start(can_dev);
  if (err) {
    LOG_ERR(
        "Failed to start CAN device: "
        "%d",
        err);
    return err;
  }
  LOG_INF("CAN device started");

  uds_default_instance.iso14229.thread_start(&uds_default_instance.iso14229);
  LOG_INF("UDS thread started");

  return 0;
}

SYS_INIT(uds_default_instance_init,
         APPLICATION,
         CONFIG_UDS_DEFAULT_INSTANCE_AUTOSTART_PRIORITY);

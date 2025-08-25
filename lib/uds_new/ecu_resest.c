

// #ifdef UDS_NEW_ENABLE_RESET
#include "iso14229/uds.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include <ardep/uds_new.h>
#include <iso14229/server.h>

LOG_MODULE_REGISTER(uds_new, LOG_LEVEL_INF);

K_MUTEX_DEFINE(custom_callback_mutex);
static ecu_reset_callback_t ecu_reset_custom_callback = NULL;

static enum ecu_reset_type ecu_reset_reset_type = 0;

/**
 * @brief Work handler that performs the actual ECU reset
 */
static void ecu_reset_work_handler(struct k_work *work) {
  LOG_INF("Performing ECU reset, type: %d", ecu_reset_reset_type);
  // Allow logging to be processed
  k_msleep(1);

  switch (ecu_reset_reset_type) {
    case ECU_RESET_HARD:
    case ECU_RESET_KEY_OFF_ON:
      // Perform system reboot
      sys_reboot(SYS_REBOOT_COLD);
      break;
    case ECU_RESET_ENABLE_RAPID_POWER_SHUT_DOWN:
    case ECU_RESET_DISABLE_RAPID_POWER_SHUT_DOWN:
      // These might need different handling, but for now treat as regular
      // reboot
      sys_reboot(SYS_REBOOT_COLD);
      break;
    default:
      if (ecu_reset_reset_type >=
              ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_START &&
          ecu_reset_reset_type <= ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_END) {
        // Manufacturer/supplier specific reset - default to cold reboot
        sys_reboot(SYS_REBOOT_COLD);
      } else {
        LOG_ERR("Unknown reset type: %d", ecu_reset_reset_type);
      }
      break;
  }
}
K_WORK_DELAYABLE_DEFINE(reset_work, ecu_reset_work_handler);

UDSErr_t handle_ecu_reset_event(struct iso14229_zephyr_instance *inst,
                                enum ecu_reset_type reset_type) {
  int ret = k_mutex_lock(&custom_callback_mutex, K_FOREVER);
  if (ret < 0) {
    LOG_ERR(
        "Failed to acquire ECU Reset custom callback mutex from event handler");
    return ret;
  }
  if (ecu_reset_custom_callback) {
    UDSErr_t callback_result = ecu_reset_custom_callback(inst, reset_type);
    k_mutex_unlock(&custom_callback_mutex);
    return callback_result;
  }
  ret = k_mutex_unlock(&custom_callback_mutex);
  if (ret < 0) {
    LOG_ERR("Failed to release ECU Reset custom callback mutex");
    return ret;
  }

  uint32_t delay_ms = inst->server.p2_ms;
  LOG_INF("Scheduling ECU reset in %u ms, type: %d", delay_ms, reset_type);
  ecu_reset_reset_type = reset_type;
  ret = k_work_schedule(&reset_work, K_MSEC(delay_ms));
  if (ret < 0) {
    LOG_ERR("Failed to schedule ECU reset work");
    return UDS_NRC_ConditionsNotCorrect;
  }
  return UDS_OK;
}

int set_ecu_reset_callback(ecu_reset_callback_t callback) {
  int ret = k_mutex_lock(&custom_callback_mutex, K_FOREVER);
  if (ret < 0) {
    LOG_ERR("Failed to acquire ECU Reset custom callback mutex");
    return ret;
  }
  ecu_reset_custom_callback = callback;
  return k_mutex_unlock(&custom_callback_mutex);
}

// #endif  // UDS_NEW_ENABLE_RESET
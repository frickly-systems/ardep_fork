#ifndef ARDEP_UDS_NEW_H
#define ARDEP_UDS_NEW_H

// Forward declaration to avoid include dependency issues
#include "ardep/uds_minimal.h"
#include "iso14229/uds.h"

int uds_new_init(struct iso14229_zephyr_instance* inst,
                 const UDSISOTpCConfig_t* iso_tp_config,
                 const struct device* can_dev,
                 void* user_context);

#ifdef CONFIG_UDS_NEW_ENABLE_RESET

enum ecu_reset_type {
  ECU_RESET_HARD = 1,
  ECU_RESET_KEY_OFF_ON = 2,
  ECU_RESET_ENABLE_RAPID_POWER_SHUT_DOWN = 4,
  ECU_RESET_DISABLE_RAPID_POWER_SHUT_DOWN = 5,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_START = 0x40,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_END = 0x5F,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_START = 0x60,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_END = 0x7E,
};

/**
 * @brief Callback type for ECU reset events
 *
 * @param inst Pointer to the UDS server instance
 * @param reset_type Type of reset to perform
 */
typedef UDSErr_t (*ecu_reset_callback_t)(struct iso14229_zephyr_instance* inst,
                                         enum ecu_reset_type reset_type,
                                         void* user_context);

/**
 * @brief Set a custom callback for ECU reset events
 *
 * The default event handling is disabled when a callback is set
 *
 * @param callback Pointer to the custom callback function
 *
 * @retval 0 on success
 * @retval Negative error code on failure
 *
 */
int set_ecu_reset_callback(ecu_reset_callback_t callback);

/**
 * @brief Schedule a delayed ECU reset after p2 timeout
 *
 * @param server Pointer to the UDS server instance
 * @param reset_type Type of reset to perform
 *
 * @retval UDS_OK on success
 * @retval UDS_NRC on failure
 */
UDSErr_t handle_ecu_reset_event(struct iso14229_zephyr_instance* inst,
                                enum ecu_reset_type reset_type);

#endif  // CONFIG_UDS_NEW_ENABLE_RESET

#endif  // ARDEP_UDS_NEW_H
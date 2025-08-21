// This would be dedicated into a separate library

#pragma once

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <iso14229/server.h>
#include <iso14229/tp/isotp_c.h>

typedef UDSErr_t (*uds_read_mem_by_addr_fn)(
    struct UDSServer* srv,
    const UDSReadMemByAddrArgs_t* read_args,
    void* user_context);

typedef UDSErr_t (*uds_uds_diag_sess_ctrl_fn)(
    struct UDSServer* srv,
    const UDSDiagSessCtrlArgs_t* read_args,
    void* user_context);

struct iso14229_zephyr_instance;

typedef UDSErr_t (*uds_callback)(struct iso14229_zephyr_instance* inst,
                                 UDSEvent_t event,
                                 void* arg,
                                 void* user_context);

struct iso14229_zephyr_instance {
  UDSServer_t server;
  UDSISOTpC_t tp;

  struct k_msgq can_phys_msgq;
  struct k_msgq can_func_msgq;

  char can_phys_buffer[sizeof(struct can_frame) * 25];
  char can_func_buffer[sizeof(struct can_frame) * 25];

  struct k_mutex event_callback_mutex;
  uds_callback event_callback;

  void* user_context;
};

typedef UDSErr_t (*uds_cb_fn)(struct UDSServer* srv,
                              UDSEvent_t event,
                              void* arg);

int iso14229_zephyr_init(struct iso14229_zephyr_instance* inst,
                         const UDSISOTpCConfig_t* iso_tp_config,
                         const struct device* can_dev,
                         void* user_context);

int iso14229_zephyr_set_callback(struct iso14229_zephyr_instance* inst,
                                 uds_callback callback);

void iso14229_zephyr_thread_tick(struct iso14229_zephyr_instance* inst);
void iso14229_zephyr_thread(struct iso14229_zephyr_instance* inst);

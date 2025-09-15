/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_ISO14229_H
#define ARDEP_ISO14229_H

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <iso14229.h>

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

  k_tid_t thread_id;
  struct k_thread thread_data;
  K_KERNEL_STACK_MEMBER(thread_stack, 1024);
  bool thread_running;
  atomic_t thread_stop_requested;
  struct k_mutex thread_mutex;

  int (*set_callback)(struct iso14229_zephyr_instance* inst,
                      uds_callback callback);
  void (*thread_tick)(struct iso14229_zephyr_instance* inst);
  int (*thread_start)(struct iso14229_zephyr_instance* inst);
  int (*thread_stop)(struct iso14229_zephyr_instance* inst);
};

int iso14229_zephyr_init(struct iso14229_zephyr_instance* inst,
                         const UDSISOTpCConfig_t* iso_tp_config,
                         const struct device* can_dev,
                         void* user_context);

#endif  // ARDEP_ISO14229_H
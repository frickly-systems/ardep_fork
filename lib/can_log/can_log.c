/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>

#include <ardep/can_log.h>

struct k_sem can_tx_sem;  // semaphore for CAN TX completion

static uint16_t can_log_id;
static uint8_t buf[128];
static uint32_t log_format_type = LOG_OUTPUT_TEXT;
static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static bool panic_mode = false;

#ifdef CONFIG_CAN_LOG_ADDRESS_PROVIDER_EXTERNAL
uint16_t can_log_get_id();
#endif

static int can_log_is_ready(const struct log_backend *const backend);

static void can_log_tx_cb_no_wait(const struct device *dev,
                                  int error,
                                  void *user_data) {
  ARG_UNUSED(dev);
  ARG_UNUSED(user_data);
  ARG_UNUSED(error);
}

static void can_log_tx_cb(const struct device *dev,
                          int error,
                          void *user_data) {
  struct k_sem *tx_sem = (struct k_sem *)user_data;
  ARG_UNUSED(dev);
  ARG_UNUSED(error);
  k_sem_give(tx_sem);
}

static int can_log_line_out(uint8_t *data, size_t length, void *output_ctx) {
  const bool synchronous = panic_mode || IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE);

  if (can_log_is_ready(NULL) != 0) {
    return length;  // not ready -> we just drop everything
  }

  struct can_frame frame;
  if (length > CAN_MAX_DLEN) {
    length = CAN_MAX_DLEN;
  }

  frame.id = can_log_id;
  frame.dlc = length;
  frame.flags = 0;
  memcpy(frame.data, data, length);

  if (synchronous) {
    // in synchronous mode we don't wait for completion, just throw it on the
    // bus
    can_send(can_dev, &frame, K_NO_WAIT, can_log_tx_cb_no_wait, NULL);
  } else {
    k_sem_take(&can_tx_sem, K_NO_WAIT);  // Reset semaphore

    can_send(can_dev, &frame, K_MSEC(CONFIG_CAN_LOG_SEND_TIMEOUT_MS),
             can_log_tx_cb, &can_tx_sem);

    // wait for transmission to complete
    k_sem_take(&can_tx_sem, K_MSEC(CONFIG_CAN_LOG_SEND_TIMEOUT_MS));
  }

  return length;
}

LOG_OUTPUT_DEFINE(can_log_output, can_log_line_out, buf, sizeof(buf));

static void can_log_process(const struct log_backend *const backend,
                            union log_msg_generic *msg) {
  log_format_func_t log_format_func = log_format_func_t_get(log_format_type);
  log_format_func(&can_log_output, &msg->log, log_backend_std_get_flags());
}

static int can_format_set(const struct log_backend *const backend,
                          uint32_t log_type) {
  log_format_type = log_type;
  return 0;
}

static void can_log_init(const struct log_backend *const backend) {
  ARG_UNUSED(backend);

  k_sem_init(&can_tx_sem, 0, 1);

#ifdef CONFIG_CAN_LOG_ADDRESS_PROVIDER_EXTERNAL
  can_log_id = can_log_get_id();
#else
  can_log_id = CONFIG_CAN_LOG_ID;
#endif
}

static void can_log_dropped(const struct log_backend *const backend,
                            uint32_t cnt) {
  log_backend_std_dropped(&can_log_output, cnt);
}

static void can_log_panic(const struct log_backend *const backend) {
  ARG_UNUSED(backend);
  panic_mode = true;
}

static int can_log_is_ready(const struct log_backend *const backend) {
  ARG_UNUSED(backend);
  if (!device_is_ready(can_dev)) {
    return -ENODEV;
  }

  enum can_state state;

  int ret = can_get_state(can_dev, &state, NULL);
  if (ret) {
    return -ENODEV;
  }

  switch (state) {
    case CAN_STATE_BUS_OFF:
    case CAN_STATE_STOPPED:
      return -EIO;
    default:
      return 0;
  }
}

struct log_backend_api can_log_backend_api = {
  .process = can_log_process,
  .format_set = can_format_set,
  .init = can_log_init,
  // without a processing thread we have to be always ready.
  .is_ready = IS_ENABLED(CONFIG_LOG_PROCESS_THREAD) ? can_log_is_ready : NULL,
  .dropped = can_log_dropped,
  .panic = can_log_panic,
};

LOG_BACKEND_DEFINE(can_log_backend, can_log_backend_api, true);

#ifdef CONFIG_CAN_LOG_AUTOSTART_BUS
static int can_log_autostart_bus_sysinit() {
  int err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    printk("CAN log: Failed to set CAN bus to normal mode: %d\n", err);
    log_backend_deactivate(&can_log_backend);
    return err;
  }

  err = can_start(can_dev);
  if (err) {
    printk("CAN log: Failed to start CAN bus: %d\n", err);
    log_backend_deactivate(&can_log_backend);
    return err;
  }

  return 0;
}

SYS_INIT(can_log_autostart_bus_sysinit,
         POST_KERNEL,
         CONFIG_CAN_LOG_AUTOSTART_BUS_PRIORITY);
#endif

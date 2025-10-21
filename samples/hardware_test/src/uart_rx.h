/*
 * Copyright (c) Frickly Systems GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_RX_H
#define UART_RX_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>

typedef int rx_callback_fn_t(const uint8_t *data, size_t length);

/**
 * @brief Initialize UART RX functionality
 *
 * @param uart_device Pointer to the UART device
 * @return 0 on success, negative error code on failure
 */
int uart_rx_init(const struct device *uart_device, rx_callback_fn_t *callback);

/**
 * @brief Start the UART RX thread
 *
 * @return 0 on success, negative error code on failure
 */
int uart_rx_start_thread(void);

/**
 * @brief Stop the UART RX thread
 *
 * @return 0 on success, negative error code on failure
 */
int uart_rx_stop_thread(void);

#endif /* UART_RX_H */

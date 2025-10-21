/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_TX_H
#define UART_TX_H

#include <zephyr/device.h>

/**
 * @brief Initialize UART TX functionality
 *
 * @param uart_device Pointer to the UART device
 * @return 0 on success, negative error code on failure
 */
int uart_tx_init(const struct device *uart_device);

/**
 * @brief Callback type for protobuf encoding functions
 *
 * @param data Pointer to the data structure to encode
 * @param buffer Buffer to encode into
 * @param buffer_size Size of the buffer
 * @param message_length Output parameter for the encoded message length
 * @return int 0 on success, negative error code on failure
 */
typedef int (*proto_encode_fn_t)(const void *data,
                                 uint8_t *buffer,
                                 size_t buffer_size,
                                 size_t *message_length);

/**
 * @brief Generic function to encode data to protobuf and transmit via UART
 *
 * This function encapsulates the common pattern of:
 * 1. Allocating buffers
 * 2. Encoding data to protobuf
 * 3. COBS encoding and transmitting
 * 4. Handling cleanup on errors
 *
 * @param data Pointer to the data structure to encode
 * @param encode_fn Callback function to encode the data to protobuf
 * @param error_msg Error message to log if encoding fails
 * @return int 0 on success, negative error code on failure
 */
int encode_and_transmit(const void *data,
                        proto_encode_fn_t encode_fn,
                        const char *error_msg);

#endif /* UART_TX_H */

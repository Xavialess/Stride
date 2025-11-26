/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#ifndef UART_SERVICE_H
#define UART_SERVICE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>

/**
 * @brief UART data structure for buffering
 */
struct uart_data_t {
	void *fifo_reserved;
	uint8_t data[CONFIG_BT_NUS_UART_BUFFER_SIZE];
	uint16_t len;
};

/**
 * @brief Initialize UART subsystem
 * 
 * @return 0 on success, negative errno on failure
 */
int uart_service_init(void);

/**
 * @brief Transmit data over UART
 * 
 * @param data Pointer to data buffer
 * @param len Length of data
 * @return 0 on success, negative errno on failure
 */
int uart_transmit(const uint8_t *data, uint16_t len);

/**
 * @brief Get next received UART data packet
 * 
 * This function blocks until data is available.
 * The caller is responsible for freeing the returned buffer with k_free().
 * 
 * @return Pointer to received data structure, or NULL on error
 */
struct uart_data_t *uart_get_rx_data(void);

#endif /* UART_SERVICE_H */


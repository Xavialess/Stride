/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <uart/uart_service.h>
#include <ble/ble_service.h>
#include <gpio/gpio.h>
#include <os/threads.h>
#include <haptics/haptic_service.h>

#define LOG_MODULE_NAME peripheral_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/**
 * @brief BLE data received callback
 * 
 * This function is called when data is received over BLE.
 * It processes the data and routes it to the appropriate service:
 * - Haptic commands (starting with 0x01-0x04) go to haptic service
 * - All other data is forwarded to UART for transmission
 */
static void on_ble_data_received(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	if (len == 0) {
		return;
	}

	/* Check if this is a haptic command (0x01-0x04) */
	if (data[0] >= 0x01 && data[0] <= 0x04) {
		LOG_DBG("Routing data to haptic service");
		int err = haptic_process_ble_data(data, len);
		if (err) {
			LOG_ERR("Failed to process haptic data (err %d)", err);
		}
		return;
	}

	/* Otherwise, forward to UART */
	LOG_DBG("Routing data to UART");
	for (uint16_t pos = 0; pos != len;) {
		struct uart_data_t *tx = k_malloc(sizeof(*tx));

		if (!tx) {
			LOG_WRN("Not able to allocate UART send data buffer");
			return;
		}

		/* Keep the last byte of TX buffer for potential LF char. */
		size_t tx_data_size = sizeof(tx->data) - 1;

		if ((len - pos) > tx_data_size) {
			tx->len = tx_data_size;
		} else {
			tx->len = (len - pos);
		}

		memcpy(tx->data, &data[pos], tx->len);
		pos += tx->len;

		/* Append the LF character when the CR character triggered
		 * transmission from the peer.
		 */
		if ((pos == len) && (data[len - 1] == '\r')) {
			tx->data[tx->len] = '\n';
			tx->len++;
		}

		int err = uart_transmit(tx->data, tx->len);
		if (err) {
			/* Transmission failed, free the buffer */
			LOG_WRN("UART transmission failed, data lost");
			k_free(tx);
		} else {
			k_free(tx);
		}
	}
}

/**
 * @brief Main application entry point
 */
int main(void)
{
	int err;

	LOG_INF("Starting Nordic UART service sample");

	/* Initialize GPIO (LEDs and buttons) */
	err = gpio_init();
	if (err) {
		LOG_ERR("GPIO initialization failed (err %d)", err);
		gpio_error_state();
	}

	/* Initialize UART */
	err = uart_service_init();
	if (err) {
		LOG_ERR("UART initialization failed (err %d)", err);
		gpio_error_state();
	}

	/* Initialize BLE with data received callback */
	err = ble_service_init(on_ble_data_received);
	if (err) {
		LOG_ERR("BLE initialization failed (err %d)", err);
		gpio_error_state();
	}

	/* Initialize haptic service */
	err = haptic_service_init();
	if (err) {
		LOG_ERR("Haptic service initialization failed (err %d)", err);
		/* Non-critical: continue without haptics */
		LOG_WRN("Continuing without haptic feedback support");
	}

	/* Start BLE advertising */
	err = ble_start_advertising();
	if (err) {
		LOG_ERR("Advertising start failed (err %d)", err);
		gpio_error_state();
	}

	/* Initialize thread manager (threads are auto-started) */
	threads_init();

	LOG_INF("Initialization complete. System running.");

	/* Main thread can now idle or perform other tasks */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

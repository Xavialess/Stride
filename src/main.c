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
 * All received data is processed as haptic motor commands.
 */
static void on_ble_data_received(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
	if (len == 0) {
		return;
	}

	/* Blink LED to show data received */
	gpio_toggle_led(LED_RUN_STATUS, 1);
	k_sleep(K_MSEC(100));
	gpio_toggle_led(LED_RUN_STATUS, 0);

	LOG_INF("Received BLE data: len=%d, data[0]=0x%02X", len, data[0]);
	int err = haptic_process_ble_data(data, len);
	if (err) {
		LOG_ERR("Failed to process motor command (err %d)", err);
	} else {
		LOG_INF("Motor command processed successfully");
	}
}

/**
 * @brief Main application entry point
 */
int main(void)
{
	int err;

	LOG_INF("Starting BLE Haptic Motor Controller");

	/* Initialize GPIO (LEDs and buttons) */
	err = gpio_init();
	if (err) {
		LOG_ERR("GPIO initialization failed (err %d)", err);
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

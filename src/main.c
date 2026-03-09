/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

/** @file
 *  @brief Stride Wristband BLE Haptic Controller
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ble/ble_service.h>
#include <ble/stride_service.h>
#include <gpio/gpio.h>
#include <os/threads.h>
#include <haptics/haptic_service.h>

#define LOG_MODULE_NAME stride_wristband
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

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

	/* Initialize BLE subsystem */
	err = ble_service_init();
	if (err) {
		LOG_ERR("BLE initialization failed (err %d)", err);
		gpio_error_state();
	}

	/* Initialize Stride custom GATT service */
	err = stride_service_init();
	if (err) {
		LOG_ERR("Stride service initialization failed (err %d)", err);
	}

	/* Initialize haptic service */
	err = haptic_service_init();
	if (err) {
		LOG_ERR("Haptic service initialization failed (err %d)", err);
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

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

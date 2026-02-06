/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#include <os/threads.h>
#include <ble/ble_service.h>
#include <gpio/gpio.h>
#include <haptics/haptic_service.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "../../drivers/haptics/drv2605l.h"

LOG_MODULE_REGISTER(threads, LOG_LEVEL_DBG);

#define RUN_LED_BLINK_INTERVAL K_MSEC(1000)

/**
 * @brief LED blink thread - handles status LED blinking
 */
void led_blink_thread_entry(void)
{
	int blink_status = 0;

	LOG_INF("LED blink thread started");

	for (;;) {
		gpio_toggle_led(LED_RUN_STATUS, (++blink_status) % 2);
		k_sleep(RUN_LED_BLINK_INTERVAL);
	}
}

/**
 * @brief Haptic thread - handles haptic feedback pattern playback
 */
void haptic_thread_entry(void)
{
	/* Wait for haptic service to be initialized */
	haptic_wait_init();

	LOG_INF("Haptic thread started");

	for (;;) {
		/* Wait for haptic data from the queue */
		struct haptic_data_t *haptic_data = haptic_get_queued_data();

		if (!haptic_data) {
			LOG_WRN("Received NULL haptic data");
			continue;
		}

		LOG_DBG("Processing haptic pattern (type: %d, len: %d)", 
		        haptic_data->type, haptic_data->len);

		/* Process based on pattern type */
		switch (haptic_data->type) {
		case HAPTIC_PATTERN_SINGLE_EFFECT:
			if (haptic_data->len >= 1) {
				int ret = drv2605l_play_effect(haptic_data->data[0]);
				if (ret < 0) {
					LOG_ERR("Failed to play effect %d (err %d)", 
					        haptic_data->data[0], ret);
				}
			}
			break;

		case HAPTIC_PATTERN_SEQUENCE:
			if (haptic_data->len > 0) {
				int ret = drv2605l_play_sequence(haptic_data->data, 
				                                  haptic_data->len);
				if (ret < 0) {
					LOG_ERR("Failed to play sequence (err %d)", ret);
				}
			}
			break;

		case HAPTIC_PATTERN_STOP:
			drv2605l_stop();
			LOG_DBG("Stopped haptic playback");
			break;

		case HAPTIC_PATTERN_CUSTOM:
			/* Custom patterns can be implemented here */
			LOG_WRN("Custom patterns not yet implemented");
			break;

		default:
			LOG_WRN("Unknown haptic pattern type: %d", haptic_data->type);
			break;
		}

		/* Free the haptic data buffer */
		k_free(haptic_data);

		/* Small delay to prevent overwhelming the driver */
		k_sleep(K_MSEC(10));
	}
}

/**
 * @brief Initialize thread management system
 */
void threads_init(void)
{
	LOG_INF("Threads initialized");
	/* Threads are statically defined and started automatically */
}

/* Define LED blink thread */
K_THREAD_DEFINE(led_blink_thread_id, CONFIG_APP_LED_BLINK_STACK_SIZE, led_blink_thread_entry, 
		NULL, NULL, NULL, CONFIG_APP_LED_BLINK_PRIORITY, 0, 0);

/* Define haptic thread */
K_THREAD_DEFINE(haptic_thread_id, CONFIG_APP_HAPTIC_STACK_SIZE, haptic_thread_entry, 
		NULL, NULL, NULL, CONFIG_APP_HAPTIC_PRIORITY, 0, 0);


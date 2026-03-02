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

void led_blink_thread_entry(void)
{
	int blink_status = 0;

	LOG_INF("LED blink thread started");

	for (;;) {
		gpio_toggle_led(LED_RUN_STATUS, (++blink_status) % 2);
		k_sleep(RUN_LED_BLINK_INTERVAL);
	}
}

static void wait_for_playback_done(void)
{
	for (int i = 0; i < 200; i++) {
		if (!drv2605l_is_playing()) {
			return;
		}
		k_sleep(K_MSEC(10));
	}
}

/**
 * Play a single effect on one physical motor, wait for completion.
 */
static int play_effect_on_motor(uint8_t effect, motor_target_t motor)
{
	motor_mux_select(motor);
	int ret = drv2605l_play_effect(effect);
	if (ret == 0) {
		wait_for_playback_done();
	}
	return ret;
}

/**
 * Play a sequence on one physical motor, wait for completion.
 */
static int play_sequence_on_motor(const uint8_t *effects, uint8_t len,
				  motor_target_t motor)
{
	motor_mux_select(motor);
	int ret = drv2605l_play_sequence(effects, len);
	if (ret == 0) {
		wait_for_playback_done();
	}
	return ret;
}

static void play_multi_step_pattern(haptic_predefined_pattern_t pattern_id)
{
	const struct haptic_pattern_def *def = haptic_get_pattern_def(pattern_id);

	if (!def || def->step_count == 0) {
		LOG_WRN("Invalid multi-step pattern %d", pattern_id);
		return;
	}

	for (uint8_t i = 0; i < def->step_count; i++) {
		const struct haptic_pattern_step *step = &def->steps[i];

		motor_mux_select(step->target);

		int ret;
		if (step->count == 1) {
			ret = drv2605l_play_effect(step->effects[0]);
		} else {
			ret = drv2605l_play_sequence(step->effects, step->count);
		}

		if (ret < 0) {
			LOG_ERR("Multi-step pattern step %d failed (err %d)",
				i, ret);
			break;
		}

		wait_for_playback_done();

		if (step->post_delay_ms > 0) {
			k_sleep(K_MSEC(step->post_delay_ms));
		}
	}

	motor_mux_select(MOTOR_LEFT);
}

void haptic_thread_entry(void)
{
	haptic_wait_init();

	LOG_INF("Haptic thread started (differential-pair mux)");

	for (;;) {
		struct haptic_data_t *haptic_data = haptic_get_queued_data();

		if (!haptic_data) {
			LOG_WRN("Received NULL haptic data");
			continue;
		}

		LOG_DBG("Processing haptic (type: %d, target: 0x%02X, len: %d)",
			haptic_data->type, haptic_data->target,
			haptic_data->len);

		switch (haptic_data->type) {
		case HAPTIC_PATTERN_SINGLE_EFFECT:
			if (haptic_data->len >= 1) {
				uint8_t effect = haptic_data->data[0];
				motor_target_t t = haptic_data->target;

				if (t == MOTOR_BOTH) {
					play_effect_on_motor(effect, MOTOR_LEFT);
					play_effect_on_motor(effect, MOTOR_RIGHT);
				} else {
					play_effect_on_motor(effect, t);
				}
			}
			break;

		case HAPTIC_PATTERN_SEQUENCE:
			if (haptic_data->len > 0) {
				motor_target_t t = haptic_data->target;

				if (t == MOTOR_BOTH) {
					play_sequence_on_motor(
						haptic_data->data,
						haptic_data->len, MOTOR_LEFT);
					play_sequence_on_motor(
						haptic_data->data,
						haptic_data->len, MOTOR_RIGHT);
				} else {
					play_sequence_on_motor(
						haptic_data->data,
						haptic_data->len, t);
				}
			}
			break;

		case HAPTIC_PATTERN_MULTI_STEP:
			if (haptic_data->len >= 1) {
				play_multi_step_pattern(
					(haptic_predefined_pattern_t)haptic_data->data[0]);
			}
			break;

		case HAPTIC_PATTERN_STOP:
			drv2605l_stop();
			motor_mux_select(MOTOR_LEFT);
			LOG_DBG("Stopped haptic playback");
			break;

		default:
			LOG_WRN("Unknown haptic pattern type: %d",
				haptic_data->type);
			break;
		}

		k_free(haptic_data);

		k_sleep(K_MSEC(10));
	}
}

void threads_init(void)
{
	LOG_INF("Threads initialized");
}

K_THREAD_DEFINE(led_blink_thread_id, CONFIG_APP_LED_BLINK_STACK_SIZE, led_blink_thread_entry, 
		NULL, NULL, NULL, CONFIG_APP_LED_BLINK_PRIORITY, 0, 0);

K_THREAD_DEFINE(haptic_thread_id, CONFIG_APP_HAPTIC_STACK_SIZE, haptic_thread_entry, 
		NULL, NULL, NULL, CONFIG_APP_HAPTIC_PRIORITY, 0, 0);

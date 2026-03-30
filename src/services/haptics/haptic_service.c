/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <haptics/haptic_service.h>
#include <power/power_mgmt.h>
#include "../../drivers/haptics/drv2605l.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(haptic_service, LOG_LEVEL_DBG);

K_FIFO_DEFINE(fifo_haptic_data);

static K_SEM_DEFINE(haptic_init_ok, 0, 1);

/* BLE data protocol definitions */
#define HAPTIC_CMD_PLAY_EFFECT      0x01
#define HAPTIC_CMD_PLAY_SEQUENCE    0x02
#define HAPTIC_CMD_PLAY_PATTERN     0x03
#define HAPTIC_CMD_STOP             0x04

/* ------------------------------------------------------------------ */
/* Effect arrays                                                       */
/* ------------------------------------------------------------------ */

static const uint8_t fx_sharp_click_100[] = {
	DRV2605L_EFFECT_SHARP_CLICK_100
};

static const uint8_t fx_strong_buzz_x2[] = {
	DRV2605L_EFFECT_STRONG_BUZZ_100,
	DRV2605L_EFFECT_STRONG_BUZZ_100
};

static const uint8_t fx_ramp_up_click[] = {
	DRV2605L_EFFECT_TRANSITION_RAMP_UP_SHORT_SMOOTH_1,
	DRV2605L_EFFECT_STRONG_CLICK_100
};

static const uint8_t fx_strong_click_x3[] = {
	DRV2605L_EFFECT_STRONG_CLICK_100,
	DRV2605L_EFFECT_STRONG_CLICK_100,
	DRV2605L_EFFECT_STRONG_CLICK_100
};

static const uint8_t fx_sharp_click_60[] = {
	DRV2605L_EFFECT_SHARP_CLICK_60
};

static const uint8_t fx_soft_bump_click[] = {
	DRV2605L_EFFECT_SOFT_BUMP_100,
	DRV2605L_EFFECT_STRONG_CLICK_100
};

static const uint8_t fx_double_click[] = {
	DRV2605L_EFFECT_DOUBLE_CLICK_100
};

static const uint8_t fx_heartbeat[] = {
	DRV2605L_EFFECT_SOFT_BUMP_100,
	DRV2605L_EFFECT_SOFT_BUMP_60
};

static const uint8_t fx_ramp_up_long[] = {
	DRV2605L_EFFECT_TRANSITION_RAMP_UP_LONG_SMOOTH_1
};

static const uint8_t fx_ramp_down_long[] = {
	DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_LONG_SMOOTH_1
};

static const uint8_t fx_pulsing[] = {
	DRV2605L_EFFECT_PULSING_STRONG_1
};

static const uint8_t fx_strong_buzz[] = {
	DRV2605L_EFFECT_STRONG_BUZZ_100
};

static const uint8_t fx_turn_strong[] = {
	DRV2605L_EFFECT_STRONG_CLICK_100,
	DRV2605L_EFFECT_STRONG_BUZZ_100,
	DRV2605L_EFFECT_STRONG_CLICK_100
};

/* Nav start: sharp ramp-up punch per motor */
static const uint8_t fx_nav_start_hit[] = {
	DRV2605L_EFFECT_TRANSITION_RAMP_UP_SHORT_SHARP_1,
	DRV2605L_EFFECT_STRONG_CLICK_100,
};

/* Nav end: smooth ramp-down fade per motor */
static const uint8_t fx_nav_end_fade[] = {
	DRV2605L_EFFECT_STRONG_CLICK_60,
	DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_LONG_SMOOTH_1,
};

/* Nav stop: long alert burst per step */
static const uint8_t fx_alert_long[] = {
	DRV2605L_EFFECT_ALERT_750MS
};

/* BLE paired: two very gentle soft bumps at 30% — barely-there confirmation */
static const uint8_t fx_ble_paired[] = {
	DRV2605L_EFFECT_SOFT_BUMP_30,
	DRV2605L_EFFECT_SOFT_BUMP_30,
};

/* Low battery bip: single soft click at 30% */
static const uint8_t fx_low_bip[] = {
	DRV2605L_EFFECT_STRONG_CLICK_30,
};

/* Critical battery bip: single sharp tick at 100% */
static const uint8_t fx_critical_bip[] = {
	DRV2605L_EFFECT_SHARP_TICK_1,
};

/* Spare A: long smooth hum — ambient/calm state indicator */
static const uint8_t fx_spare_a[] = {
	DRV2605L_EFFECT_SMOOTH_HUM_1,
};

/* Spare B: short double sharp tick — quick attention grab */
static const uint8_t fx_spare_b[] = {
	DRV2605L_EFFECT_SHORT_DOUBLE_SHARP_TICK_1,
};

/* Spare C: medium pulsing — rhythmic ongoing state */
static const uint8_t fx_spare_c[] = {
	DRV2605L_EFFECT_PULSING_MEDIUM_1,
};

/* ------------------------------------------------------------------ */
/* Pattern step tables                                                 */
/*                                                                     */
/* Hardware constraint: only one motor can be driven at a time.        */
/* "Both motors" patterns play on left first, then right (TDM).       */
/* ------------------------------------------------------------------ */

/* --- Generic patterns: play on left then right --- */

static const struct haptic_pattern_step steps_notification[] = {
	{ MOTOR_LEFT,  fx_sharp_click_100, 1, 0 },
	{ MOTOR_RIGHT, fx_sharp_click_100, 1, 0 },
};

static const struct haptic_pattern_step steps_alert[] = {
	{ MOTOR_LEFT,  fx_strong_buzz_x2, 2, 0 },
	{ MOTOR_RIGHT, fx_strong_buzz_x2, 2, 0 },
};

static const struct haptic_pattern_step steps_success[] = {
	{ MOTOR_LEFT,  fx_ramp_up_click, 2, 0 },
	{ MOTOR_RIGHT, fx_ramp_up_click, 2, 0 },
};

static const struct haptic_pattern_step steps_error[] = {
	{ MOTOR_LEFT,  fx_strong_click_x3, 3, 0 },
	{ MOTOR_RIGHT, fx_strong_click_x3, 3, 0 },
};

static const struct haptic_pattern_step steps_button_press[] = {
	{ MOTOR_LEFT,  fx_sharp_click_60, 1, 0 },
	{ MOTOR_RIGHT, fx_sharp_click_60, 1, 0 },
};

static const struct haptic_pattern_step steps_long_press[] = {
	{ MOTOR_LEFT,  fx_soft_bump_click, 2, 0 },
	{ MOTOR_RIGHT, fx_soft_bump_click, 2, 0 },
};

static const struct haptic_pattern_step steps_double_tap[] = {
	{ MOTOR_LEFT,  fx_double_click, 1, 0 },
	{ MOTOR_RIGHT, fx_double_click, 1, 0 },
};

static const struct haptic_pattern_step steps_heartbeat[] = {
	{ MOTOR_LEFT,  fx_heartbeat, 2, 0 },
	{ MOTOR_RIGHT, fx_heartbeat, 2, 0 },
};

static const struct haptic_pattern_step steps_ramp_up[] = {
	{ MOTOR_LEFT,  fx_ramp_up_long, 1, 0 },
	{ MOTOR_RIGHT, fx_ramp_up_long, 1, 0 },
};

static const struct haptic_pattern_step steps_ramp_down[] = {
	{ MOTOR_LEFT,  fx_ramp_down_long, 1, 0 },
	{ MOTOR_RIGHT, fx_ramp_down_long, 1, 0 },
};

static const struct haptic_pattern_step steps_pulse[] = {
	{ MOTOR_LEFT,  fx_pulsing, 1, 0 },
	{ MOTOR_RIGHT, fx_pulsing, 1, 0 },
};

static const struct haptic_pattern_step steps_buzz[] = {
	{ MOTOR_LEFT,  fx_strong_buzz, 1, 0 },
	{ MOTOR_RIGHT, fx_strong_buzz, 1, 0 },
};

/* --- Navigation patterns --- */

/* Start: alternating L/R sharp ramp-up punches, high power */
static const struct haptic_pattern_step steps_nav_start[] = {
	{ MOTOR_LEFT,  fx_nav_start_hit, 2, 80 },
	{ MOTOR_RIGHT, fx_nav_start_hit, 2, 80 },
	{ MOTOR_LEFT,  fx_nav_start_hit, 2, 80 },
	{ MOTOR_RIGHT, fx_nav_start_hit, 2, 0 },
};

/* Turn right: only the RIGHT motor fires */
static const struct haptic_pattern_step steps_turn_right[] = {
	{ MOTOR_RIGHT, fx_turn_strong, 3, 0 },
};

/* Turn left: only the LEFT motor fires */
static const struct haptic_pattern_step steps_turn_left[] = {
	{ MOTOR_LEFT, fx_turn_strong, 3, 0 },
};

/* Stop / obstacle: long alternating left-right buzz */
static const struct haptic_pattern_step steps_nav_stop[] = {
	{ MOTOR_LEFT,  fx_alert_long, 1, 200 },
	{ MOTOR_RIGHT, fx_alert_long, 1, 200 },
	{ MOTOR_LEFT,  fx_alert_long, 1, 200 },
	{ MOTOR_RIGHT, fx_alert_long, 1, 200 },
	{ MOTOR_LEFT,  fx_strong_buzz, 1, 150 },
	{ MOTOR_RIGHT, fx_strong_buzz, 1, 0 },
};

/* End: alternating L/R smooth ramp-down fades, mirrors nav_start */
static const struct haptic_pattern_step steps_nav_end[] = {
	{ MOTOR_LEFT,  fx_nav_end_fade, 2, 80 },
	{ MOTOR_RIGHT, fx_nav_end_fade, 2, 80 },
	{ MOTOR_LEFT,  fx_nav_end_fade, 2, 80 },
	{ MOTOR_RIGHT, fx_nav_end_fade, 2, 0 },
};

/* BLE paired: two very gentle bumps at 30% — barely-there confirmation */
static const struct haptic_pattern_step steps_ble_paired[] = {
	{ MOTOR_LEFT,  fx_ble_paired, 2, 0 },
	{ MOTOR_RIGHT, fx_ble_paired, 2, 0 },
};

/* Low battery (~15%): bip-silence-bip-silence-bip at 30%, alternating motors */
static const struct haptic_pattern_step steps_low_battery[] = {
	{ MOTOR_LEFT,  fx_low_bip, 1, 200 },
	{ MOTOR_RIGHT, fx_low_bip, 1, 200 },
	{ MOTOR_LEFT,  fx_low_bip, 1, 0 },
};

/* Critical battery (~5%): bip-silence-bip-silence-bip at 100%, alternating motors */
static const struct haptic_pattern_step steps_critical_battery[] = {
	{ MOTOR_LEFT,  fx_critical_bip, 1, 150 },
	{ MOTOR_RIGHT, fx_critical_bip, 1, 150 },
	{ MOTOR_LEFT,  fx_critical_bip, 1, 0 },
};

/* Spare A: long smooth hum on both motors */
static const struct haptic_pattern_step steps_spare_a[] = {
	{ MOTOR_LEFT,  fx_spare_a, 1, 0 },
	{ MOTOR_RIGHT, fx_spare_a, 1, 0 },
};

/* Spare B: short double sharp tick on both motors */
static const struct haptic_pattern_step steps_spare_b[] = {
	{ MOTOR_LEFT,  fx_spare_b, 1, 0 },
	{ MOTOR_RIGHT, fx_spare_b, 1, 0 },
};

/* Spare C: medium pulsing on both motors */
static const struct haptic_pattern_step steps_spare_c[] = {
	{ MOTOR_LEFT,  fx_spare_c, 1, 0 },
	{ MOTOR_RIGHT, fx_spare_c, 1, 0 },
};

/* ------------------------------------------------------------------ */
/* Master pattern lookup table                                         */
/* ------------------------------------------------------------------ */

static const struct haptic_pattern_def predefined_patterns[HAPTIC_PREDEFINED_COUNT] = {
	[HAPTIC_PATTERN_NOTIFICATION] = { steps_notification, ARRAY_SIZE(steps_notification) },
	[HAPTIC_PATTERN_ALERT]        = { steps_alert,        ARRAY_SIZE(steps_alert) },
	[HAPTIC_PATTERN_SUCCESS]      = { steps_success,      ARRAY_SIZE(steps_success) },
	[HAPTIC_PATTERN_ERROR]        = { steps_error,        ARRAY_SIZE(steps_error) },
	[HAPTIC_PATTERN_BUTTON_PRESS] = { steps_button_press, ARRAY_SIZE(steps_button_press) },
	[HAPTIC_PATTERN_LONG_PRESS]   = { steps_long_press,   ARRAY_SIZE(steps_long_press) },
	[HAPTIC_PATTERN_DOUBLE_TAP]   = { steps_double_tap,   ARRAY_SIZE(steps_double_tap) },
	[HAPTIC_PATTERN_HEARTBEAT]    = { steps_heartbeat,    ARRAY_SIZE(steps_heartbeat) },
	[HAPTIC_PATTERN_RAMP_UP]      = { steps_ramp_up,      ARRAY_SIZE(steps_ramp_up) },
	[HAPTIC_PATTERN_RAMP_DOWN]    = { steps_ramp_down,    ARRAY_SIZE(steps_ramp_down) },
	[HAPTIC_PATTERN_PULSE]        = { steps_pulse,        ARRAY_SIZE(steps_pulse) },
	[HAPTIC_PATTERN_BUZZ]         = { steps_buzz,         ARRAY_SIZE(steps_buzz) },
	[HAPTIC_PATTERN_NAV_START]        = { steps_nav_start,        ARRAY_SIZE(steps_nav_start) },
	[HAPTIC_PATTERN_TURN_RIGHT]       = { steps_turn_right,       ARRAY_SIZE(steps_turn_right) },
	[HAPTIC_PATTERN_TURN_LEFT]        = { steps_turn_left,        ARRAY_SIZE(steps_turn_left) },
	[HAPTIC_PATTERN_NAV_STOP]         = { steps_nav_stop,         ARRAY_SIZE(steps_nav_stop) },
	[HAPTIC_PATTERN_NAV_END]          = { steps_nav_end,          ARRAY_SIZE(steps_nav_end) },
	[HAPTIC_PATTERN_BLE_PAIRED]       = { steps_ble_paired,       ARRAY_SIZE(steps_ble_paired) },
	[HAPTIC_PATTERN_LOW_BATTERY]      = { steps_low_battery,      ARRAY_SIZE(steps_low_battery) },
	[HAPTIC_PATTERN_CRITICAL_BATTERY] = { steps_critical_battery, ARRAY_SIZE(steps_critical_battery) },
	[HAPTIC_PATTERN_SPARE_A]          = { steps_spare_a,          ARRAY_SIZE(steps_spare_a) },
	[HAPTIC_PATTERN_SPARE_B]          = { steps_spare_b,          ARRAY_SIZE(steps_spare_b) },
	[HAPTIC_PATTERN_SPARE_C]          = { steps_spare_c,          ARRAY_SIZE(steps_spare_c) },
};

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

static int queue_haptic_data(haptic_pattern_type_t type, motor_target_t target,
			     const uint8_t *data, uint8_t len)
{
	struct haptic_data_t *haptic_data;

	if (len > HAPTIC_MAX_DATA_SIZE) {
		LOG_ERR("Haptic data too large: %d bytes", len);
		return -EINVAL;
	}

	haptic_data = k_malloc(sizeof(*haptic_data));
	if (!haptic_data) {
		LOG_ERR("Failed to allocate haptic data buffer");
		return -ENOMEM;
	}

	haptic_data->type = type;
	haptic_data->target = target;
	haptic_data->len = len;
	if (data && len > 0) {
		memcpy(haptic_data->data, data, len);
	}

	k_fifo_put(&fifo_haptic_data, haptic_data);
	LOG_DBG("Queued haptic data (type: %d, target: 0x%02X, len: %d)",
		type, target, len);

	return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int haptic_service_init(void)
{
	int ret;

	LOG_INF("Initializing haptic service...");

	ret = drv2605l_init(DRV2605L_MOTOR_ERM);
	if (ret < 0) {
		LOG_ERR("Failed to initialize DRV2605L driver (err %d)", ret);
		return ret;
	}

	ret = motor_mux_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize motor mux (err %d)", ret);
		return ret;
	}

	k_sem_give(&haptic_init_ok);

	LOG_INF("Haptic service initialized (differential-pair mux)");
	return 0;
}

int haptic_play_effect_on(uint8_t effect, motor_target_t target)
{
	if (effect < 1 || effect > 123) {
		LOG_ERR("Invalid effect number: %d", effect);
		return -EINVAL;
	}

	return queue_haptic_data(HAPTIC_PATTERN_SINGLE_EFFECT, target, &effect, 1);
}

int haptic_play_effect(uint8_t effect)
{
	return haptic_play_effect_on(effect, MOTOR_LEFT);
}

int haptic_play_pattern(haptic_predefined_pattern_t pattern)
{
	if (pattern >= HAPTIC_PREDEFINED_COUNT) {
		LOG_ERR("Invalid pattern: %d", pattern);
		return -EINVAL;
	}

	uint8_t idx = (uint8_t)pattern;

	return queue_haptic_data(HAPTIC_PATTERN_MULTI_STEP, MOTOR_LEFT,
				 &idx, 1);
}

int haptic_play_sequence_on(const uint8_t *effects, uint8_t count,
			    motor_target_t target)
{
	if (!effects || count == 0) {
		LOG_ERR("Invalid sequence parameters");
		return -EINVAL;
	}

	if (count > HAPTIC_MAX_DATA_SIZE) {
		LOG_WRN("Sequence too long, truncating to %d effects",
			HAPTIC_MAX_DATA_SIZE);
		count = HAPTIC_MAX_DATA_SIZE;
	}

	for (uint8_t i = 0; i < count; i++) {
		if (effects[i] < 1 || effects[i] > 123) {
			LOG_ERR("Invalid effect number at index %d: %d",
				i, effects[i]);
			return -EINVAL;
		}
	}

	return queue_haptic_data(HAPTIC_PATTERN_SEQUENCE, target, effects, count);
}

int haptic_play_sequence(const uint8_t *effects, uint8_t count)
{
	return haptic_play_sequence_on(effects, count, MOTOR_LEFT);
}

/**
 * BLE Data Protocol:
 * Byte 0: Command ID
 *
 * HAPTIC_CMD_PLAY_EFFECT (0x01):
 *   Byte 1: Effect number (1-123)
 *   Byte 2: Motor target (optional, default MOTOR_LEFT)
 *           0x01 = left, 0x02 = right
 *
 * HAPTIC_CMD_PLAY_SEQUENCE (0x02):
 *   Byte 1: Number of effects
 *   Bytes 2..N: Effect numbers
 *   Byte N+1: Motor target (optional, default MOTOR_LEFT)
 *
 * HAPTIC_CMD_PLAY_PATTERN (0x03):
 *   Byte 1: Pattern ID (motor targeting is baked into the pattern)
 *
 * HAPTIC_CMD_STOP (0x04):
 *   No additional data
 */
int haptic_process_ble_data(const uint8_t *data, uint16_t len)
{
	if (!data || len < 1) {
		LOG_ERR("Invalid BLE data");
		return -EINVAL;
	}

	/* Any incoming haptic command wakes the device from idle */
	power_mgmt_activity();

	uint8_t cmd = data[0];

	LOG_DBG("Processing haptic BLE command: 0x%02X", cmd);

	switch (cmd) {
	case HAPTIC_CMD_PLAY_EFFECT: {
		if (len < 2) {
			LOG_ERR("PLAY_EFFECT: insufficient data");
			return -EINVAL;
		}
		motor_target_t target = (len >= 3) ?
			(motor_target_t)data[2] : MOTOR_LEFT;
		return haptic_play_effect_on(data[1], target);
	}

	case HAPTIC_CMD_PLAY_SEQUENCE: {
		if (len < 2) {
			LOG_ERR("PLAY_SEQUENCE: insufficient data");
			return -EINVAL;
		}
		uint8_t count = data[1];
		if (len < (uint16_t)(2 + count)) {
			LOG_ERR("PLAY_SEQUENCE: data length mismatch");
			return -EINVAL;
		}
		motor_target_t target = (len >= (uint16_t)(3 + count)) ?
			(motor_target_t)data[2 + count] : MOTOR_LEFT;
		return haptic_play_sequence_on(&data[2], count, target);
	}

	case HAPTIC_CMD_PLAY_PATTERN: {
		if (len < 2) {
			LOG_ERR("PLAY_PATTERN: insufficient data");
			return -EINVAL;
		}
		haptic_predefined_pattern_t pattern = (haptic_predefined_pattern_t)data[1];
		int ret = haptic_play_pattern(pattern);

		/* Navigation ended — return to idle after the pattern is queued */
		if (pattern == HAPTIC_PATTERN_NAV_END ||
		    pattern == HAPTIC_PATTERN_NAV_STOP) {
			LOG_INF("Navigation ended, entering idle");
			power_mgmt_request_state(POWER_STATE_IDLE);
		}

		return ret;
	}

	case HAPTIC_CMD_STOP:
		return haptic_stop();

	default:
		LOG_WRN("Unknown haptic command: 0x%02X", cmd);
		return -ENOTSUP;
	}
}

int haptic_stop(void)
{
	return queue_haptic_data(HAPTIC_PATTERN_STOP, MOTOR_NONE, NULL, 0);
}

struct haptic_data_t *haptic_get_queued_data(void)
{
	return k_fifo_get(&fifo_haptic_data, K_FOREVER);
}

const struct haptic_pattern_def *haptic_get_pattern_def(
	haptic_predefined_pattern_t pattern)
{
	if (pattern >= HAPTIC_PREDEFINED_COUNT) {
		return NULL;
	}
	return &predefined_patterns[pattern];
}

void haptic_wait_init(void)
{
	k_sem_take(&haptic_init_ok, K_FOREVER);
}

void haptic_signal_init_complete(void)
{
	k_sem_give(&haptic_init_ok);
}

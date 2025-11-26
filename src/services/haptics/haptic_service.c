/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <haptics/haptic_service.h>
#include "../../drivers/haptics/drv2605l.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(haptic_service, LOG_LEVEL_DBG);

/* FIFO queue for haptic data */
K_FIFO_DEFINE(fifo_haptic_data);

/* Semaphore for haptic initialization */
static K_SEM_DEFINE(haptic_init_ok, 0, 1);

/* BLE data protocol definitions */
#define HAPTIC_CMD_PLAY_EFFECT      0x01  /* Play single effect */
#define HAPTIC_CMD_PLAY_SEQUENCE    0x02  /* Play effect sequence */
#define HAPTIC_CMD_PLAY_PATTERN     0x03  /* Play predefined pattern */
#define HAPTIC_CMD_STOP             0x04  /* Stop playback */

/* Predefined pattern definitions (effect sequences) */
static const uint8_t pattern_notification[] = {
	DRV2605L_EFFECT_SHARP_CLICK_100
};

static const uint8_t pattern_alert[] = {
	DRV2605L_EFFECT_STRONG_BUZZ_100,
	DRV2605L_EFFECT_STRONG_BUZZ_100
};

static const uint8_t pattern_success[] = {
	DRV2605L_EFFECT_TRANSITION_RAMP_UP_SHORT_SMOOTH_1,
	DRV2605L_EFFECT_STRONG_CLICK_100
};

static const uint8_t pattern_error[] = {
	DRV2605L_EFFECT_STRONG_CLICK_100,
	DRV2605L_EFFECT_STRONG_CLICK_100,
	DRV2605L_EFFECT_STRONG_CLICK_100
};

static const uint8_t pattern_button_press[] = {
	DRV2605L_EFFECT_SHARP_CLICK_60
};

static const uint8_t pattern_long_press[] = {
	DRV2605L_EFFECT_SOFT_BUMP_100,
	DRV2605L_EFFECT_STRONG_CLICK_100
};

static const uint8_t pattern_double_tap[] = {
	DRV2605L_EFFECT_DOUBLE_CLICK_100
};

static const uint8_t pattern_heartbeat[] = {
	DRV2605L_EFFECT_SOFT_BUMP_100,
	DRV2605L_EFFECT_SOFT_BUMP_60
};

static const uint8_t pattern_ramp_up[] = {
	DRV2605L_EFFECT_TRANSITION_RAMP_UP_LONG_SMOOTH_1
};

static const uint8_t pattern_ramp_down[] = {
	DRV2605L_EFFECT_TRANSITION_RAMP_DOWN_LONG_SMOOTH_1
};

static const uint8_t pattern_pulse[] = {
	DRV2605L_EFFECT_PULSING_STRONG_1
};

static const uint8_t pattern_buzz[] = {
	DRV2605L_EFFECT_STRONG_BUZZ_100
};

/* Pattern lookup table */
struct pattern_def {
	const uint8_t *effects;
	uint8_t count;
};

static const struct pattern_def predefined_patterns[] = {
	[HAPTIC_PATTERN_NOTIFICATION] = {pattern_notification, ARRAY_SIZE(pattern_notification)},
	[HAPTIC_PATTERN_ALERT]        = {pattern_alert, ARRAY_SIZE(pattern_alert)},
	[HAPTIC_PATTERN_SUCCESS]      = {pattern_success, ARRAY_SIZE(pattern_success)},
	[HAPTIC_PATTERN_ERROR]        = {pattern_error, ARRAY_SIZE(pattern_error)},
	[HAPTIC_PATTERN_BUTTON_PRESS] = {pattern_button_press, ARRAY_SIZE(pattern_button_press)},
	[HAPTIC_PATTERN_LONG_PRESS]   = {pattern_long_press, ARRAY_SIZE(pattern_long_press)},
	[HAPTIC_PATTERN_DOUBLE_TAP]   = {pattern_double_tap, ARRAY_SIZE(pattern_double_tap)},
	[HAPTIC_PATTERN_HEARTBEAT]    = {pattern_heartbeat, ARRAY_SIZE(pattern_heartbeat)},
	[HAPTIC_PATTERN_RAMP_UP]      = {pattern_ramp_up, ARRAY_SIZE(pattern_ramp_up)},
	[HAPTIC_PATTERN_RAMP_DOWN]    = {pattern_ramp_down, ARRAY_SIZE(pattern_ramp_down)},
	[HAPTIC_PATTERN_PULSE]        = {pattern_pulse, ARRAY_SIZE(pattern_pulse)},
	[HAPTIC_PATTERN_BUZZ]         = {pattern_buzz, ARRAY_SIZE(pattern_buzz)},
};

/**
 * @brief Queue haptic data for processing
 */
static int queue_haptic_data(haptic_pattern_type_t type, const uint8_t *data, uint8_t len)
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
	haptic_data->len = len;
	if (data && len > 0) {
		memcpy(haptic_data->data, data, len);
	}

	k_fifo_put(&fifo_haptic_data, haptic_data);
	LOG_DBG("Queued haptic data (type: %d, len: %d)", type, len);

	return 0;
}

/**
 * @brief Initialize haptic service
 */
int haptic_service_init(void)
{
	int ret;

	LOG_INF("Initializing haptic service...");

	/* Initialize DRV2605L driver with ERM motor type */
	ret = drv2605l_init(DRV2605L_MOTOR_ERM);
	if (ret < 0) {
		LOG_ERR("Failed to initialize DRV2605L driver (err %d)", ret);
		return ret;
	}

	/* Signal initialization complete */
	k_sem_give(&haptic_init_ok);

	LOG_INF("Haptic service initialized");
	return 0;
}

/**
 * @brief Queue a single haptic effect for playback
 */
int haptic_play_effect(uint8_t effect)
{
	if (effect < 1 || effect > 123) {
		LOG_ERR("Invalid effect number: %d", effect);
		return -EINVAL;
	}

	return queue_haptic_data(HAPTIC_PATTERN_SINGLE_EFFECT, &effect, 1);
}

/**
 * @brief Queue a predefined haptic pattern for playback
 */
int haptic_play_pattern(haptic_predefined_pattern_t pattern)
{
	if (pattern >= ARRAY_SIZE(predefined_patterns)) {
		LOG_ERR("Invalid pattern: %d", pattern);
		return -EINVAL;
	}

	const struct pattern_def *pat = &predefined_patterns[pattern];
	return queue_haptic_data(HAPTIC_PATTERN_SEQUENCE, pat->effects, pat->count);
}

/**
 * @brief Queue a custom haptic sequence for playback
 */
int haptic_play_sequence(const uint8_t *effects, uint8_t count)
{
	if (!effects || count == 0) {
		LOG_ERR("Invalid sequence parameters");
		return -EINVAL;
	}

	if (count > HAPTIC_MAX_DATA_SIZE) {
		LOG_WRN("Sequence too long, truncating to %d effects", HAPTIC_MAX_DATA_SIZE);
		count = HAPTIC_MAX_DATA_SIZE;
	}

	/* Validate effect numbers */
	for (uint8_t i = 0; i < count; i++) {
		if (effects[i] < 1 || effects[i] > 123) {
			LOG_ERR("Invalid effect number at index %d: %d", i, effects[i]);
			return -EINVAL;
		}
	}

	return queue_haptic_data(HAPTIC_PATTERN_SEQUENCE, effects, count);
}

/**
 * @brief Process BLE data and convert to haptic pattern
 * 
 * BLE Data Protocol:
 * Byte 0: Command ID
 * 
 * For HAPTIC_CMD_PLAY_EFFECT (0x01):
 *   Byte 1: Effect number (1-123)
 * 
 * For HAPTIC_CMD_PLAY_SEQUENCE (0x02):
 *   Byte 1: Number of effects
 *   Bytes 2-N: Effect numbers
 * 
 * For HAPTIC_CMD_PLAY_PATTERN (0x03):
 *   Byte 1: Pattern ID (0-11)
 * 
 * For HAPTIC_CMD_STOP (0x04):
 *   No additional data
 */
int haptic_process_ble_data(const uint8_t *data, uint16_t len)
{
	if (!data || len < 1) {
		LOG_ERR("Invalid BLE data");
		return -EINVAL;
	}

	uint8_t cmd = data[0];

	LOG_DBG("Processing haptic BLE command: 0x%02X", cmd);

	switch (cmd) {
	case HAPTIC_CMD_PLAY_EFFECT:
		if (len < 2) {
			LOG_ERR("PLAY_EFFECT: insufficient data");
			return -EINVAL;
		}
		return haptic_play_effect(data[1]);

	case HAPTIC_CMD_PLAY_SEQUENCE:
		if (len < 2) {
			LOG_ERR("PLAY_SEQUENCE: insufficient data");
			return -EINVAL;
		}
		uint8_t count = data[1];
		if (len < 2 + count) {
			LOG_ERR("PLAY_SEQUENCE: data length mismatch");
			return -EINVAL;
		}
		return haptic_play_sequence(&data[2], count);

	case HAPTIC_CMD_PLAY_PATTERN:
		if (len < 2) {
			LOG_ERR("PLAY_PATTERN: insufficient data");
			return -EINVAL;
		}
		return haptic_play_pattern((haptic_predefined_pattern_t)data[1]);

	case HAPTIC_CMD_STOP:
		return haptic_stop();

	default:
		LOG_WRN("Unknown haptic command: 0x%02X", cmd);
		return -ENOTSUP;
	}
}

/**
 * @brief Stop current haptic playback
 */
int haptic_stop(void)
{
	return queue_haptic_data(HAPTIC_PATTERN_STOP, NULL, 0);
}

/**
 * @brief Get haptic data from FIFO queue
 */
struct haptic_data_t *haptic_get_queued_data(void)
{
	return k_fifo_get(&fifo_haptic_data, K_FOREVER);
}

/**
 * @brief Wait for haptic service initialization to complete
 */
void haptic_wait_init(void)
{
	k_sem_take(&haptic_init_ok, K_FOREVER);
}

/**
 * @brief Signal that haptic service initialization is complete
 */
void haptic_signal_init_complete(void)
{
	k_sem_give(&haptic_init_ok);
}


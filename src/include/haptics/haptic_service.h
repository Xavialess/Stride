/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HAPTIC_SERVICE_H
#define HAPTIC_SERVICE_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>

/**
 * @file haptic_service.h
 * @brief Haptic feedback service for converting BLE data to haptic patterns
 *
 * This service provides a high-level interface for haptic feedback control.
 * It receives data from BLE and converts it into haptic patterns that are
 * queued for playback by the haptic thread.
 *
 * Supports dual-motor output via an ADG884 analog mux controlled by GPIO.
 * Each pattern step can target the left motor, right motor, or both.
 */

/* Motor target enum is the single source of truth in motor_mux.h */
#include "../../drivers/haptics/motor_mux.h"

/* Maximum haptic data size */
#define HAPTIC_MAX_DATA_SIZE    32

/* Maximum steps in a multi-step pattern */
#define HAPTIC_MAX_MULTI_STEPS  8

/* Haptic pattern types */
typedef enum {
	HAPTIC_PATTERN_SINGLE_EFFECT,    /* Single effect playback */
	HAPTIC_PATTERN_SEQUENCE,         /* Sequence of effects */
	HAPTIC_PATTERN_MULTI_STEP,       /* Multi-step pattern with per-step motor targeting */
	HAPTIC_PATTERN_STOP,             /* Stop current playback */
} haptic_pattern_type_t;

/* Haptic data structure for FIFO queue */
struct haptic_data_t {
	void *fifo_reserved;
	haptic_pattern_type_t type;
	motor_target_t target;
	uint8_t data[HAPTIC_MAX_DATA_SIZE];
	uint8_t len;
};

/* One step of a multi-step pattern played by the haptic thread */
struct haptic_pattern_step {
	motor_target_t target;
	const uint8_t *effects;
	uint8_t count;
	uint16_t post_delay_ms;
};

/* A complete multi-step pattern definition */
struct haptic_pattern_def {
	const struct haptic_pattern_step *steps;
	uint8_t step_count;
};

/* Predefined haptic patterns */
typedef enum {
	HAPTIC_PATTERN_NOTIFICATION,
	HAPTIC_PATTERN_ALERT,
	HAPTIC_PATTERN_SUCCESS,
	HAPTIC_PATTERN_ERROR,
	HAPTIC_PATTERN_BUTTON_PRESS,
	HAPTIC_PATTERN_LONG_PRESS,
	HAPTIC_PATTERN_DOUBLE_TAP,
	HAPTIC_PATTERN_HEARTBEAT,
	HAPTIC_PATTERN_RAMP_UP,
	HAPTIC_PATTERN_RAMP_DOWN,
	HAPTIC_PATTERN_PULSE,
	HAPTIC_PATTERN_BUZZ,
	/* Navigation patterns */
	HAPTIC_PATTERN_NAV_START,
	HAPTIC_PATTERN_TURN_RIGHT,
	HAPTIC_PATTERN_TURN_LEFT,
	HAPTIC_PATTERN_NAV_STOP,
	HAPTIC_PATTERN_NAV_END,
	/* Notification patterns */
	HAPTIC_PATTERN_BLE_PAIRED,
	/* Battery alert patterns */
	HAPTIC_PATTERN_LOW_BATTERY,
	HAPTIC_PATTERN_CRITICAL_BATTERY,
	/* Spare patterns for future use */
	HAPTIC_PATTERN_SPARE_A,
	HAPTIC_PATTERN_SPARE_B,
	HAPTIC_PATTERN_SPARE_C,
	HAPTIC_PREDEFINED_COUNT,
} haptic_predefined_pattern_t;

/**
 * @brief Initialize haptic service
 *
 * Initializes the DRV2605L driver, motor mux GPIOs, and the FIFO queue.
 *
 * @return 0 on success, negative errno on failure
 */
int haptic_service_init(void);

/**
 * @brief Queue a single haptic effect for playback on the specified motor(s)
 *
 * @param effect Effect number (1-123)
 * @param target Motor target (LEFT, RIGHT, BOTH)
 * @return 0 on success, negative errno on failure
 */
int haptic_play_effect_on(uint8_t effect, motor_target_t target);

/**
 * @brief Queue a single haptic effect for playback on both motors
 *
 * @param effect Effect number (1-123)
 * @return 0 on success, negative errno on failure
 */
int haptic_play_effect(uint8_t effect);

/**
 * @brief Queue a predefined haptic pattern for playback
 *
 * Motor targeting is defined within the pattern itself.
 *
 * @param pattern Predefined pattern to play
 * @return 0 on success, negative errno on failure
 */
int haptic_play_pattern(haptic_predefined_pattern_t pattern);

/**
 * @brief Queue a custom haptic sequence for playback on the specified motor(s)
 *
 * @param effects Array of effect numbers (1-123)
 * @param count Number of effects in sequence
 * @param target Motor target (LEFT, RIGHT, BOTH)
 * @return 0 on success, negative errno on failure
 */
int haptic_play_sequence_on(const uint8_t *effects, uint8_t count,
			    motor_target_t target);

/**
 * @brief Queue a custom haptic sequence for playback on both motors
 *
 * @param effects Array of effect numbers (1-123)
 * @param count Number of effects in sequence
 * @return 0 on success, negative errno on failure
 */
int haptic_play_sequence(const uint8_t *effects, uint8_t count);

/**
 * @brief Process BLE data and convert to haptic pattern
 *
 * @param data BLE data buffer
 * @param len Length of BLE data
 * @return 0 on success, negative errno on failure
 */
int haptic_process_ble_data(const uint8_t *data, uint16_t len);

/**
 * @brief Stop current haptic playback and disconnect motors
 *
 * @return 0 on success, negative errno on failure
 */
int haptic_stop(void);

/**
 * @brief Get haptic data from FIFO queue (for haptic thread)
 *
 * This function blocks until data is available in the queue.
 *
 * @return Pointer to haptic data structure
 */
struct haptic_data_t *haptic_get_queued_data(void);

/**
 * @brief Get the predefined pattern table (for haptic thread multi-step playback)
 *
 * @param pattern Pattern index
 * @return Pointer to pattern definition, or NULL if invalid
 */
const struct haptic_pattern_def *haptic_get_pattern_def(haptic_predefined_pattern_t pattern);

/**
 * @brief Wait for haptic service initialization to complete
 */
void haptic_wait_init(void);

/**
 * @brief Signal that haptic service initialization is complete
 */
void haptic_signal_init_complete(void);

#endif /* HAPTIC_SERVICE_H */

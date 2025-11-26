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
 */

/* Maximum haptic data size */
#define HAPTIC_MAX_DATA_SIZE    32

/* Haptic pattern types */
typedef enum {
	HAPTIC_PATTERN_SINGLE_EFFECT,    /* Single effect playback */
	HAPTIC_PATTERN_SEQUENCE,         /* Sequence of effects */
	HAPTIC_PATTERN_CUSTOM,           /* Custom pattern from BLE data */
	HAPTIC_PATTERN_STOP,             /* Stop current playback */
} haptic_pattern_type_t;

/* Haptic data structure for FIFO queue */
struct haptic_data_t {
	void *fifo_reserved;              /* Required for FIFO */
	haptic_pattern_type_t type;       /* Pattern type */
	uint8_t data[HAPTIC_MAX_DATA_SIZE]; /* Pattern data */
	uint8_t len;                      /* Data length */
};

/* Predefined haptic patterns */
typedef enum {
	HAPTIC_PATTERN_NOTIFICATION,      /* Short notification buzz */
	HAPTIC_PATTERN_ALERT,             /* Alert pattern */
	HAPTIC_PATTERN_SUCCESS,           /* Success confirmation */
	HAPTIC_PATTERN_ERROR,             /* Error indication */
	HAPTIC_PATTERN_BUTTON_PRESS,      /* Button press feedback */
	HAPTIC_PATTERN_LONG_PRESS,        /* Long press feedback */
	HAPTIC_PATTERN_DOUBLE_TAP,        /* Double tap feedback */
	HAPTIC_PATTERN_HEARTBEAT,         /* Heartbeat pattern */
	HAPTIC_PATTERN_RAMP_UP,           /* Gradual increase */
	HAPTIC_PATTERN_RAMP_DOWN,         /* Gradual decrease */
	HAPTIC_PATTERN_PULSE,             /* Pulsing pattern */
	HAPTIC_PATTERN_BUZZ,              /* Continuous buzz */
} haptic_predefined_pattern_t;

/**
 * @brief Initialize haptic service
 * 
 * Initializes the DRV2605L driver and sets up the haptic FIFO queue.
 * 
 * @return 0 on success, negative errno on failure
 */
int haptic_service_init(void);

/**
 * @brief Queue a single haptic effect for playback
 * 
 * @param effect Effect number (1-123)
 * @return 0 on success, negative errno on failure
 */
int haptic_play_effect(uint8_t effect);

/**
 * @brief Queue a predefined haptic pattern for playback
 * 
 * @param pattern Predefined pattern to play
 * @return 0 on success, negative errno on failure
 */
int haptic_play_pattern(haptic_predefined_pattern_t pattern);

/**
 * @brief Queue a custom haptic sequence for playback
 * 
 * @param effects Array of effect numbers (1-123)
 * @param count Number of effects in sequence
 * @return 0 on success, negative errno on failure
 */
int haptic_play_sequence(const uint8_t *effects, uint8_t count);

/**
 * @brief Process BLE data and convert to haptic pattern
 * 
 * This function is called from the BLE data received callback.
 * It parses the BLE data and queues appropriate haptic patterns.
 * 
 * @param data BLE data buffer
 * @param len Length of BLE data
 * @return 0 on success, negative errno on failure
 */
int haptic_process_ble_data(const uint8_t *data, uint16_t len);

/**
 * @brief Stop current haptic playback
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
 * @brief Wait for haptic service initialization to complete
 * 
 * This should be called by the haptic thread before processing data.
 */
void haptic_wait_init(void);

/**
 * @brief Signal that haptic service initialization is complete
 */
void haptic_signal_init_complete(void);

#endif /* HAPTIC_SERVICE_H */


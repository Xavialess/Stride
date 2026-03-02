/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOTOR_MUX_H
#define MOTOR_MUX_H

#include <zephyr/types.h>

/**
 * @file motor_mux.h
 * @brief ADG884 differential-pair motor mux driver
 *
 * The DRV2605L drives a differential pair (OUT+, OUT-).  The ADG884 has two
 * SPDT channels that switch this pair as a unit:
 *
 *   Channel 1: D1 = OUT+   ->  S1A (motor 1+) or S1B (motor 2+)
 *   Channel 2: D2 = OUT-   ->  S2A (motor 1-) or S2B (motor 2-)
 *
 * Both IN1 and IN2 are tied to the same GPIO logic level so the full
 * differential signal always reaches the same motor:
 *
 *   IN = LOW  -> A-side -> Motor 1 (left)
 *   IN = HIGH -> B-side -> Motor 2 (right)
 *
 * Only one motor can be driven at a time.  MOTOR_BOTH is provided as a
 * logical target for patterns; the playback thread implements it via
 * time-division (play on left, then replay on right).
 */

typedef enum {
	MOTOR_NONE  = 0x00,
	MOTOR_LEFT  = 0x01,
	MOTOR_RIGHT = 0x02,
	MOTOR_BOTH  = 0x03,
} motor_target_t;

/**
 * @brief Initialize motor mux GPIO pins
 *
 * Configures IN1 and IN2 as outputs, both LOW (motor 1 / left selected).
 *
 * @return 0 on success, negative errno on failure
 */
int motor_mux_init(void);

/**
 * @brief Route the DRV2605L differential output to the selected motor
 *
 * Both IN pins are always driven to the same level.
 * MOTOR_LEFT  -> LOW  (A-side, motor 1)
 * MOTOR_RIGHT -> HIGH (B-side, motor 2)
 * MOTOR_NONE  -> LOW  (defaults to motor 1, but caller should not play)
 *
 * MOTOR_BOTH is not valid here; the caller must time-multiplex.
 *
 * @param target MOTOR_LEFT or MOTOR_RIGHT
 * @return 0 on success, negative errno on failure
 */
int motor_mux_select(motor_target_t target);

#endif /* MOTOR_MUX_H */

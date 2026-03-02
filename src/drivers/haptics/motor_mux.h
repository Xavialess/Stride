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
 * @brief ADG884 dual-SPDT motor mux driver
 *
 * Controls two GPIO lines (D1, D2) that gate the single DRV2605L output
 * to independent left and right ERM motors via an ADG884 analog switch.
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
 * Configures D1 and D2 as outputs, both LOW (motors disconnected).
 *
 * @return 0 on success, negative errno on failure
 */
int motor_mux_init(void);

/**
 * @brief Select which motor(s) receive the DRV2605L output
 *
 * @param target MOTOR_LEFT, MOTOR_RIGHT, MOTOR_BOTH, or MOTOR_NONE
 * @return 0 on success, negative errno on failure
 */
int motor_mux_select(motor_target_t target);

#endif /* MOTOR_MUX_H */

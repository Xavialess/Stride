/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#ifndef POWER_MGMT_H
#define POWER_MGMT_H

#include <zephyr/types.h>

/**
 * @brief Power states
 */
enum power_state {
	POWER_STATE_ACTIVE,
	POWER_STATE_IDLE,
	POWER_STATE_SLEEP,
	POWER_STATE_DEEP_SLEEP
};

/**
 * @brief Initialize power management subsystem
 * 
 * @return 0 on success, negative errno on failure
 */
int power_mgmt_init(void);

/**
 * @brief Request a power state transition
 * 
 * @param state Desired power state
 * @return 0 on success, negative errno on failure
 */
int power_mgmt_request_state(enum power_state state);

/**
 * @brief Get current power state
 * 
 * @return Current power state
 */
enum power_state power_mgmt_get_state(void);

/**
 * @brief Notify power manager of activity
 * 
 * Resets idle timer and prevents sleep transitions.
 */
void power_mgmt_activity(void);

/**
 * @brief Perform a clean system shutdown and enter System OFF
 *
 * Stops haptics, disconnects BLE, turns off LEDs, then calls sys_poweroff().
 * The chip enters System OFF (~0.3 µA). The power button (D3/P0.29) is
 * configured as a wakeup source — pressing it triggers a full hardware reset
 * and main() restarts from scratch.
 *
 * This function never returns.
 */
void power_mgmt_shutdown(void);

#endif /* POWER_MGMT_H */


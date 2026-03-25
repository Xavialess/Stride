/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#include <power/power_mgmt.h>
#include <battery/battery_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(power_mgmt, LOG_LEVEL_DBG);

static enum power_state current_state = POWER_STATE_IDLE;
static int64_t last_activity_time;

/**
 * @brief Initialize power management subsystem
 */
int power_mgmt_init(void)
{
	last_activity_time = k_uptime_get();
	current_state = POWER_STATE_IDLE;

	LOG_INF("Power management initialized (idle)");
	return 0;
}

/**
 * @brief Request a power state transition
 */
int power_mgmt_request_state(enum power_state state)
{
	if (state == current_state) {
		return 0;
	}

	LOG_INF("Power state transition: %d -> %d", current_state, state);

	if (state == POWER_STATE_IDLE) {
		battery_service_set_idle(true);
		LOG_INF("Entered idle: battery polling slowed");
	} else if (state == POWER_STATE_ACTIVE) {
		battery_service_set_idle(false);
		LOG_INF("Entered active: battery polling restored");
	}

	current_state = state;
	return 0;
}

/**
 * @brief Get current power state
 */
enum power_state power_mgmt_get_state(void)
{
	return current_state;
}

/**
 * @brief Notify power manager of activity
 */
void power_mgmt_activity(void)
{
	last_activity_time = k_uptime_get();

	if (current_state != POWER_STATE_ACTIVE) {
		power_mgmt_request_state(POWER_STATE_ACTIVE);
	}
}


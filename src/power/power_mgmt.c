/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#include <power/power_mgmt.h>
#include <battery/battery_service.h>
#include <ble/ble_service.h>
#include <haptics/haptic_service.h>
#include <gpio/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>

/* P0.29 = D3 on XIAO BLE Sense — power button wakeup pin */
#define PWR_BTN_PIN  NRF_GPIO_PIN_MAP(0, 29)

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

/**
 * @brief Perform a clean system shutdown and enter System OFF
 */
void power_mgmt_shutdown(void)
{
	LOG_INF("Shutting down...");

	haptic_stop();
	ble_service_stop();
	gpio_set_led(LED_RUN_STATUS, false);
	gpio_set_led(LED_CON_STATUS, false);

	/* Give BLE stack time to send the disconnect event to the peer */
	k_sleep(K_MSEC(200));

	LOG_INF("Entering System OFF");

	/*
	 * Manually arm the PORT event sense on P0.29 for wakeup from System OFF.
	 *
	 * sys_poweroff() does not automatically configure GPIO sense from the
	 * devicetree wakeup-source property on nRF52840. We must:
	 *   1. Disable the GPIOTE PORT interrupt (prevents spurious wakeup during config)
	 *   2. Configure the pin as level-sense input with pull-up, sense LOW
	 *      (button press pulls the pin LOW → wakeup)
	 *   3. Clear any pending PORT event
	 *   4. Re-enable the GPIOTE PORT interrupt
	 *
	 * This follows the nRF52840 datasheet §6.10.2 "Port event" sequence.
	 */
	nrf_gpiote_int_disable(NRF_GPIOTE, NRF_GPIOTE_INT_PORT_MASK);
	nrf_gpio_cfg_sense_input(PWR_BTN_PIN,
				 NRF_GPIO_PIN_PULLUP,
				 NRF_GPIO_PIN_SENSE_LOW);
	nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_PORT);
	nrf_gpiote_int_enable(NRF_GPIOTE, NRF_GPIOTE_INT_PORT_MASK);

	/* Enter System OFF — never returns. Wake source: power button on D3 (P0.29) */
	sys_poweroff();
}


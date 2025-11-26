/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#include <gpio/gpio.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio, LOG_LEVEL_DBG);

/* Forward declarations */
#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
extern void ble_confirm_passkey(bool accept);
static void button_changed(uint32_t button_state, uint32_t has_changed);
#endif

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
/**
 * @brief Button change callback
 */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (buttons & BTN_PASSKEY_ACCEPT) {
		ble_confirm_passkey(true);
	}

	if (buttons & BTN_PASSKEY_REJECT) {
		ble_confirm_passkey(false);
	}
}
#endif

/**
 * @brief Initialize GPIO (LEDs and buttons)
 */
int gpio_init(void)
{
	int err;

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
		return err;
	}
#endif

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
		return err;
	}

	LOG_INF("GPIO initialized");
	return 0;
}

/**
 * @brief Set LED state
 */
void gpio_set_led(uint32_t led_idx, bool state)
{
	if (state) {
		dk_set_led_on(led_idx);
	} else {
		dk_set_led_off(led_idx);
	}
}

/**
 * @brief Toggle LED state
 */
void gpio_toggle_led(uint32_t led_idx, uint32_t state)
{
	dk_set_led(led_idx, state);
}

/**
 * @brief Enter error state (all LEDs off, infinite loop)
 */
void gpio_error_state(void)
{
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin forever */
		k_sleep(K_MSEC(1000));
	}
}


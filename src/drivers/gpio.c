/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#include <gpio/gpio.h>
#include <power/power_mgmt.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio, LOG_LEVEL_DBG);

/* Power button — D3 (P0.29), defined in app.overlay as power_button/pwr_btn */
#define PWR_BTN_NODE DT_NODELABEL(pwr_btn)
static const struct gpio_dt_spec power_btn = GPIO_DT_SPEC_GET(PWR_BTN_NODE, gpios);
static struct gpio_callback power_btn_cb;

/*
 * Shutdown work item — defers power_mgmt_shutdown() out of ISR context.
 * k_sleep(), bt_disable(), and sys_poweroff() must not be called from an
 * interrupt handler; submitting a work item moves execution to the system
 * workqueue thread where blocking calls are allowed.
 */
static struct k_work shutdown_work;

static void shutdown_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	power_mgmt_shutdown();
}

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
 * @brief Power button ISR — schedules shutdown on the system workqueue
 *
 * Must not call blocking functions directly; submits work instead.
 */
static void power_btn_isr(const struct device *dev, struct gpio_callback *cb,
			  uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_work_submit(&shutdown_work);
}

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

	/* Configure power button (active-low, pull-up already set in devicetree) */
	if (!gpio_is_ready_dt(&power_btn)) {
		LOG_ERR("Power button GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&power_btn, GPIO_INPUT);
	if (err) {
		LOG_ERR("Cannot configure power button pin (err: %d)", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&power_btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("Cannot configure power button interrupt (err: %d)", err);
		return err;
	}

	gpio_init_callback(&power_btn_cb, power_btn_isr, BIT(power_btn.pin));
	gpio_add_callback(power_btn.port, &power_btn_cb);

	k_work_init(&shutdown_work, shutdown_work_handler);

	LOG_INF("GPIO initialized (power button on D3/P0.29)");
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


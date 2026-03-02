/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "motor_mux.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_mux, LOG_LEVEL_DBG);

#define MUX_LEFT_NODE  DT_NODELABEL(mux_left)
#define MUX_RIGHT_NODE DT_NODELABEL(mux_right)

static const struct gpio_dt_spec mux_left  = GPIO_DT_SPEC_GET(MUX_LEFT_NODE, gpios);
static const struct gpio_dt_spec mux_right = GPIO_DT_SPEC_GET(MUX_RIGHT_NODE, gpios);

static bool mux_initialized;

int motor_mux_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&mux_left)) {
		LOG_ERR("Left mux GPIO device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&mux_right)) {
		LOG_ERR("Right mux GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&mux_left, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure left mux GPIO (err %d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&mux_right, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure right mux GPIO (err %d)", ret);
		return ret;
	}

	mux_initialized = true;
	LOG_INF("Motor mux initialized (left=D1/P0.03, right=D2/P0.28)");
	return 0;
}

int motor_mux_select(motor_target_t target)
{
	int ret;

	if (!mux_initialized) {
		LOG_ERR("Motor mux not initialized");
		return -ENODEV;
	}

	ret = gpio_pin_set_dt(&mux_left, (target & MOTOR_LEFT) ? 1 : 0);
	if (ret < 0) {
		LOG_ERR("Failed to set left mux GPIO (err %d)", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&mux_right, (target & MOTOR_RIGHT) ? 1 : 0);
	if (ret < 0) {
		LOG_ERR("Failed to set right mux GPIO (err %d)", ret);
		return ret;
	}

	LOG_DBG("Motor mux: left=%s right=%s",
		(target & MOTOR_LEFT)  ? "ON" : "OFF",
		(target & MOTOR_RIGHT) ? "ON" : "OFF");

	return 0;
}

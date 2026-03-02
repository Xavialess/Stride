/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "motor_mux.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_mux, LOG_LEVEL_DBG);

/*
 * Both devicetree nodes drive the same logical level.
 * mux_left  = IN1 (channel 1, switches OUT+)
 * mux_right = IN2 (channel 2, switches OUT-)
 */
#define MUX_IN1_NODE DT_NODELABEL(mux_left)
#define MUX_IN2_NODE DT_NODELABEL(mux_right)

static const struct gpio_dt_spec mux_in1 = GPIO_DT_SPEC_GET(MUX_IN1_NODE, gpios);
static const struct gpio_dt_spec mux_in2 = GPIO_DT_SPEC_GET(MUX_IN2_NODE, gpios);

static bool mux_initialized;

int motor_mux_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&mux_in1)) {
		LOG_ERR("Mux IN1 GPIO device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&mux_in2)) {
		LOG_ERR("Mux IN2 GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&mux_in1, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure IN1 GPIO (err %d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&mux_in2, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure IN2 GPIO (err %d)", ret);
		return ret;
	}

	mux_initialized = true;
	LOG_INF("Motor mux initialized (IN=LOW -> motor 1/left, IN=HIGH -> motor 2/right)");
	return 0;
}

int motor_mux_select(motor_target_t target)
{
	int ret;
	int level;

	if (!mux_initialized) {
		LOG_ERR("Motor mux not initialized");
		return -ENODEV;
	}

	switch (target) {
	case MOTOR_LEFT:
	case MOTOR_NONE:
		level = 0;
		break;
	case MOTOR_RIGHT:
		level = 1;
		break;
	default:
		LOG_ERR("Invalid mux target 0x%02X (use MOTOR_LEFT or MOTOR_RIGHT)", target);
		return -EINVAL;
	}

	ret = gpio_pin_set_dt(&mux_in1, level);
	if (ret < 0) {
		LOG_ERR("Failed to set IN1 (err %d)", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&mux_in2, level);
	if (ret < 0) {
		LOG_ERR("Failed to set IN2 (err %d)", ret);
		return ret;
	}

	LOG_DBG("Motor mux -> %s (IN1=IN2=%d)",
		(level == 0) ? "LEFT/motor1" : "RIGHT/motor2", level);

	return 0;
}

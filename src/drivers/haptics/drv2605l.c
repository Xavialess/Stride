/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "drv2605l.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(drv2605l, LOG_LEVEL_DBG);

/* I2C device configuration */
#define DRV2605L_I2C_NODE DT_NODELABEL(drv2605l)

#if DT_NODE_HAS_STATUS(DRV2605L_I2C_NODE, okay)
static const struct i2c_dt_spec drv2605l_i2c = I2C_DT_SPEC_GET(DRV2605L_I2C_NODE);
#else
#error "DRV2605L device tree node not found or not enabled"
#endif

/* Driver state */
static bool initialized = false;
static drv2605l_motor_type_t motor_type;

/**
 * @brief Write a single byte to a DRV2605L register
 */
static int drv2605l_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = {reg, value};
	int ret;

	ret = i2c_write_dt(&drv2605l_i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%02X (err %d)", reg, ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Read a single byte from a DRV2605L register
 */
static int drv2605l_read_reg(uint8_t reg, uint8_t *value)
{
	int ret;

	ret = i2c_write_read_dt(&drv2605l_i2c, &reg, 1, value, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%02X (err %d)", reg, ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Initialize DRV2605L haptic driver
 */
int drv2605l_init(drv2605l_motor_type_t type)
{
	int ret;
	uint8_t status;

	if (!device_is_ready(drv2605l_i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	motor_type = type;

	/* Read status register to verify communication */
	ret = drv2605l_read_reg(DRV2605L_REG_STATUS, &status);
	if (ret < 0) {
		LOG_ERR("Failed to communicate with DRV2605L");
		return ret;
	}

	LOG_INF("DRV2605L status: 0x%02X", status);

	/* Exit standby mode */
	ret = drv2605l_write_reg(DRV2605L_REG_MODE, DRV2605L_MODE_INTTRIG);
	if (ret < 0) {
		return ret;
	}

	/* Select library based on motor type */
	uint8_t library = (type == DRV2605L_MOTOR_LRA) ? 
	                  DRV2605L_LIB_LRA : DRV2605L_LIB_ERM;
	ret = drv2605l_write_reg(DRV2605L_REG_LIBRARY, library);
	if (ret < 0) {
		return ret;
	}

	/* Configure feedback control for motor type */
	uint8_t feedback = (type == DRV2605L_MOTOR_LRA) ? 0x80 : 0x00;
	ret = drv2605l_write_reg(DRV2605L_REG_FEEDBACK, feedback);
	if (ret < 0) {
		return ret;
	}

	/* Set rated voltage and overdrive clamp (default values for ERM) */
	if (type == DRV2605L_MOTOR_ERM) {
		/* Rated voltage: 3V ERM typical */
		ret = drv2605l_write_reg(DRV2605L_REG_RATEDV, 0x90);
		if (ret < 0) {
			return ret;
		}

		/* Overdrive clamp voltage */
		ret = drv2605l_write_reg(DRV2605L_REG_CLAMPV, 0xFF);
		if (ret < 0) {
			return ret;
		}
	}

	/* Configure control registers */
	/* CONTROL1: Drive time for ERM (default) */
	ret = drv2605l_write_reg(DRV2605L_REG_CONTROL1, 0x93);
	if (ret < 0) {
		return ret;
	}

	/* CONTROL2: Bidirectional input, unidirectional output */
	ret = drv2605l_write_reg(DRV2605L_REG_CONTROL2, 0xF5);
	if (ret < 0) {
		return ret;
	}

	/* CONTROL3: ERM open loop, NG threshold */
	ret = drv2605l_write_reg(DRV2605L_REG_CONTROL3, 0xA0);
	if (ret < 0) {
		return ret;
	}

	initialized = true;
	LOG_INF("DRV2605L initialized (motor type: %s)", 
	        type == DRV2605L_MOTOR_LRA ? "LRA" : "ERM");

	return 0;
}

/**
 * @brief Play a single haptic effect
 */
int drv2605l_play_effect(uint8_t effect)
{
	int ret;

	if (!initialized) {
		LOG_ERR("DRV2605L not initialized");
		return -ENODEV;
	}

	if (effect < 1 || effect > 123) {
		LOG_ERR("Invalid effect number: %d (must be 1-123)", effect);
		return -EINVAL;
	}

	/* Set the waveform in sequence register 1 */
	ret = drv2605l_write_reg(DRV2605L_REG_WAVESEQ1, effect);
	if (ret < 0) {
		return ret;
	}

	/* Terminate sequence */
	ret = drv2605l_write_reg(DRV2605L_REG_WAVESEQ2, 0x00);
	if (ret < 0) {
		return ret;
	}

	/* Trigger playback */
	ret = drv2605l_write_reg(DRV2605L_REG_GO, 0x01);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Playing effect %d", effect);
	return 0;
}

/**
 * @brief Play a sequence of haptic effects
 */
int drv2605l_play_sequence(const uint8_t *effects, uint8_t count)
{
	int ret;

	if (!initialized) {
		LOG_ERR("DRV2605L not initialized");
		return -ENODEV;
	}

	if (!effects || count == 0) {
		LOG_ERR("Invalid effects sequence");
		return -EINVAL;
	}

	if (count > DRV2605L_MAX_WAVEFORM_SEQ) {
		LOG_WRN("Sequence too long, truncating to %d effects", 
		        DRV2605L_MAX_WAVEFORM_SEQ);
		count = DRV2605L_MAX_WAVEFORM_SEQ;
	}

	/* Program waveform sequence */
	for (uint8_t i = 0; i < count; i++) {
		if (effects[i] < 1 || effects[i] > 123) {
			LOG_ERR("Invalid effect number at index %d: %d", i, effects[i]);
			return -EINVAL;
		}

		ret = drv2605l_write_reg(DRV2605L_REG_WAVESEQ1 + i, effects[i]);
		if (ret < 0) {
			return ret;
		}
	}

	/* Terminate sequence */
	if (count < DRV2605L_MAX_WAVEFORM_SEQ) {
		ret = drv2605l_write_reg(DRV2605L_REG_WAVESEQ1 + count, 0x00);
		if (ret < 0) {
			return ret;
		}
	}

	/* Trigger playback */
	ret = drv2605l_write_reg(DRV2605L_REG_GO, 0x01);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Playing sequence of %d effects", count);
	return 0;
}

/**
 * @brief Stop haptic playback
 */
int drv2605l_stop(void)
{
	int ret;

	if (!initialized) {
		return -ENODEV;
	}

	/* Clear GO bit */
	ret = drv2605l_write_reg(DRV2605L_REG_GO, 0x00);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Stopped playback");
	return 0;
}

/**
 * @brief Set motor to standby mode
 */
int drv2605l_standby(void)
{
	int ret;

	if (!initialized) {
		return -ENODEV;
	}

	ret = drv2605l_write_reg(DRV2605L_REG_MODE, DRV2605L_MODE_STANDBY);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Entered standby mode");
	return 0;
}

/**
 * @brief Wake motor from standby
 */
int drv2605l_wakeup(void)
{
	int ret;

	if (!initialized) {
		return -ENODEV;
	}

	ret = drv2605l_write_reg(DRV2605L_REG_MODE, DRV2605L_MODE_INTTRIG);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Woke from standby");
	return 0;
}

/**
 * @brief Check if motor is currently playing
 */
bool drv2605l_is_playing(void)
{
	uint8_t go_bit;
	int ret;

	if (!initialized) {
		return false;
	}

	ret = drv2605l_read_reg(DRV2605L_REG_GO, &go_bit);
	if (ret < 0) {
		return false;
	}

	return (go_bit & 0x01) != 0;
}

/**
 * @brief Perform auto-calibration (for LRA motors)
 */
int drv2605l_auto_calibrate(void)
{
	int ret;
	uint8_t status;
	int timeout = 100; /* 1 second timeout */

	if (!initialized) {
		LOG_ERR("DRV2605L not initialized");
		return -ENODEV;
	}

	if (motor_type != DRV2605L_MOTOR_LRA) {
		LOG_WRN("Auto-calibration is only for LRA motors");
		return -ENOTSUP;
	}

	LOG_INF("Starting auto-calibration...");

	/* Set to auto-calibration mode */
	ret = drv2605l_write_reg(DRV2605L_REG_MODE, DRV2605L_MODE_AUTOCAL);
	if (ret < 0) {
		return ret;
	}

	/* Trigger calibration */
	ret = drv2605l_write_reg(DRV2605L_REG_GO, 0x01);
	if (ret < 0) {
		return ret;
	}

	/* Wait for calibration to complete */
	while (timeout-- > 0) {
		k_sleep(K_MSEC(10));

		ret = drv2605l_read_reg(DRV2605L_REG_GO, &status);
		if (ret < 0) {
			return ret;
		}

		if ((status & 0x01) == 0) {
			/* Calibration complete */
			break;
		}
	}

	if (timeout <= 0) {
		LOG_ERR("Auto-calibration timeout");
		return -ETIMEDOUT;
	}

	/* Check calibration result */
	ret = drv2605l_read_reg(DRV2605L_REG_STATUS, &status);
	if (ret < 0) {
		return ret;
	}

	if (status & 0x08) {
		LOG_ERR("Auto-calibration failed (DIAG bit set)");
		return -EIO;
	}

	/* Return to internal trigger mode */
	ret = drv2605l_write_reg(DRV2605L_REG_MODE, DRV2605L_MODE_INTTRIG);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("Auto-calibration completed successfully");
	return 0;
}


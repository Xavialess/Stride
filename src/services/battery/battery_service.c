/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <battery/battery_service.h>
#include <ble/stride_service.h>
#include <haptics/haptic_service.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/voltage_divider.h>

LOG_MODULE_REGISTER(battery_service, LOG_LEVEL_INF);

#define BATTERY_MEASURE_INTERVAL_SEC       60
#define BATTERY_MEASURE_INTERVAL_IDLE_SEC  300
#define BATTERY_THRESHOLD_LOW         15
#define BATTERY_THRESHOLD_CRITICAL     5

#define VBATT_NODE DT_NODELABEL(vbatt)

static const struct voltage_divider_dt_spec vbatt =
	VOLTAGE_DIVIDER_DT_SPEC_GET(VBATT_NODE);

/* LiPo discharge curve for direct BAT+ measurement (no divider).
 * Capped at 3600 mV (ADC_GAIN_1_6 saturation) = 100%.
 * Readings above 3600 mV are clamped to 100% before reaching this table. */
static const struct {
	int32_t mv;
	uint8_t pct;
} lipo_curve[] = {
	{ 3600, 100 },
	{ 3550,  90 },
	{ 3500,  80 },
	{ 3450,  70 },
	{ 3400,  60 },
	{ 3350,  50 },
	{ 3300,  40 },
	{ 3250,  30 },
	{ 3200,  20 },
	{ 3100,  10 },
	{ 3000,   0 },
};

static uint8_t mv_to_percent(int32_t mv)
{
	if (mv >= lipo_curve[0].mv) {
		return 100;
	}
	for (size_t i = 1; i < ARRAY_SIZE(lipo_curve); i++) {
		if (mv >= lipo_curve[i].mv) {
			int32_t range_mv  = lipo_curve[i - 1].mv - lipo_curve[i].mv;
			int32_t range_pct = lipo_curve[i - 1].pct - lipo_curve[i].pct;
			int32_t offset_mv = mv - lipo_curve[i].mv;

			return lipo_curve[i].pct +
			       (uint8_t)((offset_mv * range_pct) / range_mv);
		}
	}
	return 0;
}

static uint8_t battery_measure(void)
{
	uint16_t raw;
	int32_t val_mv;
	int err;

	struct adc_sequence seq = {
		.buffer      = &raw,
		.buffer_size = sizeof(raw),
	};

	err = adc_sequence_init_dt(&vbatt.port, &seq);
	if (err < 0) {
		LOG_ERR("ADC sequence init failed (%d)", err);
		return 0;
	}

	err = adc_read_dt(&vbatt.port, &seq);
	if (err < 0) {
		LOG_ERR("ADC read failed (%d)", err);
		return 0;
	}

	val_mv = (int32_t)raw;

	err = adc_raw_to_millivolts_dt(&vbatt.port, &val_mv);
	if (err < 0) {
		LOG_ERR("ADC raw-to-mV failed (%d)", err);
		return 0;
	}

	err = voltage_divider_scale_dt(&vbatt, &val_mv);
	if (err < 0) {
		LOG_ERR("Voltage divider scale failed (%d)", err);
		return 0;
	}

	/* ADC_GAIN_1_6 saturates at 3600 mV — clamp to 100% above that */
	if (val_mv >= 3600) {
		LOG_INF("Battery: %d mV (saturated -> 100%%)", val_mv);
		return 100;
	}

	LOG_INF("Battery: %d mV", val_mv);

	return mv_to_percent(val_mv);
}

static bool alert_low_fired;
static bool alert_critical_fired;
static bool battery_idle_mode;

static void battery_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(battery_work, battery_work_handler);

static void battery_work_handler(struct k_work *work)
{
	uint8_t level = battery_measure();

	int err = stride_service_notify_battery(level);
	if (err && err != -ENOTCONN) {
		LOG_WRN("Battery notify failed (err %d)", err);
	}

	if (level <= BATTERY_THRESHOLD_CRITICAL && !alert_critical_fired) {
		LOG_WRN("Battery critical (%u%%)", level);
		haptic_play_pattern(HAPTIC_PATTERN_CRITICAL_BATTERY);
		alert_critical_fired = true;
		return;
	}

	if (level <= BATTERY_THRESHOLD_LOW && !alert_low_fired) {
		LOG_WRN("Battery low (%u%%)", level);
		haptic_play_pattern(HAPTIC_PATTERN_LOW_BATTERY);
		alert_low_fired = true;
	}

	uint32_t interval = battery_idle_mode
		? BATTERY_MEASURE_INTERVAL_IDLE_SEC
		: BATTERY_MEASURE_INTERVAL_SEC;

	k_work_schedule(&battery_work, K_SECONDS(interval));
}

int battery_service_init(void)
{
	int err;

	alert_low_fired      = false;
	alert_critical_fired = false;

	if (!adc_is_ready_dt(&vbatt.port)) {
		LOG_ERR("ADC not ready");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&vbatt.port);
	if (err < 0) {
		LOG_ERR("ADC channel setup failed (%d)", err);
		return err;
	}

	LOG_INF("Battery service initialized (interval=%ds)", BATTERY_MEASURE_INTERVAL_SEC);

	k_work_schedule(&battery_work, K_SECONDS(BATTERY_MEASURE_INTERVAL_SEC));

	return 0;
}

void battery_service_notify_now(void)
{
	k_work_reschedule(&battery_work, K_NO_WAIT);
}

void battery_service_set_idle(bool idle)
{
	battery_idle_mode = idle;

	uint32_t interval = idle
		? BATTERY_MEASURE_INTERVAL_IDLE_SEC
		: BATTERY_MEASURE_INTERVAL_SEC;

	LOG_INF("Battery polling: %s (%us interval)",
		idle ? "idle" : "active", interval);

	/* Reschedule with the new interval — cancels any pending work first */
	k_work_reschedule(&battery_work, K_SECONDS(interval));
}

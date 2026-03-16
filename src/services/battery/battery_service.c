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
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(battery_service, LOG_LEVEL_INF);

#define BATTERY_MEASURE_INTERVAL_SEC  60
#define BATTERY_THRESHOLD_LOW         15  /* % — warn user with haptic pulse */
#define BATTERY_THRESHOLD_CRITICAL     5  /* % — urgent haptic + stop repeating */

/*
 * XIAO BLE Sense battery circuit:
 *   BAT+ ─── 1MΩ ─── P0.31 (AIN7) ─── 1MΩ ─── GND
 *   P0.14 (GPIO, active-low) enables the divider.
 *
 * Full charge:  ~4.2 V  →  ~2.1 V at AIN7
 * Empty cutoff: ~3.0 V  →  ~1.5 V at AIN7
 */
#define VBATT_NODE DT_NODELABEL(vbatt)

#if DT_NODE_EXISTS(VBATT_NODE)
static const struct voltage_divider_dt_spec vbatt =
	VOLTAGE_DIVIDER_DT_SPEC_GET(VBATT_NODE);
#endif

/* LiPo voltage-to-percent lookup (mV → %) — 10-point linear approximation */
static const struct {
	int32_t mv;
	uint8_t pct;
} lipo_curve[] = {
	{ 4200, 100 },
	{ 4060,  90 },
	{ 3980,  80 },
	{ 3900,  70 },
	{ 3820,  60 },
	{ 3750,  50 },
	{ 3700,  40 },
	{ 3650,  30 },
	{ 3550,  20 },
	{ 3400,  10 },
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
#if DT_NODE_EXISTS(VBATT_NODE)
	int err;
	uint16_t raw;
	int32_t val_mv;

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
		LOG_ERR("ADC raw-to-mV conversion failed (%d)", err);
		return 0;
	}

	/* Scale back up through the voltage divider (×2 for 1M/1M) */
	err = voltage_divider_scale_dt(&vbatt, &val_mv);
	if (err < 0) {
		LOG_ERR("Voltage divider scale failed (%d)", err);
		return 0;
	}

	LOG_DBG("Battery: %d mV", val_mv);

	return mv_to_percent(val_mv);
#else
	LOG_WRN("Battery ADC not configured — returning placeholder");
	return 50;
#endif
}

static void battery_work_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(battery_work, battery_work_handler);

/* Track which alerts have already fired so we don't repeat them every cycle */
static bool alert_low_fired;
static bool alert_critical_fired;

static void battery_work_handler(struct k_work *work)
{
	uint8_t level = battery_measure();

	int err = stride_service_notify_battery(level);
	if (err && err != -ENOTCONN) {
		LOG_WRN("Battery notify failed (err %d)", err);
	}

	if (level <= BATTERY_THRESHOLD_CRITICAL && !alert_critical_fired) {
		LOG_WRN("Battery critical (%u%%) — firing urgent haptic alert", level);
		haptic_play_pattern(HAPTIC_PATTERN_CRITICAL_BATTERY);
		alert_critical_fired = true;
		/* Don't reschedule — stop wasting the last few percent on work */
		return;
	}

	if (level <= BATTERY_THRESHOLD_LOW && !alert_low_fired) {
		LOG_WRN("Battery low (%u%%) — firing haptic warning", level);
		haptic_play_pattern(HAPTIC_PATTERN_LOW_BATTERY);
		alert_low_fired = true;
	}

	k_work_schedule(&battery_work, K_SECONDS(BATTERY_MEASURE_INTERVAL_SEC));
}

int battery_service_init(void)
{
	alert_low_fired      = false;
	alert_critical_fired = false;

#if DT_NODE_EXISTS(VBATT_NODE)
	int err;

	if (!adc_is_ready_dt(&vbatt.port)) {
		LOG_ERR("ADC device not ready — battery reporting disabled");
		return 0;
	}

	err = adc_channel_setup_dt(&vbatt.port);
	if (err < 0) {
		LOG_ERR("ADC channel setup failed (%d) — battery reporting disabled", err);
		return 0;
	}
#else
	LOG_WRN("No vbatt DT node — battery reporting disabled");
#endif

	LOG_INF("Battery service initialized (interval=%ds)", BATTERY_MEASURE_INTERVAL_SEC);

	k_work_schedule(&battery_work, K_SECONDS(BATTERY_MEASURE_INTERVAL_SEC));

	return 0;
}

void battery_service_notify_now(void)
{
	/* Reschedule immediately — reuses the existing work item so it also
	 * resets the 60s periodic timer from this point forward. */
	k_work_reschedule(&battery_work, K_NO_WAIT);
}

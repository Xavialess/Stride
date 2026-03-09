/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <battery/battery_service.h>
#include <ble/stride_service.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(battery_service, LOG_LEVEL_INF);

#define BATTERY_MEASURE_INTERVAL_SEC 60

static void battery_work_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(battery_work, battery_work_handler);

/**
 * Placeholder: returns 100 (USB powered).
 * Replace with real ADC reading when battery hardware is connected.
 */
static uint8_t battery_measure(void)
{
	return 100;
}

static void battery_work_handler(struct k_work *work)
{
	uint8_t level = battery_measure();

	int err = stride_service_notify_battery(level);
	if (err && err != -ENOTCONN) {
		LOG_WRN("Battery notify failed (err %d)", err);
	}

	k_work_schedule(&battery_work, K_SECONDS(BATTERY_MEASURE_INTERVAL_SEC));
}

int battery_service_init(void)
{
	LOG_INF("Battery service initialized (interval=%ds, placeholder)", BATTERY_MEASURE_INTERVAL_SEC);

	k_work_schedule(&battery_work, K_SECONDS(BATTERY_MEASURE_INTERVAL_SEC));

	return 0;
}

/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ble/stride_service.h>
#include <haptics/haptic_service.h>
#include <gpio/gpio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stride_service, LOG_LEVEL_DBG);

static uint8_t battery_level = 0;

/* ------------------------------------------------------------------ */
/* Haptic Command characteristic (Write Without Response)              */
/* ------------------------------------------------------------------ */

static ssize_t haptic_write_cb(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       const void *buf, uint16_t len,
			       uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len == 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	gpio_toggle_led(LED_RUN_STATUS, 1);
	k_sleep(K_MSEC(100));
	gpio_toggle_led(LED_RUN_STATUS, 0);

	LOG_INF("Haptic command: len=%u, cmd=0x%02X", len, data[0]);

	int err = haptic_process_ble_data(data, len);
	if (err) {
		LOG_ERR("Failed to process haptic command (err %d)", err);
	}

	return len;
}

/* ------------------------------------------------------------------ */
/* Battery Level characteristic (Read + Notify)                        */
/* ------------------------------------------------------------------ */

static ssize_t battery_read_cb(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &battery_level, sizeof(battery_level));
}

static void battery_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("Battery notifications %s", notif_enabled ? "enabled" : "disabled");
}

/* ------------------------------------------------------------------ */
/* GATT service definition                                             */
/* ------------------------------------------------------------------ */

BT_GATT_SERVICE_DEFINE(stride_svc,
	BT_GATT_PRIMARY_SERVICE(STRIDE_SVC_UUID),

	/* Haptic Command: Write Without Response */
	BT_GATT_CHARACTERISTIC(STRIDE_HAPTIC_UUID,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, haptic_write_cb, NULL),

	/* Battery Level: Read + Notify */
	BT_GATT_CHARACTERISTIC(STRIDE_BATTERY_UUID,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       battery_read_cb, NULL, NULL),
	BT_GATT_CCC(battery_ccc_changed,
		     BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int stride_service_init(void)
{
	battery_level = 0;
	LOG_INF("Stride service initialized");
	return 0;
}

int stride_service_notify_battery(uint8_t level)
{
	if (level > 100) {
		return -EINVAL;
	}

	battery_level = level;

	return bt_gatt_notify(NULL, &stride_svc.attrs[4], &battery_level, sizeof(battery_level));
}

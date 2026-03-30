/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#include <ble/ble_service.h>
#include <ble/stride_service.h>
#include <battery/battery_service.h>
#include <power/power_mgmt.h>
#include <gpio/gpio.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_DBG);

/* LED blink thread — defined in threads.c, controlled here for advertising indication */
extern const k_tid_t led_blink_thread_id;

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* BLE connection management */
static struct bt_conn *current_conn;
static struct bt_conn *auth_conn;

/* Advertising state */
static struct k_work_delayable adv_work;
static struct k_work_delayable adv_slow_work;
static bool adv_fast_phase;

#define ADV_FAST_DURATION_MS   30000   /* 30s of fast advertising after disconnect */
#define ADV_RETRY_DELAY_MS     1000    /* retry delay if adv_start fails */

/* Slow advertising parameters: 1–1.2s interval (GAP T_GAP(adv_slow_interval)) */
#define BT_LE_ADV_CONN_SLOW \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, \
			BT_GAP_ADV_SLOW_INT_MIN, BT_GAP_ADV_SLOW_INT_MAX, NULL)

/* Semaphore for BLE initialization */
static K_SEM_DEFINE(ble_init_ok, 0, 1);

/* Advertising data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, STRIDE_SVC_UUID_VAL),
};

/* Forward declarations */
static void adv_work_handler(struct k_work *work);
static void adv_slow_work_handler(struct k_work *work);
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void recycled_cb(void);

#ifdef CONFIG_BT_STRIDE_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey);
static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey);
static void auth_cancel(struct bt_conn *conn);
static void pairing_complete(struct bt_conn *conn, bool bonded);
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason);
#endif

/**
 * @brief Start fast advertising, schedule switch to slow after ADV_FAST_DURATION_MS
 */
static void adv_work_handler(struct k_work *work)
{
	const struct bt_le_adv_param *param = adv_fast_phase
		? BT_LE_ADV_CONN_FAST_2
		: BT_LE_ADV_CONN_SLOW;

	int err = bt_le_adv_start(param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err == -EALREADY) {
		LOG_DBG("Advertising already running");
		return;
	}

	if (err) {
		LOG_WRN("Advertising failed (err %d), retrying in %dms", err, ADV_RETRY_DELAY_MS);
		k_work_reschedule(&adv_work, K_MSEC(ADV_RETRY_DELAY_MS));
		return;
	}

	if (adv_fast_phase) {
		LOG_INF("Fast advertising started, switching to slow in %ds",
			ADV_FAST_DURATION_MS / 1000);
		k_work_reschedule(&adv_slow_work, K_MSEC(ADV_FAST_DURATION_MS));
	} else {
		LOG_INF("Slow advertising started");
	}

	/* Blink LED while advertising so the user knows the device is discoverable */
	k_thread_resume(led_blink_thread_id);
}

/**
 * @brief Switch from fast to slow advertising
 */
static void adv_slow_work_handler(struct k_work *work)
{
	/* Don't switch if we connected during the fast phase */
	if (current_conn) {
		return;
	}

	LOG_INF("Switching to slow advertising");
	adv_fast_phase = false;
	bt_le_adv_stop();
	k_work_reschedule(&adv_work, K_NO_WAIT);
}

/**
 * @brief Connection callback
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", addr);

	current_conn = bt_conn_ref(conn);
	gpio_set_led(LED_CON_STATUS, true);

	/* Cancel pending slow-advertising switch — no longer needed */
	k_work_cancel_delayable(&adv_slow_work);

	/* Stop advertising blink — connected, LED off until navigation starts */
	k_thread_suspend(led_blink_thread_id);
	gpio_set_led(LED_RUN_STATUS, false);

	/* Stay idle on connect — device wakes when navigation starts */

	/* Send battery level immediately so the app doesn't wait up to 60s */
	battery_service_notify_now();
}

/**
 * @brief Disconnection callback
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

	if (auth_conn) {
		bt_conn_unref(auth_conn);
		auth_conn = NULL;
	}

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		gpio_set_led(LED_CON_STATUS, false);
	}

	/* Reset to fast advertising phase on every disconnect */
	k_work_cancel_delayable(&adv_slow_work);
	adv_fast_phase = true;

	/* Enter idle when phone disconnects — BLE keeps advertising */
	power_mgmt_request_state(POWER_STATE_IDLE);
}

/**
 * @brief Connection recycled callback
 */
static void recycled_cb(void)
{
	LOG_INF("Connection object available from previous conn. Disconnect is complete!");
	ble_start_advertising();
}

#ifdef CONFIG_BT_STRIDE_SECURITY_ENABLED
/**
 * @brief Security changed callback
 */
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d %s", addr, level, err,
			bt_security_err_to_str(err));
	}
}

/**
 * @brief Passkey display callback
 */
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Passkey for %s: %06u", addr, passkey);
}

/**
 * @brief Passkey confirm callback
 */
static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	auth_conn = bt_conn_ref(conn);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Passkey for %s: %06u", addr, passkey);

	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) || IS_ENABLED(CONFIG_SOC_SERIES_NRF54LX)) {
		LOG_INF("Press Button 0 to confirm, Button 1 to reject.");
	} else {
		LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
	}
}

/**
 * @brief Auth cancel callback
 */
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing cancelled: %s", addr);
}

/**
 * @brief Pairing complete callback
 */
static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

/**
 * @brief Pairing failed callback
 */
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing failed conn: %s, reason %d %s", addr, reason,
		bt_security_err_to_str(reason));
}
#endif

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.recycled         = recycled_cb,
#ifdef CONFIG_BT_STRIDE_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

/* Auth callbacks */
#ifdef CONFIG_BT_STRIDE_SECURITY_ENABLED
static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

/**
 * @brief Initialize BLE subsystem
 */
int ble_service_init(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_STRIDE_SECURITY_ENABLED)) {
		err = bt_conn_auth_cb_register(&conn_auth_callbacks);
		if (err) {
			LOG_ERR("Failed to register authorization callbacks. (err: %d)", err);
			return err;
		}

		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err) {
			LOG_ERR("Failed to register authorization info callbacks. (err: %d)", err);
			return err;
		}
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	k_sem_give(&ble_init_ok);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	k_work_init_delayable(&adv_work, adv_work_handler);
	k_work_init_delayable(&adv_slow_work, adv_slow_work_handler);
	adv_fast_phase = true;

	LOG_INF("BLE service initialized");
	return 0;
}

/**
 * @brief Start BLE advertising
 */
int ble_start_advertising(void)
{
	k_work_reschedule(&adv_work, K_NO_WAIT);
	return 0;
}

/**
 * @brief Get current BLE connection
 */
struct bt_conn *ble_get_current_conn(void)
{
	return current_conn;
}

/**
 * @brief Wait for BLE initialization to complete
 */
void ble_wait_init(void)
{
	k_sem_take(&ble_init_ok, K_FOREVER);
}

/**
 * @brief Signal that BLE initialization is complete
 */
void ble_signal_init_complete(void)
{
	k_sem_give(&ble_init_ok);
}

/**
 * @brief Get auth connection for passkey operations
 */
struct bt_conn *ble_get_auth_conn(void)
{
	return auth_conn;
}

/**
 * @brief Stop BLE advertising and disable the Bluetooth stack
 */
void ble_service_stop(void)
{
	k_work_cancel_delayable(&adv_slow_work);
	k_work_cancel_delayable(&adv_work);

	bt_le_adv_stop();

	if (current_conn) {
		bt_conn_disconnect(current_conn, BT_HCI_ERR_REMOTE_POWER_OFF);
	}

	bt_disable();
	LOG_INF("BLE stopped");
}

/**
 * @brief Confirm passkey
 */
void ble_confirm_passkey(bool accept)
{
	if (!auth_conn) {
		return;
	}

	if (accept) {
		bt_conn_auth_passkey_confirm(auth_conn);
		LOG_INF("Numeric Match, conn %p", (void *)auth_conn);
	} else {
		bt_conn_auth_cancel(auth_conn);
		LOG_INF("Numeric Reject, conn %p", (void *)auth_conn);
	}

	bt_conn_unref(auth_conn);
	auth_conn = NULL;
}

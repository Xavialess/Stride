/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

/**
 * @brief Callback function type for BLE data reception
 * 
 * @param conn Connection handle
 * @param data Received data
 * @param len Length of received data
 */
typedef void (*ble_data_received_cb_t)(struct bt_conn *conn, const uint8_t *data, uint16_t len);

/**
 * @brief Initialize BLE subsystem
 * 
 * @param rx_callback Callback for received data
 * @return 0 on success, negative errno on failure
 */
int ble_service_init(ble_data_received_cb_t rx_callback);

/**
 * @brief Start BLE advertising
 * 
 * @return 0 on success, negative errno on failure
 */
int ble_start_advertising(void);

/**
 * @brief Send data over BLE NUS
 * 
 * @param data Pointer to data buffer
 * @param len Length of data
 * @return 0 on success, negative errno on failure
 */
int ble_send_data(const uint8_t *data, uint16_t len);

/**
 * @brief Get current BLE connection
 * 
 * @return Pointer to current connection or NULL
 */
struct bt_conn *ble_get_current_conn(void);

/**
 * @brief Wait for BLE initialization to complete
 */
void ble_wait_init(void);

/**
 * @brief Signal that BLE initialization is complete
 */
void ble_signal_init_complete(void);

/**
 * @brief Get auth connection for passkey operations
 * 
 * @return Pointer to auth connection or NULL
 */
struct bt_conn *ble_get_auth_conn(void);

/**
 * @brief Confirm or reject passkey
 * 
 * @param accept True to accept, false to reject
 */
void ble_confirm_passkey(bool accept);

#endif /* BLE_SERVICE_H */


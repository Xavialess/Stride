/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

/**
 * @brief Initialize battery monitoring
 *
 * Starts a periodic timer that measures battery level and sends
 * BLE notifications via the Stride service battery characteristic.
 *
 * @return 0 on success, negative errno on failure
 */
int battery_service_init(void);

/**
 * @brief Trigger an immediate battery measurement and BLE notification
 *
 * Call this when a BLE connection is established so the app receives
 * the current level right away without waiting for the periodic timer.
 */
void battery_service_notify_now(void);

#endif /* BATTERY_SERVICE_H */

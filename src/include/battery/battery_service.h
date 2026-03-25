/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#include <stdbool.h>

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

/**
 * @brief Set battery polling to idle (slow) or active (normal) rate
 *
 * In idle mode the polling interval is extended to 5 minutes to reduce
 * unnecessary ADC reads and I2C traffic while the device is not in use.
 *
 * @param idle true to enter slow-poll idle, false to restore normal rate
 */
void battery_service_set_idle(bool idle);

#endif /* BATTERY_SERVICE_H */

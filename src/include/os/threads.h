/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#ifndef OS_THREADS_H
#define OS_THREADS_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize and start all application threads
 * 
 * This function initializes the thread management system and
 * starts all required threads for the application.
 */
void threads_init(void);

/**
 * @brief BLE write thread entry point
 * 
 * This thread handles sending UART data over BLE
 */
void ble_write_thread_entry(void);

/**
 * @brief LED blink thread entry point
 * 
 * This thread handles the status LED blinking
 */
void led_blink_thread_entry(void);

/**
 * @brief Haptic thread entry point
 * 
 * This thread handles haptic feedback pattern playback
 */
void haptic_thread_entry(void);

#endif /* OS_THREADS_H */


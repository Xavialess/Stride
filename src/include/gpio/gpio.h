/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * Xavier Quintanilla
 */

#ifndef GPIO_H
#define GPIO_H

#include <zephyr/types.h>
#include <dk_buttons_and_leds.h>

/* LED definitions */
#define LED_RUN_STATUS    DK_LED1
#define LED_CON_STATUS    DK_LED2

/* Button definitions */
#define BTN_PASSKEY_ACCEPT DK_BTN1_MSK
#define BTN_PASSKEY_REJECT DK_BTN2_MSK

/**
 * @brief Initialize GPIO (LEDs and buttons)
 * 
 * @return 0 on success, negative errno on failure
 */
int gpio_init(void);

/**
 * @brief Set LED state
 * 
 * @param led_idx LED index
 * @param state LED state (0=off, 1=on)
 */
void gpio_set_led(uint32_t led_idx, bool state);

/**
 * @brief Toggle LED state
 * 
 * @param led_idx LED index
 * @param state State to set
 */
void gpio_toggle_led(uint32_t led_idx, uint32_t state);

/**
 * @brief Enter error state (all LEDs off, infinite loop)
 */
void gpio_error_state(void);

#endif /* GPIO_H */


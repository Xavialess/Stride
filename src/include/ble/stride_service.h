/*
 * Copyright (c) 2024 Xavier Quintanilla
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef STRIDE_SERVICE_H
#define STRIDE_SERVICE_H

#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>

/*
 * Stride Wristband custom GATT service.
 *
 * Base UUID: A5A5xxxx-C4FB-4D3B-B35A-1BF3A1B2C5D8
 *
 * Characteristics:
 *   Haptic Command   (0002) - Write Without Response
 *   Battery Level    (0003) - Read + Notify
 *   Power Control    (0004) - Write Without Response
 *                            0x00 = active, 0x01 = idle
 */

#define STRIDE_SVC_UUID_VAL \
	BT_UUID_128_ENCODE(0xA5A50001, 0xC4FB, 0x4D3B, 0xB35A, 0x1BF3A1B2C5D8)

#define STRIDE_HAPTIC_UUID_VAL \
	BT_UUID_128_ENCODE(0xA5A50002, 0xC4FB, 0x4D3B, 0xB35A, 0x1BF3A1B2C5D8)

#define STRIDE_BATTERY_UUID_VAL \
	BT_UUID_128_ENCODE(0xA5A50003, 0xC4FB, 0x4D3B, 0xB35A, 0x1BF3A1B2C5D8)

#define STRIDE_POWER_UUID_VAL \
	BT_UUID_128_ENCODE(0xA5A50004, 0xC4FB, 0x4D3B, 0xB35A, 0x1BF3A1B2C5D8)

#define STRIDE_SVC_UUID       BT_UUID_DECLARE_128(STRIDE_SVC_UUID_VAL)
#define STRIDE_HAPTIC_UUID    BT_UUID_DECLARE_128(STRIDE_HAPTIC_UUID_VAL)
#define STRIDE_BATTERY_UUID   BT_UUID_DECLARE_128(STRIDE_BATTERY_UUID_VAL)
#define STRIDE_POWER_UUID     BT_UUID_DECLARE_128(STRIDE_POWER_UUID_VAL)

/* Power command values written to STRIDE_POWER_UUID */
#define STRIDE_POWER_CMD_ACTIVE  0x00
#define STRIDE_POWER_CMD_IDLE    0x01

/**
 * @brief Initialize the Stride custom GATT service
 *
 * The GATT service itself is registered at compile time via
 * BT_GATT_SERVICE_DEFINE.
 *
 * @return 0 on success, negative errno on failure
 */
int stride_service_init(void);

/**
 * @brief Update and notify the battery level characteristic
 *
 * @param level Battery percentage (0-100)
 * @return 0 on success, negative errno on failure
 */
int stride_service_notify_battery(uint8_t level);

#endif /* STRIDE_SERVICE_H */

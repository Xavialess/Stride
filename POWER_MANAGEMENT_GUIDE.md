# Power Management for Wearable Application

## Overview

For a battery-powered wearable, power management is **critical** to extend battery life from hours to days/weeks.

## Hardware: XIAO BLE Sense Battery Support

### Built-in Features ✅
- **JST 1.25mm battery connector** on the back
- **Charging circuit** built-in (charges via USB)
- **Battery voltage**: 3.7V LiPo (single cell)
- **Typical capacity**: 100-500mAh

### No External Hardware Needed!
Just connect a 3.7V LiPo battery with JST 1.25mm connector.

---

## Power Consumption Analysis

### Current Draw (Typical)

| Mode | Current | Battery Life (200mAh) |
|------|---------|----------------------|
| **Active (BLE + haptics)** | ~10-15mA | 13-20 hours |
| **BLE connected (idle)** | ~5-8mA | 25-40 hours |
| **BLE advertising** | ~3-5mA | 40-66 hours |
| **Sleep (BLE off)** | ~100-500µA | 16-83 days |
| **Deep sleep** | ~5-10µA | 2-4 years |

### Problem: Without Power Management
- Constant advertising: ~4mA
- LED blinking: ~2mA
- Total: **~6mA = 33 hours on 200mAh battery** ⚠️

### With Power Management
- Sleep between haptic events: ~200µA
- Wake on BLE connection: instant
- Total: **~0.2mA = 1000 hours (41 days)** ✅

---

## Power Management Strategy

### Level 1: Basic (Quick Wins) 🟢

**Enable Zephyr's built-in power management:**

Add to `prj.conf`:
```kconfig
# Enable power management
CONFIG_PM=y
CONFIG_PM_DEVICE=y

# BLE power optimization
CONFIG_BT_CTLR_LOW_LAT=n
CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY=30

# Reduce advertising interval when not connected
CONFIG_BT_PERIPHERAL_PREF_MIN_INT=400
CONFIG_BT_PERIPHERAL_PREF_MAX_INT=650

# Enable low power mode
CONFIG_SOC_NRF52840_ALLOW_SPIM_DESPITE_PAN_58=y
```

**Disable LED blinking when on battery:**

In `src/Kconfig`:
```kconfig
config APP_LED_BLINK_BATTERY_DISABLE
	bool "Disable LED blinking when on battery"
	default y
	help
	  Disable LED blinking to save power when running on battery.
```

**Expected savings: 6mA → 3mA (doubles battery life!)**

---

### Level 2: Intermediate (Smart Sleep) 🟡

**Put DRV2605L in standby between haptic events:**

In `haptic_thread_entry()` after processing:
```c
/* After playing haptic */
k_sleep(K_MSEC(100));  // Wait for effect to finish
drv2605l_standby();     // Put motor driver to sleep

/* Before next haptic */
drv2605l_wakeup();      // Wake it up
```

**Disable UART when not needed:**

```c
#ifdef CONFIG_APP_POWER_MANAGEMENT
	/* Disable UART on battery to save power */
	if (!usb_connected) {
		uart_service_disable();
	}
#endif
```

**Expected savings: 3mA → 1mA (3x battery life!)**

---

### Level 3: Advanced (Deep Sleep) 🔴

**Sleep between BLE events:**

This requires more complex integration with Zephyr's PM subsystem.

```c
/* In main.c */
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

/* Allow automatic sleep */
void main(void) {
	// ... initialization ...
	
	/* Enable automatic sleep */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	
	/* Main loop can now sleep */
	while (1) {
		k_sleep(K_FOREVER);  // Will enter low-power automatically
	}
}
```

**Expected savings: 1mA → 0.2mA (10x battery life!)**

---

## Implementation Checklist

### Phase 1: Quick Wins (Do This Now!) ✅

1. **Add power management configs to `prj.conf`**
   ```kconfig
   CONFIG_PM=y
   CONFIG_PM_DEVICE=y
   CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY=30
   ```

2. **Disable LED blink on battery**
   - Modify `led_blink_thread_entry()` to check battery status
   - Or just disable the thread entirely for wearable

3. **Put DRV2605L in standby**
   - Call `drv2605l_standby()` after haptic events
   - Wake with `drv2605l_wakeup()` before next event

### Phase 2: Optimize (After Testing) 🔧

4. **Implement idle timeout**
   - Use `CONFIG_APP_IDLE_TIMEOUT_MS` from Kconfig
   - Disconnect BLE after inactivity
   - Enter deep sleep

5. **Wake on button press**
   - Configure button as wake source
   - User can wake device to reconnect

6. **Battery monitoring**
   - Read battery voltage via ADC
   - Warn user when low
   - Shutdown before damage

### Phase 3: Advanced (Optional) 🚀

7. **Motion-based wake**
   - Use IMU (XIAO Sense has LSM6DS3)
   - Wake on movement
   - Sleep when stationary

8. **Adaptive advertising**
   - Fast advertising when first powered
   - Slow advertising after timeout
   - Stop advertising after longer timeout

---

## Quick Start: Minimal Power Management

### 1. Update `prj.conf`

Add these lines:
```kconfig
# Basic power management
CONFIG_PM=y
CONFIG_PM_DEVICE=y

# BLE power optimization
CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY=30
CONFIG_BT_PERIPHERAL_PREF_MIN_INT=400
CONFIG_BT_PERIPHERAL_PREF_MAX_INT=650
```

### 2. Disable LED Blink Thread

In `src/os/threads/threads.c`, comment out:
```c
/* Disable for wearable to save power */
// K_THREAD_DEFINE(led_blink_thread_id, CONFIG_APP_LED_BLINK_STACK_SIZE, led_blink_thread_entry, 
//		NULL, NULL, NULL, CONFIG_APP_LED_BLINK_PRIORITY, 0, 0);
```

### 3. Add Standby to Haptic Thread

In `haptic_thread_entry()`:
```c
case HAPTIC_PATTERN_SINGLE_EFFECT:
	if (haptic_data->len >= 1) {
		drv2605l_wakeup();  // Wake motor driver
		int ret = drv2605l_play_effect(haptic_data->data[0]);
		if (ret < 0) {
			LOG_ERR("Failed to play effect %d (err %d)", 
			        haptic_data->data[0], ret);
		}
		k_sleep(K_MSEC(100));  // Wait for effect
		drv2605l_standby();     // Sleep motor driver
	}
	break;
```

**That's it! Battery life improved by 3-5x** 🎉

---

## Battery Life Estimates

### 200mAh Battery (typical small wearable)

| Configuration | Current | Runtime |
|---------------|---------|---------|
| No optimization | 6mA | 33 hours |
| Basic PM (Phase 1) | 2mA | 100 hours (4 days) |
| With sleep (Phase 2) | 0.5mA | 400 hours (16 days) |
| Deep sleep (Phase 3) | 0.1mA | 2000 hours (83 days) |

### 500mAh Battery (larger wearable)

| Configuration | Current | Runtime |
|---------------|---------|---------|
| No optimization | 6mA | 83 hours (3.5 days) |
| Basic PM (Phase 1) | 2mA | 250 hours (10 days) |
| With sleep (Phase 2) | 0.5mA | 1000 hours (41 days) |
| Deep sleep (Phase 3) | 0.1mA | 5000 hours (208 days) |

---

## Testing Battery Life

### Measure Current Draw

Use a multimeter in series:
```
Battery (+) → Multimeter (mA) → XIAO BAT+
Battery (-) → XIAO BAT-
```

### Expected Readings

- **After boot (advertising)**: 3-5mA
- **BLE connected (idle)**: 2-4mA
- **Haptic playing**: 10-15mA (spike)
- **With basic PM**: 1-2mA
- **Deep sleep**: 0.1-0.5mA

### Calculate Runtime

```
Runtime (hours) = Battery Capacity (mAh) / Average Current (mA)
```

Example:
- 200mAh battery
- 2mA average current
- Runtime = 200 / 2 = **100 hours**

---

## Recommended Battery

For a wearable haptic device:

### Small Form Factor
- **Capacity**: 150-250mAh
- **Size**: 20x30x5mm
- **Connector**: JST 1.25mm
- **Runtime**: 3-10 days (with basic PM)

### Standard
- **Capacity**: 400-600mAh
- **Size**: 30x40x8mm
- **Connector**: JST 1.25mm
- **Runtime**: 10-30 days (with basic PM)

### Where to Buy
- Adafruit: LiPo batteries with JST connector
- SparkFun: Various sizes available
- Amazon: Search "3.7V LiPo JST 1.25mm"

---

## Safety Notes ⚠️

### Battery Safety
1. **Never short circuit** the battery
2. **Don't over-discharge** below 3.0V
3. **Don't puncture** or damage the battery
4. **Use protection circuit** (most LiPos have this built-in)
5. **Charge at room temperature** only

### Charging
- XIAO charges at **~100mA** via USB
- Charge time: ~2-5 hours depending on capacity
- LED indicator: Red = charging, Green = charged

### Monitoring
Consider adding battery voltage monitoring:
```c
/* Read battery voltage via ADC */
uint16_t battery_voltage = read_battery_voltage();
if (battery_voltage < 3300) {  // 3.3V
	LOG_WRN("Battery low!");
	// Maybe vibrate warning pattern
	// Or disconnect BLE to save power
}
```

---

## Summary

### Minimum for Wearable (Phase 1)
✅ Connect 3.7V LiPo battery (JST connector)  
✅ Add `CONFIG_PM=y` to `prj.conf`  
✅ Disable LED blink thread  
✅ Put DRV2605L in standby between events  

**Result: 3-10 days battery life on 200mAh** 🎯

### For Production (Phase 2+)
- Implement deep sleep
- Battery monitoring
- Wake on motion
- Adaptive advertising

**Result: Weeks to months of battery life** 🚀

---

## Next Steps

1. **Test with battery first** - make sure it works
2. **Measure current draw** - know your baseline
3. **Add Phase 1 optimizations** - quick wins
4. **Test battery life** - measure improvement
5. **Add Phase 2 if needed** - for longer runtime

The XIAO BLE Sense makes battery power easy - just plug it in and go! 🔋


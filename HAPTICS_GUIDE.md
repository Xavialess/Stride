# Haptic Feedback System Guide

## Overview

This project now includes a complete haptic feedback system using the **DRV2605L** haptic motor driver connected to a Grove Motor. The system receives commands via BLE and converts them into vibration patterns.

## Hardware Setup

### Components
- **DRV2605L Haptic Driver IC**
- **Grove Motor** (ERM - Eccentric Rotating Mass)
  - Operating voltage: 3.3~5.0 V
  - Max power: 750 mW
  - I2C speed: 100 kHz
  - I2C Address: 0x5A (default)
  - 123 built-in vibration effects

### Connections
The DRV2605L connects to your Nordic board via I2C:
- **SDA**: I2C data line
- **SCL**: I2C clock line
- **VCC**: 3.3V or 5V power
- **GND**: Ground

**Important**: Update the `app.overlay` file with the correct I2C pins for your specific board!

```dts
&i2c1 {
    sda-pin = <26>;  /* Change to your board's SDA pin */
    scl-pin = <27>;  /* Change to your board's SCL pin */
    
    drv2605l: drv2605l@5a {
        compatible = "ti,drv2605l";
        reg = <0x5a>;
        status = "okay";
    };
};
```

## Architecture

### Data Flow: BLE → Haptics

```
BLE RX → bt_receive_cb() → on_ble_data_received() → haptic_process_ble_data()
    → haptic_queue_pattern() → fifo_haptic_data → Haptic Thread 
    → DRV2605L Driver → Motor Vibration
```

### Components

1. **DRV2605L Driver** (`src/drivers/haptics/drv2605l.c`)
   - Low-level I2C communication
   - Register configuration
   - Effect playback (123 built-in effects)
   - Waveform sequencing

2. **Haptic Service** (`src/services/haptics/haptic_service.c`)
   - Pattern definition and mapping
   - FIFO queue management
   - BLE data protocol parsing
   - High-level API

3. **Haptic Thread** (`src/os/threads/threads.c`)
   - Reads from haptic FIFO queue
   - Processes patterns
   - Controls timing and sequencing
   - Priority: 7 (same as BLE thread)
   - Stack: 2048 bytes

### Thread Architecture

The haptic system uses a **dedicated thread** for several important reasons:

✅ **Non-blocking**: BLE can continue receiving data while haptics are playing  
✅ **Separation of concerns**: BLE handles communication, haptics handles motor control  
✅ **Queue-based**: Thread-safe FIFO communication (consistent with existing architecture)  
✅ **Precise timing**: Haptic patterns need accurate timing control  

## BLE Protocol

The system routes BLE data based on the first byte:
- **0x01-0x04**: Haptic commands (routed to haptic service)
- **All other values**: UART passthrough (existing functionality)

### Command Format

#### 1. Play Single Effect (0x01)
```
Byte 0: 0x01 (command)
Byte 1: Effect number (1-123)
```

**Example**: Play strong click
```
[0x01, 0x01]  // Effect #1: Strong Click 100%
```

#### 2. Play Sequence (0x02)
```
Byte 0: 0x02 (command)
Byte 1: Number of effects
Bytes 2-N: Effect numbers
```

**Example**: Play double buzz
```
[0x02, 0x02, 0x0E, 0x0E]  // 2 effects: Buzz, Buzz
```

#### 3. Play Predefined Pattern (0x03)
```
Byte 0: 0x03 (command)
Byte 1: Pattern ID (0-11)
```

**Example**: Play notification pattern
```
[0x03, 0x00]  // Pattern #0: Notification
```

#### 4. Stop Playback (0x04)
```
Byte 0: 0x04 (command)
```

**Example**: Stop current vibration
```
[0x04]
```

## Predefined Patterns

The system includes 12 predefined patterns:

| ID | Pattern Name | Description | Use Case |
|----|--------------|-------------|----------|
| 0 | NOTIFICATION | Short sharp click | Incoming message |
| 1 | ALERT | Double strong buzz | Important alert |
| 2 | SUCCESS | Ramp up + click | Action confirmed |
| 3 | ERROR | Triple click | Error occurred |
| 4 | BUTTON_PRESS | Medium click | Button feedback |
| 5 | LONG_PRESS | Soft bump + click | Long press detected |
| 6 | DOUBLE_TAP | Double click | Double tap gesture |
| 7 | HEARTBEAT | Two soft bumps | Heartbeat simulation |
| 8 | RAMP_UP | Gradual increase | Power on |
| 9 | RAMP_DOWN | Gradual decrease | Power off |
| 10 | PULSE | Pulsing pattern | Ongoing activity |
| 11 | BUZZ | Continuous buzz | Alarm/timer |

## DRV2605L Effects Library

The DRV2605L has 123 built-in effects. Here are some commonly used ones:

### Clicks & Ticks
- **1**: Strong Click 100%
- **4**: Sharp Click 100%
- **7**: Soft Bump 100%
- **10**: Double Click 100%
- **12**: Triple Click 100%

### Buzzes & Alerts
- **14**: Strong Buzz 100%
- **15**: Alert 750ms
- **16**: Alert 1000ms
- **47-51**: Various buzz patterns

### Transitions
- **59-64**: Transition clicks
- **71-82**: Ramp down patterns
- **83-94**: Ramp up patterns

### Pulses
- **52-58**: Pulsing patterns (strong, medium, sharp)

See `src/drivers/haptics/drv2605l.h` for the complete list of effects.

## Usage Examples

### From BLE (nRF Connect or similar app)

1. **Simple notification**:
   ```
   Send: 0x03 0x00
   Result: Short sharp click
   ```

2. **Custom sequence** (alert pattern):
   ```
   Send: 0x02 0x03 0x0E 0x0E 0x0E
   Result: Three strong buzzes
   ```

3. **Single effect** (soft bump):
   ```
   Send: 0x01 0x07
   Result: Soft bump vibration
   ```

4. **Stop vibration**:
   ```
   Send: 0x04
   Result: Immediate stop
   ```

### From Code (C API)

```c
#include <haptics/haptic_service.h>

/* Play a single effect */
haptic_play_effect(DRV2605L_EFFECT_STRONG_CLICK_100);

/* Play a predefined pattern */
haptic_play_pattern(HAPTIC_PATTERN_NOTIFICATION);

/* Play a custom sequence */
uint8_t effects[] = {
    DRV2605L_EFFECT_SHARP_CLICK_100,
    DRV2605L_EFFECT_SOFT_BUMP_100
};
haptic_play_sequence(effects, 2);

/* Stop playback */
haptic_stop();
```

## Configuration

### Kconfig Options

```kconfig
CONFIG_APP_HAPTICS=y                    # Enable haptics (default: y)
CONFIG_APP_HAPTIC_STACK_SIZE=2048       # Thread stack size
CONFIG_APP_HAPTIC_PRIORITY=7            # Thread priority
```

### Device Tree

The I2C configuration is in `app.overlay`. **You must update the pin numbers** for your specific board:

```dts
&i2c1 {
    compatible = "nordic,nrf-twim";
    status = "okay";
    clock-frequency = <I2C_BITRATE_STANDARD>; /* 100 kHz */
    
    /* UPDATE THESE PINS FOR YOUR BOARD! */
    sda-pin = <26>;
    scl-pin = <27>;
    
    drv2605l: drv2605l@5a {
        compatible = "ti,drv2605l";
        reg = <0x5a>;
        status = "okay";
    };
};
```

### Motor Type

The driver is configured for **ERM (Eccentric Rotating Mass)** motors by default. If you're using an **LRA (Linear Resonant Actuator)**, modify `haptic_service.c`:

```c
/* In haptic_service_init() */
ret = drv2605l_init(DRV2605L_MOTOR_LRA);  // Change to LRA
```

For LRA motors, you should also run auto-calibration:

```c
drv2605l_auto_calibrate();
```

## Troubleshooting

### No vibration

1. **Check I2C pins**: Verify `app.overlay` has correct SDA/SCL pins for your board
2. **Check I2C connection**: Use `i2cdetect` or similar to verify device at 0x5A
3. **Check power**: Ensure motor has adequate power supply (3.3V or 5V)
4. **Check logs**: Enable debug logging to see driver communication

### Weak vibration

1. **Adjust voltage**: DRV2605L rated voltage register (default: 3V)
2. **Try stronger effects**: Use effects 1, 14, or 16
3. **Check motor**: Ensure motor is properly connected and functional

### I2C errors

1. **Pull-up resistors**: I2C lines need pull-up resistors (typically 4.7kΩ)
2. **Bus speed**: Try lowering I2C speed if you have long wires
3. **Multiple devices**: Check for I2C address conflicts

### Build errors

1. **Missing I2C support**: Ensure `CONFIG_I2C=y` in `prj.conf`
2. **Device tree**: Verify I2C bus is enabled in your board's DTS
3. **Pin conflicts**: Check that I2C pins aren't used by other peripherals

## Testing

### Quick Test Sequence

1. **Build and flash**:
   ```bash
   west build -b your_board
   west flash
   ```

2. **Connect via BLE** (using nRF Connect app)

3. **Send test commands**:
   - `0x03 0x00` → Should feel a sharp click
   - `0x01 0x0E` → Should feel a strong buzz
   - `0x04` → Should stop immediately

### Debug Logging

Enable debug logging in `prj.conf`:

```kconfig
CONFIG_LOG_DEFAULT_LEVEL=4  # Debug level
```

View logs:
```bash
west debug
# or
minicom -D /dev/ttyACM0
```

## Performance Considerations

### Timing
- Effect playback is non-blocking
- Sequences play automatically (hardware-driven)
- Thread processes queue with 10ms delay between patterns

### Memory
- Each queued pattern uses ~40 bytes
- FIFO queue is dynamic (limited by heap)
- Heap size: 2048 bytes (configurable in `prj.conf`)

### Power Consumption
- Motor draws up to 750mW during vibration
- DRV2605L enters standby when idle
- Consider power management for battery applications

## Future Enhancements

Potential improvements:

1. **Dynamic intensity control**: Add intensity parameter to patterns
2. **Custom waveforms**: Use real-time playback mode for custom waveforms
3. **Haptic feedback for UART**: Vibrate on specific UART events
4. **Pattern recording**: Record and replay custom sequences
5. **Power management**: Auto-standby after inactivity
6. **Multiple motors**: Support for multiple DRV2605L devices

## API Reference

See header files for complete API documentation:
- `src/drivers/haptics/drv2605l.h` - Low-level driver API
- `src/include/haptics/haptic_service.h` - High-level service API

## License

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Copyright (c) 2024 Xavier Quintanilla


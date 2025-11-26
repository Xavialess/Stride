# Seeed XIAO BLE Sense Setup Guide

## Hardware Configuration

### Board: Seeed XIAO BLE Sense (nRF52840)

### I2C Pins (for DRV2605L + Grove Motor)
- **SDA**: P0.04 (Pin 4 on the board)
- **SCL**: P0.05 (Pin 5 on the board)
- **VCC**: 3.3V
- **GND**: GND

### Physical Connections

```
Grove Motor (DRV2605L)          XIAO BLE Sense
┌─────────────────┐            ┌──────────────┐
│                 │            │              │
│  VCC (Red)      │────────────│ 3.3V         │
│  GND (Black)    │────────────│ GND          │
│  SDA (White)    │────────────│ Pin 4 (D4)   │
│  SCL (Yellow)   │────────────│ Pin 5 (D5)   │
│                 │            │              │
└─────────────────┘            └──────────────┘
```

## Build Commands

### Build for XIAO BLE Sense
```bash
west build -b xiao_ble_sense_nrf52840
```

### Clean build (if needed)
```bash
west build -b xiao_ble_sense_nrf52840 --pristine
```

### Flash to board
```bash
west flash
```

### View logs (RTT)
```bash
# In one terminal
JLinkRTTLogger -Device NRF52840_XXAA -RTTChannel 0 -if SWD -Speed 4000

# Or use RTT Viewer
JLinkRTTViewer
```

## Testing Haptics via BLE

### 1. Connect with nRF Connect App

1. Open nRF Connect app on your phone
2. Scan for "Nordic_UART_Service"
3. Connect to the device
4. Find the Nordic UART Service (NUS)
5. Enable notifications on the RX characteristic

### 2. Send Haptic Commands

**Test 1: Simple notification**
- Send (HEX): `03 00`
- Expected: Short sharp click

**Test 2: Alert pattern**
- Send (HEX): `03 01`
- Expected: Two strong buzzes

**Test 3: Single effect (strong buzz)**
- Send (HEX): `01 0E`
- Expected: One strong buzz

**Test 4: Custom sequence**
- Send (HEX): `02 03 01 01 01`
- Expected: Three clicks in sequence

**Test 5: Stop**
- Send (HEX): `04`
- Expected: Immediate stop

### 3. Test UART Passthrough (should still work)

- Send (ASCII): `Hello`
- Expected: Appears on UART terminal

## Troubleshooting

### Haptics not working?

1. **Check I2C connection**
   ```bash
   # In logs, look for:
   # "DRV2605L initialized" ✅ Good
   # "Failed to communicate with DRV2605L" ❌ Bad
   ```

2. **Verify wiring**
   - SDA on Pin 4 (P0.04)
   - SCL on Pin 5 (P0.05)
   - Check continuity with multimeter

3. **Check power**
   - DRV2605L needs 3.3V or 5V
   - Motor needs adequate current

4. **I2C address**
   - Default is 0x5A
   - Can be changed on some modules

### Build errors?

1. **Missing board definition**
   ```bash
   # Make sure nRF Connect SDK is properly installed
   west update
   ```

2. **I2C errors**
   - Check that `CONFIG_I2C=y` in `prj.conf` ✅ (already set)

### Motor vibrates weakly?

1. **Adjust voltage** (in `drv2605l.c`, line ~105):
   ```c
   ret = drv2605l_write_reg(DRV2605L_REG_RATEDV, 0x90);  // Try 0xA0 for stronger
   ```

2. **Use stronger effects**:
   - Effect 1: Strong Click 100%
   - Effect 14: Strong Buzz 100%
   - Effect 16: Alert 1000ms

## Pin Reference

### XIAO BLE Sense Pinout
```
         USB
    ┌──────────┐
D0  │ 1     14 │ 3.3V
D1  │ 2     13 │ GND
D2  │ 3     12 │ VBAT
D3  │ 4     11 │ D10
D4  │ 5     10 │ D9   ← SDA (I2C)
D5  │ 6      9 │ D8   ← SCL (I2C)
D6  │ 7      8 │ D7
    └──────────┘
```

**For I2C:**
- Pin 5 (D4) = P0.04 = SDA
- Pin 6 (D5) = P0.05 = SCL

## Expected Boot Sequence

When you flash and power on, you should see:

```
[00:00:00.123] Starting Nordic UART service sample
[00:00:00.234] GPIO initialized
[00:00:00.345] UART initialized
[00:00:00.456] Bluetooth initialized
[00:00:00.567] Initializing haptic service...
[00:00:00.678] DRV2605L status: 0xXX
[00:00:00.789] DRV2605L initialized (motor type: ERM)
[00:00:00.890] Haptic service initialized
[00:00:00.901] Advertising successfully started
[00:00:00.912] BLE write thread started
[00:00:00.923] LED blink thread started
[00:00:00.934] Haptic thread started
[00:00:00.945] Initialization complete. System running.
```

If you see "Haptic service initialization failed", check your I2C wiring!

## Quick Command Reference

| Command | Hex | Description |
|---------|-----|-------------|
| Play effect 1 | `01 01` | Strong click |
| Play effect 14 | `01 0E` | Strong buzz |
| Notification | `03 00` | Sharp click |
| Alert | `03 01` | Double buzz |
| Success | `03 02` | Ramp + click |
| Error | `03 03` | Triple click |
| Stop | `04` | Stop vibration |

## Success Checklist

✅ Build completes without errors  
✅ Flash successful  
✅ BLE connects  
✅ See "Haptic service initialized" in logs  
✅ Sending `03 00` makes motor vibrate  
✅ UART passthrough still works  

## Support

If issues persist:
1. Check RTT logs for error messages
2. Verify I2C communication with logic analyzer
3. Test DRV2605L with a simple I2C scan
4. Ensure motor is functional (test with another driver)

---

**Ready to flash!** 🚀

```bash
west build -b xiao_ble_sense_nrf52840 && west flash
```


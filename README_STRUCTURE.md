# Peripheral UART - Project Structure

## Overview

This project implements a Nordic UART Service (NUS) bridge with a scalable, domain-driven architecture designed for easy expansion with additional features like haptics, power management, and more threads.

## Directory Structure

```
peripheral_uart/
├── src/
│   ├── main.c                          # Thin main: boot, wire modules, start scheduler
│   │
│   ├── include/                        # Public module interfaces
│   │   ├── ble/
│   │   │   └── ble_service.h          # BLE service API
│   │   ├── uart/
│   │   │   └── uart_service.h         # UART service API
│   │   ├── gpio/
│   │   │   └── gpio.h                 # GPIO/LED/Button API
│   │   ├── os/
│   │   │   └── threads.h              # Thread management API
│   │   ├── power/
│   │   │   └── power_mgmt.h           # Power management API
│   │   └── haptics/
│   │       └── haptics.h              # (Future) Haptics driver API
│   │
│   ├── services/                       # Communication services
│   │   ├── ble/
│   │   │   └── ble_service.c          # BLE/NUS implementation
│   │   └── uart/
│   │       └── uart_service.c         # UART communication
│   │
│   ├── drivers/                        # Hardware drivers
│   │   ├── gpio.c                     # GPIO/LED/Button driver
│   │   └── haptics/
│   │       └── haptics.c              # (Future) Haptics driver
│   │
│   ├── os/                            # OS-level components
│   │   └── threads/
│   │       └── threads.c              # Thread definitions & management
│   │
│   ├── power/                         # Power management
│   │   └── power_mgmt.c               # Power state management
│   │
│   └── Kconfig                        # Application configuration options
│
├── CMakeLists.txt                     # Build configuration
├── Kconfig                            # Top-level Kconfig
├── prj.conf                           # Project configuration
└── README_STRUCTURE.md                # This file
```

## Design Principles

### 1. Domain-Driven Organization

Modules are grouped by responsibility:
- **`services/`** - High-level communication protocols (BLE, UART)
- **`drivers/`** - Hardware abstraction (GPIO, haptics)
- **`os/`** - Operating system components (threads, scheduling)
- **`power/`** - Power management and optimization

### 2. Clean Include Paths

All public headers are in `src/include/` with subdirectories matching their domain:
```c
#include <ble/ble_service.h>      // Not "../services/ble/ble_service.h"
#include <uart/uart_service.h>
#include <gpio/gpio.h>
```

This is achieved via CMake:
```cmake
target_include_directories(app PRIVATE src/include)
```

### 3. Encapsulated IPC

Modules expose **functions**, not raw FIFOs or queues:

**Bad (tight coupling):**
```c
struct k_fifo *uart_get_rx_fifo(void);  // Exposes internal implementation
k_fifo_get(uart_get_rx_fifo(), K_FOREVER);
```

**Good (encapsulation):**
```c
struct uart_data_t *uart_get_rx_data(void);  // Clean API
uart_get_rx_data();  // Implementation hidden
```

### 4. Kconfig-Driven Configuration

Thread parameters are configurable without code changes:

```kconfig
CONFIG_APP_BLE_WRITE_STACK_SIZE=2048
CONFIG_APP_BLE_WRITE_PRIORITY=7
CONFIG_APP_LED_BLINK_STACK_SIZE=1024
CONFIG_APP_LED_BLINK_PRIORITY=7
```

Edit `prj.conf` to tune:
```
CONFIG_APP_BLE_WRITE_STACK_SIZE=4096
CONFIG_APP_BLE_WRITE_PRIORITY=5
```

### 5. Thread Ownership

Each thread is defined near its domain code with a clean init API:

```c
// In os/threads/threads.c
K_THREAD_DEFINE(ble_write_thread_id, 
                CONFIG_APP_BLE_WRITE_STACK_SIZE,
                ble_write_thread_entry, 
                NULL, NULL, NULL, 
                CONFIG_APP_BLE_WRITE_PRIORITY, 0, 0);

// Public API in os/threads.h
void threads_init(void);
```

## Module Descriptions

### Services

#### BLE Service (`services/ble/`)
- **Purpose:** Bluetooth LE communication and NUS protocol
- **API:** `<ble/ble_service.h>`
- **Key Functions:**
  - `ble_service_init()` - Initialize BLE stack
  - `ble_start_advertising()` - Start advertising
  - `ble_send_data()` - Send data over NUS
  - `ble_confirm_passkey()` - Handle pairing

#### UART Service (`services/uart/`)
- **Purpose:** UART communication with async API
- **API:** `<uart/uart_service.h>`
- **Key Functions:**
  - `uart_service_init()` - Initialize UART
  - `uart_transmit()` - Send data
  - `uart_get_rx_data()` - Receive data (blocks until available)

### Drivers

#### GPIO Driver (`drivers/gpio.c`)
- **Purpose:** LED and button control
- **API:** `<gpio/gpio.h>`
- **Key Functions:**
  - `gpio_init()` - Initialize GPIOs
  - `gpio_set_led()` - Control LED state
  - `gpio_toggle_led()` - Toggle LED
  - `gpio_error_state()` - Enter error mode

### OS Components

#### Thread Manager (`os/threads/`)
- **Purpose:** Centralized thread management
- **API:** `<os/threads.h>`
- **Threads:**
  - **BLE Write Thread** - Forwards UART RX → BLE TX
  - **LED Blink Thread** - Status indicator
- **Key Functions:**
  - `threads_init()` - Initialize all threads

### Power Management

#### Power Manager (`power/power_mgmt.c`)
- **Purpose:** Power state management (placeholder for future expansion)
- **API:** `<power/power_mgmt.h>`
- **Key Functions:**
  - `power_mgmt_init()` - Initialize power management
  - `power_mgmt_request_state()` - Request power state change
  - `power_mgmt_activity()` - Signal activity (reset idle timer)

## Data Flow

### UART → BLE (Outgoing)
```
UART RX → uart_service (FIFO) → BLE Write Thread → ble_send_data() → BLE TX
```

### BLE → UART (Incoming)
```
BLE RX → ble_service callback → main.c:on_ble_data_received() → uart_transmit() → UART TX
```

## Adding New Features

### Adding a New Thread

1. **Define thread entry function** (in appropriate module):
```c
void my_thread_entry(void) {
    while (1) {
        // Work
        k_sleep(K_MSEC(100));
    }
}
```

2. **Add Kconfig options** in `src/Kconfig`:
```kconfig
config APP_MY_THREAD_STACK_SIZE
    int "My thread stack size"
    default 1024

config APP_MY_THREAD_PRIORITY
    int "My thread priority"
    default 8
```

3. **Define thread** in `os/threads/threads.c`:
```c
K_THREAD_DEFINE(my_thread_id,
                CONFIG_APP_MY_THREAD_STACK_SIZE,
                my_thread_entry,
                NULL, NULL, NULL,
                CONFIG_APP_MY_THREAD_PRIORITY, 0, 0);
```

### Adding Haptics Driver

1. **Create header:** `src/include/haptics/haptics.h`
2. **Create implementation:** `src/drivers/haptics/haptics.c`
3. **Add to CMakeLists.txt:**
```cmake
target_sources(app PRIVATE
    src/drivers/haptics/haptics.c
)
```
4. **Add Kconfig options** in `src/Kconfig` (already prepared)
5. **Initialize in main.c:**
```c
#include <haptics/haptics.h>
err = haptics_init();
```

### Adding a New Service

1. **Create directory:** `src/services/my_service/`
2. **Create header:** `src/include/my_service/my_service.h`
3. **Create implementation:** `src/services/my_service/my_service.c`
4. **Update CMakeLists.txt**
5. **Initialize in main.c**

## Configuration

### Thread Tuning

Edit `prj.conf` or create a board-specific overlay:

```
# Increase BLE write thread stack
CONFIG_APP_BLE_WRITE_STACK_SIZE=4096

# Higher priority for BLE write
CONFIG_APP_BLE_WRITE_PRIORITY=5

# Enable power management
CONFIG_APP_POWER_MANAGEMENT=y
CONFIG_APP_IDLE_TIMEOUT_MS=30000
```

### Enabling Haptics (Future)

```
CONFIG_APP_HAPTICS=y
CONFIG_APP_HAPTICS_THREAD_STACK_SIZE=2048
CONFIG_APP_HAPTICS_THREAD_PRIORITY=8
```

## Build System

### CMakeLists.txt Structure

```cmake
# Include paths (allows <module/file.h> style)
target_include_directories(app PRIVATE src/include)

# Source files organized by domain
target_sources(app PRIVATE
    src/main.c
    src/services/uart/uart_service.c
    src/services/ble/ble_service.c
    src/drivers/gpio.c
    src/os/threads/threads.c
    src/power/power_mgmt.c
)
```

### Kconfig Integration

```
peripheral_uart/Kconfig → sources src/Kconfig → Application options
```

## Logging

Each module has its own log module:
```c
LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(uart_service, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(gpio, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(threads, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(power_mgmt, LOG_LEVEL_DBG);
```

Filter logs in `prj.conf`:
```
CONFIG_LOG_DEFAULT_LEVEL=3  # Info level
CONFIG_BLE_SERVICE_LOG_LEVEL_DBG=y  # Debug for BLE only
```

## Error Handling

- **Initialization errors:** Call `gpio_error_state()` (all LEDs off, infinite loop)
- **Runtime errors:** Log and continue (non-critical)
- **Memory allocation failures:** Log warning, graceful degradation

## Best Practices

1. **Keep main.c thin** - Only initialization and coordination
2. **Hide implementation details** - Expose functions, not data structures
3. **Use Kconfig for tuning** - Avoid hardcoded constants
4. **One responsibility per module** - Clear separation of concerns
5. **Document public APIs** - Clear header documentation
6. **Consistent naming** - `module_action()` pattern

## Future Enhancements

- [ ] Haptics driver implementation
- [ ] Power management thread with idle detection
- [ ] Watchdog thread
- [ ] Data processing thread
- [ ] Flash storage service
- [ ] Advanced error recovery
- [ ] Thread monitoring and statistics

## Building
west build -p -b xiao_ble/nrf52840/sense --sysbuild

## License

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Copyright (c) 2018 Nordic Semiconductor ASA  
Xavier Quintanilla


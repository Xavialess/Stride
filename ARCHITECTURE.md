# Project Architecture

## Overview

This project implements a Nordic UART Service (NUS) bridge using a modular, multithreaded architecture. The code has been refactored for better separation of concerns, maintainability, and scalability.

## Project Structure

```
peripheral_uart/
├── src/
│   ├── main.c                          # Main application entry point
│   ├── include/                        # Public header files
│   │   ├── uart_handler.h              # UART module interface
│   │   ├── ble_handler.h               # BLE module interface
│   │   ├── gpio_handler.h              # GPIO module interface
│   │   └── thread_manager.h            # Thread manager interface
│   └── modules/                        # Module implementations
│       ├── uart/
│       │   └── uart_handler.c          # UART implementation
│       ├── ble/
│       │   └── ble_handler.c           # BLE implementation
│       ├── gpio/
│       │   └── gpio_handler.c          # GPIO implementation
│       └── thread_manager/
│           └── thread_manager.c        # Thread manager implementation
├── CMakeLists.txt                      # Build configuration
├── prj.conf                            # Project configuration
├── ARCHITECTURE.md                     # Architecture documentation
└── MODULE_DIAGRAM.txt                  # Visual module diagram
```

### Folder Organization

- **`src/`** - Source code root
  - **`main.c`** - Application entry point
  - **`include/`** - All public header files (module interfaces)
  - **`modules/`** - Module implementations organized by functionality
    - Each module has its own subdirectory
    - Keeps related code together
    - Easy to add new modules

## Module Descriptions

### 1. Main Application (`main.c`)

**Responsibility:** Application initialization and coordination

**Key Functions:**
- `main()` - Entry point, initializes all modules
- `on_ble_data_received()` - Callback for BLE data reception

**Dependencies:** All other modules

**Thread:** Main thread (runs initialization, then sleeps)

---

### 2. UART Handler (`uart_handler.c/h`)

**Responsibility:** UART communication management

**Key Features:**
- Asynchronous UART operations
- TX/RX buffer management using FIFOs
- USB device support
- Line control (DTR/DCD/DSR)

**Key Functions:**
- `uart_handler_init()` - Initialize UART subsystem
- `uart_transmit()` - Send data over UART
- `uart_get_rx_fifo()` - Get RX FIFO for reading received data
- `uart_get_tx_fifo()` - Get TX FIFO for queued transmissions

**Data Structures:**
- `struct uart_data_t` - UART buffer structure
- `fifo_uart_tx_data` - TX FIFO queue
- `fifo_uart_rx_data` - RX FIFO queue

**Thread Safety:** Uses Zephyr FIFOs for thread-safe communication

---

### 3. BLE Handler (`ble_handler.c/h`)

**Responsibility:** Bluetooth LE communication and connection management

**Key Features:**
- BLE initialization and advertising
- Connection management
- Nordic UART Service (NUS) implementation
- Security and pairing (optional)
- Settings persistence

**Key Functions:**
- `ble_handler_init()` - Initialize BLE subsystem
- `ble_start_advertising()` - Start BLE advertising
- `ble_send_data()` - Send data over BLE NUS
- `ble_wait_init()` - Wait for BLE initialization (for threads)
- `ble_confirm_passkey()` - Handle pairing passkey confirmation

**Callbacks:**
- Connection/disconnection events
- Security events (if enabled)
- Data reception (forwarded to application callback)

**Thread Safety:** Uses semaphore for initialization synchronization

---

### 4. GPIO Handler (`gpio_handler.c/h`)

**Responsibility:** LED and button management

**Key Features:**
- LED control (status indicators)
- Button handling (for pairing confirmation)
- Error state indication

**Key Functions:**
- `gpio_handler_init()` - Initialize LEDs and buttons
- `gpio_set_led()` - Set LED on/off
- `gpio_toggle_led()` - Toggle LED state
- `gpio_error_state()` - Enter error state (infinite loop)

**LEDs:**
- `LED_RUN_STATUS` (LED1) - System running indicator (blinks)
- `LED_CON_STATUS` (LED2) - BLE connection status

**Buttons:**
- `BTN_PASSKEY_ACCEPT` (Button 1) - Accept pairing
- `BTN_PASSKEY_REJECT` (Button 2) - Reject pairing

---

### 5. Thread Manager (`thread_manager.c/h`)

**Responsibility:** Thread creation and management

**Key Features:**
- Centralized thread management
- Thread lifecycle control
- Easy addition of new threads

**Threads:**

1. **BLE Write Thread** (`ble_write_thread_entry`)
   - Priority: 7
   - Stack: CONFIG_BT_NUS_THREAD_STACK_SIZE
   - Purpose: Read data from UART RX FIFO and send over BLE
   - Waits for BLE initialization before starting

2. **LED Blink Thread** (`led_blink_thread_entry`)
   - Priority: 7
   - Stack: 1024 bytes
   - Purpose: Blink status LED to indicate system is running
   - Interval: 1 second

**Key Functions:**
- `thread_manager_init()` - Initialize thread management
- `ble_write_thread_entry()` - BLE write thread entry point
- `led_blink_thread_entry()` - LED blink thread entry point

---

## Data Flow

### UART → BLE (Outgoing Data)

```
UART RX → uart_cb() → fifo_uart_rx_data → BLE Write Thread → ble_send_data() → BLE TX
```

1. Data arrives on UART
2. UART callback (`uart_cb`) receives data
3. Data is placed in `fifo_uart_rx_data`
4. BLE write thread reads from FIFO
5. Data is sent over BLE NUS

### BLE → UART (Incoming Data)

```
BLE RX → bt_receive_cb() → on_ble_data_received() → uart_transmit() → UART TX
```

1. Data arrives over BLE
2. BLE callback (`bt_receive_cb`) receives data
3. Application callback (`on_ble_data_received`) is invoked
4. Data is transmitted over UART
5. If transmission fails, data is queued in `fifo_uart_tx_data`

---

## Thread Synchronization

### Semaphores
- `ble_init_ok` - Ensures BLE is initialized before threads use it

### FIFOs (Thread-Safe Queues)
- `fifo_uart_tx_data` - UART TX queue
- `fifo_uart_rx_data` - UART RX queue (read by BLE write thread)

### Work Queues
- `adv_work` - Advertising work item
- `uart_work` - UART buffer allocation work item

---

## Adding New Threads

To add a new thread to the system:

1. **Define thread entry function in appropriate module:**
```c
void my_new_thread_entry(void)
{
    LOG_INF("My thread started");
    
    while (1) {
        // Thread work
        k_sleep(K_MSEC(1000));
    }
}
```

2. **Add thread definition in `thread_manager.c`:**
```c
K_THREAD_DEFINE(my_thread_id, STACK_SIZE, my_new_thread_entry, 
                NULL, NULL, NULL, PRIORITY, 0, 0);
```

3. **Declare in `thread_manager.h` if needed externally:**
```c
void my_new_thread_entry(void);
```

---

## Configuration

### Key Configuration Options (prj.conf)

- `CONFIG_BT_NUS_THREAD_STACK_SIZE` - BLE thread stack size
- `CONFIG_BT_NUS_UART_BUFFER_SIZE` - UART buffer size
- `CONFIG_BT_NUS_UART_RX_WAIT_TIME` - UART RX timeout
- `CONFIG_BT_NUS_SECURITY_ENABLED` - Enable BLE security/pairing
- `CONFIG_UART_ASYNC_API` - Enable async UART API

---

## Error Handling

### Initialization Errors
- All module init functions return error codes
- Main application calls `gpio_error_state()` on critical errors
- Error state: All LEDs off, infinite loop

### Runtime Errors
- Logged using Zephyr logging system
- Non-critical errors are logged but don't halt execution
- Memory allocation failures are logged and handled gracefully

---

## Benefits of This Architecture

1. **Separation of Concerns**
   - Each module has a single, well-defined responsibility
   - Easy to understand and maintain

2. **Modularity**
   - Modules can be tested independently
   - Easy to replace or upgrade individual components

3. **Scalability**
   - Easy to add new threads
   - Easy to add new features to existing modules

4. **Thread Safety**
   - Uses Zephyr primitives (FIFOs, semaphores)
   - Clear data flow between threads

5. **Maintainability**
   - Clear module boundaries
   - Well-documented interfaces
   - Consistent coding style

6. **Reusability**
   - Modules can be reused in other projects
   - Generic interfaces

---

## Future Enhancements

Potential areas for expansion:

1. **Additional Threads**
   - Sensor data collection thread
   - Data processing thread
   - Watchdog thread

2. **Additional Modules**
   - Power management module
   - Flash storage module
   - Protocol handler module

3. **Enhanced Features**
   - Dynamic thread priority adjustment
   - Thread monitoring and statistics
   - Advanced error recovery

---

## Build and Flash

```bash
# Build the project
west build -b <your_board>

# Flash to device
west flash

# View logs
west debug
```

---

## Debugging

### Enable Debug Logging

In `prj.conf`:
```
CONFIG_LOG_DEFAULT_LEVEL=4  # Debug level
```

### View Thread Information

Use Zephyr shell commands:
```
kernel threads
kernel stacks
```

---

## License

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Copyright (c) 2018 Nordic Semiconductor ASA
Xavier Quintanilla


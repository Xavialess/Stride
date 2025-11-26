#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_uart)

# NORDIC SDK APP START

# Add include directories (allows #include <ble/ble_service.h> style)
target_include_directories(app PRIVATE
  src/include
)

# Add source files organized by domain
target_sources(app PRIVATE
  # Main application
  src/main.c
  
  # Services
  src/services/uart/uart_service.c
  src/services/ble/ble_service.c
  src/services/haptics/haptic_service.c
  
  # Drivers
  src/drivers/gpio.c
  src/drivers/haptics/drv2605l.c
  
  # OS components
  src/os/threads/threads.c
)

# NORDIC SDK APP END

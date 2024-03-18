# Minimum version
cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include( pico_sdk_import.cmake )

# Setup project
project( piPicoWiFiStation CXX C ASM )

# Set to use polling
set( use_polling ON )

# Set to enable watchdog timer
set( use_watchdog ON )


set(PICO_BOARD pico_w)          # Obviously Pi Pico-W necessary
set(CMAKE_C_STANDARD 11)        # C11
set(CMAKE_CXX_STANDARD 17)      # C++17
set(CMAKE_OBJECT_PATH_MAX 300)

# From Pico examples
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

# CYW43 must be target
if (NOT TARGET pico_cyw43_arch AND TARGET tinyusb_device)
    message("Skipping build as WiFi and USB support is not available")

else()

    # Source files
    add_executable(
        piPicoWiFiStation
        src/wiFiStation.cpp
        example.cpp
    )

    # Header files
    target_include_directories( 
        piPicoWiFiStation PUBLIC
        ./include                          
    )

    # Necessary libraries
    target_link_libraries(
        piPicoWiFiStation
        pico_stdlib
        pico_time
    )

    if( ${use_polling} )
        target_link_libraries(
            piPicoWiFiStation
            pico_cyw43_arch_lwip_poll 
        )
        message("Using polling")
        target_compile_definitions( piPicoWiFiStation PUBLIC USE_POLLING)
    else()
        target_link_libraries(
            piPicoWiFiStation
            pico_cyw43_arch_lwip_threadsafe_background
        )
    endif()

    
    if( ${use_watchdog} )
        target_link_libraries(
            piPicoWiFiStation
            hardware_watchdog
        )
        message("Using Watchdog")
        target_compile_definitions( piPicoWiFiStation PUBLIC USE_WATCHDOG)
    endif()     


    pico_enable_stdio_usb( piPicoWiFiStation 1 )     # Enable serial data over USB
    pico_enable_stdio_uart( piPicoWiFiStation 0 )    # Disable serial data over UART

    pico_add_extra_outputs( piPicoWiFiStation )

    # Options for compilation warnings
    add_compile_options(
        -Wall
        -Wno-psabi
        #-Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        #-Wno-unused-function # we have some for the docs that aren't called
        #-Wno-maybe-uninitialized
    )

endif()
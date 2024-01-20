# piPico_WiFiStation
Pi Pico library to connect to WiFis in station mode.

# Description
This is my approach to put the Pi Pico W's Wifi-functionality into its own class. The class handles scanning for networks and connecting to networks. The project uses the non blocking cyw43_arch_[...] library.

# Installation
I am assuming that you know how to configure your own CMakeLists and build environment. If that is not the case read here: https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html
The CMakeLists is configured to build the example. If you want to use the library in your own project copy the source and header files into your project and link necessary pico libraries. See given CMakeLists for details.

## Beware
Class functions are not thoroughly tested. So check for your application the edge cases. Also the authentification types which are returned by pico_cyw43_arch library functions are not documented. See "getAuthentificationFromScanResult()" method for details.

## Example
The example uses the UART over USB for an interface with the user. When powered on the Pi Pico waits some seconds and scans for networks. You can choose a network and enter the password. You will be notified when the connection succeeds or fails.
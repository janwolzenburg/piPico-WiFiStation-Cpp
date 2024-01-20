/*!
 * @file example.cpp
 * @author janwolzenburg: (kontakt@janwolzenburg.de)
 * @brief 
 * @version 1.0
 * @date 2024-01-20
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "wiFiStation.h"


/*!
 * @brief Read line from serial interface
 * 
 * @param only_numbers When true only accept numerical characters 
 * @return string String with read characters
 */
string readLine( const bool only_numbers = false );


/*!
 * @brief Callback for repeating timer
 * 
 * @param timer Timer that called the callback 
 * @return true Continue timer
 * @return false Stop timer
 */
bool toggleLed( repeating_timer_t *timer );


int main( void ){
    
    // Init Pi Pico
    stdio_init_all();
    sleep_ms( 500 );

    // LED blinking
    repeating_timer led_timer;
    bool toggle_led = false;
    add_repeating_timer_ms( 500, toggleLed, &toggle_led, &led_timer );

    // Time to start up serial terminal and not miss any output
    sleep_ms( 4500 );

    // Initialise wifi chip
    WiFiStation::initialise( CYW43_COUNTRY_GERMANY );

    // Scan for networks and wait until scan is finished or 10 seconds passed
    WiFiStation::scanForWifis();
    absolute_time_t scan_end = make_timeout_time_ms( 5000 );
    while( WiFiStation::isScanActive() && get_absolute_time() < scan_end );
    

    // Get and print available wifis
    vector<cyw43_ev_scan_result_t> available_networks = WiFiStation::getAvailableWifis();

    uint16_t current_id = 0;
    for( const auto& network : available_networks ){
        printf("ID: %u   SSID: %-32s   RSSI: %4d dBm   Ch.: %3d   MAC: %02x:%02x:%02x:%02x:%02x:%02x   Sec.: %u\r\n",
            current_id++,
            network.ssid, network.rssi, network.channel,
            network.bssid[0], network.bssid[1], network.bssid[2], network.bssid[3], network.bssid[4], network.bssid[5],
            network.auth_mode);
    }

    // Stop execution when no networks are available
    if( available_networks.empty() ){
        printf( "No networks available. Shutdown...\r\n" );
        return -1;
    }


    // Get network id to connect to
    printf("\r\nEnter network ID to connect to: ");
    const string id_string = readLine( true );

    if( id_string.empty() ){
        printf( "No ID given. Shutdown...\r\n" );
        return -1;
    }

    // Convert to integer
    const uint32_t id = strtoul( id_string.c_str(), nullptr, 10 );
    if( id > available_networks.size() - 1 ){
        printf( "ID invalid. Shutdown...\r\n" );
        return -1;
    }

    // Get selected wifi and authentification mode
    const cyw43_ev_scan_result_t& selected_wifi = available_networks.at( id );
    const uint32_t authentification = WiFiStation::getAuthentificationFromScanResult( selected_wifi.auth_mode );

    // Get password for authentification when necessary
    string password{""};
    if( authentification != CYW43_AUTH_OPEN ){
        printf( "Enter password: " );
        password = readLine();
    }


    // Instantiate wifi station
    WiFiStation station{ string{ (const char*)( selected_wifi.ssid ) }, password, authentification };
    
    // Start connection
    station.connect();

    // Flag to know when connection was made
    bool wifi_connected = false;


    // Main loop
    while( true ){

        // Do your networking stuff
        if( !wifi_connected && station.connected() ){
            printf( "Connected to network!\r\n" );
            wifi_connected = true;
        }

        // Toggle LED
        if( toggle_led ){
            toggle_led = false;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !cyw43_arch_gpio_get( CYW43_WL_GPIO_LED_PIN ) );
        }

    }

    station.disconnect();
    WiFiStation::deinitialise();

    return 0;
}


string readLine( const bool only_numbers ){

    string input_characters{""};
    char current_character = '\0';

    // Flush all buffered characters
    while( getchar_timeout_us( 0 ) != PICO_ERROR_TIMEOUT );

    // Read until linebreak character
    while( true ){
        current_character = getchar_timeout_us( 0 );
        
        // No input
        if( current_character == PICO_ERROR_TIMEOUT )
            continue;

        // Newline characters? Echo newline and break loop
        if( current_character == '\r' ||current_character == '\n' ){
            printf( "\r\n" );
            break;
        }

        // Delete character
        if( current_character == 127 && !input_characters.empty() ){
            printf( "%c", current_character );
            input_characters.pop_back();
        }

        // Valid character? Store
        if( current_character >= ' ' && current_character <= '~' && 
            ( !only_numbers || ( current_character >= '0' && current_character <= '9' ) ) ){

            printf( "%c", current_character );
            input_characters.push_back( current_character );
        }
    }

    return input_characters;
}


bool toggleLed( repeating_timer_t *timer ){
    *static_cast<bool*>( timer->user_data ) = true;
    return true;
}
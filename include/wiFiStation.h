#ifndef WIFISTATION_H
#define WIFISTATION_H

/*!
 * @file wiFiStation.h
 * @author janwolzenburg (kontakt@janwolzenburg.de)
 * @brief 
 * @version 1.0
 * @date 2024-01-20
 * 
 */

#include <string>
using std::string;
#include <vector>
using std::vector;

#include "pico/time.h"
#include "pico/cyw43_arch.h"

// Max length of ssid
constexpr size_t ssid_size = sizeof( cyw43_ev_scan_result_t::ssid );
// Max length of password
constexpr size_t password_size = 63;


/*!
 * @brief Class to connect to a wifi as a station
 * @details Uses polling - so static functions poll() must be called regularly 
 */
class WiFiStation{

    public:

    WiFiStation( const string ssid, const string password, const uint32_t authentification );

    WiFiStation( void );

    WiFiStation( const WiFiStation& wifi_station ) = delete;

    ~WiFiStation( void );

    WiFiStation& operator=( const WiFiStation& wifi_station ) = delete;


    static int initialise( uint32_t country );

    static void deinitialise( void );

    static int scanForWifis( void );

    static bool isScanActive( void );

    static vector<cyw43_ev_scan_result_t> getAvailableWifis( void );

    static uint32_t getAuthentificationFromScanResult( const uint8_t authentification_from_scan );


    int connect( void );

    int disconnect( void );

    bool connected( void ) const{ return connected_; };


    private:

    string ssid_;                   /*!<*/
    string password_;               /*!<*/
    uint32_t authentification_;     /*!<*/
    bool connected_;                /*!<*/

    static bool one_instance_connecting_;           /*!<*/
    static bool one_instance_connected_;            /*!<*/   
    static class WiFiStation* connected_station_;   /*!<*/

    static repeating_timer_t connection_check_timer_;       /*!<*/
    static vector<cyw43_ev_scan_result_t> available_wifis_; /*!<*/
    

    static int scanResult( void *available_wifis_void_ptr, const cyw43_ev_scan_result_t *result );


    static bool checkConnection( repeating_timer_t* timer );


};

#endif
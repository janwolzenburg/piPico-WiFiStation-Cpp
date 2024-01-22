#ifndef WIFISTATION_H
#define WIFISTATION_H

/*!
 * @file wiFiStation.h
 * @author janwolzenburg
 * @brief Class definition of WiFiStation
 * @version 1.1
 * @date 2024-01-22
 * 
 */

#include <string>
using std::string;
#include <vector>
using std::vector;

#include "pico/time.h"
#include "pico/cyw43_arch.h"

#define DEBUG               // If defined debug messages will be printed

// Max length of ssid
constexpr size_t ssid_size = sizeof( cyw43_ev_scan_result_t::ssid );
// Max length of password
constexpr size_t password_size = 63;


/*!
 * @brief Class to connect to a wifi as a station
 * @details Connect to one wifi network. When connection is lost - instance will retry to connect regularly.
 *          It should be possible to have more than one instance. But only one intstance can be connected.
 *          Reqiures one timer slot. Not tested with multithreading
 */
class WiFiStation{

    public:

    static uint32_t connection_check_interval;      /*!<Time in milliseconds to check connection status*/

    /*!
     * @brief Constructor
     * 
     * @param ssid WiFi network SSID
     * @param password Password. Empty when open
     * @param authentification Authentification type. As defined in cyw43_ll.h 
     */
    WiFiStation( const string ssid, const string password, const uint32_t authentification );

    /*!
     * @brief Default constructor
     * 
     */
    WiFiStation( void );

    /*!
     * @brief No copy contructor
     *
     */
    WiFiStation( const WiFiStation& wifi_station ) = delete;

    /*!
     * @brief Destructor. Disconnects
     * 
     */
    ~WiFiStation( void );

    /*!
     * @brief Copy assignment deleted
     *
     */
    WiFiStation& operator=( const WiFiStation& wifi_station ) = delete;

    /*!
     * @brief Move assignment. Disconnects this and connects to network of wifi_station if connected
     * 
     * @param wifi_station Station to move data from
     * @return WiFiStation& Reference to this
     */
    WiFiStation& operator=( WiFiStation&& wifi_station );

    /*!
     * @brief Initialise CYW43
     * 
     * @param country Your country. From cyw43_country.h
     * @return int 0 when successful
     */
    static int initialise( uint32_t country = CYW43_COUNTRY_WORLDWIDE);

    /*!
     * @brief Disconnect and deinitialise CYW43
     * 
     */
    static void deinitialise( void );

    /*!
     * @brief Start scan for access points
     * 
     * @return int 0 on success
     */
    static int scanForWifis( void );

    /*!
     * @brief Check if a scan is currently performed
     * 
     * @return true When a scan for networks is currently active
     * @return false Otherwise
     */
    static bool isScanActive( void );

    /*!
     * @brief Get available networks found on the last scan
     * 
     * @return vector<cyw43_ev_scan_result_t> All networks sorted by RSSI
     */
    static vector<cyw43_ev_scan_result_t> getAvailableWifis( void );

    /*!
     * @brief Convert authentification type from scan result to the correct CYW43_AUTH_[...] type 
     * 
     * @param authentification_from_scan Authentification type from scan
     * @return uint32_t CYW43 authentification type
     */
    static uint32_t getAuthentificationFromScanResult( const uint8_t authentification_from_scan );


    /*!
     * @brief Connect this station to network
     * @details Does return directly. Check with connected() whether connection was successful
     * 
     * @param is_reconnect Flag to indicate whether connection is a reconnect after connection lost
     * @return int 0 on successful start of connection process
     */
    int connect( const bool is_reconnect = false );

    /*!
     * @brief Disconnect this station
     * 
     * @return int -1 when station was not connected
     */
    int disconnect( void );

    /*!
     * @brief Get SSID
     *  
     * @return string The SSID
     */
    string ssid( void ) const{ return ssid_; };

    /*!
     * @brief Get password
     * 
     * @return string The password
     */
    string password( void ) const{ return password_; };

    /*!
     * @brief Get authentification mode
     * 
     * @return uint32_t The authentification mode
     */
    uint32_t authentification( void ) const{ return authentification_; };

    /*!
     * @brief Get whether station is connected
     * 
     * @param refresh_now Refresh before return
     * 
     * @return true When connected
     * @return false When not connected
     */
    bool connected( const bool refresh_now = false );

    /*!
     * @brief Stop current connection attemtps
     * 
     */
    void stopConnecting( void );


    private:

    string ssid_;                   /*!<SSID of network*/
    string password_;               /*!<Password of network*/
    uint32_t authentification_;     /*!<CYW43 authentification type*/
    bool connected_;                /*!<Flag if this station is connected*/
    

    static bool one_instance_connecting_;           /*!<Is one instance currently trying to connect*/
    static bool one_instance_connected_;            /*!<Is one instance connected*/   
    static int last_connection_state_;              /*!<Last connection status*/
    static class WiFiStation* connected_station_;   /*!<Pointer to instance which is connecting or connected*/

    static repeating_timer_t connection_check_timer_;       /*!<Repeating timer for connection check*/
    static vector<cyw43_ev_scan_result_t> available_wifis_; /*!<Available networks*/
    

    /*!
     * @brief Callback for network scan
     * 
     * @param available_wifis_void_ptr Void pointer to available_wifis_
     * @param result One result of scan
     * @return int Always 0
     */
    static int scanResult( void *available_wifis_void_ptr, const cyw43_ev_scan_result_t *result );

    /*!
     * @brief Start repeating connection check
     * 
     * @param interval Time in milliseconds to check the connection status
     * @return true Always
     * @return false Always
     */
    static bool startConnectionCheck( const uint32_t interval = connection_check_interval );

    /*!
     * @brief Stop repeating connection check
     * 
     * @return true Always 
     * @return false Always
     */
    static bool stopConnectionCheck( void );

    /*!
     * @brief Timer callback for repeated connection check
     * 
     * @param timer Pointer to repeating timer
     * @return true Always
     * @return false Never
     */
    static bool checkConnection( repeating_timer_t* timer );

};

#endif
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

    static int initialise( uint32_t country );

    static void deinitialise( void );

    static void poll( void );

    static int scanForWifis( void );

    static bool isScanActive( void );

    static vector<cyw43_ev_scan_result_t> getAvailableWifis( void );

    static uint32_t getAuthentificationFromScanResult( const uint8_t authentification_from_scan );

    WiFiStation( const string ssid, const string password, const uint32_t authentification ) : 
        ssid_( ssid ),
        password_( password ),
        authentification_( authentification ),
        connected_( false )
    {};

    WiFiStation( void ) :
        WiFiStation{ "", "", CYW43_AUTH_OPEN }
    {};

    ~WiFiStation( void ){
        disconnect();
    };

    WiFiStation( const WiFiStation& wifi_station ) = delete;

    WiFiStation& operator=( const WiFiStation& wifi_station ) = delete;

    WiFiStation& operator=( const WiFiStation&& wifi_station ){


        ssid_ = wifi_station.ssid_;
        password_ = wifi_station.password_;
        authentification_ = wifi_station.authentification_;
        connected_ = false;

        if( wifi_station.connected_ ){
            connect( 1, 500 );
        }

        return *this;
    }

    bool isConnected( void ){
        
        if( !connected_ )
            return false;

        int status = cyw43_tcpip_link_status( &cyw43_state, CYW43_ITF_STA ); 	

        return station_connected_ = connected_ = ( status == CYW43_LINK_UP );

    }

    bool connected( void ) const{ return connected_; };

    int connect( uint8_t tries, uint32_t timeout ){

        // Connected via different instance
        if( station_connected_ && !connected_ )
            return -1;


        // Already connected
        if( isConnected() )
            return 0;

        const char* password = nullptr;

        if( authentification_ != CYW43_AUTH_OPEN ){
            if( password_.empty() )
                return -1; 
            password = password_.c_str();
        } 

        if( ssid_.empty() )
            return -1;
        
        if( authentification_ != CYW43_AUTH_OPEN &&
            authentification_ != CYW43_AUTH_WPA2_AES_PSK &&
            authentification_ != CYW43_AUTH_WPA2_MIXED_PSK &&
            authentification_ != CYW43_AUTH_WPA_TKIP_PSK )
            return -1;

        uint8_t current_try = 0;
        int connection_status = 0;
        while( ( connection_status = cyw43_arch_wifi_connect_timeout_ms(    ssid_.c_str(), 
                                                                            password, 
                                                                            authentification_, 
                                                                            timeout ) ) != 0 &&
                current_try++ <= tries );

        if( connection_status == 0 ){
            connected_ = true;
            station_connected_ = true;
        }

        return connection_status;

    };

    int disconnect( void ){
        
        if( !connected_ )
            return -1;
        
        cyw43_wifi_leave( &cyw43_state, CYW43_ITF_STA );
        connected_ = false;
        station_connected_ = false;

        return 0;
    };


    private:
    string ssid_;
    string password_;
    uint32_t authentification_;
    bool connected_;

    static bool station_connected_;
    static vector<cyw43_ev_scan_result_t> available_wifis_;
    
    static int scanResult( void *available_wifis_void_ptr, const cyw43_ev_scan_result_t *result ){
        vector<cyw43_ev_scan_result_t>* available_wifis = static_cast<vector<cyw43_ev_scan_result_t>*>( available_wifis_void_ptr );
    
        if( result == nullptr) return 0;
        available_wifis->push_back( *result );
       
        
        return 0;
    }

};

#endif
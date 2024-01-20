/*!
 * @file wiFiStation.cpp
 * @author janwolzenburg (kontakt@janwolzenburg.de)
 * @brief 
 * @version 1.0
 * @date 2024-01-20
 * 
 */

#include "wiFiStation.h"


bool WiFiStation::station_connected_ = false;
vector<cyw43_ev_scan_result_t> WiFiStation::available_wifis_ = vector<cyw43_ev_scan_result_t>( 0, cyw43_ev_scan_result_t{} );

WiFiStation::WiFiStation( const string ssid, const string password, const uint32_t authentification ) : 
    ssid_( ssid ),
    password_( password ),
    authentification_( authentification ),
    connected_( false )
{};

WiFiStation::WiFiStation( void ) :
    WiFiStation{ "", "", CYW43_AUTH_OPEN }
{};

WiFiStation::~WiFiStation( void ){
    disconnect();
};

int WiFiStation::initialise( uint32_t country ){
    int return_code = cyw43_arch_init_with_country(CYW43_COUNTRY_GERMANY);
    if( return_code != 0 )
        return return_code;

    cyw43_arch_enable_sta_mode();
    return 0;
}

void WiFiStation::deinitialise( void ){
    cyw43_arch_deinit();
}

void WiFiStation::poll( void ){
    cyw43_arch_poll();
    // Check if still connected
};

int WiFiStation::scanForWifis( void ){

    cyw43_wifi_scan_options_t scan_options = {0};

    available_wifis_.clear();

    int scan_error = cyw43_wifi_scan( &cyw43_state, &scan_options, static_cast<void*>( &available_wifis_ ), scanResult );
    
    return scan_error;
    
};

bool WiFiStation::isScanActive( void ){
    return cyw43_wifi_scan_active( &cyw43_state );
};

vector<cyw43_ev_scan_result_t> WiFiStation::getAvailableWifis( void ){
    return available_wifis_;
};

uint32_t WiFiStation::getAuthentificationFromScanResult( const uint8_t authentification_from_scan ){
    uint32_t real_authentification_mode = CYW43_AUTH_OPEN;
                    
    // Don't know if this is correct
    switch( authentification_from_scan ){
        default:
            real_authentification_mode = CYW43_AUTH_OPEN;
        break;
        case 1: // WEP_PSK ???
            real_authentification_mode = CYW43_AUTH_WPA_TKIP_PSK;
        break;
        case 3: // WEP_PSK + WPA ???
            real_authentification_mode = CYW43_AUTH_WPA_TKIP_PSK;
        break;
        case 5: // WEP_PSK + WPA2 ??? This works
            real_authentification_mode = CYW43_AUTH_WPA2_MIXED_PSK;
        break;
    
    }

    return real_authentification_mode;
};
/*!
 * @file wiFiStation.cpp
 * @author janwolzenburg
 * @brief Implementation of WiFiStation class
 * @version 1.1
 * @date 2024-01-22
 * 
 */

#include <algorithm>
#include "wiFiStation.h"

#ifdef DEBUG
    #define DEPUG_PRINTF(...) printf("DEBUG: " __VA_ARGS__)
#else
    #define DEPUG_PRINTF(...) do {} while (0)
#endif


uint32_t WiFiStation::connection_check_interval = 500;

bool WiFiStation::one_instance_connecting_ = false;
bool WiFiStation::one_instance_connected_ = false;
int WiFiStation::last_connection_state_ = -10;
WiFiStation* WiFiStation::connected_station_ = nullptr;

repeating_timer_t WiFiStation::connection_check_timer_ = repeating_timer_t{};
vector<cyw43_ev_scan_result_t> WiFiStation::available_wifis_ = vector<cyw43_ev_scan_result_t>( 0, cyw43_ev_scan_result_t{} );


WiFiStation::WiFiStation( const string ssid, const string password, const uint32_t authentification ) : 
    ssid_( ssid ),
    password_( password ),
    authentification_( authentification ),
    connected_( false )
{
    if( ssid_.length() > ssid_size ){
        ssid_.erase( ssid_size );
        DEPUG_PRINTF( "SSID to long!\r\n" );
    }

    if( password_.length() > password_size ){
        password_.erase( password_size );
        DEPUG_PRINTF( "Password to long!\r\n" );
    }

    if( authentification_ != CYW43_AUTH_OPEN &&
        authentification_ != CYW43_AUTH_WPA2_AES_PSK &&
        authentification_ != CYW43_AUTH_WPA2_MIXED_PSK &&
        authentification_ != CYW43_AUTH_WPA_TKIP_PSK ){

        authentification_ = CYW43_AUTH_OPEN;
        DEPUG_PRINTF( "Authentification mode invalid!\r\n" );
    }
}


WiFiStation::WiFiStation( void ) :
    WiFiStation{ "", "", CYW43_AUTH_OPEN }
{}


WiFiStation::~WiFiStation( void ){
    disconnect();
}


WiFiStation& WiFiStation::operator=( WiFiStation&& wifi_station ){
    
    stopConnectionCheck();

    // Disconnect this station before copying from other
    if( connected_ )
        this->disconnect();

    // Copy data
    ssid_ = wifi_station.ssid_;
    password_ = wifi_station.password_;
    authentification_ = wifi_station.authentification_;
    connected_ = wifi_station.connected_;

    // If active station is source update pointer to this
    if( connected_station_ == &wifi_station ){
        connected_station_ = this;
    }

    startConnectionCheck();
    return *this;

}


int WiFiStation::initialise( uint32_t country ){
    int return_code = cyw43_arch_init_with_country( country );
    if( return_code != 0 ){
        DEPUG_PRINTF( "CYW43 initialisatiion failed with %i\r\n", return_code );
        return return_code;
    }

    // Enter station mode
    cyw43_arch_enable_sta_mode();

    // Cancel timer if registered
    cancel_repeating_timer( &connection_check_timer_ );

    return 0;

}


void WiFiStation::deinitialise( void ){
    if( connected_station_ != nullptr ){
        connected_station_->disconnect();
    }
    cyw43_arch_deinit();
}


int WiFiStation::scanForWifis( void ){

    cyw43_wifi_scan_options_t scan_options = {0};

    available_wifis_.clear();

    int scan_error = cyw43_wifi_scan( &cyw43_state, &scan_options, static_cast<void*>( &available_wifis_ ), scanResult );
    
    return scan_error;
    
}


bool WiFiStation::isScanActive( void ){
    return cyw43_wifi_scan_active( &cyw43_state );
}


vector<cyw43_ev_scan_result_t> WiFiStation::getAvailableWifis( void ){

    std::sort(  available_wifis_.begin(), available_wifis_.end(), 
                [](const cyw43_ev_scan_result_t& a, const cyw43_ev_scan_result_t& b)
                    { return a.rssi > b.rssi; });

    return available_wifis_;
}


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
}


int WiFiStation::connect( const bool is_reconnect ){

    // Already connected
    if( connected_ ){
        DEPUG_PRINTF( "This station already connected!\r\n" );
        return 0;
    }

    // Connected via different instance or connection is in progress
    if( one_instance_connected_ || one_instance_connecting_ ){
        DEPUG_PRINTF( "Different station already connected or trying to connect!\r\n" );
        return -1;
    }
        

    // Local char pointer for password
    const char* password = nullptr;

    // Check if password is giebn when necessary
    if( authentification_ != CYW43_AUTH_OPEN ){
        if( password_.empty() ){
            DEPUG_PRINTF( "Password cannot be ampty when network is not open!\r\n" );
            return -1; 
        }
        password = password_.c_str();
    } 

    // SSID given? 
    if( ssid_.empty() ){
        DEPUG_PRINTF("No SSID given!\r\n");
        return -1;
    }
        
    
    // Authetification valid?
    if( authentification_ != CYW43_AUTH_OPEN &&
        authentification_ != CYW43_AUTH_WPA2_AES_PSK &&
        authentification_ != CYW43_AUTH_WPA2_MIXED_PSK &&
        authentification_ != CYW43_AUTH_WPA_TKIP_PSK ){
        
        DEPUG_PRINTF("Authentification mode invalid!\r\n");
        return -1;
    }

    one_instance_connecting_ = true;
    connected_station_ = this;

    DEPUG_PRINTF("Connecting...\r\n");

    // Force leave of wifi before connecting to new
    cyw43_wifi_leave( &cyw43_state, CYW43_ITF_STA );

    // Try to connect non blocking
    int connection_status = cyw43_arch_wifi_connect_async(  ssid_.c_str(), 
                                                            password, 
                                                            authentification_ );

    if( connection_status != 0 ){
        DEPUG_PRINTF( "Could not start to connect. Error %u\r\n", connection_status );
        return -1;
    }

    // Add repeating timer. Longer interval when reconnecting
    if( startConnectionCheck( is_reconnect ? connection_check_interval * 4 : connection_check_interval ) == false ){
        DEPUG_PRINTF( "Repeating timer for connection check could not be started!\r\n" );   
        return -1;
    }

    return 0;
}


int WiFiStation::disconnect( void ){
    
    if( !connected_ )
        return -1;
    
    stopConnectionCheck();
    cyw43_wifi_leave( &cyw43_state, CYW43_ITF_STA );
    connected_ = false;
    one_instance_connected_ = false;
    connected_station_ = nullptr;

    return 0;
}


bool WiFiStation::connected( void ) const{ 
    checkConnection( &connection_check_timer_ );
    return connected_; 
}


void WiFiStation::stopConnecting( void ){
    if( one_instance_connecting_ && connected_station_ == this ){
        
        one_instance_connecting_ = false;
        connected_station_ = nullptr;
        connected_ = false;
        one_instance_connected_ = false;

        stopConnectionCheck();
        cyw43_wifi_leave( &cyw43_state, CYW43_ITF_STA );

    }
}


int WiFiStation::scanResult( void *available_wifis_void_ptr, const cyw43_ev_scan_result_t *result ){
    vector<cyw43_ev_scan_result_t>* available_wifis = static_cast<vector<cyw43_ev_scan_result_t>*>( available_wifis_void_ptr );

    if( result == nullptr) return 0;
    available_wifis->push_back( *result );
    
    
    return 0;
}


bool WiFiStation::startConnectionCheck( const uint32_t interval ){
    stopConnectionCheck();
    return add_repeating_timer_ms( interval, checkConnection, nullptr, &connection_check_timer_ );
}


bool WiFiStation::stopConnectionCheck( void ){
    return cancel_repeating_timer( &connection_check_timer_ );
}


bool WiFiStation::checkConnection( repeating_timer_t* timer ){

    // No station connected or connection -> leave but keep timer running
    if( connected_station_ == nullptr ){
        return true;
    }

    // Get current status
    int connection_status = cyw43_tcpip_link_status( &cyw43_state, CYW43_ITF_STA );

    // Check if still connected
    if( !one_instance_connecting_ && connected_station_->connected_ && 
        connection_status != CYW43_LINK_UP && connection_status != CYW43_LINK_NOIP ){
        
        // Previously a station was connected. Not anymore
        DEPUG_PRINTF( "Connection lost!\r\n" );
        connected_station_->connected_ = false;
        one_instance_connecting_ = true;

        // Restart check with longer interval
        startConnectionCheck( 4 * connection_check_interval );
    }


    // Print status change
    if( connection_status != last_connection_state_ ){
        // Check state
        switch( connection_status ){
            
            // Joining network
            case CYW43_LINK_JOIN: DEPUG_PRINTF( "Joining...\r\n" ); break;

            // Joined but no IP
            case CYW43_LINK_NOIP: DEPUG_PRINTF( "Connected, but no IP...\r\n" ); break;

            // Connection with ip
            case CYW43_LINK_UP: DEPUG_PRINTF( "Station connected!\r\n" ); break;

            // Bad authentification
            case CYW43_LINK_BADAUTH: DEPUG_PRINTF( "Bad authentification!\r\n" ); break;

            case CYW43_LINK_FAIL: DEPUG_PRINTF( "Link fail!\r\n" ); break;

            case CYW43_LINK_DOWN: DEPUG_PRINTF( "Link down!\r\n" );break;
            
            case CYW43_LINK_NONET: DEPUG_PRINTF( "No network!\r\n" ); break;

            default: break;
        }
    }

    // Save current connection status
    connected_station_->last_connection_state_ = connection_status;


    // One station trying to connect and link is up
    if( one_instance_connecting_ && connection_status == CYW43_LINK_UP ){
        one_instance_connected_ = true;
        one_instance_connecting_ = false;
        connected_station_->connected_ = true;            
        startConnectionCheck();
    }

    // One station trying to connect and authentification failure
    if( one_instance_connecting_ && connection_status == CYW43_LINK_BADAUTH ){
                       
        // Stop joining
        one_instance_connecting_ = false;                
        one_instance_connected_ = false;
        connected_station_ = nullptr;
        
        cyw43_wifi_leave( &cyw43_state, CYW43_ITF_STA );
        stopConnectionCheck();
    }

    return true;
}
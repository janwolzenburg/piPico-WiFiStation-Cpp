/*!
 * @file wiFiStation.cpp
 * @author janwolzenburg (kontakt@janwolzenburg.de)
 * @brief 
 * @version 1.0
 * @date 2024-01-20
 * 
 */

#include "wiFiStation.h"


bool WiFiStation::one_instance_connecting_ = false;
repeating_timer_t WiFiStation::connection_timer_ = repeating_timer_t{};
WiFiStation* WiFiStation::connecting_station_ = nullptr;
bool WiFiStation::one_instance_connected_ = false;
vector<cyw43_ev_scan_result_t> WiFiStation::available_wifis_ = vector<cyw43_ev_scan_result_t>( 0, cyw43_ev_scan_result_t{} );

WiFiStation::WiFiStation( const string ssid, const string password, const uint32_t authentification ) : 
    ssid_( ssid ),
    password_( password ),
    authentification_( authentification ),
    connected_( false ),
    connection_tries_left_( 0 )
{}

WiFiStation::WiFiStation( void ) :
    WiFiStation{ "", "", CYW43_AUTH_OPEN }
{}

WiFiStation::~WiFiStation( void ){
    disconnect();
}

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

    // Check if initial connection succeeded
    if( one_instance_connecting_ ){
        int connection_status = cyw43_wifi_link_status( &cyw43_state, CYW43_ITF_STA );

        // Check state
        switch( connection_status ){
            case CYW43_LINK_JOIN:
                one_instance_connected_ = true;
                if( connecting_station_ != nullptr ){
                    connecting_station_->connected_ = true;
                }
                
                cancel_connection();
            break;

            case CYW43_LINK_BADAUTH:
                cancel_connection();
            break;

            default:
            break;
        }

    }

    cyw43_arch_poll();
}

bool WiFiStation::cancel_connection( void ){
    one_instance_connecting_ = false;
    cancel_repeating_timer( &connection_timer_ );
    connecting_station_ = nullptr;
    return false;
}

bool WiFiStation::try_connect_callback( repeating_timer_t* timer ){

    WiFiStation* station = static_cast<WiFiStation*>( timer->user_data );

    // No tries left or other station is connected
    if( station->connection_tries_left_ == 0 || one_instance_connected_ )
        return cancel_connection();

    // Local char pointer for password
    const char* password = nullptr;

    // Check if password is giebn when necessary
    if( station->authentification_ != CYW43_AUTH_OPEN ){
        if( station->password_.empty() )
            return cancel_connection();; 
        password = station->password_.c_str();
    } 

    // SSID given? 
    if( station->ssid_.empty() )
        return cancel_connection();;
    
    // Authetification valid?
    if( station->authentification_ != CYW43_AUTH_OPEN &&
        station->authentification_ != CYW43_AUTH_WPA2_AES_PSK &&
        station->authentification_ != CYW43_AUTH_WPA2_MIXED_PSK &&
        station->authentification_ != CYW43_AUTH_WPA_TKIP_PSK )
        return cancel_connection();;

    // Try to connect non blocking
    int connection_status = cyw43_arch_wifi_connect_async(  station->ssid_.c_str(), 
                                                            password, 
                                                            station->authentification_ );

    // Maximum tries reached -> cancel timer
    if( --(station->connection_tries_left_) == 0 )
        return cancel_connection();

    // Continue timer
    return true;
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

WiFiStation& WiFiStation::operator=( const WiFiStation&& wifi_station ){


    ssid_ = wifi_station.ssid_;
    password_ = wifi_station.password_;
    authentification_ = wifi_station.authentification_;
    connected_ = false;

    if( wifi_station.connected_ ){
        connect( 1, 500 );
    }

    return *this;
}

bool WiFiStation::isConnected( void ){
    
    if( !connected_ )
        return false;

    int status = cyw43_tcpip_link_status( &cyw43_state, CYW43_ITF_STA ); 	

    return one_instance_connected_ = connected_ = ( status == CYW43_LINK_UP );

}

int WiFiStation::connect( uint8_t tries, uint32_t timeout ){

    // Connected via different instance or connection is in progress
    if( one_instance_connected_ && !connected_ && !one_instance_connecting_ )
        return -1;

    // Already connected
    if( isConnected() )
        return 0;

    // Set flags and connections state
    connection_tries_left_ = tries;

    // Add repeating timer
    if( add_repeating_timer_ms( timeout, try_connect_callback, this, &connection_timer_ ) == false )
        return -2;

    one_instance_connecting_ = true;
    connecting_station_ = this;

    return 0;

}

int WiFiStation::disconnect( void ){
    
    if( !connected_ )
        return -1;
    
    cyw43_wifi_leave( &cyw43_state, CYW43_ITF_STA );
    connected_ = false;
    one_instance_connected_ = false;

    return 0;
}
    
int WiFiStation::scanResult( void *available_wifis_void_ptr, const cyw43_ev_scan_result_t *result ){
    vector<cyw43_ev_scan_result_t>* available_wifis = static_cast<vector<cyw43_ev_scan_result_t>*>( available_wifis_void_ptr );

    if( result == nullptr) return 0;
    available_wifis->push_back( *result );
    
    
    return 0;
}
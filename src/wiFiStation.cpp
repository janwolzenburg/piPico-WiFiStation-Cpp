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
bool WiFiStation::one_instance_connected_ = false;
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
        printf( "SSID to long!\r\n" );
    }

    if( password_.length() > password_size ){
        password_.erase( password_size );
        printf( "Password to long!\r\n" );
    }

    if( authentification_ != CYW43_AUTH_OPEN &&
        authentification_ != CYW43_AUTH_WPA2_AES_PSK &&
        authentification_ != CYW43_AUTH_WPA2_MIXED_PSK &&
        authentification_ != CYW43_AUTH_WPA_TKIP_PSK ){

        authentification_ = CYW43_AUTH_OPEN;
        printf( "Authentification mode invalid!\r\n" );
    }
}

WiFiStation::WiFiStation( void ) :
    WiFiStation{ "", "", CYW43_AUTH_OPEN }
{}

WiFiStation::~WiFiStation( void ){
    disconnect();
}

int WiFiStation::initialise( uint32_t country ){
    int return_code = cyw43_arch_init_with_country( country );
    if( return_code != 0 ){
        printf( "CYW43 initialisatiion failed with %i\r\n", return_code );
        return return_code;
    }

    cyw43_arch_enable_sta_mode();

    // Add repeating timer
    if( add_repeating_timer_ms( 500, checkConnection, nullptr, &connection_check_timer_ ) == false ){
        printf( "Repeating timer for connection check could not be started!\r\n" );   
        return -2;
    }

    return 0;
}

void WiFiStation::deinitialise( void ){
    cyw43_arch_deinit();
}

bool WiFiStation::checkConnection( repeating_timer_t* timer ){

    // No station connected or connection -> leave but keep timer running
    if( connected_station_ == nullptr ){
        return true;
    }

    int connection_status = cyw43_wifi_link_status( &cyw43_state, CYW43_ITF_STA );

    // One station trying to connect?
    if( one_instance_connecting_ ){
        
        // Check state
        switch( connection_status ){
            
            // Connection up
            case CYW43_LINK_JOIN:
            case CYW43_LINK_UP:
                one_instance_connected_ = true;
                one_instance_connecting_ = false;
                connected_station_->connected_ = true;            
                
                printf( "Station connected!\r\n" );
            break;

            // Bad authentification
            case CYW43_LINK_BADAUTH:
                printf( "Bad authentification!\r\n" );
                one_instance_connecting_ = false;
            break;

            default:
                // Retry?
            break;
        }
        
    }

    // Check if still connected
    if( connected_station_->connected_ && connection_status != CYW43_LINK_UP && connection_status != CYW43_LINK_JOIN){
        printf( "Connection lost!\r\n" );
        connected_station_->connected_ = false;
        one_instance_connected_ = false;   
    }

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


int WiFiStation::connect( uint8_t tries, uint32_t timeout ){

    // Already connected
    if( connected_ ){
        printf( "This station already connected!\r\n" );
        return 0;
    }

    // Connected via different instance or connection is in progress
    if( one_instance_connected_ || one_instance_connecting_ ){
        printf( "Different station already connected or trying to connect!\r\n" );
        return -1;
    }
        

    // Local char pointer for password
    const char* password = nullptr;

    // Check if password is giebn when necessary
    if( authentification_ != CYW43_AUTH_OPEN ){
        if( password_.empty() ){
            printf( "Password cannot be ampty when network is not open!\r\n" );
            return -1; 
        }
        password = password_.c_str();
    } 

    // SSID given? 
    if( ssid_.empty() ){
        printf("No SSID given!\r\n");
        return -1;
    }
        
    
    // Authetification valid?
    if( authentification_ != CYW43_AUTH_OPEN &&
        authentification_ != CYW43_AUTH_WPA2_AES_PSK &&
        authentification_ != CYW43_AUTH_WPA2_MIXED_PSK &&
        authentification_ != CYW43_AUTH_WPA_TKIP_PSK ){
        
        printf("Authentification mode invalid!\r\n");
        return -1;
    }

    one_instance_connecting_ = true;
    connected_station_ = this;

    printf("Connecting...\r\n");

    // Try to connect non blocking
    int connection_status = cyw43_arch_wifi_connect_async(  ssid_.c_str(), 
                                                            password, 
                                                            authentification_ );

    if( connection_status != 0 ){
        printf( "Could not start to connect. Error %u\r\n", connection_status );
        return -1;
    }

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
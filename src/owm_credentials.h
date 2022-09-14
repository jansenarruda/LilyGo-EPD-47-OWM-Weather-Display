const bool DebugDisplayUpdate = false;

// Change to your WiFi credentials
const char* ssid     = "ssid";
const char* password = "password";

// Use your own API key by signing up for a free developer account at https://openweathermap.org/
String apikey       = "apikey";                      // See: https://openweathermap.org/
const char server[] = "api.openweathermap.org";
//http://api.openweathermap.org/data/2.5/forecast?q=Melksham,UK&APPID=your_OWM_API_key&mode=json&units=metric&cnt=40
//http://api.openweathermap.org/data/2.5/weather?q=Melksham,UK&APPID=your_OWM_API_key&mode=json&units=metric&cnt=1

//Set your location according to OWM locations
String City             = "Your city";                     // Your home city See: http://bulk.openweathermap.org/sample/
String Latitude         = "-28.5475";                      // Latitude of your location in decimal degrees
String Longitude        = "-45.6361";                      // Longitude of your location in decimal degrees
String Language         = "pt_br";                            // NOTE: Only the weather description is translated by OWM
String Country          = "BR";                            // Your _ISO-3166-1_two-letter_country_code country code, on OWM find your nearest city and the country code is displayed
                                                           // Examples: Arabic (AR) Czech (CZ) English (EN) Greek (EL) Persian(Farsi) (FA) Galician (GL) Hungarian (HU) Japanese (JA)
                                                           // Korean (KR) Latvian (LA) Lithuanian (LT) Macedonian (MK) Slovak (SK) Slovenian (SL) Vietnamese (VI)
String Hemisphere       = "south";                         // or "south"  
String Units            = "M";                             // Use 'M' for Metric or I for Imperial 
//const char* Timezone    = "<-03>3";                        // Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
                                                           // See below for examples
const char* Timezone = "BRT";      // Brazil 
const char* ntpServer   = "a.ntp.br";                      // Or, choose a time server close to you, but in most cases it's best to use pool.ntp.org to find an NTP server
                                                           // then the NTP system decides e.g. 0.pool.ntp.org, 1.pool.ntp.org as the NTP syem tries to find  the closest available servers
                                                           // EU "0.europe.pool.ntp.org"
                                                           // US "0.north-america.pool.ntp.org"
                                                           // See: https://www.ntppool.org/en/                                                           
int gmtOffset_sec     = -10800;                            // UK normal time is GMT, so GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800, BR SP (-3hrs) -10800
int daylightOffset_sec = 0;                                // In the UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset
int ntpTimeout          = 5000;                            // time (in milliseconds) to wait response from ntp server. Ajust to your network reality
gpio_num_t wakeupButton = GPIO_NUM_34;                     // button to wakeup, if don't want to use, commment next line (#define USE_BUTTON_WAKEUP)
// Example time zones
//const char* Timezone = "MET-1METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";       // Central Europe
//const char* Timezone = "EST-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "EST5EDT,M3.2.0,M11.1.0";           // EST USA  
//const char* Timezone = "CST6CDT,M3.2.0,M11.1.0";           // CST USA
//const char* Timezone = "MST7MDT,M4.1.0,M10.5.0";           // MST USA
//const char* Timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";      // Auckland
//const char* Timezone = "EET-2EEST,M3.5.5/0,M10.5.5/0";     // Asia
//const char* Timezone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3":   // Australia
//const char* Timezone = "BRT3BRST,M10.3.0/0,M2.3.0/0";      // Brazil 
#define USE_BLE
typedef struct {
  String name;
  String display_name;
  String mac_address;
  String cor;
} ble_sensors;

ble_sensors sensors[] = {
                          {"name","display name","mac addr","label"},
                          {"name","display name","mac addr","label"},
                          {"name","display name","mac addr","label"},
                        };

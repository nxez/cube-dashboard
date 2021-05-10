/**
Copyright (c) 2021 by NXEZ.com
See more at https://www.nxez.com
*/

// >>> Uncomment one of the following 2 lines to define which OLED display interface type you are using
//#define spiOLED
#define i2cOLED

#ifdef spiOLED
#include "SSD1306Spi.h"
#endif
#ifdef i2cOLED
#include "SSD1306Wire.h"
#include "SH1106.h"
#endif
#include "OLEDDisplayUi.h"

// Setup
const int UPDATE_INTERVAL_SECS = 5 * 60;
const int UPDATE_INTERVAL_SECS_DHT = 10;

#ifdef spiOLED
// Pin definitions for SPI OLED
#define OLED_CS D8    // Chip select
#define OLED_DC D2    // Data/Command
#define OLED_RESET D0 // RESET  - If you get an error on this line, either change Tools->Board to the board you are using or change "D0" to the appropriate pin number for your board.
#endif

#ifdef i2cOLED
// Pin definitions for I2C OLED
//const int I2C_DISPLAY_ADDRESS = 0x3C;
const int I2C_DISPLAY_ADDRESS = 0x3C;
const int SDA_PIN = D2;
const int SDC_PIN = D1;
#endif

// -----------------------------------
// Example Locales (uncomment only 1)
#define HoChiMinh
//------------------------------------

#ifdef HoChiMinh
//DST rules for Central European Time Zone
//#define UTC_OFFSET +8
// Uncomment for 24 Hour style clock
#define STYLE_24HR

#define NTP_SERVERS "pool.ntp.org"

#endif

#ifdef spiOLED
SSD1306Spi display(OLED_RESET, OLED_DC, OLED_CS); // SPI OLED
#endif
#ifdef i2cOLED
//SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN); // I2C OLED
SH1106 display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN); // I2C OLED
#endif

OLEDDisplayUi ui(&display);


// WIFI
const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PWD = "WIFI_PWD";

//MQTT Broker
char mqttBroker[16] = "192.168.1.204";
char mqttBrokerPort[6] = "1883";

/***************************
 * End Settings
 **************************/
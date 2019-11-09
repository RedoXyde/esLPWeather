#pragma once
#include "common.h"

#define CFG_SSID_SIZE 		32
#define CFG_PSK_SIZE  		64
#define CFG_HOSTNAME_SIZE 16

// Custom reporting: data is sent using POST "payload" data
#define CFG_REPORT_HOST_SIZE    32
#define CFG_REPORT_URL_SIZE     128
#define CFG_REPORT_DEFAULT_PORT 80
#define CFG_REPORT_DEFAULT_HOST "my.iot.server.com"
#define CFG_REPORT_DEFAULT_URL  "/?a=newEvent&uuid=aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee&token=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx&type=1"
#define CFG_REPORT_MSG_SIZE     90

// Port pour l'OTA
#define DEFAULT_OTA_PORT     8266
#define DEFAULT_OTA_AUTH     __appName "_OTA"
//#define DEFAULT_OTA_AUTH     ""

// Bit definition for different configuration modes
#define CFG_DEBUG	      0x0002	// Enable serial debug
#define CFG_BAD_CRC     0x8000  // Bad CRC when reading configuration

// Web Interface Configuration Form field names
#define CFG_FORM_SSID     FPSTR("ssid")
#define CFG_FORM_PSK      FPSTR("psk")
#define CFG_FORM_HOST     FPSTR("host")
#define CFG_FORM_AP_PSK   FPSTR("ap_psk")
#define CFG_FORM_OTA_AUTH FPSTR("ota_auth")
#define CFG_FORM_OTA_PORT FPSTR("ota_port")

#define CFG_FORM_REPORT_HOST  FPSTR("report_host")
#define CFG_FORM_REPORT_PORT  FPSTR("report_port")
#define CFG_FORM_REPORT_URL   FPSTR("report_url")
#define CFG_FORM_REPORT_MSG   FPSTR("report_msg")

#define CFG_FORM_IP  FPSTR("wifi_ip");
#define CFG_FORM_GW  FPSTR("wifi_gw");
#define CFG_FORM_MSK FPSTR("wifi_msk");

#pragma pack(push)  // push current alignment to stack
#pragma pack(1)     // set alignment to 1 byte boundary


// 256 Bytes
typedef struct
{
  char  host[CFG_REPORT_HOST_SIZE+1];     // 33  FQDN
  char  url[CFG_REPORT_URL_SIZE+1];       // 129 Post URL
  uint16_t port;                          // 2   Protocol port (HTTP/HTTPS)
  char  msg[CFG_REPORT_MSG_SIZE+1];       // 91  Message 
  uint8_t filler;                         // +1 = 256
} _report;


// Config saved into eeprom
// 1024 bytes total including CRC
typedef struct
{
  char  ssid[CFG_SSID_SIZE+1]; 		 //    33   SSID
  char  psk[CFG_PSK_SIZE+1]; 		   //    65   Pre shared key
  char  host[CFG_HOSTNAME_SIZE+1]; //    17   Hostname
  char  ap_psk[CFG_PSK_SIZE+1];    //    65   Access Point Pre shared key
  char  ota_auth[CFG_PSK_SIZE+1];  //    65   OTA Authentication password
  uint32_t config;           		   //     4   Bit field register
  uint16_t ota_port;         		   //     2   OTA port
  uint8_t  filler[131+128+256];      		   //   131   in case adding data in config avoiding loosing current conf by bad crc
  _report  report;                 //   256   Custom reporting configuration
  uint16_t crc;                    //     2   CRC
} _Config;                         // =1024


// Exported variables/object instancied in main sketch
// ===================================================
extern _Config config;

#pragma pack(pop)

// Declared exported function from route.cpp
// ===================================================
void cfgInit(void);
bool cfgRead(bool clear_on_error=true);
bool cfgSave(void);
void cfgShow(void);
void cfgReset(void);

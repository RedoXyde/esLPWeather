#include "config.h"

#include <EEPROM.h>

// Configuration structure for whole program
_Config config;

void cfgInit(void)
{
  // Clear our global flags
  config.config = 0;

  // Our configuration is stored into EEPROM
  // EEPROM.begin(sizeof(_Config));
  EEPROM.begin(1024);

  dbgF("Config size="); dbg(sizeof(_Config));
  dbgF("  report=");   dbg(sizeof(_report));
  dbg(")" EOL);
  dbgFlush();

    // Read Configuration from EEP
  if (cfgRead())
      dbgF("Good CRC!" EOL);
  else {
    cfgReset(); // Reset Configuration
    cfgSave(); // save back
    config.config |= CFG_BAD_CRC; // Indicate the error in global flags

    dbgF("Reset to default" EOL);
  }

}

uint16_t crc16Update(uint16_t crc, uint8_t a)
{
  int i;
  crc ^= a;
  for (i = 0; i < 8; ++i)  {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }
  return crc;
}

/* ======================================================================
Function: eeprom_dump
Purpose : dump eeprom value to serial
Input 	: -
Output	: -
Comments: -
====================================================================== */
void eepromDump(uint8_t bytesPerRow)
{
  uint16_t  i,
            j=0;

  // default to 16 bytes per row
  if (bytesPerRow==0)
    bytesPerRow=16;

  dbgF(EOL);

  // loop thru EEP address
  for (i = 0; i < sizeof(_Config); i++) {
    // First byte of the row ?
    if (j==0) {
			// Display Address
      dbg_s("%04X : ", i);
    }

    // write byte in hex form
    dbg_s("%02X ", EEPROM.read(i));

		// Last byte of the row ?
    // start a new line
    if (++j >= bytesPerRow) {
			j=0;
      dbgF(EOL);
		}
  }
}

/* ======================================================================
Function: readConfig
Purpose : fill config structure with data located into eeprom
Input 	: true if we need to clear actual struc in case of error
Output	: true if config found and crc ok, false otherwise
Comments: -
====================================================================== */
bool cfgRead(bool clear_on_error)
{
	uint16_t crc = ~0;
	uint8_t * pconfig = (uint8_t *) &config ;
	uint8_t data ;

	// For whole size of config structure
	for (uint16_t i = 0; i < sizeof(_Config); ++i) {
		// read data
		data = EEPROM.read(i);

		// save into struct
		*pconfig++ = data ;

		// calc CRC
		crc = crc16Update(crc, data);
	}

	// CRC Error ?
	if (crc != 0) {
		// Clear config if wanted
    if (clear_on_error)
		  memset(&config, 0, sizeof( _Config ));
		return false;
	}

	return true ;
}

bool cfgSave(void)
{
  uint8_t * pconfig ;
  bool ret_code;

  //eepromDump(32);

  // Init pointer
  pconfig = (uint8_t *) &config ;

	// Init CRC
  config.crc = ~0;

	// For whole size of config structure, pre-calculate CRC
  for (uint16_t i = 0; i < sizeof (_Config) - 2; ++i)
    config.crc = crc16Update(config.crc, *pconfig++);

	// Re init pointer
  pconfig = (uint8_t *) &config ;

  // For whole size of config structure, write to EEP
  for (uint16_t i = 0; i < sizeof(_Config); ++i)
    EEPROM.write(i, *pconfig++);

  // Physically save
  EEPROM.commit();

  // Read Again to see if saved ok, but do
  // not clear if error this avoid clearing
  // default config and breaks OTA
  ret_code = cfgRead(false);

  dbgF("Write config ");

  if (ret_code)
    dbgF("OK!" EOL);
  else
    dbgF("Error!" EOL);

  //eepromDump(32);

  // return result
  return (ret_code);
}

void cfgShow()
{
  dbgF("===== Wifi" EOL);
  dbgF("ssid     :"); dbg(config.ssid); dbgF(EOL);
  dbgF("psk      :"); dbg(config.psk); dbgF(EOL);
  dbgF("host     :"); dbg(config.host); dbgF(EOL);
  dbgF("ap_psk   :"); dbg(config.ap_psk); dbgF(EOL);
  dbgF("===== Network" EOL);
  dbgF("ip       :"); dbg(config.netcfg.ip); dbgF(EOL);
  dbgF("netmask  :"); dbg(config.netcfg.msk); dbgF(EOL);
  dbgF("gateway  :"); dbg(config.netcfg.gw); dbgF(EOL);
  dbgF("dns      :"); dbg(config.netcfg.dns); dbgF(EOL);
  dbgF("===== OTA" EOL);
  dbgF("OTA auth :"); dbg(config.ota_auth); dbgF(EOL);
  dbgF("OTA port :"); dbg(config.ota_port); dbgF(EOL);
  dbgF("===== System" EOL);
  dbgF("Config   :");
  if (config.config & CFG_DEBUG)   dbgF("DEBUG ");
  dbgF(EOL);
  dbgF("===== POST Reporting" EOL);
  dbgF("host     :"); dbg(config.report.host); dbgF(EOL);
  dbgF("port     :"); dbg(config.report.port); dbgF(EOL);
  dbgF("url      :"); dbg(config.report.url); dbgF(EOL);
  dbgF("msg      :"); dbg(config.report.msg); dbgF(EOL);

}


void cfgReset(void)
{
  // Start cleaning all that stuff
  memset(&config, 0, sizeof(_Config));

  // Set default Hostname
  sprintf_P(config.host, PSTR(__appName "-%06X"), ESP.getChipId());
  strcpy_P(config.ota_auth, PSTR(DEFAULT_OTA_AUTH));
  config.ota_port = DEFAULT_OTA_PORT ;

  // Add other init default config here
  strcpy_P(config.netcfg.ip , PSTR(CFG_NET_DEFAULT_IP));
  strcpy_P(config.netcfg.gw , PSTR(CFG_NET_DEFAULT_GW));
  strcpy_P(config.netcfg.msk, PSTR(CFG_NET_DEFAULT_MSK));
  strcpy_P(config.netcfg.dns, PSTR(CFG_NET_DEFAULT_DNS));

  strcpy_P(config.report.host, CFG_REPORT_DEFAULT_HOST);
  config.report.port = CFG_REPORT_DEFAULT_PORT;
  strcpy_P(config.report.url, CFG_REPORT_DEFAULT_URL);

  // save back
  cfgSave();
}

#include "app.h"
#include "ota.h"
#include "webserver.h"
#include "webclient.h"

bool wifiConnect(void);
int WifiHandleConn(boolean setup = false);

void configMode(void);

#define pinLED   05  // this is the on-board LED - write this LOW to turn on
#define pinDONE  15  // write this PIN HIGH to kill your own power
#define pinWAKE  16 // check this pin right when you boot up to see if the external switch woke you up - will be LOW if externally woken

#define VBATT_MIN 3.4

void powerOff(uint16_t d)
{
  dbgF("Poweroff attempt !" EOL);
  dbgFlush();
  digitalWrite(pinDONE, HIGH);
  delay(d);
  digitalWrite(pinDONE, LOW);
  delay(d);
}

void setup()
{
  system_update_cpu_freq(160);

  pinMode(pinWAKE, INPUT); 
  bool extWake = digitalRead(pinWAKE) == LOW;
  pinMode(pinDONE, OUTPUT);
  pinMode(pinLED,  OUTPUT); // Low to turn LED on

  sysinfo.vBatt = (4 - 3.5)/(712 - 621) ;
  sysinfo.vBatt = analogRead(A0) * sysinfo.vBatt + (4 - sysinfo.vBatt * 712);

  dbgInit();
  cfgInit();

  digitalWrite(pinLED, LOW);
  
  dbgF(EOL"" EOL"==============" EOL);
  dbgF("App "); dbgF(__version); dbgF(EOL);
  dbg_s("Wake source: %s" EOL, extWake ? "External": "Timer"); 
  dbg_s("vBatt: %f" EOL, sysinfo.vBatt);
  dbgFlush();

  if (extWake ||
      sysinfo.vBatt < VBATT_MIN
     ) 
  {
    // Connect to Wifi
    dbgF("Connect to Wifi" EOL);
    if(wifiConnect())
    {
      // Push
      dbgF("Push notification");
      if(!reportPost())
        dbgF(" failed" EOL);
      dbgF(EOL);
    }
  }

  powerOff(250);
  dbgF("Still up ! Go into enter config Mode" EOL);
  configMode();  
}

void loop()
{
  server.handleClient();
  ArduinoOTA.handle();
  //delay(10);
}

#define DEBUG_APP_WIFI

bool wifiConnect(void)
{
  if (!(*config.ssid))
    return false;
  int ret=0;

  uint16_t timeout = 500; // 25 * 200 ms = 5 sec time out
  #ifdef DEBUG_APP_WIFI
  dbgF("Connecting to: ");
  dbg(config.ssid);
  dbgFlush();
  #endif

  WiFi.mode(WIFI_STA);

  // Do wa have a PSK ?
  if (*config.psk) {
    // protected network
    #ifdef DEBUG_APP_WIFI
    dbgF(" with key '");
    dbg(config.psk);
    dbgF("'...");
    dbgFlush();
    #endif
    WiFi.begin(config.ssid, config.psk);
  } else {
    // Open network
    #ifdef DEBUG_APP_WIFI
    dbgF("unsecure AP");
    dbgFlush();
    #endif
    WiFi.begin(config.ssid);
  }

  while ( ((ret = WiFi.status()) != WL_CONNECTED) && timeout )
  {
    delay(20);
    --timeout;
  }

  // connected ? disable AP, client mode only
  #ifdef DEBUG_APP_WIFI
  if (ret == WL_CONNECTED)
  {
    dbgF("Connected!" EOL);
    dbg_s("IP address   : %s" EOL, WiFi.localIP().toString().c_str());
    dbg_s("MAC address  : %s" EOL, WiFi.macAddress().c_str());
  }
  #endif
  return ret == WL_CONNECTED;
}

void configMode(void)
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  //WiFi.mode(WIFI_AP_STA);
  //WiFi.disconnect();

  // Check File system init
  if (!SPIFFS.begin())
  {
    // Serious problem
    dbgF("SPIFFS Mount failed" EOL);
  } else {

    dbgF("SPIFFS Mount succesfull" EOL);

    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      dbg_s("FS File: %s, size: %d\n", fileName.c_str(), fileSize);
    }
    dbgF(EOL);
  }

  // start Wifi connect or soft AP
  WifiHandleConn(true);

  otaInit();
  webserverInit();

  // Display configuration
  cfgShow();

  dbgF("HTTP server started" EOL);
}

int WifiHandleConn(boolean setup)
{
  if (setup) 
  {

    dbgF("========== SDK Saved parameters Start" EOL);
    WiFi.printDiag(DEBUG_SERIAL);
    dbgF("========== SDK Saved parameters End" EOL);
    dbgFlush();

    // no correct SSID
    if (!*config.ssid) {
      dbgF("no Wifi SSID in config, trying to get SDK ones...");

      // Let's see of SDK one is okay
      if ( WiFi.SSID() == "" ) {
        dbgF("Not found may be blank chip!" EOL);
      } else {
        *config.psk = '\0';

        // Copy SDK SSID
        strcpy(config.ssid, WiFi.SSID().c_str());

        // Copy SDK password if any
        if (WiFi.psk() != "")
          strcpy(config.psk, WiFi.psk().c_str());

        dbgF("found one!" EOL);

        // save back new config
        cfgSave();
      }
    }

    if(!wifiConnect())
    {
      char ap_ssid[32];
      dbgF("Error!" EOL);
      dbgFlush();

      // STA+AP Mode without connected to STA, autoconnect will search
      // other frequencies while trying to connect, this is causing issue
      // to AP mode, so disconnect will avoid this

      // Disable auto retry search channel
      WiFi.disconnect();
      WiFi.mode(WIFI_AP);

      // SSID = hostname
      strcpy(ap_ssid, config.host );
      dbgF("Switching to AP ");
      dbg(ap_ssid);
      dbgF(EOL);
      dbgFlush();

      // protected network
      if (*config.ap_psk) {
        dbgF(" with key '");
        dbg(config.ap_psk);
        dbgF("'" EOL);
        WiFi.softAP(ap_ssid, config.ap_psk);
      // Open network
      } else {
        dbgF(" with no password" EOL);
        WiFi.softAP(ap_ssid);
      }
      //WiFi.mode(WIFI_AP_STA);

      dbg_s("IP address   : %s" EOL, WiFi.softAPIP().toString().c_str() );
      dbg_s("MAC address  : %s" EOL, WiFi.softAPmacAddress().c_str());
    }

    // Set OTA parameters
    ArduinoOTA.setPort(config.ota_port);
    ArduinoOTA.setHostname(config.host);
    ArduinoOTA.setPassword(config.ota_auth);
    ArduinoOTA.begin();

    // just in case your sketch sucks, keep update OTA Available
    // Trust me, when coding and testing it happens, this could save
    // the need to connect FTDI to reflash
    // Usefull just after 1st connexion when called from setup() before
    // launching potentially buggy main()
    for (uint8_t i=0; i<= 10; i++)
    {
      ArduinoOTA.handle();
    }
  } // if setup

  return WiFi.status();
}

_sysinfo sysinfo;

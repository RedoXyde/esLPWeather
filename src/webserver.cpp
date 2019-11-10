#include "app.h"
#include "config.h"
#include "webserver.h"

// Optimize string space in flash, avoid duplication
const char FP_JSON_START[] PROGMEM = "{\r\n";
const char FP_JSON_END[] PROGMEM = "\r\n}\r\n";
const char FP_QCQ[] PROGMEM = "\":\"";
const char FP_QCNL[] PROGMEM = "\",\r\n\"";
const char FP_RESTART[] PROGMEM = "OK, Redémarrage en cours\r\n";
const char FP_NL[] PROGMEM = "\r\n";

ESP8266WebServer server(80);

void spiffsJSONTable();


void webserverInit(void)
{

  server.on("/", handleRoot);
  server.on("/config_form.json", handleFormConfig);
  server.on("/system.json", sysJSONTable);
  server.on("/config.json", confJSONTable);
  server.on("/spiffs.json", spiffsJSONTable);
  server.on("/wifiscan.json", wifiScanJSON);
  server.on("/factory_reset", handleFactoryReset);
  server.on("/reset", handleReset);

  // handler for the hearbeat
  server.on("/hb.htm", HTTP_GET, [&](){
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/html", R"(OK)");
  });

  // handler for the /update form POST (once file upload finishes)
  server.on("/update", HTTP_POST,
    // handler once file upload finishes
    [&]() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    },
    // handler for upload, get's the sketch bytes,
    // and writes them through the Update object
    [&]() {
      HTTPUpload& upload = server.upload();

      if(upload.status == UPLOAD_FILE_START) {
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        WiFiUDP::stopAll();
        dbg_s("Update: %s\n", upload.filename.c_str());

        //start with max available size
        if(!Update.begin(maxSketchSpace))
          Update.printError(Serial1);

      } else if(upload.status == UPLOAD_FILE_WRITE) {
        dbgF(".");
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          Update.printError(Serial1);

      } else if(upload.status == UPLOAD_FILE_END) {
        //true to set the size to the current progress
        if(Update.end(true))
          dbg_s("Update Success: %u. Rebooting..." EOL, upload.totalSize);
        else
          Update.printError(Serial1);

      } else if(upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        dbgF("Update was aborted" EOL);
      }
    }
  );

  // All other not known
  server.onNotFound(handleNotFound);

  // serves all SPIFFS Web file with 24hr max-age control
  // to avoid multiple requests to ESP
  server.serveStatic("/font", SPIFFS, "/font","max-age=86400");
  server.serveStatic("/js",   SPIFFS, "/js"  ,"max-age=86400");
  server.serveStatic("/css",  SPIFFS, "/css" ,"max-age=86400");
  server.begin();
}

/* ======================================================================
Function: formatSize
Purpose : format a asize to human readable format
Input   : size
Output  : formated string
Comments: -
====================================================================== */
String formatSize(size_t bytes)
{
  if (bytes < 1024){
    return String(bytes) + F(" Byte");
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0) + F(" KB");
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0) + F(" MB");
  } else {
    return String(bytes/1024.0/1024.0/1024.0) + F(" GB");
  }
}

/* ======================================================================
Function: getContentType
Purpose : return correct mime content type depending on file extension
Input   : -
Output  : Mime content type
Comments: -
====================================================================== */
String getContentType(String filename) {
  if(filename.endsWith(".htm")) return F("text/html");
  else if(filename.endsWith(".html")) return F("text/html");
  else if(filename.endsWith(".css")) return F("text/css");
  else if(filename.endsWith(".json")) return F("text/json");
  else if(filename.endsWith(".js")) return F("application/javascript");
  else if(filename.endsWith(".png")) return F("image/png");
  else if(filename.endsWith(".gif")) return F("image/gif");
  else if(filename.endsWith(".jpg")) return F("image/jpeg");
  else if(filename.endsWith(".ico")) return F("image/x-icon");
  else if(filename.endsWith(".xml")) return F("text/xml");
  else if(filename.endsWith(".pdf")) return F("application/x-pdf");
  else if(filename.endsWith(".zip")) return F("application/x-zip");
  else if(filename.endsWith(".gz")) return F("application/x-gzip");
  else if(filename.endsWith(".otf")) return F("application/x-font-opentype");
  else if(filename.endsWith(".eot")) return F("application/vnd.ms-fontobject");
  else if(filename.endsWith(".svg")) return F("image/svg+xml");
  else if(filename.endsWith(".woff")) return F("application/x-font-woff");
  else if(filename.endsWith(".woff2")) return F("application/x-font-woff2");
  else if(filename.endsWith(".ttf")) return F("application/x-font-ttf");
  return "text/plain";
}

/* ======================================================================
Function: handleFileRead
Purpose : return content of a file stored on SPIFFS file system
Input   : file path
Output  : true if file found and sent
Comments: -
====================================================================== */
bool handleFileRead(String path) {
  if ( path.endsWith("/") )
    path += "index.htm";

  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";

  dbgF("handleFileRead ");
  dbg(path);

  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if( SPIFFS.exists(pathWithGz) ){
      path += ".gz";
      dbgF(".gz");
    }

    dbgF(" found on FS" EOL);

    File file = SPIFFS.open(path, "r");
    //size_t sent =
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  dbgF(EOL);

  server.send(404, "text/plain", "File Not Found");
  return false;
}

/* ======================================================================
Function: handleFormConfig
Purpose : handle main configuration page
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleFormConfig(void)
{
  String response="";
  int ret ;

  // We validated config ?
  if (server.hasArg("save"))
  {
    int itemp;
    dbgF("===== Posted configuration" EOL);

    // WifInfo
    strncpy(config.ssid ,   server.arg("ssid").c_str(),     CFG_SSID_SIZE );
    strncpy(config.psk ,    server.arg("psk").c_str(),      CFG_PSK_SIZE );
    strncpy(config.host ,   server.arg("host").c_str(),     CFG_HOSTNAME_SIZE );
    strncpy(config.ap_psk , server.arg("ap_psk").c_str(),   CFG_PSK_SIZE );
    strncpy(config.ota_auth,server.arg("ota_auth").c_str(), CFG_PSK_SIZE );
    itemp = server.arg("ota_port").toInt();
    config.ota_port = (itemp>=0 && itemp<=65535) ? itemp : DEFAULT_OTA_PORT ;

    strncpy(config.netcfg.ip,   server.arg(CFG_FORM_NET_IP).c_str(),  CFG_IP_ADDRESS_MAX_SIZE );
    strncpy(config.netcfg.gw,   server.arg(CFG_FORM_NET_GW).c_str(),  CFG_IP_ADDRESS_MAX_SIZE );
    strncpy(config.netcfg.msk,   server.arg(CFG_FORM_NET_MSK).c_str(),  CFG_IP_ADDRESS_MAX_SIZE );
    strncpy(config.netcfg.dns,   server.arg(CFG_FORM_NET_DNS).c_str(),  CFG_IP_ADDRESS_MAX_SIZE );

    // Report
    strncpy(config.report.host,   server.arg("report_host").c_str(),  CFG_REPORT_HOST_SIZE );
    strncpy(config.report.url,    server.arg("report_url").c_str(),   CFG_REPORT_URL_SIZE );
    strncpy(config.report.msg,    server.arg("report_msg").c_str(),   CFG_REPORT_MSG_SIZE );
    itemp = server.arg("report_port").toInt();
    config.report.port = (itemp>=0 && itemp<=65535) ? itemp : CFG_REPORT_DEFAULT_PORT ;

    if ( cfgSave() ) {
      ret = 200;
      response = "OK";
    } else {
      ret = 412;
      response = "Unable to save configuration";
    }

    cfgShow();
  }
  else
  {
    ret = 400;
    response = "Missing Form Field";
  }

  dbgF("Sending response ");
  dbg(ret);
  dbgF(":");
  dbg(response);
  dbgF(EOL);
  server.send ( ret, "text/plain", response);
}

/* ======================================================================
Function: handleRoot
Purpose : handle main page /
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleRoot(void)
{
  handleFileRead("/");
}

/* ======================================================================
Function: formatNumberJSON
Purpose : check if data value is full number and send correct JSON format
Input   : String where to add response
          char * value to check
Output  : -
Comments: 00150 => 150
          ADCO  => "ADCO"
          1     => 1
====================================================================== */
void formatNumberJSON( String &response, char * value)
{
  // we have at least something ?
  if (value && strlen(value))
  {
    boolean isNumber = true;
//    uint8_t c;
    char * p = value;

    // just to be sure
    if (strlen(p)<=16) {
      // check if value is number
      while (*p && isNumber) {
        if ( *p < '0' || *p > '9' )
          isNumber = false;
        p++;
      }

      // this will add "" on not number values
      if (!isNumber) {
        response += '\"' ;
        response += value ;
        response += F("\"") ;
      } else {
        // this will remove leading zero on numbers
        p = value;
        while (*p=='0' && *(p+1) )
          p++;
        response += p ;
      }
    } else {
      dbgF("formatNumberJSON error!" EOL);
    }
  }
}

/* ======================================================================
Function: getSysJSONData
Purpose : Return JSON string containing system data
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void getSysJSONData(String & response)
{
  response = "";
  char buffer[32];
  int32_t adc = ( 1000 * analogRead(A0) / 1024 );

  // Json start
  response += F("[\r\n");

  response += "{\"na\":\"Uptime\",\"va\":\"";
  response += millis()/1000;
  response += "\"},\r\n";

  response += "{\"na\":\"Battery (V)\",\"va\":\"";
  response += sysinfo.vBatt;
  response += "\"},\r\n";

  response += "{\"na\":\"Temperature (°C)\",\"va\":\"";
  response += sysinfo.temperature;
  response += "\"},\r\n";

  response += "{\"na\":\"Pressure (hPa)\",\"va\":\"";
  response += sysinfo.pressure;
  response += "\"},\r\n";

  response += "{\"na\":\"Humidity (%)\",\"va\":\"";
  response += sysinfo.humidity;
  response += "\"},\r\n";

  response += "{\"na\":\"Version\",\"va\":\"" __version "\"},\r\n";

  response += "{\"na\":\"Compile le\",\"va\":\"" __DATE__ " " __TIME__ "\"},\r\n";

  response += "{\"na\":\"SDK Version\",\"va\":\"";
  response += system_get_sdk_version() ;
  response += "\"},\r\n";

  response += "{\"na\":\"Chip ID\",\"va\":\"";
  sprintf_P(buffer, "0x%0X",system_get_chip_id() );
  response += buffer ;
  response += "\"},\r\n";

  response += "{\"na\":\"Boot Version\",\"va\":\"";
  sprintf_P(buffer, "0x%0X",system_get_boot_version() );
  response += buffer ;
  response += "\"},\r\n";

  response += "{\"na\":\"Flash Real Size\",\"va\":\"";
  response += formatSize(ESP.getFlashChipRealSize()) ;
  response += "\"},\r\n";

  response += "{\"na\":\"Firmware Size\",\"va\":\"";
  response += formatSize(ESP.getSketchSize()) ;
  response += "\"},\r\n";

  response += "{\"na\":\"Free Size\",\"va\":\"";
  response += formatSize(ESP.getFreeSketchSpace()) ;
  response += "\"},\r\n";

  response += "{\"na\":\"Analog\",\"va\":\"";
  adc = ( (1000 * analogRead(A0)) / 1024);
  sprintf_P( buffer, PSTR("%d mV"), adc);
  response += buffer ;
  response += "\"},\r\n";

  FSInfo info;
  SPIFFS.info(info);

  response += "{\"na\":\"SPIFFS Total\",\"va\":\"";
  response += formatSize(info.totalBytes) ;
  response += "\"},\r\n";

  response += "{\"na\":\"SPIFFS Used\",\"va\":\"";
  response += formatSize(info.usedBytes) ;
  response += "\"},\r\n";

  response += "{\"na\":\"SPIFFS Occupation\",\"va\":\"";
  sprintf_P(buffer, "%d%%",100*info.usedBytes/info.totalBytes);
  response += buffer ;
  response += "\"},\r\n";

  // Free mem should be last one
  response += "{\"na\":\"Free Ram\",\"va\":\"";
  response += formatSize(system_get_free_heap_size()) ;
  response += "\"}\r\n"; // Last don't have comma at end

  // Json end
  response += F("]\r\n");
}

/* ======================================================================
Function: sysJSONTable
Purpose : dump all sysinfo values in JSON table format for browser
Input   : -
Output  : -
Comments: -
====================================================================== */
void sysJSONTable()
{
  String response = "";

  getSysJSONData(response);

  // Just to debug where we are
  dbgF("Serving /system page...");
  server.send ( 200, "text/json", response );
  dbgF("Ok!" EOL);
}

/* ======================================================================
Function: spiffsJSONTable
Purpose : dump all spiffs system in JSON table format for browser
Input   : -
Output  : -
Comments: -
====================================================================== */
void spiffsJSONTable()
{
  String response = "";
  getSpiffsJSONData(response);
  server.send ( 200, "text/json", response );
}

/* ======================================================================
Function: getConfigJSONData
Purpose : Return JSON string containing configuration data
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void getConfJSONData(String & r)
{
  // Json start
  r = FPSTR(FP_JSON_START);

  r+="\"";
  r+=CFG_FORM_SSID;      r+=FPSTR(FP_QCQ); r+=config.ssid;           r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_PSK;       r+=FPSTR(FP_QCQ); r+=config.psk;            r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_HOST;      r+=FPSTR(FP_QCQ); r+=config.host;           r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_AP_PSK;    r+=FPSTR(FP_QCQ); r+=config.ap_psk;         r+= FPSTR(FP_QCNL);

  r+=CFG_FORM_OTA_AUTH;  r+=FPSTR(FP_QCQ); r+=config.ota_auth;       r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_OTA_PORT;  r+=FPSTR(FP_QCQ); r+=config.ota_port;       r+= FPSTR(FP_QCNL);

  r+=CFG_FORM_NET_IP;  r+=FPSTR(FP_QCQ); r+=config.netcfg.ip;        r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_NET_GW;  r+=FPSTR(FP_QCQ); r+=config.netcfg.gw;        r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_NET_MSK;  r+=FPSTR(FP_QCQ); r+=config.netcfg.msk;      r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_NET_DNS;  r+=FPSTR(FP_QCQ); r+=config.netcfg.dns;      r+= FPSTR(FP_QCNL);

  r+=CFG_FORM_REPORT_HOST; r+=FPSTR(FP_QCQ); r+=config.report.host;  r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_REPORT_PORT; r+=FPSTR(FP_QCQ); r+=config.report.port;  r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_REPORT_URL;  r+=FPSTR(FP_QCQ); r+=config.report.url;   r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_REPORT_MSG;  r+=FPSTR(FP_QCQ); r+=config.report.msg;   r+= F("\"");
  // Json end
  r += FPSTR(FP_JSON_END);

}

/* ======================================================================
Function: confJSONTable
Purpose : dump all config values in JSON table format for browser
Input   : -
Output  : -
Comments: -
====================================================================== */
void confJSONTable()
{
  String response = "";
  getConfJSONData(response);
  // Just to debug where we are
  dbgF("Serving /config page...");
  server.send ( 200, "text/json", response );
  dbgF("Ok!" EOL);
}

/* ======================================================================
Function: getSpiffsJSONData
Purpose : Return JSON string containing list of SPIFFS files
Input   : Response String
Output  : -
Comments: -
====================================================================== */
void getSpiffsJSONData(String & response)
{
//  char buffer[32];
  bool first_item = true;

  // Json start
  response = FPSTR(FP_JSON_START);

  // Files Array
  response += F("\"files\":[\r\n");

  // Loop trough all files
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    if (first_item)
      first_item=false;
    else
      response += ",";

    response += F("{\"na\":\"");
    response += fileName.c_str();
    response += F("\",\"va\":\"");
    response += fileSize;
    response += F("\"}\r\n");
  }
  response += F("],\r\n");


  // SPIFFS File system array
  response += F("\"spiffs\":[\r\n{");

  // Get SPIFFS File system informations
  FSInfo info;
  SPIFFS.info(info);
  response += F("\"Total\":");
  response += info.totalBytes ;
  response += F(", \"Used\":");
  response += info.usedBytes ;
  response += F(", \"ram\":");
  response += system_get_free_heap_size() ;
  response += F("}\r\n]");

  // Json end
  response += FPSTR(FP_JSON_END);
}

/* ======================================================================
Function: wifiScanJSON
Purpose : scan Wifi Access Point and return JSON code
Input   : -
Output  : -
Comments: -
====================================================================== */
void wifiScanJSON(void)
{
  String response = "";
  bool first = true;

  // Just to debug where we are
  dbg(F("Serving /wifiscan page..."));

  int n = WiFi.scanNetworks();

  // Json start
  response += F("[\r\n");

  for (uint8_t i = 0; i < n; ++i)
  {
    int8_t rssi = WiFi.RSSI(i);

    /*
    uint8_t percent;

    // dBm to Quality
    if(rssi<=-100)      percent = 0;
    else if (rssi>=-50) percent = 100;
    else                percent = 2 * (rssi + 100);
    */

    if (first)
      first = false;
    else
      response += F(",");

    response += F("{\"ssid\":\"");
    response += WiFi.SSID(i);
    response += F("\",\"rssi\":") ;
    response += rssi;
    response += FPSTR(FP_JSON_END);
  }

  // Json end
  response += FPSTR("]\r\n");

  dbgF("sending...");
  server.send ( 200, "text/json", response );
  dbgF("Ok!" EOL);
}


/* ======================================================================
Function: handleFactoryReset
Purpose : reset the module to factory settingd
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleFactoryReset(void)
{
  // Just to debug where we are
  dbgF("Serving /factory_reset page.");
  cfgReset();
  ESP.eraseConfig();
  dbgF("Sending...");
  server.send ( 200, "text/plain", FPSTR(FP_RESTART) );
  dbgF("Ok!" EOL);
  delay(1000);
  ESP.restart();
  while (true)
    delay(1);
}

/* ======================================================================
Function: handleReset
Purpose : reset the module
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleReset(void)
{
  // Just to debug where we are
  dbgF("Serving /reset page...");
  server.send ( 200, "text/plain", FPSTR(FP_RESTART) );
  dbgF("Ok!" EOL);
  delay(1000);
  ESP.restart();
  while (true)
    delay(1);
}

/* ======================================================================
Function: handleNotFound
Purpose : default WEB routing when URI is not found
Input   : -
Output  : -
Comments: -
====================================================================== */
void handleNotFound(void)
{
  String response = "";
  boolean found = false;

  // try to return SPIFFS file
  found = handleFileRead(server.uri());



  // All trys failed
  if (!found)
  {
    // send error message in plain text
    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += server.uri();
    message += F("\nMethod: ");
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += F("\nArguments: ");
    message += server.args();
    message += FPSTR(FP_NL);

    for ( uint8_t i = 0; i < server.args(); i++ ) {
      message += " " + server.argName ( i ) + ": " + server.arg ( i ) + FPSTR(FP_NL);
    }

    server.send ( 404, "text/plain", message );
  }
}

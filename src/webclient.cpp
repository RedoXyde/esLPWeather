#include "app.h"
#include "config.h"
#include "webclient.h"

#include <ESP8266HTTPClient.h>

//#define DEBUG_HTTP_POST

bool httpPost(const char* host, const uint16_t port, char * url, uint8_t* payload, const size_t size)
{
  #ifdef DEBUG_HTTP_POST
  dbgF("Starting lookup" EOL);
  long startTime = millis();
  #endif
  IPAddress serverIP;
  WiFi.hostByName(host, serverIP);
  #ifdef DEBUG_HTTP_POST
  dbg_s("[DNS] [%dms] Finished lookup", millis() - startTime);
  #endif
  HTTPClient http;
  bool ret = false;
  // configure traged server and url
  http.begin(host, port, url);

  // start connection and send HTTP header

  int httpCode=0;
  if(payload != NULL)
  {
    #ifdef DEBUG_HTTP_POST
    dbg_s(EOL "POST http%s://%s:%d%s" EOL, port==443?"s":"", host, port, url);
    dbg_s("Payload: %s" EOL,(char*)payload);
    #endif
    httpCode = http.POST(payload,size);
  }
  else
    httpCode = http.GET();
  if(httpCode) {
      #ifdef DEBUG_HTTP_POST
      dbg_s("Reply code: %d" EOL,httpCode);
      #endif
      // HTTP header has been send and Server response header has been handled
      // file found at server
      if(httpCode == 200) 
      {
        String payload = http.getString();
        #ifdef DEBUG_HTTP_POST
        dbg_s("Reply data: %s" EOL,payload.c_str());
        #endif
        ret = true;
      }
  }
  #ifdef DEBUG_HTTP_POST 
  else
      dbgF("failed!");
  #endif
  return ret;
}

/* ======================================================================
Function: reportPost
Purpose : Do a http post to custom server
Input   :
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean reportPost(void)
{
  if (!(*config.report.host))
    return false;
  String p = "{";
  // Message
  p += "\"message\":\"";
  p += config.report.msg;
  p += "\",";
  
  // vBatt
  p += "\"battery\":";
  p += sysinfo.vBatt;
  p += ",";

  // extWake
  p += "\"wakeSource\":\"";
  p += sysinfo.extWake ? "External" : "Timer";
  p += "\"";

#ifdef HAS_BME280
  p += ",";

  // Temperature
  p += "\"temperature\":";
  p += sysinfo.temperature;
  p += ",";

  // Pressure
  p += "\"pressure\":";
  p += sysinfo.pressure;
  p += ",";

  // Humidity
  p += "\"humidity\":";
  p += sysinfo.humidity;
  p += "";
#endif

  p += "}";
  return httpPost(config.report.host, config.report.port,
                  config.report.url,
                  (uint8_t*)p.c_str(), p.length()
                 );
}

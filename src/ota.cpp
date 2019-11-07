#include "ota.h"

void otaInit(void)
{
  // OTA callbacks
  ArduinoOTA.onStart([](void)
  {
    dbgF("Update Started" EOL);
  });

  ArduinoOTA.onEnd([](void)
  {
    dbgF("Update finished restarting" EOL);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    //Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    dbg_s("Update Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) dbgF("Auth Failed" EOL);
    else if (error == OTA_BEGIN_ERROR) dbgF("Begin Failed" EOL);
    else if (error == OTA_CONNECT_ERROR) dbgF("Connect Failed" EOL);
    else if (error == OTA_RECEIVE_ERROR) dbgF("Receive Failed" EOL);
    else if (error == OTA_END_ERROR) dbgF("End Failed" EOL);
    ESP.restart();
  });
}

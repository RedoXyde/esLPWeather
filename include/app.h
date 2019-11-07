#pragma once
#include "common.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <FS.h>

extern "C" {
#include "user_interface.h"
}

#include "webserver.h"
#include "webclient.h"
#include "config.h"

// sysinfo informations
typedef struct
{
  float  vBatt;
  float  temp;
} _sysinfo;

// Exported variables/object instancied in main sketch
// ===================================================

extern _sysinfo sysinfo;

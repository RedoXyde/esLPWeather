#pragma once
#include "common.h"
#include <ESP8266WebServer.h>
#include <FS.h>

// Web response max size
#define RESPONSE_BUFFER_SIZE 4096

// Exported variables/object instancied in main sketch
// ===================================================
extern char response[];
extern uint16_t response_idx;

extern ESP8266WebServer server;

void webserverInit(void);

void handleTest(void);
void handleRoot(void);
void handleFormConfig(void) ;
void handleNotFound(void);
void getSysJSONData(String & r);
void sysJSONTable(void);
void getConfJSONData(String & r);
void confJSONTable(void);
void getSpiffsJSONData(String & r);
void sendJSON(void);
void wifiScanJSON(void);
void handleFactoryReset(void);
void handleReset(void);

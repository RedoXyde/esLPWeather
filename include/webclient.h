#pragma once
#include "common.h"

bool httpPost(const char* host, const uint16_t port, char * url, uint8_t* payload=NULL, const size_t size=0);
bool reportPost(void);

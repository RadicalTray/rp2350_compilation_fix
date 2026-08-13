#pragma once

#include <Arduino_10BASE_T1S.h>

extern Arduino_10BASE_T1S_UDP udp_server;

void initUDPServer();
void loopUDPServer(unsigned long now);

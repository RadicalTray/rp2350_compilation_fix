#pragma once

#include <Arduino_10BASE_T1S.h>

extern Arduino_10BASE_T1S_UDP udp_server;

// NOTE: Arduino_10BASE_T1S_UDP uses IP_ADDR_ANY (0.0.0.0) for udp ip address
void initUDPServer(uint16_t port);
void loopUDPServer(unsigned long now);

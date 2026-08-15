#pragma once

#include <Arduino_10BASE_T1S.h>

extern Arduino_10BASE_T1S_UDP udp_client;

void initUDPClient(uint16_t port);
void loopUDPClient(unsigned long now, IPAddress server_ip, uint16_t server_port);

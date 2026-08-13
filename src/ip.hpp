#pragma once

uint8_t const T1S_PLCA_NODE_ID = 0; // PLCA coordinator or something

// Phy
IPAddress const ip_addr     {192, 168,  42, 100 + T1S_PLCA_NODE_ID};
IPAddress const network_mask{255, 255, 255,   0};
IPAddress const gateway     {192, 168,  42, 100};

T1SPlcaSettings const t1s_plca_settings{T1S_PLCA_NODE_ID};
T1SMacSettings  const t1s_default_mac_settings;

// Loopback
IPAddress const LO_IP_ADDR = {127,   0,   0,   1};

// UDP
IPAddress const UDP_SERVER_IP_ADDR = ip_addr;
uint16_t  const UDP_CLIENT_PORT    = 8889;
uint16_t  const UDP_SERVER_PORT    = 8888;

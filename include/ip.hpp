#pragma once

// Phy
uint8_t const PHY_T1S_PLCA_NODE_ID = 0; // PLCA coordinator or something
IPAddress const PHY_IP_ADDR     {192, 168,  42, 100 + PHY_T1S_PLCA_NODE_ID};
IPAddress const PHY_NETWORK_MASK{255, 255, 255,   0};
IPAddress const PHY_GATEWAY     {192, 168,  42, 100};

T1SPlcaSettings const PHY_T1S_PLCA_SETTINGS{PHY_T1S_PLCA_NODE_ID};
T1SMacSettings  const PHY_T1S_DEFAULT_MAC_SETTINGS;

// Loopback
IPAddress const LO_IP_ADDR = {127,   0,   0,   1};

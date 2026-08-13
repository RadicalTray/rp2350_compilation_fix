#include "phy.hpp"

#include <SPI.h>
#include "ip.hpp"

// Declares t1s_io and t1s_phy
Arduino_10BASE_T1S_PHY_TC6(SPI, PIN_SPI1_SS, PIN_SPI1_RST, PIN_SPI1_IRQ);

void OnPlcaStatus(bool success, bool plcaStatus) {
  if (!success) {
    Serial.println("PLCA status register read failed");
    return;
  }

  if (plcaStatus)
    Serial.println("PLCA Mode active");
  else {
    Serial.println("CSMA/CD fallback");
    t1s_phy.enablePlca();
  }
}

void initPhy() {
  // TODO: The difference between old change (pure SPI1) and new change (change SPI0 to use SPI1) is that
  //  this still uses PIN_SPI0_SS (5u) instead of PIN_SPI1_SS (17u)
  //
  //  TLDR: use everything PIN_SPI1 (SPIETH) but SS is PIN_SPI0_SS (SPI0)
  SPI.setRX(PIN_SPI1_MISO);
  SPI.setTX(PIN_SPI1_MOSI);
  SPI.setSCK(PIN_SPI1_SCK);

  pinMode(PIN_SPI1_IRQ, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_SPI1_IRQ),
                  []() { t1s_io.onInterrupt(); },
                  FALLING);

  if (!t1s_io.begin()) {
    Serial.println("'TC6_Io::begin(...)' failed.");
    for (;;) { }
  }

  MacAddress const mac_addr = MacAddress::create_from_uid();

  if (!t1s_phy.begin(
    ip_addr,
    network_mask,
    gateway,
    mac_addr,
    t1s_plca_settings,
    t1s_default_mac_settings
  )) {
    Serial.println("'TC6::begin(...)' failed.");
    for (;;) { }
  }

  Serial.print("IP\t");
  Serial.println(ip_addr);
  Serial.println(mac_addr);
  Serial.println(t1s_plca_settings);
  Serial.println(t1s_default_mac_settings);
}

// An example stolen from the Arduino_10BASE_T1S repository

#include <Arduino_10BASE_T1S.h>
#include <SPI.h>
#include "udp.h"

static uint8_t const T1S_PLCA_NODE_ID = 1;

static IPAddress const ip_addr     {192, 168,  42, 100 + T1S_PLCA_NODE_ID};
static IPAddress const network_mask{255, 255, 255,   0};
static IPAddress const gateway     {192, 168,  42, 100};

static T1SPlcaSettings const t1s_plca_settings{T1S_PLCA_NODE_ID};
static T1SMacSettings const t1s_default_mac_settings;

Arduino_10BASE_T1S_PHY_TC6(SPI, PIN_SPI1_SS, PIN_SPI1_RST, PIN_SPI1_IRQ);

void setup() {
  initSerial();
  initT1S();
  initUDP();
}

void loop() {
  /* Services the hardware and the protocol stack.
   * Must be called cyclic. The faster the better.
   */
  t1s_phy.service();

  loopUDP();
}

static void OnPlcaStatus(bool success, bool plcaStatus) {
  if (!success)
  {
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

static void initT1S() {
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

  /* Initialize IO module. */
  if (!t1s_io.begin())
  {
    Serial.println("'TC6_Io::begin(...)' failed.");
    for (;;) { }
  }

  MacAddress const mac_addr = MacAddress::create_from_uid();

  if (!t1s_phy.begin(ip_addr
    , network_mask
    , gateway
    , mac_addr
    , t1s_plca_settings
    , t1s_default_mac_settings))
  {
    Serial.println("'TC6::begin(...)' failed.");
    for (;;) { }
  }

  Serial.print("IP\t");
  Serial.println(ip_addr);
  Serial.println(mac_addr);
  Serial.println(t1s_plca_settings);
  Serial.println(t1s_default_mac_settings);
}

static void initSerial() {
  Serial.begin(115200);
  while (!Serial) { }
}

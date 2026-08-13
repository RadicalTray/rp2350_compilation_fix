#include "phy.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  Serial.print("ENABLE_LOOPBACK = "); Serial.println(ENABLE_LOOPBACK);
  Serial.print("LWIP_NETIF_LOOPBACK = "); Serial.println(LWIP_NETIF_LOOPBACK);

  initPhy();
  initUDPServer();
  initUDPClient();
}

void loop() {
  auto const now = millis();

  loopPhy(now);
  loopUDPServer(now);
  loopUDPClient(now);
}

#include "phy.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"

#include <lwip/netdb.h>

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

  // clear output queue
  // required since loopback packets are handled by lwip which puts outputs onto a queue
  netif_poll_all();

  loopPhy(now);
  loopUDPServer(now);
  loopUDPClient(now);
}

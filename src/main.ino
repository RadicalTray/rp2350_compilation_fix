#include "phy.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"

#include <lwip/netdb.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  initPhy();
  initUDPServer();
  initUDPClient();

  int i = 0;
  struct netif *netif;
  NETIF_FOREACH(netif) {
    i += 1;
    Serial.print("netif ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(ip4_addr1(&netif->ip_addr));
    Serial.print(".");
    Serial.print(ip4_addr2(&netif->ip_addr));
    Serial.print(".");
    Serial.print(ip4_addr3(&netif->ip_addr));
    Serial.print(".");
    Serial.print(ip4_addr4(&netif->ip_addr));
    Serial.println("");
  }
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

#include "phy.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"
#include "ip.hpp"

#include <lwip/netdb.h>

uint16_t const UDP_SERVER_PORT = 8888;
uint16_t const UDP_CLIENT_PORT = 8889;

// TODO:
//  - try tcp
void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  initPhy();
  initUDPServer(UDP_SERVER_PORT);
  initUDPClient(UDP_CLIENT_PORT);

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
  static unsigned long prev = 0;
  static IPAddress server_ip = LO_IP_ADDR;

  auto const now = millis();

  if (now - prev > 10000) {
    prev = now;

    // switch ip to test
    if (server_ip == LO_IP_ADDR)
      server_ip = PHY_IP_ADDR;
    else
      server_ip = LO_IP_ADDR;
  }

  loopPhy(now);

  // clear lwip's output queue
  // required since loopback packets are handled by lwip which puts outputs onto a queue
  netif_poll_all();

  loopUDPServer(now);
  loopUDPClient(now, server_ip, UDP_SERVER_PORT);
  // can't immediately do another loopUDPClient() because it waits 1 second before sending
  // another packet; the second call won't do anything.
}

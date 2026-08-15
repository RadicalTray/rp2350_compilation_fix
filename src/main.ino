#include "phy.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"
#include "ip.hpp"

#include <lwip/netdb.h>

uint16_t const UDP_SERVER_PORT = 8888;
uint16_t const UDP_CLIENT_PORT = 8889;

// TODO:
//  - try loopback (127.0.0.1) for udp server
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

// Server can only receive from only one IP for some reason.
// Buffer full, weird LWIP behaviour I don't know about?
//
// TODO: make a separate UDP Client, in case the buffer in Arduino_10BASE_T1S_UDP's full.
void loop() {
  auto const now = millis();

  loopPhy(now);

  // clear output queue
  // required since loopback packets are handled by lwip which puts outputs onto a queue
  netif_poll_all();

  loopUDPServer(now);
  Serial.println("Trying Loopback IP");
  loopUDPClient(now, LO_IP_ADDR, UDP_SERVER_PORT);

  {
    loopPhy(now);
    netif_poll_all();

    loopUDPServer(now);
    Serial.println("Trying Loopback IP");
    loopUDPClient(now, PHY_IP_ADDR, UDP_SERVER_PORT);
  }

  // Don't print too much
  delay(1000);
}

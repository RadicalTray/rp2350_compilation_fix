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

// Server can only receive from only one IP for some reason.
// A full buffer (PHY / Arduino_10BASE_T1S_UDP), some weird LWIP behaviour I don't know about?
//
//  From Arduino_10BASE_T1S_UDP:
//  /* Discard UdpRxPacket object previously held by _rx_pkt
//   * and replace it with the new one.
//   */
//
// TODO:
//  -   LIKELY: service the server before sending another packet so the server doesn't discard it
//  - UNLIKELY: make a separate UDP Client, in case the buffer in Arduino_10BASE_T1S_UDP's full.
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

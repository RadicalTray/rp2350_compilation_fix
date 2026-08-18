#include "phy.hpp"
#include "ip.hpp"

#include <lwip/netdb.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  initPhy();

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
  }

  loopPhy(now);

  // clear lwip's output queue
  // required since loopback packets are handled by lwip which puts outputs onto a queue
  netif_poll_all();
}

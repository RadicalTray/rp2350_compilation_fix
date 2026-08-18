#include "phy.hpp"
#include "ip.hpp"
#include "http_server.hpp"

#include <lwip/netdb.h>

uint8_t led_state = LOW;
unsigned long last_led = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  pinMode(20, OUTPUT);
  digitalWrite(20, HIGH);
  delay(2000);
  digitalWrite(20, LOW);

  initPhy();

  http_server_init(80);

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

  if (now - last_led > 1000) {
    led_state = LOW;
  }

  loopPhy(now);

  // clear lwip's output queue
  // required since loopback packets are handled by lwip which puts outputs onto a queue
  //
  // this also processes tcp (http), i presume. cuz how else?
  // it somehow spawns another thread? that would be weird
  netif_poll_all();

  // consume
  if (http_led) {
    http_led = 0;
    led_state = HIGH;
    last_led = now;
  }

  digitalWrite(20, led_state);
}

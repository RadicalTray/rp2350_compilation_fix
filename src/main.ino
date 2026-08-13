#include "phy.hpp"
#include "udp_client.hpp"
#include "udp_server.hpp"

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  initPhy();
  initUDPServer();
  initUDPClient();
}

void loop() {
  t1s_phy.service();
  loopUDPServer();
  loopUDPClient();
}

#include "phy.hpp"
#include "ip.hpp"
#include "http_server.hpp"

#include <WebServer.h>
#include <WebSocketsServer.h>

// try deleting this (remove netif_poll_all() cuz ugly)
//
// what if this makes platformio use framework-arduinopico's lwip, hmmm??? :think:
#include <lwip/netdb.h>

#define USE_SERIAL Serial

uint16_t led = 20;
uint8_t led_state = LOW;
unsigned long last_led = 0;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      {
        USE_SERIAL.printf("[%u] Disconnected!\n", num);
        break;
      }
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
        break;
      }
    case WStype_TEXT:
      {
        USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

        if(payload[0] == '#') {
          // we get RGB data

          uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
          uint32_t red = (rgb >> 16) & 0xFF;
          USE_SERIAL.printf("Red: %d\n", red);
          if (red > 125) {
            digitalWrite(led, HIGH);
          } else {
            digitalWrite(led, LOW);
          }

          // decode rgb data
          //
          // analogWrite(LED_RED,    ((rgb >> 16) & 0xFF));
          // analogWrite(LED_GREEN,  ((rgb >> 8) & 0xFF));
          // analogWrite(LED_BLUE,   ((rgb >> 0) & 0xFF));
        }

        break;
      }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { }

  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);

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

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // handle index
  server.on("/", []() {
      // send index.html
      server.send(200, "text/html", "<html><head><script>var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script></head><body>LED Control:<br/><br/>R: <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>G: <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>B: <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/></body></html>");
      });
  server.begin();
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
  //
  // TODO: try removing this line
  //  might only need this when we use loopback
  netif_poll_all();

  webSocket.loop();
  server.handleClient();
}

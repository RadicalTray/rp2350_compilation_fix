#include "udp_client.hpp"

#include "ip.hpp"

Arduino_10BASE_T1S_UDP udp_client;

void initUDPClient() {
  if (!udp_client.begin(UDP_CLIENT_PORT)) {
    Serial.println("begin(...) failed for UDP client");
    for (;;) { }
  }
  Serial.println("UDP_Client");
}

void loopUDPClient(unsigned long now) {
  static unsigned long prev_udp_packet_sent = 0;

  if ((now - prev_udp_packet_sent) > 1000) {
    static int tx_packet_cnt = 0;

    prev_udp_packet_sent = now;

    /* Prepare UDP packet. */
    uint8_t udp_tx_msg_buf[256] = {0};
    int const tx_packet_size = snprintf((char *)udp_tx_msg_buf, sizeof(udp_tx_msg_buf), "Single-Pair Ethernet / 10BASE-T1S: packet cnt = %d", tx_packet_cnt);

    /* Send a UDP packet to the UDP server. */
    udp_client.beginPacket(UDP_SERVER_IP_ADDR, UDP_SERVER_PORT);
    udp_client.write(udp_tx_msg_buf, tx_packet_size);
    udp_client.endPacket();

    Serial.print("[");
    Serial.print(millis());
    Serial.print("] UDP_Client sending: \"");
    Serial.print(reinterpret_cast<char *>(udp_tx_msg_buf));
    Serial.println("\"");

    tx_packet_cnt++;
  }

  /* Check for incoming UDP packets. */
  int const rx_packet_size = udp_client.parsePacket();
  if (rx_packet_size) {
    /* Print some metadata from received UDP packet. */
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] Received ");
    Serial.print(rx_packet_size);
    Serial.print(" bytes from ");
    Serial.print(udp_client.remoteIP());
    Serial.print(" port ");
    Serial.print(udp_client.remotePort());
    Serial.print(", data = \"");

    /* Read from received UDP packet. */
    size_t const UDP_RX_MSG_BUF_SIZE = 16 + 1; /* Reserve the last byte for the '\0' termination. */
    uint8_t udp_rx_msg_buf[UDP_RX_MSG_BUF_SIZE] = {0};
    int bytes_read = udp_client.read(udp_rx_msg_buf, UDP_RX_MSG_BUF_SIZE - 1);
    while (bytes_read != 0) {
      /* Print received data to Serial. */
      udp_rx_msg_buf[bytes_read] = '\0'; /* Terminate buffer so that we can print it as a C-string. */
      Serial.print(reinterpret_cast<char *>(udp_rx_msg_buf));

      /* Continue reading. */
      bytes_read = udp_client.read(udp_rx_msg_buf, UDP_RX_MSG_BUF_SIZE - 1);
    }
    Serial.println("\"");

    /* Finish reading the current packet. */
    udp_client.flush();
  }
}

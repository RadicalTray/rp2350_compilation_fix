#include "udp_server.hpp"

#include "ip.hpp"

Arduino_10BASE_T1S_UDP udp_server;

void initUDPServer() {
  if (!udp_server.begin(UDP_SERVER_PORT)) {
    Serial.println("begin(...) failed for UDP server");
    for (;;) { }
  }
  Serial.println("UDP_Server");
}

void loopUDPServer(unsigned long now) {
  /* Check for incoming UDP packets. */
  int const rx_packet_size = udp_server.parsePacket();
  if (rx_packet_size) {
    std::vector<uint8_t> udp_tx_buf;
    IPAddress const destination_ip = udp_server.remoteIP();
    uint16_t const destination_port = udp_server.remotePort();

    /* Print some metadata from received UDP packet. */
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] UDP_Server: Received ");
    Serial.print(rx_packet_size);
    Serial.print(" bytes from ");
    Serial.print(udp_server.remoteIP());
    Serial.print(" port ");
    Serial.print(udp_server.remotePort());
    Serial.print(", data = \"");

    /* Read from received UDP packet. */
    size_t const UDP_RX_MSG_BUF_SIZE = 16 + 1; /* Reserve the last byte for the '\0' termination. */
    uint8_t udp_rx_msg_buf[UDP_RX_MSG_BUF_SIZE] = {0};
    int bytes_read = udp_server.read(udp_rx_msg_buf, UDP_RX_MSG_BUF_SIZE - 1);
    while (bytes_read != 0) {
      /* Copy received data into transmit buffer for echo functionality. */
      std::copy(udp_rx_msg_buf, udp_rx_msg_buf + bytes_read, std::back_inserter(udp_tx_buf));

      /* Print received data to Serial. */
      udp_rx_msg_buf[bytes_read] = '\0'; /* Terminate buffer so that we can print it as a C-string. */
      Serial.print(reinterpret_cast<char *>(udp_rx_msg_buf));

      /* Continue reading. */
      bytes_read = udp_server.read(udp_rx_msg_buf, UDP_RX_MSG_BUF_SIZE - 1);
    }
    Serial.println("\"");

    /* Finish reading the current packet. */
    udp_server.flush();

    /* Send back a reply, to the IP address and port we got the packet from. */
    udp_server.beginPacket(destination_ip, destination_port);
    udp_server.write(udp_tx_buf.data(), udp_tx_buf.size());
    udp_server.endPacket();
  }
}

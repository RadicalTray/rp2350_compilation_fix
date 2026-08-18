#pragma once

#include <lwip/tcp.h>

extern int http_led;

void http_server_init(u16_t port);

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/stats.h>
#include <lwip/tcp.h>
#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include "http_server.hpp"

enum http_state {
  HS_NONE = 0,
  HS_ACCEPTED,
  HS_RECEIVED,
  HS_CLOSING
};

struct http_conn {
  u8_t state;
  u8_t retries;
  struct tcp_pcb *pcb;
  struct pbuf *p;
};

static struct tcp_pcb *http_server_pcb;
static u32_t request_counter = 0;

/* ── helpers ─────────────────────────────────────────────────────── */

static void
http_free(struct http_conn *hs)
{
  if (hs != NULL) {
    if (hs->p) pbuf_free(hs->p);
    mem_free(hs);
  }
}

static void
http_close(struct tcp_pcb *tpcb, struct http_conn *hs)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  http_free(hs);
  tcp_close(tpcb);
}

/* Copy pbuf chain into a flat buffer. Returns bytes copied. */
static u16_t
http_copy_pbuf(char *dest, u16_t max_len, struct pbuf *p)
{
  u16_t copied = 0;
  for (struct pbuf *q = p; q != NULL && copied < max_len; q = q->next) {
    u16_t n = q->len;
    if (copied + n > max_len) n = max_len - copied;
    memcpy(dest + copied, q->payload, n);
    copied += n;
  }
  return copied;
}

/* Find "\r\n\r\n" in a flat buffer. Returns index or -1. */
static int
http_find_headers_end(const char *buf, u16_t len)
{
  for (u16_t i = 0; i + 3 < len; i++) {
    if (buf[i]=='\r' && buf[i+1]=='\n' && buf[i+2]=='\r' && buf[i+3]=='\n')
      return i;
  }
  return -1;
}

/* ── response builders ───────────────────────────────────────────── */

static void
http_send(struct tcp_pcb *tpcb, struct http_conn *hs,
          const char *data, u16_t len)
{
  err_t err = tcp_write(tpcb, data, len, TCP_WRITE_FLAG_COPY);
  if (err == ERR_OK) {
    tcp_output(tpcb);
    hs->state = HS_CLOSING;
  }
}

static void
http_send_cstr(struct tcp_pcb *tpcb, struct http_conn *hs, const char *s)
{
  http_send(tpcb, hs, s, strlen(s));
}

/* ── routes ──────────────────────────────────────────────────────── */

static const char PAGE_HTML[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<!DOCTYPE html><html><head>"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>10BASE-T1S</title>"
"<style>"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{font-family:system-ui,sans-serif;background:#0f172a;color:#e2e8f0;"
"max-width:640px;margin:2rem auto;padding:0 1rem}"
"h1{font-size:1.4rem;margin-bottom:1rem;color:#38bdf8}"
".card{background:#1e293b;border-radius:8px;padding:1rem;margin-bottom:1rem}"
".card h2{font-size:.9rem;color:#94a3b8;margin-bottom:.5rem;text-transform:uppercase;letter-spacing:.05em}"
"pre{white-space:pre-wrap;font-size:.85rem;color:#a5f3fc}"
"label{display:block;font-size:.8rem;color:#94a3b8;margin-bottom:.3rem}"
"input[type=text]{width:100%;padding:.5rem;border:1px solid #334155;border-radius:4px;"
"background:#0f172a;color:#e2e8f0;font-size:.9rem}"
"button{margin-top:.5rem;padding:.5rem 1rem;border:none;border-radius:4px;"
"cursor:pointer;font-size:.85rem;font-weight:600}"
".btn{background:#2563eb;color:#fff}.btn:hover{background:#1d4ed8}"
".btn-sec{background:#334155;color:#e2e8f0}.btn-sec:hover{background:#475569}"
"#echo-out{margin-top:.5rem;padding:.5rem;background:#0f172a;border-radius:4px;"
"min-height:1.5rem;font-size:.85rem;color:#86efac}"
".ts{font-size:.75rem;color:#64748b;margin-top:.5rem}"
"</style></head><body>"
"<h1>RP2350 10BASE-T1S Server</h1>"

"<div class=\"card\">"
"<h2>Device Info</h2>"
"<pre id=\"info\">Loading...</pre>"
"</div>"

"<div class=\"card\">"
"<h2>Echo</h2>"
"<label for=\"echo-in\">Send text to the device:</label>"
"<input type=\"text\" id=\"echo-in\" placeholder=\"Type something...\">"
"<button class=\"btn\" onclick=\"doEcho()\">Send</button>"
"<div id=\"echo-out\"></div>"
"</div>"

"<div class=\"card\">"
"<h2>Counter</h2>"
"<pre id=\"ctr\">-</pre>"
"<button class=\"btn\" onclick=\"doIncrement()\">Increment</button>"
"</div>"

"<div class=\"card\">"
"<h2>Log</h2>"
"<pre id=\"log\" style=\"max-height:12rem;overflow-y:auto\"></pre>"
"</div>"

"<div class=\"ts\">Last refresh: <span id=\"ts\">-</span></div>"

"<script>"
"function log(m){"
"var e=document.getElementById('log');"
"e.textContent=new Date().toLocaleTimeString()+' '+m+'\\n'+e.textContent;"
"}"
"function refresh(){"
"fetch('/api/info').then(r=>r.json()).then(d=>{"
"document.getElementById('info').textContent="
"'Uptime: '+d.uptime_s+'s\\n'"
"+'Requests served: '+d.requests+'\\n'"
"+'Counter: '+d.counter;"
"document.getElementById('ctr').textContent=d.counter;"
"document.getElementById('ts').textContent=new Date().toLocaleTimeString();"
"log('Info refreshed');"
"}).catch(e=>log('Error: '+e));"
"}"
"function doEcho(){"
"var v=document.getElementById('echo-in').value;"
"if(!v)return;"
"fetch('/api/echo',{method:'POST',body:v})"
".then(r=>r.text()).then(t=>{"
"document.getElementById('echo-out').textContent='Response: '+t;"
"log('Echoed: '+v);"
"}).catch(e=>log('Error: '+e));"
"}"
"function doIncrement(){"
"fetch('/api/counter',{method:'POST'})"
".then(r=>r.json()).then(d=>{"
"document.getElementById('ctr').textContent=d.counter;"
"log('Counter -> '+d.counter);"
"}).catch(e=>log('Error: '+e));"
"}"
"refresh();setInterval(refresh,3000);"
"</script></body></html>";

/* ── request parsing & dispatch ──────────────────────────────────── */

static void
http_dispatch(struct tcp_pcb *tpcb, struct http_conn *hs)
{
  /* Flat buffer of the full request */
  char buf[512];
  u16_t n = http_copy_pbuf(buf, sizeof(buf) - 1, hs->p);
  buf[n] = '\0';

  /* Acknowledge & free the pbuf */
  tcp_recved(tpcb, hs->p->tot_len);
  pbuf_free(hs->p);
  hs->p = NULL;

  /* Parse request line: "METHOD /path HTTP/1.x" */
  char method[8] = {0};
  char path[128] = {0};
  sscanf(buf, "%7s %127s", method, path);

  /* Find body (after \r\n\r\n) */
  int hdr_end = http_find_headers_end(buf, n);
  const char *body = (hdr_end >= 0) ? buf + hdr_end + 4 : NULL;
  u16_t body_len = body ? (n - hdr_end - 4) : 0;

  request_counter++;

  Serial.print("[");
  Serial.print(millis());
  Serial.print("] HTTP: ");
  Serial.print(method);
  Serial.print(" ");
  Serial.println(path);

  /* ── GET / (or /index.html) ── */
  if (strcmp(method, "GET") == 0 &&
      (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)) {
    http_send_cstr(tpcb, hs, PAGE_HTML);
    return;
  }

  /* ── GET /api/info ── */
  if (strcmp(method, "GET") == 0 && strcmp(path, "/api/info") == 0) {
    char resp[256];
    int len = snprintf(resp, sizeof(resp),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Connection: close\r\n"
      "\r\n"
      "{\"uptime_s\":%lu,\"requests\":%lu,\"counter\":%lu}",
      millis() / 1000,
      (unsigned long)request_counter,
      (unsigned long)request_counter);
    http_send(tpcb, hs, resp, len);
    return;
  }

  /* ── POST /api/echo ── */
  if (strcmp(method, "POST") == 0 && strcmp(path, "/api/echo") == 0) {
    char resp[320];
    /* Wrap body in JSON string: escape quotes */
    char safe[128] = {0};
    if (body) {
      u16_t j = 0;
      for (u16_t i = 0; i < body_len && j < sizeof(safe) - 2; i++) {
        if (body[i] == '"') safe[j++] = '\\';
        safe[j++] = body[i];
      }
      safe[j] = '\0';
    }
    int len = snprintf(resp, sizeof(resp),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "\r\n"
      "%s",
      body ? safe : "");
    http_send(tpcb, hs, resp, len);
    return;
  }

  /* ── POST /api/counter ── */
  if (strcmp(method, "POST") == 0 && strcmp(path, "/api/counter") == 0) {
    char resp[128];
    int len = snprintf(resp, sizeof(resp),
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Connection: close\r\n"
      "\r\n"
      "{\"counter\":%lu}",
      (unsigned long)request_counter);
    http_send(tpcb, hs, resp, len);
    return;
  }

  /* ── 404 ── */
  http_send_cstr(tpcb, hs,
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "\r\n"
    "404 Not Found\n");
}

/* ── lwIP callbacks ──────────────────────────────────────────────── */

static void
http_error(void *arg, err_t err)
{
  LWIP_UNUSED_ARG(err);
  http_free((struct http_conn *)arg);
}

static err_t
http_poll(void *arg, struct tcp_pcb *tpcb)
{
  struct http_conn *hs = (struct http_conn *)arg;
  if (hs == NULL) { tcp_abort(tpcb); return ERR_ABRT; }
  if (hs->p != NULL) {
    http_dispatch(tpcb, hs);
  } else if (hs->state == HS_CLOSING) {
    http_close(tpcb, hs);
  }
  return ERR_OK;
}

static err_t
http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct http_conn *hs = (struct http_conn *)arg;
  LWIP_UNUSED_ARG(len);
  hs->retries = 0;
  if (hs->state == HS_CLOSING) {
    http_close(tpcb, hs);
  }
  return ERR_OK;
}

static err_t
http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct http_conn *hs = (struct http_conn *)arg;
  if (p == NULL) {
    hs->state = HS_CLOSING;
    if (hs->p == NULL) http_close(tpcb, hs);
    return ERR_OK;
  }
  if (err != ERR_OK) { pbuf_free(p); return err; }

  /* Accumulate data */
  if (hs->p == NULL) {
    hs->p = p;
  } else {
    pbuf_cat(hs->p, p);
  }

  /* Check for complete headers */
  if (hs->p != NULL && hs->p->tot_len >= 4) {
    u16_t check_len = hs->p->tot_len;
    if (check_len > 512) check_len = 512;
    char check[512];
    http_copy_pbuf(check, check_len, hs->p);
    if (http_find_headers_end(check, check_len) >= 0) {
      http_dispatch(tpcb, hs);
    }
  }
  return ERR_OK;
}

static err_t
http_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  if ((err != ERR_OK) || (newpcb == NULL)) return ERR_VAL;

  tcp_setprio(newpcb, TCP_PRIO_MIN);

  struct http_conn *hs = (struct http_conn *)mem_malloc(sizeof(struct http_conn));
  if (hs == NULL) return ERR_MEM;

  hs->state = HS_ACCEPTED;
  hs->pcb = newpcb;
  hs->retries = 0;
  hs->p = NULL;

  tcp_arg(newpcb, hs);
  tcp_recv(newpcb, http_recv);
  tcp_err(newpcb, http_error);
  tcp_poll(newpcb, http_poll, 2);
  tcp_sent(newpcb, http_sent);
  return ERR_OK;
}

/* ── public ──────────────────────────────────────────────────────── */

void
http_server_init(u16_t port)
{
  http_server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (http_server_pcb == NULL) {
    Serial.println("HTTP: tcp_new failed");
    return;
  }
  err_t err = tcp_bind(http_server_pcb, IP_ANY_TYPE, port);
  if (err != ERR_OK) {
    Serial.println("HTTP: bind failed");
    return;
  }
  http_server_pcb = tcp_listen(http_server_pcb);
  tcp_accept(http_server_pcb, http_accept);
  Serial.print("HTTP server listening on port ");
  Serial.println(port);
}

#include "wifi.h"
#include "softserial.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>

#define MEMCMP_CONST(ptr, const_str) memcmp(ptr, const_str, sizeof(const_str))

static void print_response() {
  char c;
  printf("*** START RESPONSE ***\n");
  while(!softserial_available());
  _delay_ms(50);
  while(softserial_available()) {
    c = softserial_getc();
    if(c != '\r') putchar(c);
  }
  if(c != '\n') putchar('\n');
  printf("*** END RESPONSE ***\n");
}

void wifi_init() {
  softserial_init();

  // Set direction of PORTB, pin 4 to output (Wireless enable)
  DDRB |= _BV(DDB4);
}

void wifi_enable() {
  PORTB |= _BV(PORTB4);
}

void wifi_disable() {
  PORTB &= ~_BV(PORTB4);
}

static void wifi_repeat_until_ok(const char *cmd) {
  char line[32];
  int line_len;

  while(1) {
    softserial_puts(cmd);
    _delay_ms(200);
    while(softserial_available()) {
      line_len = softserial_readline(line, ARRAYSIZE(line));
      if(MEMCMP_CONST(line, "OK\r\n") == 0) return;
    }
  }
}

bool wifi_is_connected() {
  char line[32];
  int line_len;
  bool connected = false;

  softserial_clear_buffer();

  softserial_puts("AT+CWJAP?\r\n");
  _delay_ms(200);

  while(!connected && softserial_available()) {
    line_len = softserial_readline(line, ARRAYSIZE(line));
    connected = MEMCMP_CONST(line, "OK\r\n") == 0;
  }

  softserial_clear_buffer();

  return connected;
}

void wifi_connect() {
  char response[256];
  char response_len;

  printf("[WIFI] Connecting...\n");

  wifi_enable();
  _delay_ms(100);

  // Reset wireless
  softserial_puts("AT+RST\r\n");
  _delay_ms(100);
  softserial_clear_buffer();

  wifi_repeat_until_ok("AT\r\n");

  softserial_puts("AT+CWMODE=1\r\n");
  _delay_ms(100);
  print_response();

  softserial_printf("AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASS);
  print_response();

  wifi_repeat_until_ok("AT+CWJAP?\r\n");

  softserial_puts("AT+CIPMUX=0\r\n");
  print_response();

  printf("[WIFI] Connected.\n");
}

static bool wifi_is_send_ok() {
  char line[32];
  int line_len;
  bool send_ok = false;

  while(!send_ok && softserial_available()) {
    line_len = softserial_readline(line, ARRAYSIZE(line));
    send_ok = MEMCMP_CONST(line, "SEND OK\r\n") == 0;
  }

  softserial_clear_buffer();

  return send_ok;
}

void wifi_send(const char *message) {
  printf("[WIFI] Sending...\n");

  softserial_printf("AT+CIPSTART=\"UDP\",\"%s\",%d\r\n", WIFI_DEST_IP, WIFI_DEST_PORT);
  print_response();

  softserial_printf("AT+CIPSEND=%d\r\n", strlen(message));
  print_response();

  softserial_clear_buffer();

  softserial_puts(message);
}

bool wifi_wait_for_send() {
  // Wait for SEND OK (or timeout)
  bool successful_send = false;
  const int max_attempts = 20;
  for(int i = 0; i < max_attempts; ++i) {
    if(wifi_is_send_ok()) {
      successful_send = true;
      break;
    }
    _delay_ms(200);
  }

  return successful_send;
}

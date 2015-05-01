#include "wifi.h"
#include "softserial.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>

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

static char ends_with(const char* str, int str_len, const char* suffix, int suffix_len) {
  if(str_len >= suffix_len && memcmp(&str[str_len - suffix_len], suffix, suffix_len) == 0) {
    return 1;
  } else {
    return 0;
  }
}

static void wifi_repeat_until_ok(const char *cmd) {
  char response[32];
  char response_len;

  while(1) {
    softserial_puts(cmd);
    _delay_ms(100);
    response_len = softserial_getsn(response, ARRAYSIZE(response));
    if(ends_with(response, response_len, "OK\r\n", 4)) {
      break;
    }
  }
}

void wifi_wake() {
  printf("[WIFI] Waking...\n");
  wifi_enable();

  // Wait for reconnect
  wifi_repeat_until_ok("AT+CWJAP?\r\n");

  printf("[WIFI] Reconnected.\n");
}

void wifi_sleep() {
  // FIXME: Why does this debug message get mangled?
  printf("[WIFI] Sleeping...\n");
  wifi_disable();
}

void wifi_connect() {
  char response[256];
  char response_len;

  // TODO: Just connect here
  // FIXME: Read lines can be split if we are unlucky - need to buffer
  // lines here. That is, keep reading until we hit a \r\n.

  printf("[WIFI] Connecting...\n");

  wifi_enable();
  _delay_ms(100);

  // Reset
  softserial_puts("AT+RST\r\n");
  _delay_ms(100);

  // Flush junk
  while(softserial_available()) softserial_getc();

  wifi_repeat_until_ok("AT\r\n");

  softserial_puts("AT+CWMODE=1\r\n");
  _delay_ms(100);
  while(softserial_available()) softserial_getc();

  softserial_puts("AT+CWJAP=\"NETGEAR12\",\"sillyrabbit129\"\r\n");
  while(!softserial_available());
  _delay_ms(100);

  wifi_repeat_until_ok("AT\r\n");

  softserial_puts("AT+CIPMUX=0\r\n");
  _delay_ms(100);
  while(softserial_available()) softserial_getc();

  printf("[WIFI] Connected.\n");
}

void wifi_send(const char *message) {
  printf("[WIFI] Sending...\n");
  softserial_puts("AT+CIPSTART=\"UDP\",\"192.168.0.6\",9252\r\n");
  _delay_ms(100);
  char cipsend[32];
  snprintf(cipsend, ARRAYSIZE(cipsend), "AT+CIPSEND=%d\r\n", strlen(message));
  softserial_puts(cipsend);
  _delay_ms(100);
  while(softserial_available()) softserial_getc();

  // Message
  softserial_puts(message);

  // TODO: Handle failure case. Timeout?
  const char *success_response = "SEND OK\r\n";
  int pos = 0;
  while(pos < strlen(success_response)) {
    char c = softserial_getc();
    if(c == success_response[pos]) {
      ++pos;
    } else {
      pos = 0;
    }
  }
}

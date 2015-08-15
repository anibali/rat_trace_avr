#include "wifi.h"
#include "softserial.h"
#include "util.h"
#include "pin.h"

#include <stdio.h>
#include <stdlib.h>
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
  DDRC |= _BV(DDC3);
}

void wifi_enable() {
  PORTC |= _BV(PORTC3);
}

void wifi_disable() {
  PORTC &= ~_BV(PORTC3);
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

void wifi_sendn(const void *message, int message_len) {
  printf("[WIFI] Sending...\n");

  softserial_printf("AT+CIPSTART=\"UDP\",\"%s\",%d\r\n", WIFI_DEST_IP, WIFI_DEST_PORT);
  print_response();

  softserial_printf("AT+CIPSEND=%d\r\n", message_len);
  print_response();

  softserial_clear_buffer();

  const char *message_chars = message;
  for(int i = 0; i < message_len; ++i) {
    softserial_putc(message_chars[i]);
  }
}

void wifi_send(const char *message) {
  wifi_sendn(message, strlen(message));
}

static bool wifi_is_send_ok() {
  char line[32];
  bool send_ok = false;

  while(!send_ok && softserial_available()) {
    softserial_readline(line, ARRAYSIZE(line));
    send_ok = (MEMCMP_CONST(line, "SEND OK\r\n") == 0);
  }

  return send_ok;
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

  //softserial_clear_buffer();

  return successful_send;
}

// NOTE: Big-endian
typedef struct {
  uint8_t li :2;
  uint8_t version :3;
  uint8_t mode :3;
  uint8_t stratum;
  uint8_t poll;
  uint8_t precision;
  uint32_t root_delay;
  uint32_t root_dispersion;
  uint32_t reference_id;
  uint32_t reference_time;
  uint32_t reference_time_frac;
  uint32_t origin_time;
  uint32_t origin_time_frac;
  uint32_t receive_time;
  uint32_t receive_time_frac;
  uint32_t transmit_time;
  uint32_t transmit_time_frac;
} NTP_Packet;

uint32_t swap_endian(uint32_t val) {
  return
    ((val & 0xFF      ) << 24 ) |
    ((val & 0xFF00    ) << 8  ) |
    ((val & 0xFF0000  ) >> 8  ) |
    ((val & 0xFF000000) >> 24 );
}

void wifi_request_ntp(uint32_t *time_val, Wifi_Error *error) {
  const int char_timeout_ms = 500;

  NTP_Packet packet = {0};
  packet.li = 3;
  packet.version = 4;
  packet.mode = 3;
  // TODO: More here...

  printf("[WIFI] Sending NTP request...\n");

  // TODO: Move definition of NTP server IP elsewhere
  softserial_printf("AT+CIPSTART=\"UDP\",\"129.250.35.250\",123\r\n");
  _delay_ms(1000);
  print_response();

  softserial_printf("AT+CIPSEND=%d\r\n", sizeof(NTP_Packet));
  print_response();

  softserial_clear_buffer();

  const char *message_chars = (char*)&packet;
  for(int i = 0; i < sizeof(NTP_Packet); ++i) {
    softserial_putc(message_chars[i]);
  }

  _delay_ms(1000);

  // "Received data" marker
  const char* marker = "\n+IPD,";
  int marker_pos = 1;

  // Wait for response
  while(marker_pos < strlen(marker)) {
    char c;
    if(!softserial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
    }
    if(c == marker[marker_pos]) {
      ++marker_pos;
    } else {
      marker_pos = 0;
    }
  }

  char response_len_str[16];

  for(int i = 0; true; ++i) {
    char c;
    if(!softserial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
    }
    if(c >= '0' && c <= '9') {
      response_len_str[i] = c;
    } else if(c == ':') {
      response_len_str[i] = '\0';
      break;
    } else {
      if(error != NULL) *error = Wifi_Error_Unknown;
    }
  }

  int response_len = atoi(response_len_str);

  if(response_len != sizeof(NTP_Packet)) {
    if(error != NULL) *error = Wifi_Error_Unknown;
  }

  char* packet_bytes = (char*)&packet;
  for(int i = 0; i < response_len; ++i) {
    char c;
    if(!softserial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
    }
    packet_bytes[i] = c;
  }

  softserial_clear_buffer();

  // Will error if remote closed connection already, that's fine
  softserial_printf("AT+CIPCLOSE\r\n");
  _delay_ms(100);
  print_response();

  // TODO: Use other data in some way
  *time_val = swap_endian(packet.transmit_time);
  *time_val -= 3155673600ul; // Y2K time

  if(error != NULL) *error = Wifi_Error_None;
}

#if 0
uint32_t wifi_request_time(Wifi_Error *error) {
  const int char_timeout_ms = 500;

  wifi_send("TIME");
  _delay_ms(1000);

  // "Received data" marker
  const char* marker = "\n+IPD,";
  int marker_pos = 1;

  // Wait for response
  while(marker_pos < strlen(marker)) {
    char c;
    if(!softserial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
      return 0;
    }
    if(c == marker[marker_pos]) {
      ++marker_pos;
    } else {
      marker_pos = 0;
    }
  }

  char response_len_str[16];

  for(int i = 0; true; ++i) {
    char c;
    if(!softserial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
      return 0;
    }
    if(c >= '0' && c <= '9') {
      response_len_str[i] = c;
    } else if(c == ':') {
      response_len_str[i] = '\0';
      break;
    } else {
      if(error != NULL) *error = Wifi_Error_Unknown;
      return 0;
    }
  }

  int response_len = atoi(response_len_str);

  char response[64];

  if(response_len > ARRAYSIZE(response) - 1) {
    if(error != NULL) *error = Wifi_Error_Buffer_Size;
    return 0;
  }

  for(int i = 0; i < response_len; ++i) {
    char c;
    if(!softserial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
      return 0;
    }
    response[i] = c;
  }

  response[response_len] = '\0';

  uint32_t time_val = strtoul(response, NULL, 10);

  softserial_clear_buffer();

  // Will error if remote closed connection already, that's fine
  softserial_printf("AT+CIPCLOSE\r\n");
  _delay_ms(100);
  print_response();

  if(error != NULL) *error = Wifi_Error_None;
  return time_val;
}

void wifi_test_tcp() {
  printf("[WIFI] Test...\n");

  softserial_printf("AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", "173.254.30.60", 80);
  _delay_ms(1000);
  print_response();

  const char *http = "GET /Test.txt HTTP/1.0\r\n\r\nHost: thearduinoguy.org\r\n\r\n";

  softserial_printf("AT+CIPSEND=%d\r\n", strlen(http));
  print_response();

  softserial_clear_buffer();
  softserial_printf(http);

  _delay_ms(1000);

  char line_buf[128];
  int content_length = -1;

  while(softserial_available()) {
    softserial_readline(line_buf, ARRAYSIZE(line_buf));

    if(memcmp(line_buf, "Content-Length: ", 16) == 0) {
      content_length = atoi(&line_buf[16]);
    } else if(content_length >= 0 && line_buf[0] == '\r') {
      break;
    }
  }

  // TODO: Protect against overflow
  char content[128];
  for(int i = 0; i < content_length; ++i) {
    content[i] = softserial_getc();
  }
  content[content_length] = '\0';

  printf("CONTENT: %s\n", content);

  softserial_clear_buffer();

  // Will error if remote closed connection already, that's fine
  softserial_printf("AT+CIPCLOSE\r\n");
  _delay_ms(100);
  print_response();
}
#endif

#include "wifi.h"

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "util.h"
#include "softserial.h"
#include "util.h"
#include "pin.h"

static const char AT_CIPSTART_UDP[] PROGMEM = "AT+CIPSTART=\"UDP\",\"%s\",%d\r\n";
static const char AT_CIPCLOSE[] PROGMEM     = "AT+CIPCLOSE\r\n";
static const char AT_CIPSEND[] PROGMEM      = "AT+CIPSEND=%d\r\n";
static const char AT_RST[] PROGMEM          = "AT+RST\r\n";
static const char AT_CWJAP[] PROGMEM        = "AT+CWJAP=\"%s\",\"%s\"\r\n";
static const char AT_CWJAPq[] PROGMEM       = "AT+CWJAP?\r\n";
static const char AT_CWMODE_1[] PROGMEM     = "AT+CWMODE=1\r\n";
static const char AT_CIPMUX_0[] PROGMEM     = "AT+CIPMUX=0\r\n";

static FILE *serial_output;
static FILE *serial_input;
static int (*serial_available)();

#define MEMCMP_CONST(ptr, const_str) memcmp(ptr, const_str, sizeof(const_str))

static void print_response() {
  char c;
  printf_P(PSTR("*** START RESPONSE ***\n"));
  while(!serial_available());
  _delay_ms(50);
  while(serial_available()) {
    c = fgetc(serial_input);
    if(c != '\r') putchar(c);
  }
  if(c != '\n') putchar('\n');
  printf_P(PSTR("*** END RESPONSE ***\n"));
}

void wifi_init(FILE *output, FILE *input, int (*available)()) {
  pin_set_direction(Pin_Wifi_Enable, Direction_Output);

  serial_output = output;
  serial_input = input;
  serial_available = available;
}

void wifi_enable() {
  pin_digital_write(Pin_Wifi_Enable, Logic_High);
}

void wifi_disable() {
  pin_digital_write(Pin_Wifi_Enable, Logic_Low);
}

static void wifi_repeat_until_ok(const char *cmd) {
  char line[32];

  for(int i = 0;;++i) {
    printf_P(PSTR("%s - Attempt %2d\n"), cmd, i);
    fprintf(serial_output, "%s", cmd);
    _delay_ms(200);

    while(serial_available()) {
      fgets(line, ARRAYSIZE(line), serial_input);
      if(MEMCMP_CONST(line, "OK\r\n") == 0) return;
    }

    _delay_ms(300);
  }
}

bool wifi_is_connected() {
  char line[32];
  bool connected = false;

  while(serial_available()) fgetc(serial_input);

  fputs_P(AT_CWJAPq, serial_output);

  _delay_ms(50);

  while(!connected && serial_available()) {
    fgets(line, ARRAYSIZE(line), serial_input);
    connected = MEMCMP_CONST(line, "OK\r\n") == 0;
  }

  while(serial_available()) fgetc(serial_input);

  return connected;
}

void wifi_connect() {
  char response[256];
  char response_len;

  printf_P(PSTR("[WIFI] Connecting...\n"));

  wifi_enable();
  _delay_ms(50);

  // Reset wireless
  fputs_P(AT_RST, serial_output);
  _delay_ms(50);
  while(serial_available()) fgetc(serial_input);

  wifi_repeat_until_ok("AT\r\n");

  fputs_P(AT_CWMODE_1, serial_output);
  _delay_ms(50);
  print_response();

  fprintf_P(serial_output, AT_CWJAP, WIFI_SSID, WIFI_PASS);
  print_response();

  // TODO: Retry connect after x failures
  while(!wifi_is_connected());

  fputs_P(AT_CIPMUX_0, serial_output);
  print_response();

  printf_P(PSTR("[WIFI] Connected.\n"));
}

void wifi_sendn(const void *message, int message_len) {
  printf_P(PSTR("[WIFI] Sending...\n"));

  char cmd[64];
  sprintf_P(cmd, AT_CIPSTART_UDP, WIFI_DEST_IP, WIFI_DEST_PORT);
  wifi_repeat_until_ok(cmd);

  fprintf_P(serial_output, AT_CIPSEND, message_len);
  print_response();

  while(serial_available()) fgetc(serial_input);

  const char *message_chars = message;
  for(int i = 0; i < message_len; ++i) {
    fputc(message_chars[i], serial_output);
  }
}

void wifi_send(const char *message) {
  wifi_sendn(message, strlen(message));
}

bool wifi_is_send_ok() {
  char line[32];
  bool send_ok = false;

  while(!send_ok && serial_available()) {
    fgets(line, ARRAYSIZE(line), serial_input);
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
    _delay_ms(100);
  }

  //while(serial_available()) fgetc(serial_input);

  return successful_send;
}

// NOTE: Big-endian
typedef struct {
  uint8_t li_version_mode;
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

static bool serial_getc_timeout(char *c, const int timeout_ms) {
  for(int i = 0; i < timeout_ms / 10; ++i) {
    if(!serial_available()) {
      _delay_ms(10);
    } else {
      *c = fgetc(serial_input);
      return true;
    }
  }

  return false;
}

// NOTE: Takes about 3s to run
void wifi_access_point_info() {
  printf_P(PSTR("[WIFI] Listing APs...\n"));
  fprintf_P(serial_output, PSTR("AT+CWLAP=\"%s\"\r\n"), WIFI_SSID);

  char message[128];
  char *msg_pos = message;

  bool success = false;
  const int max_attempts = 20;
  for(int i = 0; !success && i < max_attempts; ++i) {
    _delay_ms(200);
    fgets(msg_pos, ARRAYSIZE(message) - (msg_pos - message), serial_input);
    success = (MEMCMP_CONST(msg_pos, "OK\r\n") == 0);
    msg_pos = message + strlen(message) - 1;
  }
}

void wifi_request_ntp(uint32_t *time_val, Wifi_Error *error) {
  const int char_timeout_ms = 500;

  NTP_Packet packet = {0};
  packet.li_version_mode = (3 << 0) | (4 << 2) | (3 << 5);
  // TODO: More here...

  printf_P(PSTR("[WIFI] Sending NTP request...\n"));

  char cmd[64];
  sprintf_P(cmd, AT_CIPSTART_UDP, NTP_IP, NTP_PORT);
  wifi_repeat_until_ok(cmd);

  fprintf_P(serial_output, AT_CIPSEND, sizeof(NTP_Packet));
  print_response();

  while(serial_available()) fgetc(serial_input);

  const char *message_chars = (char*)&packet;
  for(int i = 0; i < sizeof(NTP_Packet); ++i) {
    fputc(message_chars[i], serial_output);
  }

  _delay_ms(1000);

  // "Received data" marker
  const char* marker = "\n+IPD,";
  int marker_pos = 1;

  // Wait for response
  while(marker_pos < strlen(marker)) {
    char c;
    if(!serial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
      return;
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
    if(!serial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
      return;
    }
    if(c >= '0' && c <= '9') {
      response_len_str[i] = c;
    } else if(c == ':') {
      response_len_str[i] = '\0';
      break;
    } else {
      if(error != NULL) *error = Wifi_Error_Unknown;
      return;
    }
  }

  int response_len = atoi(response_len_str);

  if(response_len != sizeof(NTP_Packet)) {
    if(error != NULL) *error = Wifi_Error_Unknown;
    return;
  }

  char* packet_bytes = (char*)&packet;
  for(int i = 0; i < response_len; ++i) {
    char c;
    if(!serial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
      return;
    }
    packet_bytes[i] = c;
  }

  while(serial_available()) fgetc(serial_input);

  // Will error if remote closed connection already, that's fine
  fprintf_P(serial_output, AT_CIPCLOSE);
  _delay_ms(100);
  print_response();

  // TODO: Use other data in some way
  *time_val = swap_endian(packet.transmit_time);
  //*time_val -= 3155673600ul; // Y2K time
  *time_val -= 0x83AA7E80; // Unix time

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
    if(!serial_getc_timeout(&c, char_timeout_ms)) {
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
    if(!serial_getc_timeout(&c, char_timeout_ms)) {
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
    if(!serial_getc_timeout(&c, char_timeout_ms)) {
      if(error != NULL) *error = Wifi_Error_Timeout;
      return 0;
    }
    response[i] = c;
  }

  response[response_len] = '\0';

  uint32_t time_val = strtoul(response, NULL, 10);

  while(serial_available()) fgetc(serial_input);

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

  while(serial_available()) fgetc(serial_input);
  softserial_printf(http);

  _delay_ms(1000);

  char line_buf[128];
  int content_length = -1;

  while(serial_available()) {
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

  while(serial_available()) fgetc(serial_input);

  // Will error if remote closed connection already, that's fine
  softserial_printf("AT+CIPCLOSE\r\n");
  _delay_ms(100);
  print_response();
}
#endif

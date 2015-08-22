#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
  Wifi_Error_None,
  Wifi_Error_Timeout,
  Wifi_Error_Buffer_Size,
  Wifi_Error_Unknown
} Wifi_Error;

#define NTP_IP          "129.250.35.250"
#define NTP_PORT        123

#define WIFI_SSID       "NETGEAR12"
#define WIFI_PASS       "sillyrabbit129"
#define WIFI_DEST_IP    "192.168.0.12"
#define WIFI_DEST_PORT  9252

// #define WIFI_SSID       "TP-LINK"
// #define WIFI_PASS       "anetworkpassword"
// #define WIFI_DEST_IP    "192.168.1.2"
// #define WIFI_DEST_PORT  9252

void wifi_init(FILE *output, FILE *input, int (*available)());
void wifi_connect();
void wifi_sendn(const void *message, int message_len);
void wifi_send(const char *message);
bool wifi_wait_for_send();
void wifi_enable();
void wifi_disable();

bool wifi_is_connected();

void wifi_request_ntp(uint32_t *time_val, Wifi_Error *error);
uint32_t wifi_request_time(Wifi_Error *error);

#endif

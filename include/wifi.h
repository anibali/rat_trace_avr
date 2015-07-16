#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  Wifi_Error_None,
  Wifi_Error_Timeout,
  Wifi_Error_Buffer_Size,
  Wifi_Error_Unknown
} Wifi_Error;

#define WIFI_SSID       "NETGEAR12"
#define WIFI_PASS       "sillyrabbit129"
#define WIFI_DEST_IP    "192.168.0.8"
#define WIFI_DEST_PORT  9252

//#define WIFI_SSID       "TP-LINK"
//#define WIFI_PASS       "anetworkpassword"
// #define WIFI_DEST_IP    "192.168.1.2"
// #define WIFI_DEST_PORT  9252

void wifi_init();
void wifi_connect();
void wifi_sendn(const void *message, int message_len);
void wifi_send(const char *message);
bool wifi_wait_for_send();
void wifi_enable();
void wifi_disable();

bool wifi_is_connected();

uint32_t wifi_request_time(Wifi_Error *error);

#endif

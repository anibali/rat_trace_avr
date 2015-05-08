#ifndef __WIFI_H__
#define __WIFI_H__

#define WIFI_SSID       "TP-LINK"
#define WIFI_PASS       "anetworkpassword"
#define WIFI_DEST_IP    "192.168.1.2"
#define WIFI_DEST_PORT  9252

void wifi_init();
void wifi_connect();
void wifi_send();
void wifi_wake();
void wifi_sleep();

#endif

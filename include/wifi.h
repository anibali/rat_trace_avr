#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdio.h>

#define WIFI_SSID "NETGEAR12"
#define WIFI_PASS "sillyrabbit129"

void wifi_init();
void wifi_connect();
void wifi_send();
void wifi_wake();
void wifi_sleep();

#endif

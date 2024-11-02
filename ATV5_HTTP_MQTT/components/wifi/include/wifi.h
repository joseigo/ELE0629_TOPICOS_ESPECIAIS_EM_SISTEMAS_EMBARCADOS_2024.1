#ifndef WIFI_H
#define WIFI_H

void wifi_init_sta(void);
void wifi_init_ap(const char *ssid, const char *pass);
#endif
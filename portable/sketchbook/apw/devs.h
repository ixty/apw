// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : devs.h                                                     //
// Description : devices (ap/clients) manager                               //
// ======================================================================== //

#ifndef _DEVS_H
#define _DEVS_H

#include "status.h"

#define MAX_DEVS_APS        192
#define MAX_DEVS_CLIENTS    192


int             dev_ap_index(wifi_ap_t * ap);
wifi_ap_t *     dev_ap_find(uint8_t * mac, int create);
void            dev_ap_update(wifi_ap_t * ap, int8_t rssi, char * essid, uint8_t channel, uint8_t enc, uint8_t ht);
int             dev_ap_has_cli(wifi_ap_t * ap, wifi_client_t * cli);
void            devs_ap_del(wifi_ap_t * ap);

wifi_client_t * dev_cli_find(uint8_t * mac);
void            dev_cli_update(wifi_client_t * cli, wifi_ap_t * ap, int8_t rssi);
void            dev_cli_bind(wifi_client_t * cli, wifi_ap_t * ap);
int             dev_cli_unbind(wifi_client_t * cli);
void            devs_cli_del(wifi_client_t * cli);

int             devs_cleanup(void);

void devs_lock(void);
void devs_unlock(void);

#endif

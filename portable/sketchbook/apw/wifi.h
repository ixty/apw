// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : wifi.cpp                                                   //
// Description : core of the program, handles everything wifi               //
// ======================================================================== //

#ifndef _WIFI_H
#define _WIFI_H

#include <stdint.h>
#include <stddef.h>

extern "C" {
    #include "user_interface.h"
}

struct RxControl
{
    signed rssi:8;
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;
    unsigned legacy_length:12;
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;
    unsigned CWB:1;
    unsigned HT_length:16;
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1;
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4;
    unsigned:12;
};

typedef struct frame_t
{
    uint8_t     type;
    uint8_t     flags;
    uint16_t    duration;
    uint8_t     addr1[6];
    uint8_t     addr2[6];
    uint8_t     addr3[6];
    uint16_t    seq;
    // -----------------
    uint8_t     data[];
} frame_t;


#define AP_ENC_OPEN         0
#define AP_ENC_WEP          1
#define AP_ENC_WPA1         2
#define AP_ENC_WPA1_PSK     2
#define AP_ENC_WPA1_MGT     3
#define AP_ENC_WPA2         4
#define AP_ENC_WPA2_PSK     4
#define AP_ENC_WPA2_MGT     5

#define AP_BAND_NORMAL      0
#define AP_BAND_HT          1
#define AP_BAND_VHT         2

// main funcs
void wifi_init(void);
void wifi_loop(void);
void wifi_switch_mode(void);
void wifi_switch_updown(int down);
void wifi_switch_leftright(int right);
int  wifi_send_pkt(uint8_t * frame, size_t len);
void wifi_shutdown(void);

// frame parsing
void wifi_sniff(uint8_t * buf, uint16_t len);
int  parse_beacon(frame_t * frame, size_t len, char essid[33], uint8_t * channel, uint8_t * enc, uint8_t * ht);
int  frame_get_macs(frame_t * frame, size_t frame_len, uint8_t ** cli, uint8_t ** bss);

// frame crafting
uint16_t craft_beacon(uint8_t * buf, uint8_t * bss, char * essid, uint16_t seq);
uint16_t craft_deauth(uint8_t * buf, uint8_t * client, uint8_t * ap, uint16_t seq);
uint16_t craft_data(uint8_t * buf, uint8_t * cli, uint8_t * bss, int from_ds, uint16_t seq);

// timer callbacks
void cb_cswitch(void);
void cb_report(void);
void cb_beacon(void);
void cb_detect(void);
void cb_cleanup(void);


#endif

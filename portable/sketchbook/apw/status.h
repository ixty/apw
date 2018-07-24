// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : status.h                                                   //
// Description : our main structs + globals                                 //
// ======================================================================== //

#ifndef _STATUS_H
#define _STATUS_H

#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <stdint.h>
#include <stddef.h>

struct wifi_ap_s;

// structure for a connected client device
typedef struct wifi_client_s
{
    uint8_t         mac[6];         // mac address
    uint32_t        time_f;         // first seen timestamp
    uint32_t        time_l;         // last seen timestamp
    uint32_t        pkt_count;      // number of packets seen
    int8_t          rssi;           // last (or average) signal strenght
    char            vendor[9];      // oui lookup

    struct wifi_ap_s *      ap;     // ptr to current access point
    struct wifi_client_s *  apnext; // next client connected to AP

    struct wifi_client_s *  next;   // next in circular linked list
    struct wifi_client_s *  prev;   // prev in circular linked list
} wifi_client_t;


// structure for detected access point
typedef struct wifi_ap_s
{
    uint8_t         mac[6];         // mac address
    uint32_t        time_f;         // first seen timestamp
    uint32_t        time_l;         // last seen timestamp
    char            vendor[9];      // oui lookup

    int8_t          rssi;           // last (or average) signal strenght
    uint8_t         channel;        // advertised channel
    char            essid[33];      // network name (max 32b + null)
    uint32_t        essid_time;     // timestamp of last essid change
    uint16_t        essid_count;    // number of essid switches
    uint32_t        beacons;        // number of beacons seen
    uint8_t         enc;            // 0=open, 1=wep,  2=wpa1-psk, 3=wpa1-mgt, 4=wpa2-psk, 5=wpa2-mgt
    uint8_t         ht;             // 0=bg,   1=ht(n) 2=vht(ac)

    wifi_client_t * clients;        // linked list of current clients
    uint8_t         clients_count;  // number of clients

    struct wifi_ap_s *  next;       // next in circular linked list
    struct wifi_ap_s *  prev;       // prev in circular linked list
} wifi_ap_t;


typedef enum wmode_t
{
    MODE_DETECT,
    MODE_DEAUTH,
    MODE_BEACON,

    MODE_MAX,
} wmode_t;

#define wmode_str(x) (char*)(x == MODE_DETECT ? "detect" : (x == MODE_DEAUTH ? "deauth" : (x == MODE_BEACON ? "beacon" : "?")))

// main program status struct
typedef struct status_t
{
    // wlan control
    uint8_t         phy;            // band mode (B, G or N)
    uint8_t         channel;        // current channel
    uint32_t        chanhop;        // interval between channel hops
    uint8_t         channel_focus;  // do we want to focus on a specific channel (chan hop but stay there half the time)

    // internal status
    wmode_t         mode;           // detect / deauth / beacon
    uint32_t        attack_time;
    uint32_t        pkt_sent;
    uint32_t        pkt_recv;
    uint32_t        pkt_errs;

    // device lists
    wifi_ap_t *     aps;
    uint32_t        aps_count;

    wifi_client_t * clients;
    uint32_t        clients_count;

    // attack detection
    uint32_t        detected_pkt_deauth;
    uint32_t        detected_fake_aps;
    uint8_t         detected_deauth;
    uint8_t         detected_karma;
    uint8_t         detected_beacon;

    // attack stats
    uint32_t        spoofed_aps;
    uint32_t        spoofed_clients;

    // input buttons
    int             ui_level;       // navigation level (only for detect right now)
    int             ui_idx;         // current selection index

    // detect options
    wifi_ap_t *     cur_ap;         // currently selected / displayed AP
    wifi_client_t * cur_cli;        // currently selected / displayed client

    // deauth options
    uint8_t         deauth_mode;    // 0 = attack everybody, 1 = blacklist, 2 = whitelist
    uint8_t         deauth_mac[6];  // blacklist or whitelist (single AP mac)

    // graph view data
    int             graph_data[128];
    int             graph_count;

} status_t;


// global variable
extern status_t status;

#endif

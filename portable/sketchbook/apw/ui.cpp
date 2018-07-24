// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : ui.cpp                                                     //
// Description : handle all i/o (serial + screen + buttons + leds)          //
// ======================================================================== //

#include "status.h"
#include "wifi.h"
#include "ui.h"
#include "utils.h"
#include "devs.h"

#define RST_OLED 16
#define DISPLAY_FPS 8

OLED display(4, 5);

void ui_init(void)
{
    // init serial io
    Serial.begin(115200);
    Serial.println("\n");
    Serial.println("> init_ui\n");

    // init oled screen
    pinMode(RST_OLED, OUTPUT);
    digitalWrite(RST_OLED, LOW);    // turn D2 low to reset OLED
    delay(20);
    digitalWrite(RST_OLED, HIGH);   // while OLED is running, must set D2 in high

    // init buttons
    pinMode(D6, INPUT_PULLUP);
    pinMode(D7, INPUT_PULLUP);
    pinMode(D8, OUTPUT);
    delay(10);

    // display intro & clear screen
    display.begin();
    ui_intro();
    display.clear();
}

void ui_clear(void)
{
    display.print("                ", 0, 0);
    display.print("                ", 1, 0);
    display.print("                ", 2, 0);
    display.print("                ", 3, 0);
}

void ui_loop(void)
{
    static uint32_t time_draw = 0;
    uint32_t now = millis();

    // update
    if(now - time_draw > (1000 / DISPLAY_FPS))
    {
        time_draw = now;

        ui_input();
        ui_draw();
    }
}

void ui_led(int on)
{
    int stat = -1;

    if(stat != on)
    {
        digitalWrite(D8, on ? HIGH : LOW);
        stat = on;
    }
}

void ui_intro(void)
{
    display.clear();
    display.print("mbh :)", 1, 5);
    for(int i=0; i<8; i++)
    {
        display.print("*", 0, 7-i);
        display.print("*", 0, 8+i);
        display.print("*", 3, 7-i);
        display.print("*", 3, 8+i);
        delay(33);
    }
    delay(500);
}

void ui_input(void)
{
    static uint8_t  last_btn_btn1 = 0;
    static uint8_t  last_btn_btn2 = 0;
    static uint8_t  last_btn_both = 0;
    static uint32_t time_long_btn1 = 0;
    static uint32_t time_long_btn2 = 0;
    static uint32_t time_long_both = 0;

    uint32_t now = millis();

    // read button status
    int btn1 = digitalRead(D7) == LOW;
    int btn2 = digitalRead(D6) == LOW;

    // handle long presses
    HANDLE_LONG_PRESS(both, btn1 && btn2,   LONG_PRESS_SHUTDOWN,    ui_click(3, 1));
    HANDLE_LONG_PRESS(btn1, btn1,           LONG_PRESS_BUTTON,      ui_click(1, 1));
    HANDLE_LONG_PRESS(btn2, btn2,           LONG_PRESS_BUTTON,      ui_click(2, 1));

    // handle clicks
    HANDLE_CLICK(both,      btn1 && btn2,                           ui_click(3, 0));
    HANDLE_CLICK(btn1,      btn1,                                   ui_click(1, 0));
    HANDLE_CLICK(btn2,      btn2,                                   ui_click(2, 0));
}

void ui_click(int btn, int hold)
{
    ui_printf("> click btn: %d long: %d\n", btn, hold);

    // hold double click
    if(btn == 3 && hold)
        wifi_shutdown();

    // double click
    else if(btn == 3)
        wifi_switch_mode();

    // hold btn1
    else if(btn == 1 && hold)
        wifi_switch_updown(0);

    // click btn1
    else if(btn == 1)
        wifi_switch_leftright(0);

    // hold btn2
    else if(btn == 2 && hold)
        wifi_switch_updown(1);

    // click btn2
    else if(btn == 2)
        wifi_switch_leftright(1);
}

// spinner + band + channel + mode
void ui_draw_header(void)
{
    static int  spin_ind = 0;
    char spin_chars[]    = { '|', '/', '-', '\\' };
    char phy_modes[]     = { 'B', 'G', 'N' };
    char modestr[9];

    if(status.mode == MODE_DETECT)
        snprintf(modestr, sizeof(modestr), "scan");
    else if(status.mode == MODE_DEAUTH && status.deauth_mode == 0)
        snprintf(modestr, sizeof(modestr), "deauth *");
    else if(status.mode == MODE_DEAUTH && status.deauth_mode == 1)
        snprintf(modestr, sizeof(modestr), "deauth +");
    else if(status.mode == MODE_DEAUTH && status.deauth_mode == 2)
        snprintf(modestr, sizeof(modestr), "deauth -");
    else if(status.mode == MODE_BEACON)
        snprintf(modestr, sizeof(modestr), "beacon");
    else
        snprintf(modestr, sizeof(modestr), "?");

    ui_line(0, "%c %c %-2d  %s", spin_chars[(spin_ind++) % 4], phy_modes[status.phy-1], wifi_get_channel(), modestr);
}

void ui_draw_sendstats(int line)
{
    char tmp1[8], tmp2[8];
    fmt_num(status.pkt_sent, tmp1, sizeof(tmp1));
    fmt_num(status.pkt_errs, tmp2, sizeof(tmp2));
    ui_line(line, "p: %s e: %s", tmp1, tmp2);
}

// draw pps
void ui_draw_pps(int line)
{
    static uint32_t last_time = 0;
    static uint32_t last_pkt_sent = 0;
    static uint32_t pps = 0;
    uint32_t now = millis();

    if(!last_time || last_pkt_sent > status.pkt_sent)
    {
        pps = 0;
        last_time = now;
        last_pkt_sent = status.pkt_sent;
    }
    else if(now - last_time > 1000)
    {
        pps = (status.pkt_sent - last_pkt_sent) * 1000 / (now - last_time);
        last_time = now;
        last_pkt_sent = status.pkt_sent;
    }
    ui_line(line, "pps: %d", pps);
}

void ui_draw_alert(void)
{
    ui_line(0, "+--------------+");
    ui_line(1, "|  ! attack !  | ");
    if(status.detected_deauth && status.detected_beacon && status.detected_karma)
        ui_line(2, "|    D B K    |");
    else if(status.detected_deauth && status.detected_beacon)
        ui_line(2, "|deauth  beacon|");
    else if(status.detected_deauth && status.detected_karma)
        ui_line(2, "|deauth  fakeap|");
    else if(status.detected_beacon && status.detected_karma)
        ui_line(2, "|beacon  fakeap|");
    else if(status.detected_deauth)
        ui_line(2, "|    deauth    |");
    else if(status.detected_beacon)
        ui_line(2, "|    beacon    |");
    else if(status.detected_karma)
        ui_line(2, "|    fakeAP    |");
    else
        ui_line(3, "|      ??      |");
    ui_line(3, "+--------------+");

}

void ui_draw_ap(wifi_ap_t * ap)
{
    char * enc_str[] = { "open", "wep", "wpa1", "eap1", "wpa2", "eap2" };
    char uptime[8];
    // char macstr[8];

    if(!ap)
    {
        ui_line(0, " *  lost AP  * ");
        ui_line(1, "");
        ui_line(2, "");
        ui_line(3, "");
        return;
    }

    // bin_to_hex(ap->mac + 3, 3, macstr, sizeof(macstr));
    fmt_uptime(millis() - ap->time_l, uptime, sizeof(uptime));

    ui_line(0, "#%d /%d %s %d", dev_ap_index(ap) + 1, status.aps_count, uptime, ap->rssi);
    ui_line(1, "%s", ap->essid);
    // ui_line(2, "%s:%s", ap->vendor, macstr);
    ui_line(2, "%s", ap->vendor);
    ui_line(3, "%-2d%c %s cli %d", ap->channel, ap->ht ? 'N' : 'G', enc_str[ap->enc], ap->clients_count);
}


void ui_draw_cli(wifi_client_t * cli, size_t ind)
{
    char macstr[16];
    char uptime[8];
    char pktc[8];

    if(!cli)
    {
        ui_line(0, " * no clients * ");
        ui_line(1, "");
        ui_line(2, "");
        ui_line(3, "");
        return;
    }


    bin_to_hex(cli->mac, 6, macstr, sizeof(macstr));
    fmt_uptime(millis() - cli->time_l, uptime, sizeof(uptime));
    fmt_num(cli->pkt_count, pktc, sizeof(pktc));

    if(cli->ap)
        ui_line(0, "# %d / %d", ind + 1, cli->ap->clients_count);
    else
        ui_line(0, "# %d / ?", ind + 1);
    ui_line(1, "%s", macstr);
    ui_line(2, "%s", cli->vendor);
    ui_line(3, "%d %sp %s", cli->rssi, pktc, uptime);
}

void ui_draw(void)
{
    uint32_t now = millis();

    if(status.mode == MODE_DETECT)
    {
        // alert detected?
        if(status.detected_deauth || status.detected_beacon || status.detected_karma )
        {
            ui_led(1);
            ui_draw_alert();
            return;
        }

        ui_led(0);
        // standard status display
        if(status.ui_level == 0)
        {
            ui_draw_header();
            ui_line(1, "ap %-5dcli %-4d", status.aps_count, status.clients_count);
            ui_line(2, "");

            char uptime[16];
            fmt_uptime(now, uptime, sizeof(uptime));
            ui_line(3, "up: %s", uptime);
        }
        // AP view
        else if(status.ui_level == 1)
        {
            devs_lock();
            ui_draw_ap(status.cur_ap);
            devs_unlock();
        }
        // Actions on AP
        else if(status.ui_level == 2)
        {
            int ind = status.ui_idx % 4;
            if(ind < 0)
                ind += 4;
            ui_line(0, "%c clients",        ind == 0 ? '>' : ' ');
            ui_line(1, "%c deauth net",     ind == 1 ? '>' : ' ');
            ui_line(2, "%c deauth except",  ind == 2 ? '>' : ' ');
            ui_line(3, "%c rssi graph",     ind == 3 ? '>' : ' ');
        }
        // clients view
        else if(status.ui_level == 3)
        {
            devs_lock();
            ui_draw_cli(status.cur_cli, status.ui_idx);
            devs_unlock();
        }
        // AP rssi graph
        else if(status.ui_level == 4)
        {
            static uint32_t last_ap_time = 0;

            if(!status.cur_ap)
                ui_line(0, "  * lost AP *  ");
            else
            {
                int val = 100 + status.cur_ap->rssi;
                if(last_ap_time == status.cur_ap->time_l)
                    val = 0;
                last_ap_time = status.cur_ap->time_l;

                if(status.graph_count < 128)
                    status.graph_data[status.graph_count++] = val;
                else
                {
                    memmove(&status.graph_data[0], &status.graph_data[1], 127 * sizeof(int));
                    status.graph_data[status.graph_count-1] = val;
                }
                draw_graph(status.graph_data, status.graph_count, 0, 75);
            }
        }
        // client rssi graph
        else if(status.ui_level == 5)
        {
            static uint32_t last_client_time = 0;

            if(!status.cur_cli)
                ui_line(0, "* lost client *");
            else
            {
                int val = 100 + status.cur_cli->rssi;
                if(last_client_time == status.cur_cli->time_l)
                    val = 0;
                last_client_time = status.cur_cli->time_l;

                if(status.graph_count < 128)
                    status.graph_data[status.graph_count++] = val;
                else
                {
                    memmove(&status.graph_data[0], &status.graph_data[1], 127 * sizeof(int));
                    status.graph_data[status.graph_count-1] = val;
                }

                draw_graph(status.graph_data, status.graph_count, 0, 75);
            }
        }
    }
    else if(status.mode == MODE_BEACON)
    {
        ui_draw_header();

        // number of fake APs & clients we spoofed
        char tmp1[8], tmp2[8];
        fmt_num(status.spoofed_aps,     tmp1, sizeof(tmp1));
        fmt_num(status.spoofed_clients, tmp2, sizeof(tmp2));
        ui_line(1, "a: %s c: %s",       tmp1, tmp2);

        // packet sending stats
        ui_draw_sendstats(2);
        ui_draw_pps(3);
    }
    else if(status.mode == MODE_DEAUTH)
    {
        char tmp[17];
        ui_draw_header();

        // display target
        bin_to_hex(status.deauth_mac, 6, tmp, sizeof(tmp));
        if(status.deauth_mode == 1)
            ui_line(1, "att %s", tmp);
        else if(status.deauth_mode == 2)
            ui_line(1, "not %s", tmp);
        else
            ui_line(1, "attack *");

        // currently attacking someone?
        if(status.attack_time && status.attack_time + 250 > now)
        {
            ui_led(1);
            ui_line(2, "deauthing!");
        }
        else
        {
            ui_led(0);
            ui_line(2, "");
        }

        ui_draw_pps(3);
    }
}

void draw_graph(int * vals, int count, int min, int max)
{
    uint8_t columns[128];
    int sch = 8 * 4;        // 4 rows of 8 pixels

    // max screen size
    if(count > 128)
        count = 128;

    // auto scale?
    if(!max)
        for(size_t i=0; i<count; i++)
            if(vals[i] > max)
                max = (vals[i] - min);
    if(!max)
        return;

    for(size_t i=0; i<count; i++)
        columns[i] = sch * (vals[i] - min) / max;

    for(int row=0; row<4; row++)
    {
        display.setXY(row, 0);

        for(int i=0; i<count; i++)
        {
            uint8_t c = 0;
            int n_pix = (int)columns[i] - (3 - row) * 8;
            for(int j=0; j<n_pix && j<8; j++)
                c |= 1 << (7 - j);
            display.SendChar(c);
        }
        for(int i=count; i<128; i++)
            display.SendChar(0);
    }
}


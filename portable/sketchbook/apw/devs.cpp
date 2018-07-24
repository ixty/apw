// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : devs.cpp                                                   //
// Description : devices (ap/clients) manager                               //
// ======================================================================== //

#include "devs.h"
#include "Arduino.h"
#include "ui.h"
#include "utils.h"

extern "C" {
    #include "user_interface.h"
}


// ========================================================================= //
// AP functions
// ========================================================================= //

// get index of AP in our list from ptr
int dev_ap_index(wifi_ap_t * ap)
{
    if(!status.aps)
        return -1;

    wifi_ap_t * a = status.aps;
    int n = 0;
    do
    {
        if(a == ap)
            return n;
        n += 1;
        a = a->next;
    } while(a != status.aps);

    return -1;
}

// find (or create) an AP slot
wifi_ap_t * dev_ap_find(uint8_t * mac, int create)
{
    // filter bad macs
    if(is_bad_mac(mac))
        return NULL;

    // find existing ap
    if(status.aps)
    {
        wifi_ap_t * ap = status.aps;
        do
        {
            if(!memcmp(ap->mac, mac, 6))
                return ap;
            ap = ap->next;
        } while(ap != status.aps);
    }

    if(!create)
        return NULL;

    // create new one
    if(status.aps_count < MAX_DEVS_APS)
    {
        wifi_ap_t * ap = (wifi_ap_t *)malloc(sizeof(wifi_ap_t));
        if(!ap)
            return NULL;

        memset(ap, 0, sizeof(wifi_ap_t));
        memcpy(ap->mac, mac, 6);
        ap->time_l = ap->time_f = millis();
        search_vendor(mac, ap->vendor);

        clist_push_back(&status.aps, ap);
        status.aps_count += 1;
        return ap;
    }

    return NULL;
}

// update an AP with info parsed
void dev_ap_update(wifi_ap_t * ap, int8_t rssi, char * essid, uint8_t channel, uint8_t enc, uint8_t ht)
{
    if(!ap) return;

    ap->time_l      = millis();
    ap->beacons    += 1;
    ap->rssi        = rssi;
    if(essid && essid[0])
    {
        // changing essid?
        if(ap->essid[0] && strcmp(ap->essid, "<hidden>") && strcmp(ap->essid, essid))
        {
            char tmp[20];
            bin_to_hex(ap->mac, 6, tmp, sizeof(tmp));
            ui_printf("essid change %s ('%s'[%d] -> '%s'[%d])\n", tmp, ap->essid, strlen(ap->essid), essid, strlen(essid));

            ap->essid_count += 1;
            ap->essid_time = ap->time_l;
            snprintf(ap->essid, sizeof(ap->essid), "%s", essid);
        }
        // first time seeing this ap
        else if(strcmp(ap->essid, essid))
        {
            ap->essid_time = 0;
            ap->essid_count = 0;
            snprintf(ap->essid, sizeof(ap->essid), "%s", essid);
        }
    }
    else if(!ap->essid[0])
        snprintf(ap->essid, sizeof(ap->essid), "<hidden>");
    if(channel)
        ap->channel = channel;
    if(enc)
        ap->enc = enc;
    if(ht)
        ap->ht = ht;
}

// utility to tell if an ap has a specific client
int dev_ap_has_cli(wifi_ap_t * ap, wifi_client_t * cli)
{
    for(wifi_client_t * c = ap->clients; c != NULL; c = c->apnext)
        if(c == cli)
            return 1;

    return 0;
}


// ========================================================================= //
// client functions
// ========================================================================= //

// find or create client slot
wifi_client_t * dev_cli_find(uint8_t * mac)
{
    // filter bad macs
    if(is_bad_mac(mac))
        return NULL;

    // find existing client
    if(status.clients)
    {
        wifi_client_t * cli = status.clients;
        do
        {
            if(!memcmp(cli->mac, mac, 6))
                return cli;
            cli = cli->next;
        } while(cli != status.clients);
    }

    // create new one
    if(status.clients_count < MAX_DEVS_CLIENTS)
    {
        wifi_client_t * cli = (wifi_client_t *)malloc(sizeof(wifi_client_t));
        if(!cli)
            return NULL;

        memset(cli, 0, sizeof(wifi_client_t));
        memcpy(cli->mac, mac, 6);
        cli->time_l = cli->time_f = millis();
        search_vendor(mac, cli->vendor);


        clist_push_back(&status.clients, cli);
        status.clients_count += 1;
        return cli;
    }

    return NULL;
}

// bind a client to an AP, client must be unbound! before this call
void dev_cli_bind(wifi_client_t * cli, wifi_ap_t * ap)
{
    cli->ap             = ap;
    cli->apnext         = ap->clients;
    ap->clients         = cli;
    ap->clients_count  += 1;
}

// unbinds cli from ap
int dev_cli_unbind(wifi_client_t * cli)
{
    wifi_ap_t *     ap = cli->ap;
    wifi_client_t * c, * p;

    // not bound? abort
    if(!ap)
        return -1;

    // make sure cli is bound to ap first
    if(!dev_ap_has_cli(ap, cli))
    {
        cli->ap = NULL;
        cli->apnext = NULL;
        return -1;
    }

    // first client in linked list?
    if(ap->clients == cli)
    {
        ap->clients = ap->clients->apnext;
        cli->ap = NULL;
        cli->apnext = NULL;
        ap->clients_count -= 1;
        return 0;
    }

    p = ap->clients;
    c = p->apnext;

    while(c)
    {
        if(c == cli)
        {
            p->apnext = c->apnext;
            ap->clients_count -= 1;
            cli->apnext = NULL;
            cli->ap = NULL;
            return 0;
        }
        p = c;
        c = c->apnext;
    }

    return -1;
}

// update client slot with info & binds it to current access point if needed
void dev_cli_update(wifi_client_t * cli, wifi_ap_t * ap, int8_t rssi)
{
    if(!cli)    return;

    // update client dev
    cli->time_l = millis();
    cli->pkt_count += 1;
    cli->rssi = rssi;

    if(!ap)     return;

    // not bound yet?
    if(!cli->ap)
        dev_cli_bind(cli, ap);

    // bound to a different AP?
    else if(cli->ap != ap)
    {
        dev_cli_unbind(cli);
        dev_cli_bind(cli, ap);
    }
}


// ========================================================================= //
// devs cleanup
// ========================================================================= //

// remove a client from our list & fixes everything thats needed
void devs_cli_del(wifi_client_t * cli)
{
    // unbind first
    dev_cli_unbind(cli);

    // unlink from circular linked list
    clist_unlink(&status.clients, cli);
    status.clients_count -= 1;

    // currently selected / displayed?
    if(status.cur_cli == cli)
        status.cur_cli = NULL;

    // free memory
    free(cli);
}

// remove a client from our list & fixes everything thats needed
void devs_ap_del(wifi_ap_t * ap)
{
    // unbind all clients first
    while(ap->clients)
    {
        wifi_client_t * c = ap->clients;
        ap->clients = c->apnext;
        c->ap = NULL;
        c->apnext = NULL;
    }

    // unlink from circular linked list
    clist_unlink(&status.aps, ap);
    status.aps_count -= 1;

    // currently selected / displayed?
    if(status.cur_ap == ap)
        status.cur_ap = NULL;

    // free memory
    free(ap);
}


// remove APs and clients that we havent seen in a long time
int devs_cleanup(void)
{
    devs_lock();

    uint32_t now = millis();
    int n = 0;

    // clients first
    if(status.clients)
    {
        wifi_client_t * cli = status.clients;
        do
        {
            if(cli->time_l + 60000 < now)
            {
                wifi_client_t * next = (cli == cli->next) ? NULL : cli->next;
                devs_cli_del(cli);
                n += 1;
                cli = next;
            }
            else
                cli = cli->next;
        } while(cli && cli != status.clients);
    }

    // APs now
    if(status.aps)
    {
        wifi_ap_t * ap = status.aps;
        do
        {
            int is_old = ap->time_l + 60000 < now;
            int is_fake = (ap->time_l + 5000 < now) && (ap->beacons == 1) && !(ap->clients);

            if(is_old || is_fake)
            {
                wifi_ap_t * next = (ap == ap->next) ? NULL : ap->next;
                devs_ap_del(ap);
                n += 1;
                ap = next;

                if(is_fake)
                    status.detected_fake_aps += 1;
            }
            else
                ap = ap->next;
        } while(ap && ap != status.aps);
    }

    devs_unlock();
    return n;
}

// ========================================================================= //
// SMP safety :)
// ========================================================================= //

// esp8266 is dual core :)
// gota protect promisc cb access when we cleanup old devices!
static volatile int     devs_spinlock = 0;
void devs_lock(void)
{
    while(devs_spinlock)
        delay(0);
    devs_spinlock = 1;
}

void devs_unlock(void)
{
    devs_spinlock = 0;
}


// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : utils.h                                                    //
// Description : various utility functions                                  //
// ======================================================================== //

#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <stddef.h>

#include "clist.h"

int is_bad_mac(uint8_t * mac);

char hexchar(uint8_t val);
int bin_to_hex(const uint8_t * bin_data, size_t bin_len, char * buf, size_t max_len);

int fmt_num(size_t n, char * buf, size_t buf_len);
int fmt_uptime(uint32_t now, char * buf, size_t buf_len);

#define WTIMER(time, callback)                  \
    static uint32_t callback##_timer = 0;       \
    if(!callback##_timer)                       \
        callback##_timer = now;                 \
    else if(callback##_timer + time < now)      \
    {                                           \
        callback##_timer = now;                 \
        callback();                             \
        return;                                 \
    }

int rand_essid(char * buf, size_t buf_size);
void rand_mac(uint8_t * mac);

int clamp_modulo(int val, int max);

// code stolen from https://github.com/spacehuhn/esp8266_deauther
int search_vendor(uint8_t * mac, char vendor[9]);

#endif

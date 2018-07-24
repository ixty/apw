// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : utils.cpp                                                  //
// Description : various utility functions                                  //
// ======================================================================== //

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// filter bad macs
int is_bad_mac(uint8_t * mac)
{
    if(!memcmp(mac, "\x00\x00\x00\x00\x00\x00", 6))
        return 1;
    if(!memcmp(mac, "\xff\xff\xff\xff\xff\xff", 6))
        return 1;
    if(!memcmp(mac, "\x33\x33", 2))
        return 1;

    // taken from https://github.com/spacehuhn/esp8266_deauther/blob/master/esp8266_deauther/functions.h
    if((mac[0] == 0x01) && (mac[1] == 0x00) && ((mac[2] == 0x5E) || (mac[2] == 0x0C))) return 1;

    if((mac[0] == 0x01) && (mac[1] == 0x0C) && (mac[2] == 0xCD) &&
        ((mac[3] == 0x01) || (mac[3] == 0x02) || (mac[3] == 0x04)) &&
        ((mac[4] == 0x00) || (mac[4] == 0x01))) return 1;

    if((mac[0] == 0x01) && (mac[1] == 0x00) && (mac[2] == 0x0C) && (mac[3] == 0xCC) && (mac[4] == 0xCC) &&
        ((mac[5] == 0xCC) || (mac[5] == 0xCD))) return 1;

    if((mac[0] == 0x01) && (mac[1] == 0x1B) && (mac[2] == 0x19) && (mac[3] == 0x00) && (mac[4] == 0x00) &&
        (mac[5] == 0x00)) return 1;

    return 0;
}

// bin to hex conversion
char hexchar(uint8_t val)
{
    val &= 0xf;
    if(val < 0xa)
        return '0' + val;
    else
        return 'a' + val - 10;
}

// used to convert mac to str
int bin_to_hex(const uint8_t * bin_data, size_t bin_len, char * buf, size_t max_len)
{
    const uint8_t  * b = bin_data;
    const uint8_t  * b_end = bin_data + bin_len;
    char           * p = buf;
    char           * p_end = buf + max_len;

    if (max_len == 0)
        return -1;
    while (b < b_end && p+2+1 <= p_end)
    {
        *p++ = hexchar((*b) >> 4);
        *p++ = hexchar((*b) & 0x0f);
        b++;
    }
    *p++ = '\0';
    return p - buf;
}

// make number display smaller
int fmt_num(size_t n, char * buf, size_t buf_len)
{
    if(n == 0)
        return snprintf(buf, buf_len, "0");
    if(n < 1000)
        return snprintf(buf, buf_len, "%d", n);
    if(n < 1000 * 1000)
        return snprintf(buf, buf_len, "%dK", n / 1000);
    if(n < 1000 * 1000 * 1000)
        return snprintf(buf, buf_len, "%dM", n / 1000000.0f);
    return snprintf(buf, buf_len, "%dG", n / 1000000000.0f);
}

// display elapsed time
int fmt_uptime(uint32_t now, char * buf, size_t buf_len)
{
    uint32_t t = now / 1000;
    if(t < 60)
        snprintf(buf, buf_len, "%ds", t);
    else if(t < 60 * 60)
        snprintf(buf, buf_len, "%dm%ds", t / 60, t % 60);
    else if(t < 60 * 60 * 24)
        snprintf(buf, buf_len, "%dh%dm", t / (60 * 60), (t % (60 * 60)) / 60);
    else
        snprintf(buf, buf_len, "%dd", t / (60 * 60 * 24));
}

// make a random string
int rand_essid(char * buf, size_t buf_size)
{
    const char *  charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_&!?#$^~<>[]()+/=.,;:\\";
    int     max     = 85; // strlen(charset)

    memset(buf, 0, buf_size);
    for(int i=0; i<rand() % buf_size; i++)
        buf[i] = charset[rand() % max];

    return 0;
}

// // totally random mac addr
// void rand_mac(uint8_t * mac)
// {
//     int r1 = rand();
//     int r2 = rand();

//     mac[0] = (r1 >> 0)  & 0xff;
//     mac[1] = (r1 >> 8)  & 0xff;
//     mac[2] = (r1 >> 16) & 0xff;
//     mac[3] = (r2 >> 0)  & 0xff;
//     mac[4] = (r2 >> 8)  & 0xff;
//     mac[5] = (r2 >> 16) & 0xff;
// }


int clamp_modulo(int val, int max)
{
    if(!max)
        return 0;

    while(val < 0)
        val += max;
    while(val >= max)
        val -= max;

    return val;
}

// adapted from https://github.com/spacehuhn/esp8266_deauther/
#include "Arduino.h"
#include "oui.h"

void rand_mac(uint8_t* mac)
{
    int num = random(sizeof(data_vendors) / 11 - 1);
    uint8_t i;

    for(i=0; i<3; i++)
        mac[i] = pgm_read_byte_near(data_macs + num * 5 + i);

    for(i=3; i<6; i++)
        mac[i] = random(256);
}

int bin_search_vendor(uint8_t * search, int beg, int end)
{
    uint8_t cur[3];
    int     res;
    int     mid = (beg + end) / 2;

    while(beg <= end)
    {
        cur[0] = pgm_read_byte_near(data_macs + mid * 5);
        cur[1] = pgm_read_byte_near(data_macs + mid * 5 + 1);
        cur[2] = pgm_read_byte_near(data_macs + mid * 5 + 2);

        res = memcmp(search, cur, 3);

        if (res == 0)
            return mid;
        else if(res < 0)
        {
            end = mid - 1;
            mid = (beg + end) / 2;
        }
        else if(res > 0)
        {
            beg = mid + 1;
            mid = (beg + end) / 2;
        }
    }
    return -1;
}

int search_vendor(uint8_t * mac, char vendor[9])
{
    int idx = bin_search_vendor(mac, 0, sizeof(data_macs) / 5 - 1);
    int pos = pgm_read_byte_near(data_macs + idx * 5 + 3) | pgm_read_byte_near(data_macs + idx * 5 + 4) << 8;

    if(idx >= 0)
    {
        for(int i=0; i<8; i++)
            vendor[i] = (char)pgm_read_byte_near(data_vendors + pos * 8 + i);
        return 0;
    }
    return -1;
}

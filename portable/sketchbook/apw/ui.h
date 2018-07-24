// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : ui.h                                                       //
// Description : handle all i/o (serial + screen + buttons + leds)          //
// ======================================================================== //

#ifndef _UI_H
#define _UI_H

#include <stdarg.h>
#include <Wire.h>
#include "OLED.h"

#include "status.h"

void    ui_init(void);
void    ui_loop(void);

void    ui_led(int on);
void    ui_draw(void);
void    ui_input(void);
void    ui_click(int btn, int hold);
void    ui_clear(void);

void    ui_intro(void);

#define ui_line(l, str...)              \
do                                      \
{                                       \
    char tmb[17];                       \
    memset(tmb, 0, sizeof(tmb));        \
    snprintf(tmb, sizeof(tmb), str);    \
    for(int i=strlen(tmb); i<16; i++)   \
        tmb[i] = ' ';                   \
    display.print(tmb, l, 0);           \
} while(0)

#define ui_printf(x...) Serial.printf(x)

extern OLED display;

// process buttons
#define LONG_PRESS_SHUTDOWN 2000
#define LONG_PRESS_BUTTON   600


// little macro so our code is cleaner
#define HANDLE_LONG_PRESS(name, cond, delay, code)              \
    if(cond)                                                    \
    {                                                           \
        if(!time_long_##name)                                   \
            time_long_##name = now;                             \
    }                                                           \
    else                                                        \
    {                                                           \
        if(time_long_##name && time_long_##name + delay < now)  \
        {                                                       \
            last_btn_btn1 = 0;                                  \
            last_btn_btn2 = 0;                                  \
            last_btn_both = 0;                                  \
            code;                                               \
        }                                                       \
        time_long_##name = 0;                                   \
    }

// little macro so our code is cleaner
#define HANDLE_CLICK(name, cond, code)                          \
    if(last_btn_##name && !(cond))                              \
    {                                                           \
        code;                                                   \
        last_btn_btn1 = 0;                                      \
        last_btn_btn2 = 0;                                      \
        last_btn_both = 0;                                      \
    }                                                           \
    if(cond)                                                    \
        last_btn_##name = 1;                                    \
    else                                                        \
        last_btn_##name = 0;


void draw_graph(int * vals, int count, int min, int max);

#endif

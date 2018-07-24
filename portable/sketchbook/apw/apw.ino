// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : apw.ino                                                    //
// Description : main program file                                          //
// ======================================================================== //

#include "status.h"
#include "ui.h"
#include "wifi.h"

// mass deauth

status_t status = {};

void setup(void)
{
    ui_init();
    wifi_init();
}

void loop(void)
{
    wifi_loop();
    ui_loop();

    yield();
}

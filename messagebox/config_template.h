// Duplicate this file into config.h and fill the blanks according to your needs.
#pragma once

#define HOSTNAME "########"

// How to connect to WiFi
#define WIFI_SSID "################"
#define WIFI_PASSWORD "################"

/* The imap host name e.g. imap.gmail.com for GMail or outlook.office365.com for Outlook */
#define IMAP_HOST "imap.gmail.com"
#define IMAP_PORT 993

/* Login. For gmail use an app password */
#define EMAIL "###@############"
#define IMAP_PASSWORD "#############"

// *********  Hardware  *********
// The documentation on the Lilygo T_Block is quite sparse, and most of what I know has been
// reverse engineered from reading the code.
//
// The one I use in this experience are T-Block with non touch e-ink screen. To configure the
// library one need to setup those macros before including the header.
#define LILYGO_WATCH_BLOCK     // To use T-Watch Block , please uncomment this line
#define LILYGO_EINK_GDEP015OC1 //Separate ink screen, no backlight, no touch

#include <LilyGoWatch.h>

// Duplicate this file into config.h and fill the blanks according to your needs.
#pragma once

#define HOSTNAME "########"

// The imap host name e.g. imap.gmail.com for GMail or outlook.office365.com for Outlook
#define IMAP_HOST "imap.gmail.com"
#define IMAP_PORT 993

// Login. For gmail use an app password
#define EMAIL "###@############"
#define IMAP_PASSWORD "#############"
#define IMAP_FOLDER "MessageBox"

// Network to connect to
#define WIFI_SSID "########"
#define WIFI_PASSWORD "#########"

// The hardware this code is targetting. Uncomment one of those
#define HARDWARE_LILYGO_T_BLOCK
// #define HARDWARE_LILYGO_T_H239
#include <MessageBoxHardware.h>

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define CONFIG_SECTOR_NUM   0x7E
#define CONFIG_ADDRESS      0x7E000

#define STATE_UNKNOWN       0u
#define STATE_START         1u
#define STATE_CONNECTED     2u
#define STATE_RECEIVING     3u
#define STATE_RECEIVED      4u
#define STATE_MAX           5U

// Enable debugging message
#define DEBUG               1

// Get status time
#define UPDATE_TIME_MS      500

#endif


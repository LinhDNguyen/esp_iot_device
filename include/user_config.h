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

// MQTT
/*DEFAULT CONFIGURATIONS*/

// #define MQTT_HOST           "192.168.11.122" //or "mqtt.yourdomain.com"
// #define MQTT_PORT           1880
// #define MQTT_KEEPALIVE      120  /*second*/

// #define MQTT_CLIENT_ID      "DVES_%08X"
// #define MQTT_USER           "DVES_USER"
// #define MQTT_PASS           "DVES_PASS"

// #define STA_SSID "DVES_HOME"
// #define STA_PASS "yourpassword"
// #define STA_TYPE AUTH_WPA2_PSK

// #define DEFAULT_SECURITY    0

#define MQTT_BUF_SIZE       1024
#define MQTT_RECONNECT_TIMEOUT  5   /*second*/
#define QUEUE_BUFFER_SIZE               2048

#define PROTOCOL_NAMEv31    /*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311         /*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif


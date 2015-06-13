#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "smartconfig.h"
#include "espconn.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);
void ICACHE_FLASH_ATTR network_init();

// Global VARs
static uint8 curState = STATE_UNKNOWN;
static volatile os_timer_t connectTimer;

// Functions
void user_rf_pre_init(void)
{
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg);
static void ICACHE_FLASH_ATTR networkDisconCb(void *arg);
static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len);
static void ICACHE_FLASH_ATTR networkSentCb(void *arg);
void ICACHE_FLASH_ATTR network_init();

LOCAL os_timer_t network_timer;

static void ICACHE_FLASH_ATTR networkSentCb(void *arg) {
    os_printf("\r\nSEND CB");
}

static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len) {
    struct espconn *conn=(struct espconn *)arg;
    os_printf("\r\nReceived: [%s]", data);
    curState = STATE_RECEIVED;
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg) {
    struct espconn *conn=(struct espconn *)arg;

    char *data = "GET /test HTTP/1.0\r\n\r\n\r\n";
    sint8 d = espconn_sent(conn,data,strlen(data));

    espconn_regist_recvcb(conn, networkRecvCb);
    os_printf("\r\nSENT: [%s]", data);
    curState = STATE_RECEIVING;
}

static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err) {
    os_printf("Reconnect\n\r");
    // Change state to make new connection
    curState = STATE_CONNECTED;
}

static void ICACHE_FLASH_ATTR networkDisconCb(void *arg) {
    os_printf("Disconnect\n\r");
    // Change state to make new connection
    curState = STATE_CONNECTED;
}


void ICACHE_FLASH_ATTR network_start() {
    static struct espconn conn;
    static ip_addr_t ip;
    static esp_tcp tcp;
    char page_buffer[20];
    os_printf("\r\nnetwork_start:");

    conn.type=ESPCONN_TCP;
    conn.state=ESPCONN_NONE;
    conn.proto.tcp=&tcp;
    conn.proto.tcp->local_port=espconn_port();
    conn.proto.tcp->remote_port=8000;
    conn.proto.tcp->remote_ip[0] = 192;
    conn.proto.tcp->remote_ip[1] = 168;
    conn.proto.tcp->remote_ip[2] = 1;
    conn.proto.tcp->remote_ip[3] = 120;
    espconn_regist_connectcb(&conn, networkConnectedCb);
    espconn_regist_disconcb(&conn, networkDisconCb);
    espconn_regist_reconcb(&conn, networkReconCb);
    espconn_regist_recvcb(&conn, networkRecvCb);
    espconn_regist_sentcb(&conn, networkSentCb);
    espconn_connect(&conn);
}

void ICACHE_FLASH_ATTR network_check_ip(void) {
    struct ip_info ipconfig;
    os_timer_disarm(&network_timer);
    wifi_get_ip_info(STATION_IF, &ipconfig);
    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    char page_buffer[20];
    os_sprintf(page_buffer,"IP: %d.%d.%d.%d",IP2STR(&ipconfig.ip));
    os_printf("\r\n%s", page_buffer);
    network_start();
    } else {
    os_printf("No ip found\n\r");
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
    os_timer_arm(&network_timer, 1000, 0);
    }
}

void ICACHE_FLASH_ATTR network_init() {
    os_timer_disarm(&network_timer);
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
    os_timer_arm(&network_timer, 5000, 0);
}

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    uint8 phone_ip[4] = {0};
    struct station_config *sta_conf = pdata;

    switch(status) {
        case SC_STATUS_WAIT:
            os_printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            os_printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
            break;
        case SC_STATUS_LINK:
            os_printf("SC_STATUS_LINK\n");

            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            os_printf("SC_STATUS_LINK_OVER\n");
            os_memcpy(phone_ip, (uint8*)pdata, 4);
            os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            smartconfig_stop();
            wifi_station_set_auto_connect(1);
            break;
    }

}

// Check the wifi connectivity.
void timer_check_connection(void *arg)
{
    smartconfig_start(SC_TYPE_ESPTOUCH, smartconfig_done);
}

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    if (curState == STATE_CONNECTED)
    {
        curState = STATE_RECEIVING;
        // Update Server
        network_init();
    }
    os_delay_us(1000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

void ICACHE_FLASH_ATTR wifi_event_cb(System_Event_t *evt)
{
    uint8 newchannel = 0;
    switch (evt->event) {
        case EVENT_STAMODE_CONNECTED: //When connected to AP, change SoftAP to same channel to keep DHCPS working
        newchannel = evt->event_info.connected.channel;
        os_printf("\r\nWifi connected at channel %d\r\n", newchannel);
        curState = STATE_START;
        // Disable timer when connected
        os_timer_disarm(&connectTimer);
        break;

        case EVENT_STAMODE_GOT_IP:
        os_printf("\r\nWifi got IP...\r\n");
        curState = STATE_CONNECTED;
        // Disable timer when connected
        os_timer_disarm(&connectTimer);
        break;

        case EVENT_STAMODE_DISCONNECTED:
        curState = STATE_START;
        os_printf("\r\nWifi disconnected\r\n");
        break;
    }

}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
    char ssid[32] = {0};
    char password[64] = {0};
    struct station_config stationConf;

    // Initialize UART0 to use as debug
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    os_printf("\n\nSDK version:%s\n", system_get_sdk_version());

    // Use GPIO4 led to indicate wifi status
    wifi_status_led_install(4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);

    wifi_station_set_auto_connect(1);

    //Set station mode
    wifi_set_opmode(STATION_MODE);

    // Check Flash saved configuration
    wifi_station_get_config_default(&stationConf);

    //Set ap settings
    wifi_station_set_config_current(&stationConf);
    wifi_set_event_handler_cb(wifi_event_cb);
    wifi_station_set_reconnect_policy(1);

    // Configure the timer to check connection timeout.
    // After 5s, if wifi is not connected, start the smartconfig
    os_timer_disarm(&connectTimer);
    os_timer_setfn(&connectTimer, (os_timer_func_t *)timer_check_connection, NULL);
    os_timer_arm(&connectTimer, 10000, 0);

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );
}

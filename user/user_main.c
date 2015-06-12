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

// Global VARs
static uint8 curState = STATE_UNKNOWN;
static volatile os_timer_t curTimer;

// Functions
void user_rf_pre_init(void)
{
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
    os_delay_us(10000);
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
        os_timer_disarm(&curTimer);
        break;

        case EVENT_STAMODE_GOT_IP:
        os_printf("\r\nWifi got IP...\r\n");
        curState = STATE_NETCONNECTED;
        // Disable timer when connected
        os_timer_disarm(&curTimer);
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
    os_timer_disarm(&curTimer);
    os_timer_setfn(&curTimer, (os_timer_func_t *)timer_check_connection, NULL);
    os_timer_arm(&curTimer, 10000, 0);

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );
}

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

void user_rf_pre_init(void)
{
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
        break;

        case EVENT_STAMODE_GOT_IP:
        os_printf("\r\nWifi got IP...\r\n");
        break;
    }

}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
    char ssid[32] = WIFI_SSID;
    char password[64] = WIFI_PASSWORD;
    struct station_config stationConf;

    // Initialize UART0 to use as debug
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    os_printf("\n\nSDK version:%s\n", system_get_sdk_version());

    // Use GPIO4 led to indicate wifi status
    wifi_status_led_install(4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);

    //Set station mode
    wifi_set_opmode( 0x1 );

    //Set ap settings
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);
    wifi_set_event_handler_cb(wifi_event_cb);

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );
}

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
static volatile os_timer_t requestTimer;
static esp_tcp global_tcp;                                  // TCP connect var (see espconn.h)
static struct espconn global_tcp_connect;                   // Connection struct (see espconn.h)

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

static void ICACHE_FLASH_ATTR ThingSpeakThingSpeaktcpNetworkRecvCb(void *arg, char *data, unsigned short len)
{

   os_printf("\r\nTS: RECV - [%s]",data);
   curState = STATE_RECEIVED;
}

static void ICACHE_FLASH_ATTR ThingSpeaktcpNetworkConnectedCb(void *arg)
{
    struct espconn *tcpconn=(struct espconn *)arg;
    char data[120];

    curState = STATE_RECEIVED;

    espconn_regist_recvcb(tcpconn, ThingSpeakThingSpeaktcpNetworkRecvCb);

    os_printf("\r\nTS: TCP connected");

    os_sprintf(data, "GET /test HTTP/1.1\r\nHost: 192.168.1.120:5000\r\nUser-agent: the best\r\nAccept: */*\r\n\r\n");
    os_printf ("TS: Sending - [%s]",data);
    espconn_sent(&global_tcp_connect, data, strlen(data));
    // espconn_recv_hold(tcpconn);
}

static void ICACHE_FLASH_ATTR ThingSpeaktcpNetworkReconCb(void *arg, sint8 err)
{
  os_printf("\r\nTS: TCP reconnect - %d", err);
  // curState = STATE_CONNECTED;
  network_init();
}


static void ICACHE_FLASH_ATTR ThingSpeaktcpNetworkDisconCb(void *arg)
{
  os_printf("\r\nTS: TCP disconnect");
  curState = STATE_CONNECTED;
}
static void ICACHE_FLASH_ATTR init_tcp_conn(void)
{
    os_printf("\r\n===init_tcp_conn()");
    global_tcp_connect.type = ESPCONN_TCP;                                  // We want to make a TCP connection
    global_tcp_connect.state = ESPCONN_NONE;                                // Set default state to none
    global_tcp_connect.proto.tcp = &global_tcp;                             // Give a pointer to our TCP var
    global_tcp_connect.proto.tcp->local_port = espconn_port();              // Ask a free local port to the API

    // google.com.vn 216.58.221.99
    global_tcp_connect.proto.tcp->remote_port = 8000;                       // Set remote port (bcbcostam)
    global_tcp_connect.proto.tcp->remote_ip[0] = 192;                       // Your computer IP
    global_tcp_connect.proto.tcp->remote_ip[1] = 168;                       // Your computer IP
    global_tcp_connect.proto.tcp->remote_ip[2] = 1;                         // Your computer IP
    global_tcp_connect.proto.tcp->remote_ip[3] = 120;                       // Your computer IP

    espconn_regist_connectcb(&global_tcp_connect, ThingSpeaktcpNetworkConnectedCb); // Register connect callback
    espconn_regist_disconcb(&global_tcp_connect, ThingSpeaktcpNetworkDisconCb);     // Register disconnect callback
    espconn_regist_reconcb(&global_tcp_connect, ThingSpeaktcpNetworkReconCb);       // Register reconnection function
    espconn_connect(&global_tcp_connect);                                           // Start connection
}

void ICACHE_FLASH_ATTR network_start(void)
{
    os_printf("\r\n===network_start()");
    init_tcp_conn();            // Init tcp connection
}

void ICACHE_FLASH_ATTR network_check_ip(void)
{
    struct ip_info ipconfig;

    os_printf("\r\n===network_check_ip()");

    os_timer_disarm(&requestTimer);                // Disarm timer
    wifi_get_ip_info(STATION_IF, &ipconfig);        // Get Wifi info

    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0)
    {
        network_start();                            // Everything in order
    }
    else
    {
        os_printf("Waiting for IP...\n\r");
        os_timer_setfn(&requestTimer, (os_timer_func_t *)network_check_ip, NULL);
        os_timer_arm(&requestTimer, 10000, 0);
    }
}

// network init function
void ICACHE_FLASH_ATTR network_init()
{
    os_printf("\r\n===network_init()");
    os_timer_disarm(&requestTimer);
    os_timer_setfn(&requestTimer, (os_timer_func_t *)network_check_ip, NULL);
    os_timer_arm(&requestTimer, 5000, 0);
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
    else if (curState == STATE_RECEIVED)
    {
        os_printf("\r\n\r\n===============RECEIVED!!!===============");
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

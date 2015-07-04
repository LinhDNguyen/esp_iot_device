#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"

#include "config_cmd.h"

static struct espconn serverConn;
static esp_tcp serverTcp;

//Connection pool
serverConnData connData;
static char txbuffer[MAX_TXBUFFER];

//send all data in conn->txbuffer
//returns result from espconn_sent if data in buffer or ESPCONN_OK (0)
//only internal used by espbuffsent, serverSentCb
static sint8  ICACHE_FLASH_ATTR sendtxbuffer(serverConnData *conn) {
	sint8 result = ESPCONN_OK;
	if (connData.txbufferlen != 0)	{
		connData.readytosend = false;
		result= espconn_sent(connData.conn, (uint8_t*)connData.txbuffer, connData.txbufferlen);
		connData.txbufferlen = 0;	
		if (result != ESPCONN_OK)
			os_printf("sendtxbuffer: espconn_sent error %d on conn %p\n", result, conn);
	}
	return result;
}

//send formatted output to transmit buffer and call sendtxbuffer, if ready (previous espconn_sent is completed)
sint8 ICACHE_FLASH_ATTR  espbuffsentprintf(serverConnData *conn, const char *format, ...) {
	uint16 len;
	va_list al;
	va_start(al, format);
	len = ets_vsnprintf(connData.txbuffer + connData.txbufferlen, MAX_TXBUFFER - connData.txbufferlen - 1, format, al);
	va_end(al);
	if (len <0)  {
		os_printf("espbuffsentprintf: txbuffer full on conn %p\n", conn);
		return len;
	}
	connData.txbufferlen += len;
	if (connData.readytosend)
		return sendtxbuffer(&connData);
	return ESPCONN_OK;
}


//send string through espbuffsent
sint8 ICACHE_FLASH_ATTR espbuffsentstring(serverConnData *conn, const char *data) {
	return espbuffsent(&connData, data, strlen(data));
}

//use espbuffsent instead of espconn_sent
//It solve problem: the next espconn_sent must after espconn_sent_callback of the pre-packet.
//Add data to the send buffer and send if previous send was completed it call sendtxbuffer and  espconn_sent
//Returns ESPCONN_OK (0) for success, -128 if buffer is full or error from  espconn_sent
sint8 ICACHE_FLASH_ATTR espbuffsent(serverConnData *conn, const char *data, uint16 len) {
	if (connData.txbufferlen + len > MAX_TXBUFFER) {
		os_printf("\r\nespbuffsent: txbuffer full txbufferlen=%d, len=%d, max=%d", connData.txbufferlen, len, MAX_TXBUFFER);
		return -128;
	}
	os_memcpy(connData.txbuffer + connData.txbufferlen, data, len);
	connData.txbufferlen += len;
	if (connData.readytosend) 
		return sendtxbuffer(&connData);
	return ESPCONN_OK;
}

//callback after the data are sent
static void ICACHE_FLASH_ATTR serverSentCb(void *arg) {
	connData.readytosend = true;
	sendtxbuffer(&connData); // send possible new data in txbuffer
}

static void ICACHE_FLASH_ATTR serverRecvCb(void *arg, char *data, unsigned short len) {
	int i;
	char *p, *e;

	if (len >= 5 && data[0] == '+' && data[1] == '+' && data[2] == '+' && data[3] =='A' && data[4] == 'T') {
		config_parse(&connData, data, len);
	}
}

static void ICACHE_FLASH_ATTR serverReconCb(void *arg, sint8 err) {
	serverConnData *conn = arg;
	if (conn==NULL) return;
	//Yeah... No idea what to do here. ToDo: figure something out.
}

static void ICACHE_FLASH_ATTR serverDisconCb(void *arg) {
	if (connData.conn!=NULL) {
		if (connData.conn->state==ESPCONN_NONE || connData.conn->state==ESPCONN_CLOSE) {
			connData.conn=NULL;
		}
	}
}

static void ICACHE_FLASH_ATTR serverConnectCb(void *arg) {
	struct espconn *conn = arg;

	connData.conn=conn;
	connData.txbufferlen = 0;
	connData.readytosend = true;
	os_printf("\r\nClient connected!!! txbufferlen=%d", connData.txbufferlen);

	espconn_regist_recvcb(conn, serverRecvCb);
	espconn_regist_reconcb(conn, serverReconCb);
	espconn_regist_disconcb(conn, serverDisconCb);
	espconn_regist_sentcb(conn, serverSentCb);
}

void ICACHE_FLASH_ATTR serverInit(int port) {
	connData.conn = NULL;
	connData.txbuffer = txbuffer;
	connData.txbufferlen = 0;
	connData.readytosend = true;
	
	serverConn.type=ESPCONN_TCP;
	serverConn.state=ESPCONN_NONE;
	serverTcp.local_port=port;
	serverConn.proto.tcp=&serverTcp;

	espconn_regist_connectcb(&serverConn, serverConnectCb);
	espconn_accept(&serverConn);
	espconn_regist_time(&serverConn, SERVER_TIMEOUT, 0);
}

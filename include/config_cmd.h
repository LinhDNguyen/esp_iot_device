#ifndef __CONFIG_CMD_H__
#define __CONFIG_CMD_H__

#include <ip_addr.h>
#include <c_types.h>
#include <espconn.h>

#define SERVER_TIMEOUT 28799

//Max send buffer len
#define MAX_TXBUFFER 1024
typedef struct serverConnData serverConnData;

struct serverConnData {
        struct espconn *conn;
		char *txbuffer; //the buffer for the data to send
		uint16  txbufferlen; //the length  of data in txbuffer
 		bool readytosend; //true, if txbuffer can send by espconn_sent
};

typedef struct config_commands {
	char *command;
	void(*function)(serverConnData *conn, uint8_t argc, char *argv[]);
} config_commands_t;


void config_parse(serverConnData *conn, char *buf, int len);
void ICACHE_FLASH_ATTR serverInit(int port);
sint8  ICACHE_FLASH_ATTR espbuffsent(serverConnData *conn, const char *data, uint16 len);
sint8  ICACHE_FLASH_ATTR espbuffsentstring(serverConnData *conn, const char *data);
sint8  ICACHE_FLASH_ATTR espbuffsentprintf(serverConnData *conn, const char *format, ...);

#endif /* __CONFIG_CMD_H__ */

#ifndef __COMMON_H_
#define __COMMON_H_

#include <hsf.h>
#include <stdlib.h>
#include <string.h>

#include <cyassl/openssl/ssl.h>
#include <cyassl/internal.h>
#include <cyassl/cyassl_config.h>

#include "cJSON.h"


#define SSLDebug_ON		0 		// 是否打开 SSL Debug
#define CLOUD_SSL	    1			// 是否需要SSL连接


void httpdecode(char *p, int isFind);
int str2int(char* pStr);
int addressis_ip(const char * ipaddr);

int sys_argsinit(void);
int get_ip_port(char *ip_port);


typedef struct
{
	int 			fd;
	CYASSL_CTX*     ctx;
	CYASSL*         ssl;
}ssl_t;

void ssl_init(void);
int ssl_connect(int fd, ssl_t* pssl);
int ssl_check(ssl_t* pssl);
int ssl_close(ssl_t* pssl);

void jdudp_thread(void* arg);
void jdssl_thread(void* arg);
void jdnetwdg_thread(void* arg);
void jdupload_process(void);//处理京东数据上传
void heart_tick_report(void);
void set_network_state(int state, int on);
void JD_Dev_Reset(void);


int jdsmart_start(void);
int jdssl_send(char *send_data);
int device_status_report(void);
int get_rssi_value(void);
int Httpc_get(char *purl, int *dl_status);

char *my_itoa(int num,char *str,int radix);


#endif

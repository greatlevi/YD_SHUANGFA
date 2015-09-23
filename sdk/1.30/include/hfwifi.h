
/* hfwifi.h
 *
 * Copyright (C) 2013-2014 ShangHai High-flying Electronics Technology Co.,Ltd.
 *
 * This file is part of HSF.
 * 
 * Modify:
 * 2013-12-31 : Create by Jim
 */

 #ifndef __HF_WIFI_H_H__
 #define __HF_WIFI_H_H__

#define AUTH_TYPE_OPEN				0
#define AUTH_TYPE_SHARED			1
#define AUTH_TYPE_WPAPSK			2
#define AUTH_TYPE_WPA2PSK			3
#define AUTH_TYPE_WPAPSKWPA2PSK	4

#define ENC_TYPE_NONE					0
#define ENC_TYPE_WEP					1
#define ENC_TYPE_TKIP					2
#define ENC_TYPE_AES					3
#define ENC_TYPE_TKIPAES				4


#define VENDOR_OUI_ALIBABA                     { 0xD8, 0x96, 0xE0 }


//Information Elements from Alibaba router.
typedef struct _IE_ALIBABA_T {
    uint8_t     id;
    uint8_t     length;
    uint8_t     oui[3];
    uint8_t     type;
    uint8_t     version;
    uint8_t     challenge[16];
    uint8_t     reserve;	
}  __attribute__((__packed__)) IE_ALIBABA_T, *P_IE_ALIBABA_T;

typedef struct _WIFI_SCAN_RESULT_ITEM
 {
	uint8_t auth;
	uint8_t encry;
	uint8_t channel;
	uint8_t rssi;
	char    ssid[32+1];
	uint8_t mac[6];
	int       rssi_dbm;
	int       sco;
	IE_ALIBABA_T  alibaba_ie;
 }WIFI_SCAN_RESULT_ITEM,*PWIFI_SCAN_RESULT_ITEM;

typedef int (*hfwifi_scan_callback_t)( PWIFI_SCAN_RESULT_ITEM );


int HSF_API hfwifi_scan(hfwifi_scan_callback_t p_callback);
int HSF_API hfwifi_enable_ap_idle_auto_reset(int max_idle_time);


 #endif



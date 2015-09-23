/**
******************************************************************************
* @file     zc_bc.h
* @authors  cxy
* @version  V1.0.0
* @date     10-Sep-2014
* @brief   broadcast
******************************************************************************
*/

#ifndef  __ZC_JD_H__ 
#define  __ZC_JD_H__
#include <zc_common.h>
#include <zc_protocol_controller.h>
#include <cJSON.h>

//#define JD_PROUUID "EGYHST"       //Customer UUID, XiWu   
//#define JDCONFIG_OFFSET 512          // JD Config Offset


typedef struct jdsmart_down_cmd {
	u8  *stream_id;
	u32 PN;         //Process    No
	u32 SN;         //Sequence No
	u8  *value;
    u8  *attribute;
} jdsmart_down_cmd, *jdsmart_down_cmd_ptr;


#pragma pack(1)
typedef struct {
	u32 magic;
	u32 len;
	u32 enctype;
	u8 checksum;
} common_header_t;

typedef struct {
	u32 type;
	u8 cmd[2];
} cmd_header;

typedef struct 
{
	u8 magic[6];
	u8 feedid[64];
	u8 accesskey[64];
	u8 iplist[2][30];		// 服务器列表
	u16_t fw_version;
}jdNVargs_t;
#pragma pack()

//extern int g_Jdfd;


#ifdef __cplusplus
extern "C" {
#endif
void ZC_SendJDQueryReq(u8 *pu8Msg, u32 u32RecvLen,struct sockaddr_in addr);    
#ifdef __cplusplus
}
#endif

#endif
/******************************* FILE END ***********************************/


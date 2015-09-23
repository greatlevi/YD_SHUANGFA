#ifndef __JDSMART_H_
#define __JDSMART_H_

#include "cJSON.h"
#include "zc_jd.h"

#define JD_Version	"Current Version:1.0(2015/01/26)\r\n"
#define version_log "\r\nVersion 1.0 data:2015/01/26 notes:original version.\r\n"

//#define JD_PROUUID "5IUP6T"       //Customer UUID, XiWu   
#define HFGPIO_F_USER_LINK			(HFGPIO_F_USER_DEFINE+1)


#define DOWNLOAD_OK 7001
#define DOWNLOAD_FAIL 7002
#define UPGRADE_OK 7003
#define UPGRADE_FAIL 7004

#define HTTPS_IP "111.206.227.243"   //HTTPS Server
#define HOST_NAME "apismart.jd.com"
#define POST_PORT	 443
#define JDCONFIG_OFFSET 512           // 京东配置文件偏移地址

#define SHORT_FILE strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__
#define custom_log(M, ...) do {\
                                  u_printf("[%5d][%-7.7s:%4d] " M "\n", hfsys_get_time(), SHORT_FILE, __LINE__, ##__VA_ARGS__);\
                                 }while(0==1)


#define HF_STA_CONNECTED      0
#define HF_STA_DISCONNECTED   1
#define HF_DHCP_OK            2
#define HF_CLO_CONNECTED      3
#define HF_CLO_DISCONNECTED   4
#define HF_CLO_OTA            5
#define HF_NET_CONNECTED      6
#define HF_NET_DISCONNECTED   7


#define MSG_HEAD                      0xAA
#define MSG_TAIL                      0x55
#define MSG_COMMON_LEN               3
#define ECC_OFFSET                    2
#define MSG_WITH_DATA_LEN            7
#define MSG_WHTHOUT_DATA_LEN         6

typedef enum
{
    CMD_COMMON = 0x00,
    CMD_DOWN_POWER = 0x01,
    CMD_DOWN_SPEED,
    CMD_DOWN_SOUND,
    CMD_DOWN_LIGHT,
    CMD_DOWN_MODE,
    CMD_DOWN_PM25,
    CMD_UP_POWER = 0x81,
    CMD_UP_SPEED,
    CMD_UP_SOUND,
    CMD_UP_LIGHT,
    CMD_UP_MODE,
    CMD_UP_PM25 
}CONTROL_CMD_TYPE;

typedef struct
{
    CONTROL_CMD_TYPE type;
    unsigned char record[20];
}JD_JASON_ATTRIBUTE;


typedef enum
{
    YD_HEAD,
    YD_CODE,
    YD_DATA_LEN  
}YD_MSG;


#if 0
typedef struct jdsmart_down_cmd {
	uint8_t  *stream_id;
	uint32_t PN;         //Process    No 第几个
	uint32_t SN;         //Sequence No   总共几个 
	uint8_t  *value;
    uint8_t  *attribute;
} jdsmart_down_cmd, *jdsmart_down_cmd_ptr;
#endif

typedef int (*jdsmart_callback)(jdsmart_down_cmd_ptr);

#if 0
#pragma pack(1)
typedef struct {
	unsigned int magic;
	unsigned int len;
	unsigned int enctype;
	unsigned char checksum;
} common_header_t;

typedef struct {
	unsigned int type;
	unsigned char cmd[2];
} cmd_header;

typedef struct 
{
	char magic[6];
	char feedid[64];
	char accesskey[64];
	char iplist[2][30];		// 服务器列表
	u16_t fw_version;
}jdNVargs_t;
#pragma pack()
#endif

extern jdNVargs_t jdArgs;   //京东参数
extern hfthread_sem_t uart_psem;

#define JD_CALLBACK_NUM 2

enum JDSMART_CALLBACK {
    JD_GET_DEVICE_STATUS = 0,
    JD_SET_DEVICE_STATUS = 1
};

struct device_info {
	uint8_t  mac[18];
	uint8_t  jdserver_ip[20];
	uint32_t jdserver_port;
	uint8_t  linkst_router:1;
	uint8_t  linkst_internet:1;
	uint8_t  linkst_cloud:1;
	uint8_t  linkst_ota:1;
	uint8_t  linkst_reserved:4;
    jdsmart_callback dev_callback[JD_CALLBACK_NUM];
	
	uint8_t ssl_tx[400];   // 发送SSL数据的缓冲区;
	uint8_t ssl_rx[600];   // 接收SSL数据的缓冲区;
	uint8_t post_tx[700];  //发送upload数据缓冲区；
};

#define JD_FLAG_ENABLE_NETWDG	(0x00000001)

typedef struct 
{
	uint8_t flags;
	uint32_t wdg_timeout;
	uint32_t wdg_time;
}NetWdg_t;

typedef struct
{
	char air_quality;
	char power;
	char mode;
	char wind_speed;
	char timer;
	char filter_change1;
	char state;
	char wind_state;
	char timer_state;
} uartcmd_t;


//extern NetWdg_t NetWdg;
extern struct device_info Jd_GlobalVar;

extern void HF_Sleep(void);
extern void HF_BcInit(void);
extern void HF_WakeUp(void);
extern void jdupload_process(void);

#endif

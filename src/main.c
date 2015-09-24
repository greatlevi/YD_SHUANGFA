/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *-pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return  -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
#include <hsf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ac_hal.h>
#include <zc_hf_adpter.h>
#include <zc_common.h>
#include "common.h"
#include <cyassl/openssl/ssl.h>
#include <cyassl/internal.h>
#include <cyassl/cyassl_config.h>

#include "./jdsmart/jdsmart.h"

extern struct device_info Jd_GlobalVar;
#if 0

#else
const char format_upload[]=
"{\"code\":103,\"device\":{\"feed_id\":\"%s\",\"access_key\":\"%s\"},\
\"streams\":[{\"stream_id\":\"%s\",\"datapoints\":[{\"value\":%d}]}]}\n";
//{\"stream_id\":\"lefttime\",\"datapoints\":[{\"value\":%d}]}]}\n";
#endif
const char format_common[]=
"{\"code\":103,\"device\":{\"feed_id\":\"%s\",\"access_key\":\"%s\"},\
\"streams\":[{\"stream_id\":\"power\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"speed\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"PM25\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"sound\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"light\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"mode\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"lefttime\",\"datapoints\":[{\"value\":%d}]}]}\n";


//char post_tx[700];

//extern JD_JASON_ATTRIBUTE g_struJdAttribute[6];

static int jd_send_data(unsigned char *data, unsigned char len);
extern int hf_start_jdsmart(void);


#ifdef __LPT100__
static int module_type= HFM_TYPE_LPT100;
const int hf_gpio_fid_to_pid_map_table[HFM_MAX_FUNC_CODE]=
{
	HF_M_PIN(2),	//HFGPIO_F_JTAG_TCK
	HF_M_PIN(3),	//HFGPIO_F_JTAG_TDO
	HF_M_PIN(4),	//HFGPIO_F_JTAG_TDI
	HF_M_PIN(5),	//HFGPIO_F_JTAG_TMS
	HFM_NOPIN,		//HFGPIO_F_USBDP
	HFM_NOPIN,		//HFGPIO_F_USBDM
	HF_M_PIN(39),	//HFGPIO_F_UART0_TX
	HF_M_PIN(40),	//HFGPIO_F_UART0_RTS
	HF_M_PIN(41),	//HFGPIO_F_UART0_RX
	HF_M_PIN(42),	//HFGPIO_F_UART0_CTS
	
	HF_M_PIN(27),	//HFGPIO_F_SPI_MISO
	HF_M_PIN(28),	//HFGPIO_F_SPI_CLK
	HF_M_PIN(29),	//HFGPIO_F_SPI_CS
	HF_M_PIN(30),	//HFGPIO_F_SPI_MOSI
	
	HFM_NOPIN,	//HFGPIO_F_UART1_TX,
	HFM_NOPIN,	//HFGPIO_F_UART1_RTS,
	HFM_NOPIN,	//HFGPIO_F_UART1_RX,
	HFM_NOPIN,	//HFGPIO_F_UART1_CTS,
	
	HF_M_PIN(11),	//HFGPIO_F_NLINK
	HF_M_PIN(12),	//HFGPIO_F_NREADY
	HF_M_PIN(45),	//HFGPIO_F_NRELOAD
	HFM_NOPIN,	//HFGPIO_F_SLEEP_RQ
	HFM_NOPIN,	//HFGPIO_F_SLEEP_ON
	
	HF_M_PIN(18),	//HFGPIO_F_WPS
	HFM_NOPIN,		//HFGPIO_F_RESERVE1
	HFM_NOPIN,		//HFGPIO_F_RESERVE2
	HFM_NOPIN,		//HFGPIO_F_RESERVE3
	HFM_NOPIN,		//HFGPIO_F_RESERVE4
	HFM_NOPIN,		//HFGPIO_F_RESERVE5
	
	HFM_NOPIN,	//HFGPIO_F_USER_DEFINE
};
#elif defined(__LPT200__)
static int module_type= HFM_TYPE_LPT200;
const int hf_gpio_fid_to_pid_map_table[HFM_MAX_FUNC_CODE]=
{
	HF_M_PIN(2),	//HFGPIO_F_JTAG_TCK
	HF_M_PIN(3),	//HFGPIO_F_JTAG_TDO
	HF_M_PIN(4),	//HFGPIO_F_JTAG_TDI
	HF_M_PIN(5),	//HFGPIO_F_JTAG_TMS
	HFM_NOPIN,		//HFGPIO_F_USBDP
	HFM_NOPIN,		//HFGPIO_F_USBDM
	HF_M_PIN(39),	//HFGPIO_F_UART0_TX
	HF_M_PIN(40),	//HFGPIO_F_UART0_RTS
	HF_M_PIN(41),	//HFGPIO_F_UART0_RX
	HF_M_PIN(42),	//HFGPIO_F_UART0_CTS
	
	HF_M_PIN(27),	//HFGPIO_F_SPI_MISO
	HF_M_PIN(28),	//HFGPIO_F_SPI_CLK
	HF_M_PIN(29),	//HFGPIO_F_SPI_CS
	HF_M_PIN(30),	//HFGPIO_F_SPI_MOSI
	
	HFM_NOPIN,	//HFGPIO_F_UART1_TX,
	HFM_NOPIN,	//HFGPIO_F_UART1_RTS,
	HFM_NOPIN,	//HFGPIO_F_UART1_RX,
	HFM_NOPIN,	//HFGPIO_F_UART1_CTS,
	
	HF_M_PIN(43),	//HFGPIO_F_NLINK
	HF_M_PIN(44),	//HFGPIO_F_NREADY
	HF_M_PIN(45),	//HFGPIO_F_NRELOAD
	HFM_NOPIN,	//HFGPIO_F_SLEEP_RQ
	HFM_NOPIN,	//HFGPIO_F_SLEEP_ON
		
	HF_M_PIN(7),		//HFGPIO_F_WPS
	HFM_NOPIN,		//HFGPIO_F_RESERVE1
	HFM_NOPIN,		//HFGPIO_F_RESERVE2
	HFM_NOPIN,		//HFGPIO_F_RESERVE3
	HFM_NOPIN,		//HFGPIO_F_RESERVE4
	HFM_NOPIN,		//HFGPIO_F_RESERVE5
	
	HFM_NOPIN,	//HFGPIO_F_USER_DEFINE
};
#elif defined(__LPB100__)
static int module_type= HFM_TYPE_LPB100;
const int hf_gpio_fid_to_pid_map_table[HFM_MAX_FUNC_CODE]=
{
	HF_M_PIN(2),	//HFGPIO_F_JTAG_TCK
	HFM_NOPIN,	//HFGPIO_F_JTAG_TDO
	HFM_NOPIN,	//HFGPIO_F_JTAG_TDI
	HF_M_PIN(5),	//HFGPIO_F_JTAG_TMS
	HFM_NOPIN,		//HFGPIO_F_USBDP
	HFM_NOPIN,		//HFGPIO_F_USBDM
	HF_M_PIN(39),	//HFGPIO_F_UART0_TX
	HF_M_PIN(40),	//HFGPIO_F_UART0_RTS
	HF_M_PIN(41),	//HFGPIO_F_UART0_RX
	HF_M_PIN(42),	//HFGPIO_F_UART0_CTS
	
	HF_M_PIN(27),	//HFGPIO_F_SPI_MISO
	HF_M_PIN(28),	//HFGPIO_F_SPI_CLK
	HF_M_PIN(29),	//HFGPIO_F_SPI_CS
	HF_M_PIN(30),	//HFGPIO_F_SPI_MOSI
	
	HFM_NOPIN,	//HFGPIO_F_UART1_TX,
	HFM_NOPIN,	//HFGPIO_F_UART1_RTS,
	HFM_NOPIN,	//HFGPIO_F_UART1_RX,
	HFM_NOPIN,	//HFGPIO_F_UART1_CTS,
	
	HF_M_PIN(43),	//HFGPIO_F_NLINK
	HF_M_PIN(44),	//HFGPIO_F_NREADY
	HF_M_PIN(45),	//HFGPIO_F_NRELOAD
	HF_M_PIN(7),	//HFGPIO_F_SLEEP_RQ
	HF_M_PIN(8),	//HFGPIO_F_SLEEP_ON
		
	HF_M_PIN(15),		//HFGPIO_F_WPS
	HFM_NOPIN,		//HFGPIO_F_RESERVE1
	HFM_NOPIN,		//HFGPIO_F_RESERVE2
	HFM_NOPIN,		//HFGPIO_F_RESERVE3
	HFM_NOPIN,		//HFGPIO_F_RESERVE4
	HFM_NOPIN,		//HFGPIO_F_RESERVE5
	
	HFM_NOPIN,	//HFGPIO_F_USER_DEFINE
};
#else
#error "invalid project !you must define module type(__LPB100__,__LPT100__,_LPT200__)"
#endif

#if 0
int	jdcloud_sdk_version(pat_session_t s,int argc, char** argv, char *rsp,int len)
{
	if(0 != argc)
		return -4;
	hfuart_send(HFUART0, JD_Version, strlen(JD_Version), 50);
	hfuart_send(HFUART0, version_log, strlen(version_log), 50);

	return 0;
}

#endif
#define YADU_LVER   "zhao_fan 9(2015/7/30)"
static int yadu_lver(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	sprintf(rsp, "=%s", YADU_LVER);
	return 0;

}
const hfat_cmd_t user_define_at_cmds_table[]=
{
	{"LVER", yadu_lver, "   AT+LVER=YADULVER\r\n", NULL},
    {NULL,NULL,NULL,NULL} //the last item must be null
};


extern u32 g_u32GloablIp;

extern u16 SUB_DOMAIN_ID;

void Smt_Timeout(void * arg)
{
    msleep(90000);
    hfsmtlk_stop();
    return;
}

static void show_reset_reason(void)
{
    uint32_t reset_reason=0;
    int smtlk = 0;
    hfsys_nvm_write(0, (char *)&smtlk,4);	
    reset_reason = hfsys_get_reset_reason();
    
    
#if 0
    u_printf("reset_reasion:%08x\n",reset_reason);
#else   
    if(reset_reason&HFSYS_RESET_REASON_ERESET)
    {
        u_printf("ERESET\n");
    }
    if(reset_reason&HFSYS_RESET_REASON_IRESET0)
    {
        u_printf("IRESET0\n");
    }
    if(reset_reason&HFSYS_RESET_REASON_IRESET1)
    {
        u_printf("IRESET1\n");
    }
    if(reset_reason==HFSYS_RESET_REASON_NORMAL)
    {
        u_printf("RESET NORMAL\n");
    }
    if(reset_reason&HFSYS_RESET_REASON_WPS)
    {
        u_printf("RESET FOR WPS\n");
    }
    if(reset_reason&HFSYS_RESET_REASON_SMARTLINK_START)
    {
        u_printf("RESET FOR SMARTLINK START\n");
        hfthread_create(Smt_Timeout,"SmtTimeOut",256,NULL,1,NULL,NULL);
    }
    if(reset_reason&HFSYS_RESET_REASON_SMARTLINK_OK)		
    {
        HF_ReadDataFormFlash();
       	g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig = 0;			
        HF_WriteDataToFlash((u8 *)&g_struZcConfigDb, sizeof(ZC_ConfigDB));
		int smtlk = 1;
#if 0
		jdNVargs_t jdArgs;
		jdArgs.magic[0] = '0xff';
#endif			
		hfsys_nvm_write(0, (char *)&smtlk,4);        // 在这里加入2min超时标记
		//hffile_userbin_write(JDCONFIG_OFFSET, (char*)&jdArgs, sizeof(jdNVargs_t));
        u_printf("RESET FOR SMARTLINK OK\n");
    }
    if(reset_reason&HFSYS_RESET_REASON_WPS_OK)
    {
        u_printf("RESET FOR WPS OK\n");
    }
#endif
    
    return;
}

extern AC_OptList uart_send_pstruOptList;
extern u8 uart_send_MsgId;
extern u8 g_u8DevMsgBuildBuffer[];

char reply_data[]={0xAA,0xFF,0x55};
unsigned char g_u8recv_data[50]={0};

typedef enum {
    PKT_UNKNOWN,
    PKT_PUREDATA,
} PKT_TYPE;
#if 0
void testpm()
{
    u8 testpm[7] = {0xaa, 0x06, 0x00, 0xb0, 0x00, 0x55};

    hfuart_send(HFUART0,(char *)testpm,6,1000);
		
}

void testvoice()
{
    u8 testpm[7] = {0XAA, 0X02, 0X01, 0x12, 0xbf,0x00,0x55};
    hfuart_send(HFUART0,(char *)testpm,7,1000);
}
#endif
int UartProcess(unsigned char *data,u8 len)
{
    //int ret;
    char domain_id[20]={'\0'};
    //unsigned int left_time;
	//int length;
    memset(domain_id,'\0',20);
    switch(data[1])
    {
        case 0xF0:
        {
            hfat_send_cmd("AT+WSLK\r\n",strlen("AT+WSLK\r\n"),domain_id,16);
            if(strstr("+ok=Disconnected",domain_id)!=NULL)
            {
                ZC_Printf("recv smtlk\n");
                hfuart_send(HFUART0,reply_data,3,100);
                msleep(200);
                hfsmtlk_start();
            }
            break;
        }
        case 0xF1:
        {
            ZC_ConfigUnBind(ZC_MAGIC_FLAG);
            ZC_Printf("recv rest\n");
            hfuart_send(HFUART0,reply_data,3,100);
            msleep(500);
            hfsys_reload();
            hfsys_reset();
            break;
        }
        case 0xF2:
        {
            u32 MagicFlag = ZC_MAGIC_FLAG;				
            ZC_Printf("recv subdomain\n");
            SUB_DOMAIN_ID = data[3]+data[4]*256;
            hffile_userbin_write(304, (char *)&SUB_DOMAIN_ID, sizeof(u64));
            hffile_userbin_write(300, (char *)&MagicFlag, sizeof(u32));
            ZC_Printf("subdomain = %d",SUB_DOMAIN_ID);
            hfuart_send(HFUART0,reply_data,3,100);
            break;
        }
        case 0xF3:
        {
            ZC_Printf("recv ready query\n");
            if(hfgpio_fpin_is_high(HFGPIO_F_NREADY)==0)
            {
                 if(SUB_DOMAIN_ID != 0xFFFF)
                    hfuart_send(HFUART0,reply_data,3,100);
            }
            break;
        }
        default:
        {
            u16 u16DataLen;
        	if(SUB_DOMAIN_ID == 0xFFFF)
        	{
        		return 0;
        	}
            if(data[1] < 16)
            {
            	ZC_Printf("recv report\n");
            	AC_BuildMessage(ACCORD_RELPY_DATA,0,
            			(u8*)data, len,
            			NULL, 
            			g_u8DevMsgBuildBuffer, &u16DataLen);
            	AC_SendMessage(g_u8DevMsgBuildBuffer, u16DataLen);
#if 1
                /* 主动上报的消息 */
                if (CMD_COMMON == data[1])
                {
                    jd_send_data(data, len);
                }
#endif
            }
            else
            {
                /* 回的响应包 */
            	ZC_Printf("recv query\n");
            	AC_BuildMessage(RESPOND_SERVER_DATA,uart_send_MsgId,
            			(u8*)data, len,
            			NULL, 
            			g_u8DevMsgBuildBuffer, &u16DataLen);
            	AC_SendMessage(g_u8DevMsgBuildBuffer, u16DataLen);
#if 1
                jd_send_data(data, len);
#endif
            }
            break;
        }
    }

    return 0;
}

static int jd_send_data(unsigned char *data, unsigned char len)
{
    int ret;
    //uartcmd_t cmd;
    unsigned char type;
    unsigned int value, left_time;
    unsigned char datalen;
    const char *pStreamId;
    //int length;
    /* 原始数据的头 */
    //unsigned char* pBuf = data;
    
    type = data[YD_CODE];
#if 1
    //unsigned int i;
    switch (type)
    {
        case CMD_COMMON:
            break;
        case CMD_UP_POWER:
            //choice = CMD_DOWN_POWER;
            pStreamId = "power";
            break;
        case CMD_UP_SPEED:
            //choice = CMD_DOWN_SPEED;
            pStreamId = "speed";
            break;
        case CMD_UP_SOUND:
            //choice = CMD_DOWN_SOUND;
            pStreamId = "sound";
            break;
        case CMD_UP_LIGHT:
            //choice = CMD_DOWN_LIGHT;
            pStreamId = "light";
            break;
        case CMD_UP_MODE:
            //choice = CMD_DOWN_MODE;
            pStreamId = "mode";            
            break;
        case CMD_UP_PM25:
            //choice = CMD_DOWN_PM25;
            pStreamId = "PM25";
            break;
        default:
            custom_log("Invalid msg type[%d]", type);
            return -1;
    }
#endif
    //custom_log("recv from mcu, type is 0x%02x\n", type);
    if (CMD_COMMON == type)
    {
        goto COMMON_ACT;
    }
#if 1
    datalen = data[YD_DATA_LEN];

    if (datalen == 1)
    {
        value = data[YD_DATA_LEN + 1];
    }
    else if (datalen == 2)
    {
        //data[YD_DATA_LEN + 1] = 0;
        //data[YD_DATA_LEN + 2] = 5;
        value = data[YD_DATA_LEN + 1] + (data[YD_DATA_LEN + 2] << 8);
    }
    else
    {
        custom_log("invalid len %d", datalen);
        return -1;
    }
    
    (void)sprintf((char *)Jd_GlobalVar.post_tx, format_upload,
                            //g_struJdAttribute[choice].record,
                            jdArgs.feedid,
                            jdArgs.accesskey,
                            pStreamId,
                            value);
    goto SEND;  
#endif

COMMON_ACT:
#if 0
    for (ret = 0; ret < len; ret++)
        custom_log("data[%d] = 0x%02x", ret, data[ret]);
#endif

//#define   FILT_LIFE              3650
    //data[5] = 0;
    //data[6] = 6;
    value = data[5] + (data[6] << 8);
    //custom_log("value is %d", value);
    //value = data[6] + data[5] << 8;
    left_time = data[11] + (data[12] << 8);
    //left_time = (FILT_LIFE - left_time) * 100 / FILT_LIFE;
    //left_time = 56;
    (void)sprintf((char *)Jd_GlobalVar.post_tx, format_common,
                    jdArgs.feedid,
                    jdArgs.accesskey,
                    data[3],
                    data[4],
                    value,
                    data[7],
                    data[8],
                    data[9],
                    //data[10],
                    left_time);
SEND:
    /* 发送 */
	ret = jdssl_send((char *)Jd_GlobalVar.post_tx);
	if(ret <= 0)
	{
		//hfdbg_set_level(10);
		custom_log("[E1] ssl upload send failed");
        //sleep(1);
        //hfdbg_set_level(0);

	}
	else
	{
		custom_log("[+] ssl upload send %u bytes", ret);
	}

    return ret;
 
}

u8 g_curtype =PKT_UNKNOWN;
u8 g_recvdata = 0;
u8 g_u8CurPktLen = 0;

int UartRecv(u8 data)
{
    switch(g_curtype)
    {
        case PKT_UNKNOWN:
        {
            if(data==0xAA)
            {
                ZC_Printf("recv start = 0x%02x\n", data);
                g_curtype = PKT_PUREDATA;
                g_recvdata = 0;
                g_u8recv_data[g_recvdata++] = data; 
                g_u8CurPktLen = 0;
            }
            break;
        }
        case PKT_PUREDATA:
        {
            g_u8recv_data[g_recvdata++] = data;
            if (3 == g_recvdata)
            {
                ZC_Printf("recv len = %d\n", data);
                g_u8CurPktLen = data;
            }

            if (g_recvdata == g_u8CurPktLen + 6)
            {
                if (data==0x55)
                {
                    UartProcess(g_u8recv_data,g_recvdata);
                    g_curtype = PKT_UNKNOWN;
                    g_recvdata = 0;  
                    g_u8CurPktLen = 0;
                }
                else
                {
                    ZC_Printf("error data end\n", data);
                    g_curtype = PKT_UNKNOWN;
                    g_recvdata = 0; 
                    g_u8CurPktLen = 0;
                }
                break;
            }
        }
        
    }
    return 0;
}

extern u32 g_u32StopJdTask;
static int USER_FUNC uart_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len) 
{
    int i = 0;
    if (1 == g_u32StopJdTask)
        return 0;
    for(i = 0; i < len; i++)
    {
        UartRecv(data[i]); 
    }

    return len; 
} 

int USER_FUNC app_main (void)
{
	time_t now=time(NULL);
	hfdbg_set_level(0);
    //custom_log("*****hi*******\n");
    //sleep(1);
    //hfdbg_set_level(0);
	hfgpio_fset_out_high(HFGPIO_F_USER_LINK);
#if 0
	custom_log("[CALLBACK DEMO]sdk version(%s),the app_main start time is %d %s\n",hfsys_get_sdk_version(),now,ctime(&now));
#endif
	if(hfgpio_fmap_check(module_type)!=0)
	{
		while(1)
		{
			HF_Debug(DEBUG_ERROR,"gpio map file error\n");
			msleep(1000);
		}
	}
	
	while(!hfnet_wifi_is_active())
	{
		msleep(50);
	}

    show_reset_reason();
    if(hfnet_start_uart(HFTHREAD_PRIORITIES_LOW,(hfnet_callback_t)uart_recv_callback)!=HF_SUCCESS)
    {
        HF_Debug(DEBUG_WARN,"start uart fail!\n");
    }
	hf_start_jdsmart();

    hfthread_create(HF_Init,"CONNECT",256,NULL,1,NULL,NULL);



	return 1;
	
}



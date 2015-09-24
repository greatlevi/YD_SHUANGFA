#include <hsf.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "jdsmart.h"
#include "zc_jd.h"
#include "common.h"

//JD_JASON_ATTRIBUTE g_struJdAttribute[6];
//int SUB_DOMAIN_ID;
#if 0
const char format_upload[]=
"{\"code\":102,\"result\":0,\"control_resp\":\"Succeed\",\"attribute\":%s,\
\"device\":{\"feed_id\":\"%s\",\"accees_key\":\"%s\"},\
\"stream\":{\"stream_id\":\"%s\",\"current_value\":\"%d\"}}\n";

const char format_common[]=
"{\"code\":103,\"device\":{\"feed_id\":\"%s\",\"access_key\":\"%s\"},\
\"streams\":[{\"stream_id\":\"power\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"speed\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"PM25\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"sound\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"light\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"mode\",\"datapoints\":[{\"value\":%d}]},\
{\"stream_id\":\"lefttime\",\"datapoints\":[{\"value\":%d}]}]}\n";
#endif

static unsigned short calc_ecc(unsigned char *buf, unsigned int num);
static int my_atoi(const char * s);
static void build_message(CONTROL_CMD_TYPE type, unsigned char *pvalue);
static void  json_to_binary(jdsmart_down_cmd_ptr cmd);

#if 0
static void check_reset_reason(void)
{
	uint32_t reset_reason=0;

	int smtlk = 0;
	
	hfsys_nvm_write(0, (char *)&smtlk,4);	
	reset_reason = hfsys_get_reset_reason();
	
	if(reset_reason&HFSYS_RESET_REASON_ERESET)
	{
		custom_log("HFSYS_RESET_REASON_ERESET");
	}
	if(reset_reason&HFSYS_RESET_REASON_IRESET0)
	{
		custom_log("HFSYS_RESET_REASON_IRESET0");
	}
	if(reset_reason&HFSYS_RESET_REASON_IRESET1)
	{
		custom_log("HFSYS_RESET_REASON_IRESET1");
	}
	if(reset_reason==HFSYS_RESET_REASON_NORMAL)
	{
		custom_log("HFSYS_RESET_REASON_NORMAL");
	}
	if(reset_reason&HFSYS_RESET_REASON_WPS)
	{
		custom_log("HFSYS_RESET_REASON_WPS");
	}
	if(reset_reason&HFSYS_RESET_REASON_SMARTLINK_START)
	{		
		custom_log("HFSYS_RESET_REASON_SMARTLINK_START");
	}
	if(reset_reason&HFSYS_RESET_REASON_SMARTLINK_OK)
	{
		int smtlk = 1;
    #if 0
		jdNVargs_t jdArgs;
		jdArgs.magic[0] = '0xff';
	#endif		
		hfsys_nvm_write(0, (char *)&smtlk,4);        // 在这里加入2min超时标记
		//hffile_userbin_write(JDCONFIG_OFFSET, (char*)&jdArgs, sizeof(jdNVargs_t));
		custom_log("HFSYS_RESET_REASON_SMARTLINK_OK");
	}
	
	return;
}
#endif
void set_network_state(int state, int on)
{
	switch(state)
	{
		case HF_STA_CONNECTED:
			Jd_GlobalVar.linkst_router   = 1;
			break;
		case HF_STA_DISCONNECTED:
			Jd_GlobalVar.linkst_router   = 0;
			Jd_GlobalVar.linkst_internet = 0;
			Jd_GlobalVar.linkst_cloud    = 0;
			break;
		case HF_DHCP_OK:
			Jd_GlobalVar.linkst_router   = 1;
			break;
		case HF_CLO_CONNECTED:
			Jd_GlobalVar.linkst_router   = 1;
			Jd_GlobalVar.linkst_internet = 1;
			Jd_GlobalVar.linkst_cloud    = 1;
			//custom_log("HF_CLO_CONNECTED,linkst_cloud:%d", Jd_GlobalVar.linkst_cloud);
			break;
		case HF_CLO_DISCONNECTED:
			Jd_GlobalVar.linkst_cloud    = 0;
			//custom_log("HF_CLO_CONNECTED,linkst_cloud:%d", Jd_GlobalVar.linkst_cloud);
			break;
		case HF_CLO_OTA:
			if(on)
				Jd_GlobalVar.linkst_ota  = 1;
			else
				Jd_GlobalVar.linkst_ota  = 0;
			break;
		case HF_NET_CONNECTED:
			Jd_GlobalVar.linkst_internet = 1;
			break;
		case HF_NET_DISCONNECTED:
			Jd_GlobalVar.linkst_internet = 0;
			break;
	}
}

static int hfsys_event_callback( uint32_t event_id,void * param) 
{ 
    switch(event_id) 
    { 
        case HFE_WIFI_STA_CONNECTED: 
            set_network_state(HF_STA_CONNECTED, 1);
            hfgpio_fset_out_low(HFGPIO_F_USER_LINK);
            u_printf("wifi sta connected!!\n"); 
            break; 
        case HFE_WIFI_STA_DISCONNECTED: 
            set_network_state(HF_STA_DISCONNECTED, 0);
            hfgpio_fset_out_high(HFGPIO_F_USER_LINK);
            u_printf("wifi sta disconnected!!\n"); 

            HF_Sleep();
            HF_BcInit();
            break; 
        case HFE_DHCP_OK: 
        { 
            g_u32GloablIp = *((uint32_t*)param); 
            g_u32GloablIp = ZC_HTONL(g_u32GloablIp);
            u_printf("dhcp ok %08X!\n",g_u32GloablIp); 
            set_network_state(HF_DHCP_OK, 1);
            HF_WakeUp();
        } 
            break; 
        case HFE_SMTLK_OK: 
            u_printf("smtlk ok!\n"); 
            break; 
        case HFE_CONFIG_RELOAD: 
            u_printf("reload!\n"); 
            break; 
        default: 
            break; 
    } 
    return 0; 
} 


//char reply_data[]={0xAA,0xFF,0x55};

#if 0
static int USER_FUNC UART_handlePacket(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{	
	char* pBuf = Jd_GlobalVar.post_tx;
	memcpy(pBuf,data,len);
   
    jdupload_process();
    return len;

}
#endif
/*****************************************************************************
 *	  Function Name       : ssl_recv_callback
 *	  Description            : 服务器下发控制命令，组成串口协议数据下发
 *	  INPUT	                : 	
                                       type				Type, integer or string
                                       array_num		Serial number of Skey
                                       array_size			Toatl number of Skey
                                       sKey				Name of stream_id
                                       string			Value corresponds to the sKey.(string type)
                                       number			Value corresponds to the sKey.(integer type)
 *      Output                  : 
 *
 *	  Returns	                :
 ****************************************************************************/
static int USER_FUNC jdsmart_hander_set_device_status(jdsmart_down_cmd_ptr cmd)
{
#if 0  

	custom_log("====>>>SN:[%d],PN:[%d] stream_id:%s, value:%s", 
		                     cmd->SN, cmd->PN, cmd->stream_id, cmd->value);
#endif
  
    json_to_binary(cmd);
    
	return 1;
}


/* add for transfer json to binary */
static void  json_to_binary(jdsmart_down_cmd_ptr cmd)
{
    CONTROL_CMD_TYPE type;
    if (0 == strcmp((const char *)cmd->stream_id, "light"))
    {
        type = CMD_DOWN_LIGHT;
    }
    else if (0 == strcmp((const char *)cmd->stream_id, "mode"))
    {
        type = CMD_DOWN_MODE;
    }
    else if (0 == strcmp((const char *)cmd->stream_id, "PM25"))
    {
        type = CMD_DOWN_PM25;
    }
    else if (0 == strcmp((const char *)cmd->stream_id, "power"))
    {
        type = CMD_DOWN_POWER;    
    }
    else if (0 == strcmp((const char *)cmd->stream_id, "sound"))
    {
        type = CMD_DOWN_SOUND;
    }
    else if (0 == strcmp((const char *)cmd->stream_id, "speed"))
    {
        type = CMD_DOWN_SPEED;
    }
    else
    {
        //custom_log("Unknown cmd stream_id[%s]", cmd->stream_id);
        return;
    }
    /* 记录一下 attibute */
    //strcpy((char *)g_struJdAttribute[type].record, (const char *)cmd->attribute);
    
    build_message(type, cmd->value);
    return;
}

static void build_message(CONTROL_CMD_TYPE type, unsigned char *pvalue)
{
	//unsigned int i;
    unsigned char len, value, eccPosition, totalLen;
    unsigned short eccValue;
    unsigned char buf[MSG_WITH_DATA_LEN];
    //unsigned int i;
    eccPosition = 3;
    switch (type)
    {
        case CMD_DOWN_POWER:
        case CMD_DOWN_SPEED:
        case CMD_DOWN_SOUND:
        case CMD_DOWN_LIGHT:
        case CMD_DOWN_MODE:
            len = 1;
            /* 计算一下数据内容 */
            value = my_atoi((const char *)pvalue);
            break;
        case CMD_DOWN_PM25:
            len = 0;
            break;
        default:
            return;
    }
    
    buf[YD_HEAD] = MSG_HEAD;
    buf[YD_CODE] = type;
    buf[YD_DATA_LEN] = len;
    if (len > 0)
    {
        buf[YD_DATA_LEN + 1] = value;
        eccPosition = YD_DATA_LEN + ECC_OFFSET;
    }
    /* 求校验和 */
    eccValue = calc_ecc(buf, MSG_COMMON_LEN + len);
    if (YD_DATA_LEN + ECC_OFFSET == eccPosition)
    {
        buf[YD_DATA_LEN + ECC_OFFSET] = (unsigned char)(eccValue & 0xFF);
        buf[YD_DATA_LEN + ECC_OFFSET + 1] = (unsigned char)((eccValue & 0xFF00) >> 8);
        buf[YD_DATA_LEN + ECC_OFFSET + 2] = MSG_TAIL;
        totalLen = MSG_WITH_DATA_LEN;
    }
    else
    {
        buf[YD_DATA_LEN + 1] = (unsigned char)(eccValue & 0xFF);
        buf[YD_DATA_LEN + 2] = (unsigned char)((eccValue & 0xFF00) >> 8);
        buf[YD_DATA_LEN + 3] = MSG_TAIL;
        totalLen = MSG_WHTHOUT_DATA_LEN;
    }
#if 0
    for (i = 0; i < totalLen; i++)
    {
        custom_log("data[%d] = 0x%02x", i, buf[i]);
    }
#endif
    hfuart_send(HFUART0, (char *)buf, totalLen, 100);
}

static int my_atoi(const char * s)
{
    int n = 0;
    int i = 0;
    while (s[i] != '\0')
    {
        n *= 10;
        n += s[i] - '0';
        i++;
    }
    return n;
}

static unsigned short calc_ecc(unsigned char *buf, unsigned int num)
{
    unsigned short result = 0;
    unsigned int i;
    for (i = 0; i < num; i++)
    {
        result += buf[i];
    }
    return result;
}

static int USER_FUNC jdsmart_hander_get_device_status(jdsmart_down_cmd_ptr arg)
{
#if 0 

	uartcmd_t cmd;
    unsigned char type, choice, len;
    unsigned int value, left_time;
    const char *pStreamId;
	int length;
    /* 原始数据的头 */
	char* pBuf = Jd_GlobalVar.post_tx;
   
    type = Jd_GlobalVar.post_tx[YD_CODE];
    
    switch (type)
    {
        case CMD_COMMON:
            break;
        case CMD_UP_POWER:
            choice = CMD_DOWN_POWER;
            pStreamId = "power";
            break;
        case CMD_UP_SPEED:
            choice = CMD_DOWN_SPEED;
            pStreamId = "speed";
            break;
        case CMD_UP_SOUND:
            choice = CMD_DOWN_SOUND;
            pStreamId = "sound";
            break;
        case CMD_UP_LIGHT:
            choice = CMD_DOWN_LIGHT;
            pStreamId = "light";
            break;
        case CMD_UP_MODE:
            choice = CMD_DOWN_MODE;
            pStreamId = "mode";            
            break;
        case CMD_UP_PM25:
            choice = CMD_DOWN_PM25;
            pStreamId = "PM25";
            break;
        default:
            custom_log("Invalid msg type[%d]", type);
            return -1;
    }
    custom_log("recv from mcu, type is 0x%02x\n", type);
    if (CMD_COMMON == type)
        goto COMMON_ACT;

    len = Jd_GlobalVar.post_tx[YD_DATA_LEN];

    if (len == 1)
    {
        value = Jd_GlobalVar.post_tx[YD_DATA_LEN + 1];
    }
    else if (len == 2)
    {
        value = Jd_GlobalVar.post_tx[YD_DATA_LEN + 1] + Jd_GlobalVar.post_tx[YD_DATA_LEN + 2] << 8;
    }
    else
    {
        custom_log("invalid len %d", len);
        return -1;
    }
    
    length = sprintf(pBuf, format_upload,
	                        g_struJdAttribute[choice].record,
							jdArgs.feedid,
							jdArgs.accesskey,
							pStreamId,
							value);
	return length;

COMMON_ACT:

    value = Jd_GlobalVar.post_tx[5] + Jd_GlobalVar.post_tx[6] << 8;
    left_time = Jd_GlobalVar.post_tx[10] + Jd_GlobalVar.post_tx[11] << 8;
    length = sprintf(pBuf, format_common,
							jdArgs.feedid,
							jdArgs.accesskey,
							Jd_GlobalVar.post_tx[3],
							Jd_GlobalVar.post_tx[4],
							value,
							Jd_GlobalVar.post_tx[7],
							Jd_GlobalVar.post_tx[8],
							Jd_GlobalVar.post_tx[9],
							left_time);
    return length;
#endif
    return 0;
}

int register_network_states_event()
{	
    int err = hfsys_register_system_event((hfsys_event_callback_t)hfsys_event_callback);

    if (err != HF_SUCCESS) {
        custom_log("register system event fail");
		return -1;
    }
		return err;
}

hfthread_sem_t uart_psem;
jdNVargs_t jdArgs;

jdNVargs_t *jd_getGlobalContext()
{
	hffile_userbin_read(JDCONFIG_OFFSET, (char*)&jdArgs, sizeof(jdNVargs_t));
	if(memcmp(jdArgs.magic, "HF_JD", 5))   //not same
	{
		hffile_userbin_zero();
		hffile_userbin_read(JDCONFIG_OFFSET, (char*)&jdArgs, sizeof(jdNVargs_t));
	}
	else 
	{
	  custom_log("Feedid:[%s]", jdArgs.feedid);
	}

	if(hfthread_sem_new(&uart_psem, 0) == -HF_FAIL)
	{
		custom_log("create uart_psem failed");
	}
    return &jdArgs;
}

const char *jd_get_mac_addr()
{
	static uint8_t sys_mac[18];
	const char cmd[] = "AT+WSMAC\r\n";
	char *words[6]={NULL};
	char rsp[64]={0};

	hfat_send_cmd((char*)cmd, sizeof(cmd), rsp, 64);
	if(hfat_get_words(rsp, words, 6)>0)
	{
		char* pMac = words[1];
		for(int i=0; i<6; i++)
		{
			sys_mac[3*i+0] = pMac[2*i+0];
			sys_mac[3*i+1] = pMac[2*i+1];
			sys_mac[3*i+2] = '-';
		}
		sys_mac[3*5+2] = 0;
	}
	return (const char *)sys_mac;
}


void jd_arg_init()
{
	char buffer[64];
    int ret, resetflag = 0;
	
	ret = hfat_send_cmd("AT+UART\r\n", sizeof("AT+URAT\r\n"), buffer, 64);
    if(strcmp(buffer + 4, "9600,8,1,None,NFC") != 0)
    {
        ret = hfat_send_cmd("AT+UART=9600,8,1,NONE,NFC,0\r\n", sizeof("AT+UART=9600,8,1,NONE,NFC,0\r\n"), buffer, 64);
        if(ret != HF_SUCCESS)
        {
        	custom_log("Failed to set UART0");
        }
        resetflag = 1;
    }

	if(resetflag == 1)
    {
        JD_Dev_Reset();
    }
}

struct device_info Jd_GlobalVar;

int hf_start_jdsmart(void)
{
	jd_arg_init();
	//check_reset_reason();
	register_network_states_event();

	jd_getGlobalContext();
	
	memcpy(Jd_GlobalVar.mac, jd_get_mac_addr(), 18);
	
	Jd_GlobalVar.dev_callback[JD_GET_DEVICE_STATUS] = jdsmart_hander_get_device_status;
	Jd_GlobalVar.dev_callback[JD_SET_DEVICE_STATUS] = jdsmart_hander_set_device_status;
	
	if(hfnet_start_assis(ASSIS_PORT)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start httpd fail\n");
	}
#if 0
	if(hfnet_start_uart(HFTHREAD_PRIORITIES_LOW,(hfnet_callback_t)UART_handlePacket)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start uart fail!\n");
	}
#endif
	if(hfnet_start_httpd(HFTHREAD_PRIORITIES_MID)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start httpd fail\n");
	}

	int ret = jdsmart_start();
	
	return ret;
}


#include <stdio.h>
#include "common.h"
#include "jdsmart.h"

void JD_Dev_Reset()
{
    msleep(1000);
    custom_log("------------Reboot now");

	hfsys_softreset();
    return;
	
}
void httpdecode(char *p, int isFind)	// isFind:????????
{
    int i = 0;
    int isLine = 0;
    isLine = !isFind;
    while(*(p + i))
    {
        if(isLine==0) //????
        {
        	if(*(p+i)=='\n')
        	{
        		if(*(p+i+2)=='\n')
        		{
        			i+=1;
        			isLine = 1;
        		}
        	}
        	i++;
        	continue;
        }
        if ((*p = *(p + i)) == '%')
        {
            *p = *(p + i + 1) >= 'A' ? ((*(p + i + 1) & 0XDF) - 'A') + 10 : (*(p + i + 1) - '0');
            *p = (*p) * 16;
            *p += *(p + i + 2) >= 'A' ? ((*(p + i + 2) & 0XDF) - 'A') + 10 : (*(p + i + 2) - '0');
            i += 2;
        }
        else if (*(p + i) == '+')
        {
            *p = ' ';
        }
        p++;
    }
    *p = '\0';
}
int str2int(char* pStr)
{
  cJSON jValue = {0};
  parse_number(&jValue, pStr);
  return jValue.valueint;
}

char *my_itoa(int num,char *str,int radix)
{
	const char table[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char *ptr = str;
	bool negative = false;
	if(num == 0)
	{
		*ptr++='0';
		*ptr='\0';
		return str;
	}
	if(num < 0)
	{
		*ptr++='-';
		num*=-1;
		negative = true;
	}
	while(num)
	{
		*ptr++ = table[num%radix];
		num/=radix;
	}
	*ptr = '\0';
	char *start =(negative?str+1:str);
	ptr--;
	while(start<ptr)
	{
		char temp = *start;
		*start = *ptr;
		*ptr = temp;
		start++;
		ptr--;
	}
	return str;
}


/* return 1 is a ipaddress */
int addressis_ip(const char * ipaddr)
{
	char ii, ipadd;
	int i, j;
	
	ii=0;
	for (j= 0; j< 4; j++)
	{
		ipadd=0;
		for (i=0; i< 4; i++, ii++)
		{
			if (*(ipaddr+ii)=='.')
				if (i== 0)
					return 0;		//the first shall not be '.'
				else
				{
					ii++;
					break;			//this feild finished
				}
			else if ((i==3)&&(j!=3))	//not the last feild, the number shall less than 4 bits
				return 0;
			else if ((*(ipaddr+ii) > '9')||(*(ipaddr+ii) < '0'))
			{
				if ((*(ipaddr+ii) == '\0')&&(j==3)&&(i!=0))
				{
					break;
				}
				else
					return 0;			//pls input number
			}
			else
				ipadd= ipadd*10+(*(ipaddr+ii)-'0');
			if (ipadd > 255)
				return 0;
		}
	}
	return 1;
}


CYASSL_METHOD* method = NULL;
void ssl_init(void)
{
    //int ret;
	//InitMemoryTracker();//for debug, it can show how many memory used in SSL
#if(SSLDebug_ON==1)
	CyaSSL_Debugging_ON();//for debug
#endif
	CyaSSL_Init();
    //custom_log("CyaSSL_Init ret is %d",ret);
	method=CyaSSLv3_client_method();
    //ShowMemoryTracker();
}

int ssl_connect(int fd, ssl_t* pssl)
{
	int ret;
	static int connect_fail = 0;
	CYASSL_CTX*     ctx     = 0;
	CYASSL*         ssl     = 0;
  	ssl_init();
	ctx = CyaSSL_CTX_new(method);
	if (ctx == NULL)
	{
		HF_Debug(DEBUG_LEVEL_LOW, "unable to get ctx");
		return 1;
	}
	pssl->ctx = ctx;

	CyaSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);//disable verify certificates
	
	ssl = CyaSSL_new(ctx);
	if (ssl == NULL)
	{
		CyaSSL_CTX_free(ctx);
		pssl->ctx = NULL;
		HF_Debug(DEBUG_LEVEL_LOW, "unable to get SSL object");
		return 1;
	}
	pssl->ssl = ssl;
	
	ret=CyaSSL_set_fd(ssl, fd);
	if (ret==1)
 	{
		pssl->fd = fd;
		connect_fail = 0;
	  	return 0;
  	}
	else
	{
	
		int  err = CyaSSL_get_error(ssl, 0);
		//custom_log("CyaSSL_connect ERROR:%d\n",err);
		char buffer[80];
		//custom_log("err = %d, %s\n", err,CyaSSL_ERR_error_string(err, buffer));
		HF_Debug(DEBUG_LEVEL_LOW, "err = %d, %s\n", err,CyaSSL_ERR_error_string(err, buffer));
		HF_Debug(DEBUG_LEVEL_LOW, "SSL_connect failed");
		
		CyaSSL_free(ssl);
		pssl->ssl = NULL;
		
		CyaSSL_CTX_free(ctx);
		pssl->ctx = NULL;
		connect_fail++;
		if(connect_fail >= 10)
			hfsys_reset();
	//	close(fd);
		return 1;
		
	}
}

int ssl_check(ssl_t* pssl)
{
	if(pssl->fd == -1)
		return 1;
	if(pssl->ssl==NULL)
	{
		close(pssl->fd);
		pssl->fd = -1;
		return 1;
	}
	return 0;
}

int ssl_close(ssl_t* pssl)
{
	if(pssl->ssl!=NULL)
	{
		CyaSSL_shutdown(pssl->ssl);
		CyaSSL_free(pssl->ssl);
		pssl->ssl = NULL;
	}
	if(pssl->ctx!=NULL)
	{
		CyaSSL_CTX_free(pssl->ctx);
		pssl->ctx = NULL;
	}
	if(pssl->fd!=-1)
	{
		close(pssl->fd);
		pssl->fd=-1;
	}
	
	set_network_state(HF_CLO_DISCONNECTED, 0);
	
	return 0;
}

int get_ip_port(char *ip_port)
{	
	char *p = strtok(ip_port,":");
	if(p)
		MEMCPY(Jd_GlobalVar.jdserver_ip, p,strlen(p));
	else
		return -1;
	
	p = strtok(NULL, ":");
	if(p)
		Jd_GlobalVar.jdserver_port = str2int(p);
	else
		return -1;

	custom_log("JDSSL_IP:[%s],JDSSL_PORT:[%d]\n", Jd_GlobalVar.jdserver_ip, Jd_GlobalVar.jdserver_port);
	return 1;
}

int get_rssi_value()
{
	char *words[6]={NULL}; 
	char rsp[64]={0};
	uint32_t rssi;
	
	hfat_send_cmd("AT+WSLQ\r\n", sizeof("AT+WSLQ\r\n"), rsp, 64); 
	if(hfat_get_words(rsp, words, 6)>0) 
	{
		if(strcmp(words[1],"Disconnected") == 0)
			rssi = 0;
		else
		{
			char *p = strtok(words[2],"%");
			if(p)
				rssi = str2int(p);
			else
				rssi = 0;
		}
	}
	return rssi;
}
#if 0
int HSF_API JD_enable_netwatchdog(NetWdg_t *netwdg,uint32_t time)
{
	if(netwdg==NULL)
	{
		custom_log("netwdg is null");
		return -1;
	}
	
	netwdg->flags       |= JD_FLAG_ENABLE_NETWDG;
	netwdg->wdg_timeout  = time;
	netwdg->wdg_time     = 0;
	return 1;
}

int HSF_API JD_reset_netwatchdog(NetWdg_t *netwdg)
{
	if(netwdg==NULL)
	{
		custom_log("netwdg is null");
		return -1;
	}
	
	netwdg->wdg_time = 0;
	return 0;
}

int HSF_IAPI JD_netwatchdog_check(NetWdg_t *netwdg)
{
	if(netwdg==NULL)
	{
		custom_log("netwdg is null");
		return -1;
	}
	
	if(netwdg->flags&JD_FLAG_ENABLE_NETWDG)
	{
		if(netwdg->wdg_time>=netwdg->wdg_timeout)
		{
			//hfsys_softreset();
			custom_log("Netwatchdog timeout:%d", netwdg->wdg_time);
			set_network_state(HF_NET_DISCONNECTED,0);
		}
		else
		{
			set_network_state(HF_NET_CONNECTED,0);
		}
		netwdg->wdg_time++;
	}

	return HF_SUCCESS;
	
}
#endif


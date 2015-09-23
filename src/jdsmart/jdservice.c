#include <stdio.h>
#include <hsf.h>
#include "common.h"
#include "jdsmart.h"
#include "api.h"

//extern int HSF_API JD_reset_netwatchdog(NetWdg_t *netwdg);
//extern int HSF_API JD_enable_netwatchdog(NetWdg_t *netwdg,uint32_t time);
//extern int HSF_IAPI JD_netwatchdog_check(NetWdg_t *netwdg);
ssl_t ssl_data = {-1,0,0};
extern u32 g_u32StopJdTask;
//extern JD_JASON_ATTRIBUTE g_struJdAttribute[6];

int tcp_connect_ssl_server(uint8_t *url,int port)
{
	int fd;	
	int ret;
	struct sockaddr_in addr;
	struct sockaddr_in local_addr;
	char *addrp=url;
	
	if((memcmp(url, "HTTPS://", 8)==0)||(memcmp(url, "https://", 8)==0))
		addrp= (char *)(url+8);

	ip_addr_t dest_addr;
	addressis_ip((const char *)(addrp));
	netconn_gethostbyname((const char *)(addrp), &dest_addr);
	inet_aton((char *)(addrp), (ip_addr_t *) &dest_addr);

	uint16_t local_port=((Timer1GetTime()>>16)+(Timer1GetTime())&0xFFFF)&0x1FFF;
	local_port += 0x2FFF;
	custom_log("Client local port:[%d]", local_port);

	memset((char*)&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr=dest_addr.addr;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd<0) 
    {
        //custom_log("error fd:[%d]", fd);
        return -1;
    }
	else	
	{
		memset((char *)&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_len = sizeof(local_addr);
		local_addr.sin_port = htons(local_port);
		local_addr.sin_addr.s_addr= htonl(INADDR_ANY);
		bind(fd,(struct sockaddr *)&local_addr,sizeof(local_addr));

		int tmp=1;
		if(setsockopt(fd, SOL_SOCKET,SO_KEEPALIVE,&tmp,sizeof(tmp))<0)
		{
			//custom_log("set SO_KEEPALIVE fail");
		}
		tmp = 15;//40s
		if(setsockopt(fd, IPPROTO_TCP,TCP_KEEPIDLE,&tmp,sizeof(tmp))<0)
		{
			//custom_log("set TCP_KEEPIDLE fail");
		}
		tmp = 3;
		if(setsockopt(fd, IPPROTO_TCP,TCP_KEEPINTVL,&tmp,sizeof(tmp))<0)
		{
		//custom_log("set TCP_KEEPINTVL fail");
		}
		tmp = 3;
		if(setsockopt(fd, IPPROTO_TCP,TCP_KEEPCNT,&tmp,sizeof(tmp))<0)
		{
			//custom_log("set TCP_KEEPCNT fail");
		}
		ret=connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	 	if (-1 == ret)
    	{
      		close(fd);
			custom_log("[E] Connect Failed,Continue......");
			return -1;
    	}
		else
		{
			custom_log("[+] Connect OK");
			return fd;
		}	
	}  
}

int JDssl_local_upgrade(cJSON *pRoot)
{
		return 1;
}

const char upgrade_response[]="{\"code\":107,\"firm_version\":%d,\"status\":%d,\"session_id\":\"%s\",\"device\":{\"feed_id\":\"%s\",\"access_key\":\"%s\"}}\n";

/*****************************************************************************
 *	  Function Name       : JDssl_APP_Upgrade
 *	  Description            : 收到升级指令，处理升级
 *	  INPUT	                : 	
 *      Output                  : 
 *
 *	  Returns	                :
 ****************************************************************************/
 #if 1
int JDssl_APP_upgrade(cJSON *pRoot)
{
	int dl_status,up_result;
	const char ssl_upgrade[]="{\"code\":105,\"result\":0,\"serial\":%d,\"device\":{\"feed_id\":\"%s\",\"access_key\":\"%s\"}}\n";
			
	cJSON *uPdate  = cJSON_GetObjectItem(pRoot, "update");
	int fw_version = cJSON_GetObjectItem(uPdate, "firm_version")->valueint;
	int serial     = cJSON_GetObjectItem(pRoot, "serial")->valueint;

	int length = sprintf(Jd_GlobalVar.ssl_tx, ssl_upgrade, serial, jdArgs.feedid, jdArgs.accesskey);

	set_network_state(HF_CLO_OTA, 1);
	
	int ret = CyaSSL_write(ssl_data.ssl, (char *)Jd_GlobalVar.ssl_tx, length);
	if(ret<=0)
	{
    	custom_log("[E] React to upgrade_request failed");
	}
	else
	{
		custom_log("[+] React to upgrade_request send %u bytes", ret);
	}

	//httpc dowload firmware
	up_result = Httpc_get(cJSON_GetObjectItem(uPdate,"firm_url")->valuestring, &dl_status);

	if(dl_status == DOWNLOAD_FAIL)
		up_result = dl_status;

	custom_log("Upgrade result:%d",up_result);
	
	length = sprintf(Jd_GlobalVar.ssl_tx, upgrade_response,
				   cJSON_GetObjectItem(uPdate,"firm_version")->valueint,
				   up_result, 
			       cJSON_GetObjectItem(pRoot,"session_id")->valuestring,
			       jdArgs.feedid,jdArgs.accesskey);
	
	ret = CyaSSL_write(ssl_data.ssl, (char *)Jd_GlobalVar.ssl_tx, length);
	if(ret <= 0)
	{
        custom_log("[E] Upgrade result send failed");
	}
    else
    {
        custom_log("[+] Upgrade result send %u bytes", ret);
		if(up_result == UPGRADE_OK)
		{
			jdArgs.fw_version = fw_version;
			hffile_userbin_write(JDCONFIG_OFFSET, (char*)&jdArgs, sizeof(jdNVargs_t));
			custom_log("------------Upgrade Successfully,reboot soon-----------");
			msleep(20);
			hfsys_reset();
		}
    }
	
	set_network_state(HF_CLO_OTA, 0);
	return 1;
}
#endif
/*****************************************************************************
 *	  Function Name       : JDssl_HeartTick
 *	  Description            : 收到心跳回包，喂狗
 *	  INPUT	                : 	
 *      Output                  : 
 *
 *	  Returns	                :
 ****************************************************************************/
 #if 0
int JDssl_hearttick()
{
	JD_reset_netwatchdog(&NetWdg);
	hfthread_reset_softwatchdog(NULL);
	return 1;
}
 #endif
static int JDCmdProcess(char *pBuf)
{ 

	cJSON *pRoot = cJSON_Parse(pBuf);

	cJSON *pJCode = cJSON_GetObjectItem(pRoot, "code");
	int code_id = pJCode->valueint;
	if(pRoot==NULL) goto ERR2;

	switch (code_id)
	{
	
	case 1001:
	   	custom_log("HB<--%s", pBuf);
		//JDssl_hearttick();
		set_network_state(HF_CLO_CONNECTED, 0);
		break;

	case 1003:
	   	custom_log("CMD rsp<--%s", pBuf);
		//JDssl_hearttick();
		break;
		
	case 1005:
		custom_log("OTA<--%s",pBuf);
		JDssl_APP_upgrade(pRoot);
		break;
		
	case 1006:
		custom_log("UPDATE===>>%s",pBuf);
		JDssl_local_upgrade(pRoot);
		break;
		
	case 1002:
		{
			//custom_log("CMD<--1002");
			//hfthread_reset_softwatchdog(NULL);
			cJSON* pJCon = cJSON_GetObjectItem(pRoot, "control");
			if(pRoot==NULL) goto ERR1;
						
			int iCount = cJSON_GetArraySize(pJCon);//查找control下有几个节点
			int i = 0;
			for (i = 0;  i< iCount; ++i)
			{
				cJSON* pItem = cJSON_GetArrayItem(pJCon, i);
				if (NULL == pItem)
				{
					 continue;
				}
				cJSON* pTmp = cJSON_GetObjectItem(pItem, "stream_id");
				uint8_t* sKey = pTmp->valuestring;
				
				pTmp = cJSON_GetObjectItem(pItem, "current_value");

				jdsmart_down_cmd jd_cmd;
				jd_cmd.stream_id = sKey;
				jd_cmd.PN        = i;
				jd_cmd.SN        = iCount;
				
				if(pTmp->type==cJSON_String)  /* 字符串的话直接赋值 */
					jd_cmd.value = pTmp->valuestring;	
				else if(pTmp->type==cJSON_Number)
				{
					char str[25];
					my_itoa(pTmp->valueint, str, 25);
					jd_cmd.value = str;
				}
        #if 0
				cJSON* pJatt = cJSON_GetObjectItem(pRoot, "attribute");
			    char* out = cJSON_PrintUnformatted( pJatt);
                jd_cmd.attribute = out;
        #endif
				Jd_GlobalVar.dev_callback[JD_SET_DEVICE_STATUS](&jd_cmd);
                //hfmem_free(out);
			}

			//recv app's control pack ok, and do response.
#if 1			
			cJSON* pJatt = cJSON_GetObjectItem(pRoot, "attribute");
			char* out = cJSON_PrintUnformatted( pJatt);
            /* 保存 attribute*/
            
			int length = sprintf(Jd_GlobalVar.ssl_tx, "{\"code\": 102,\"attribute\":%s,\"result\":0,\"control_resp\":\"Succeed\",\"device\":{\"feed_id\":\"%s\",\"accees_key\":\"%s\"}}\n",
									out,
									jdArgs.feedid,
									jdArgs.accesskey);
            //custom_log("%s", Jd_GlobalVar.ssl_tx);
			hfmem_free(out); /* cJSON_PrintUnformatted申请资源了? */
			
			int ret = jdssl_send((char *)Jd_GlobalVar.ssl_tx);
			if(ret<=0)
			{
        		custom_log("[E] RSP send failed");
			}
    		else
    		{
        		custom_log("[+] RSP send %u bytes", ret);
    		}
#endif
		}
		break;
	}

	cJSON_Delete(pRoot);

	return 0;
ERR1:
	cJSON_Delete(pRoot);
ERR2:
	//custom_log("Server json error before: [%s]",cJSON_GetErrorPtr());
	return 0;
}

 /*****************************************************************************
 *	  Function Name       : jdupload_thread
 *	  Description            : 将串口数据组包上传
 *	  INPUT	                : 	
 *      Output                  : 
 *
 *	  Returns	                :
 ****************************************************************************/
void jdupload_process()
{
	if(Jd_GlobalVar.linkst_cloud == 1)
		hfthread_sem_signal(uart_psem);  //post signal to advice jdupload_thread.
	else
		custom_log("Link is down,Skip upload");
}

 /*****************************************************************************
 *	  Function Name       : heart_report
 *	  Description            : 发送心跳包
 *	  INPUT	                : 	
 *      Output                  : 
 *
 *	  Returns	                :
 ****************************************************************************/
void heart_tick_report()
{
	int ret;
  	const char sslHeart[]="{\"code\":101,\"device\":{\"feed_id\":\"%s\",\"access_key\":\"%s\",\"firm_version\":%d,\"rssi\":%d}}\n";

	int length = sprintf(Jd_GlobalVar.ssl_tx, sslHeart,
            jdArgs.feedid, jdArgs.accesskey, jdArgs.fw_version,get_rssi_value());

    custom_log("*** %s", Jd_GlobalVar.ssl_tx);
	
#ifdef CLOUD_SSL
	if(NULL != ssl_data.ssl)
	{
		ret = jdssl_send(Jd_GlobalVar.ssl_tx);
		if(ret <= 0)
		{
			custom_log("[E] heart tick send failed");
		}
		else
		{
			custom_log("[+] heart tick send %u bytes", length);
		}
	}
#else
    fast_send(fd, heart_buf, strlen(heart_buf));
#endif
}

int device_status_report()
 {
	int ret;
	
	ret = Jd_GlobalVar.dev_callback[JD_GET_DEVICE_STATUS](NULL);
	if(ret == -1)
		return -1;
	ret = jdssl_send((char *)Jd_GlobalVar.post_tx);
	if(ret <=0)
	{
	    //hfdbg_set_level(10);
        custom_log("[E] ssl upload send failed");
        //sleep(1);
        //hfdbg_set_level(0);
		//custom_log("[E] ssl upload send failed");
	}
	else
	{
		custom_log("[+] ssl upload send %u bytes", ret);
	}
	return ret;
 }
extern u16_t tcp_get_available(int s);

int jdssl_send(char *send_data)
{
	int ret, available;
	int retransmission =0;
	fd_set wfds;
    struct timeval tv;

	FD_ZERO(&wfds);
	if(ssl_data.fd != -1)
		FD_SET(ssl_data.fd, &wfds);

	tv.tv_sec = 3;
	tv.tv_usec = 0;

	do
	{
		switch(ret = select(ssl_data.fd +1,NULL,&wfds,NULL,&tv))
		{
         	case 0:
		 		return 0;
			
         	case (-1):
				//custom_log("[E]select wfds error:%d",ret);
		 		return -1;

			default:
				if(FD_ISSET(ssl_data.fd,&wfds))
				{
					available = tcp_get_available(ssl_data.fd);
					//custom_log("available:%d", available);
					
					ret = CyaSSL_write(ssl_data.ssl, send_data, strlen(send_data));
					if(ret <=0)
					{
						//custom_log("[E] CyaSSL_write failed, Erron:0x%02X",errno);
                        char errorString[80];
                        int err = CyaSSL_get_error(ssl_data.ssl, 0);
                        CyaSSL_ERR_error_string(err, errorString);
                        //hfdbg_set_level(10);
                        custom_log("%s", errorString);
                        //sleep(1);
                        //hfdbg_set_level(0);
						retransmission++;
						if(retransmission == 2)
						{
							ssl_close(&ssl_data);
							retransmission = 0;
							if(errno == 0x0B || errno == 0x6B)
								hfsys_softreset();
						}
						msleep(2000);
						
					}
					else
					{
						//custom_log("[+] CyaSSL_write %u bytes", ret);
						retransmission = 0;
					}
				}
		};
	}while(retransmission);
	return ret;
}

 /*****************************************************************************
 *	  Function Name       : jdssl_thread
 *	  Description            : 接收服务器下发包，发送心跳维持链路
 *	  INPUT	                : 	
 *      Output                  : 
 *
 *	  Returns	                :
 ****************************************************************************/
void jdssl_thread(void* arg)
{
	int recvlen, ret = 0;
	static uint32_t rhythm =0;
    static unsigned int total = 0;
	while(jdArgs.feedid[0] == '\0')  //Make sure device has been added.
	{
		custom_log("Initialization device,waiting.......");
		msleep(2000);
	}

	//hfthread_enable_softwatchdog(NULL,60*2);
	
	while(1)
	{
        if (1 == g_u32StopJdTask)
        {
        #if 1
            hfthread_destroy(NULL);
        #else
            msleep(2000);
            continue;
        #endif
        }
		if(Jd_GlobalVar.linkst_router == 0){            //Router power off.
			custom_log("Lost Router's signal,waiting.......");
			
			if(Jd_GlobalVar.linkst_cloud == 1){
				custom_log("[E] Network anomalies occur,close socket!!");
				ssl_close(&ssl_data);
			}
			msleep(1000);
			continue;
		}
		//Router wan shut down. close socket, keep reconected.
		else if((Jd_GlobalVar.linkst_internet == 0) && (Jd_GlobalVar.linkst_cloud == 1)){
			custom_log("Router is offline,waiting.......");
			ssl_close(&ssl_data);
			msleep(1000);
		}
		
		if(ssl_check(&ssl_data))
		{
			char ssl_server[50];
			(void)memcpy(ssl_server, jdArgs.iplist[0],strlen(jdArgs.iplist[0]));
			
			ret = get_ip_port(ssl_server);
			if(ret == -1)
			{
				custom_log("[E] Failed to get server info");
				msleep(1000);
				continue;
			}
			int fd = tcp_connect_ssl_server(Jd_GlobalVar.jdserver_ip, Jd_GlobalVar.jdserver_port);
			if(fd == -1){
				msleep(2000);
				continue;
			}
			else
				ssl_connect(fd, &ssl_data);
		}
		if(ssl_data.fd!=-1)
		{
		    if ((total++) % 10 == 0)
		        custom_log("task jd...");
			struct timeval tv;
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(ssl_data.fd, &readfds);
			tv.tv_sec = 0;
			tv.tv_usec = 1000;    //need longer;
			select(ssl_data.fd +1, &readfds, NULL,NULL,&tv);
			if(FD_ISSET(ssl_data.fd,&readfds))
			{
				recvlen = CyaSSL_read(ssl_data.ssl, Jd_GlobalVar.ssl_rx, sizeof(Jd_GlobalVar.ssl_rx)-1);
				if(recvlen<=0)
				{
					// 关闭端口
					ssl_close(&ssl_data);
					custom_log("[E] ctrl ssl recv error(%d)", recvlen);
				}
				else
				{
					Jd_GlobalVar.ssl_rx[recvlen] = 0;
					char* pbuf = (char *)Jd_GlobalVar.ssl_rx;
					httpdecode(pbuf, 0);         // urldecode
					JDCmdProcess(pbuf);				
				}				
			}
        #if 0
			if((hfthread_sem_wait(uart_psem, 1))>=0)
				device_status_report();
		#endif	
			if(((hfsys_get_time() - rhythm) >= 35000)||(hfsys_get_time()< rhythm)||(rhythm== 0))
			{
				rhythm = hfsys_get_time();
				heart_tick_report();
			}
		}
        msleep(100);
	}
}
#if 0
NetWdg_t NetWdg;
void jdnetwdg_thread(void* arg)
{
	JD_enable_netwatchdog(&NetWdg, 50);
	
	while(1)
	{
		JD_netwatchdog_check(&NetWdg);
		msleep(1000);
	}

}
#endif
int jdsmart_start()
{
	while(Jd_GlobalVar.linkst_router == 0){
		custom_log("Network is down,waiting.......");
		msleep(1000);
		continue;
	}

	//hfthread_create(jdnetwdg_thread, "jdnetwdg", 128, NULL, 1, NULL, NULL);
	//hfthread_create(jdudp_thread, "jdudp", 512, NULL, 1, NULL, NULL);
#if 1
	hfthread_create(jdssl_thread, "jdssl_thread", 1024, NULL, 1, NULL, NULL);
#endif	
	return 1;
}


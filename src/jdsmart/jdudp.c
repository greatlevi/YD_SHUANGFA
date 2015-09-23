#include <stdio.h>
#include "common.h"
#include "jdsmart.h"

extern u8 JD_PROUUID[7];

static cJSON* PacketAnalyse(cmd_header* *pCmd, char *pBuf, int length);
static int PacketBuild(char* pBuf, int enctype, int type, cJSON *root);

static int PacketBuild(char* pBuf, int enctype, int type, cJSON *root)
{
	common_header_t* pCommon = (common_header_t*)pBuf;
	pCommon->magic = 0x55AA;
	pCommon->enctype = 0;	// 加密类型,如果是0表示调试阶段不加密

	pBuf += sizeof(common_header_t);
	cmd_header* pCmd = (cmd_header*)(pBuf);
	pCmd->type = type;
	memcpy(pCmd->cmd, "OK", 2);

	char* pData = pBuf+sizeof(cmd_header);

	char* psJson = cJSON_PrintUnformatted(root);
	int length = strlen(psJson);
	memcpy(pData, psJson, length);
	hfmem_free(psJson);

	pCommon->len = length+sizeof(cmd_header);

	int i = 0;
	unsigned char sum =0;
	for(i=0; i<pCommon->len; i++)
	{
		sum+= *(pBuf+i);
	}
	pCommon->checksum = sum;
	return pCommon->len+ sizeof(common_header_t);
}


static cJSON* PacketAnalyse(cmd_header* *pCmd, char *pBuf, int length)
{
	common_header_t* pCommon = (common_header_t*)pBuf;
	if( (pCommon->magic&0xFFFFFF00)== 0x30303000)
	{
		static cmd_header Cmd;
		Cmd.type = (pCommon->magic&0xFF)-'0';
		*pCmd = &Cmd;
		cJSON* pRet = cJSON_Parse(pBuf+4);
		return pRet;
	}
	else if( (pCommon->magic != 0x55AA) || ( length!=(pCommon->len+ sizeof(common_header_t))) )
	{
		//sprintf(RetStr, "UPD Packet ERR: Magic:%x--Length:%d--Count:%d ", pCommon->magic, pCommon->len, length);
		return NULL;
	}
	pBuf +=  sizeof(common_header_t);
	int i = 0;
	char sum = 0;
	for(i=0; i<pCommon->len; i++)
	{
		sum +=  *(pBuf+i);
	}
	*pCmd = (cmd_header*)pBuf;
	pBuf +=  sizeof(cmd_header);
	if(pCommon->checksum != sum)
	{
		*pCmd = NULL;
		//sprintf(RetStr, "%s %x--%d--%d ", "456", pCommon->magic, pCommon->len, length);
		return NULL;
	}
	//custom_log("%s ---\n",  pBuf);
	cJSON* pRet = cJSON_Parse(pBuf);
	return pRet;
}

void app_search_device(char *rxMessage, int rLength, int ufd, struct sockaddr_in toaddr)
{
	int iStatus,len_udp_back;
	int sin_len=sizeof(toaddr);
	cmd_header* pCmd = NULL;
	
	cJSON* jDevice = PacketAnalyse(&pCmd, rxMessage, rLength);
	if(jDevice == NULL)
	{
		custom_log("PacketAnalyse error\n\r");
		return;
	}
	cJSON *root = cJSON_CreateObject();							    //注意用完必须释放内存
	if(root == NULL)
	{
		custom_log("cJSON_CreateObject failed");
		cJSON_Delete(jDevice);
		return;
	}
				
	switch(pCmd->type)
	{
		case 1:
			{
				int smtlk = 0;
				hfsys_nvm_read(0,(char *)&smtlk,4);
				if(!((hfsys_get_time() < 2000*60) && (smtlk == 1)))
				{
					smtlk = 0;
					hfsys_nvm_write(0,(char *)&smtlk,4);
					break;
				}	
				cJSON *pItem = cJSON_GetObjectItem(jDevice, "productuuid");
                //custom_log("uuid is %s, JD_PROUUID is %s", pItem->valuestring, JD_PROUUID);
				if( (strcmp(JD_PROUUID, pItem->valuestring) )||(pItem->valuestring[0]==0)||(strcmp("0", pItem->valuestring) ))
				{
					if((jdArgs.feedid[0]=='\0') || (jdArgs.feedid[0] == 0xFF))
					{
						cJSON_AddStringToObject(root,"feedid", "0");
						custom_log("Configuration file not exists");
					}
					else
					{
						cJSON_AddStringToObject(root,"feedid", jdArgs.feedid);
						custom_log("Configuration file exists");
					}
					cJSON_AddStringToObject(root,"mac", Jd_GlobalVar.mac);
					cJSON_AddStringToObject(root,"productuuid", JD_PROUUID);
								
					int length = PacketBuild(rxMessage, 2, 2, root);
					iStatus = sendto(ufd, rxMessage, length, 0,(struct sockaddr *)&toaddr, (socklen_t)&sin_len);
					custom_log("HeartBeat_OK");
					if(iStatus<0)
					{
						custom_log("sl_send1");
						return;
					}
				}				
				break;
			}
		case 3:
			{
				/* Description:
				  *       smtlk ok and device starts in 2 min, app can write.
				  */
				int smtlk = 0;
				hfsys_nvm_read(0,(char *)&smtlk,4);
				if(!((hfsys_get_time() < 2000*60) && (smtlk == 1)))
				{
					smtlk = 0;
					hfsys_nvm_write(0,(char *)&smtlk,4);
					break;
				}					
				cJSON *pItem = cJSON_GetObjectItem(jDevice,"feedid");
				strcpy(jdArgs.feedid, pItem->valuestring);
				pItem = cJSON_GetObjectItem(jDevice,"accesskey");
				strcpy(jdArgs.accesskey, pItem->valuestring);
				pItem = cJSON_GetObjectItem(jDevice,"server");
				
				int i =0;
				cJSON * serverip_list = pItem->child;
				while(serverip_list)
				{
					strcpy(jdArgs.iplist[i],serverip_list->valuestring);
					serverip_list = serverip_list->next;
					i++;
					custom_log("iplist[%d]:%s", i,jdArgs.iplist[i]);
				}
				memcpy(jdArgs.magic, "HF_JD", 5);
				hffile_userbin_write(JDCONFIG_OFFSET, (char*)&jdArgs, sizeof(jdNVargs_t));
			
				cJSON_AddNumberToObject(root,"code",0);
				cJSON_AddStringToObject(root,"msg","write feed_id and accesskey successfully!");
				len_udp_back = PacketBuild(rxMessage, 2, 4, root);
				iStatus = sendto(ufd, rxMessage,len_udp_back, 0,(struct sockaddr *)&toaddr, (socklen_t)&sin_len);
				char* p = cJSON_PrintUnformatted(jDevice);
				custom_log("WriteOK_%s",p);
				hfmem_free(p);
					
				if(iStatus<0)
				{
					custom_log("sl_send2");
				}							
				break;
			}	                //case 3:
		default:
				break;
	}		//switch(pCmd->type)
	cJSON_Delete(root);
	cJSON_Delete(jDevice);
}
#if 0
uint8_t rxMessage[600];
void jdudp_thread(void* arg)
{
	struct sockaddr_in myaddr;
	int sin_len;
	int ufd, maxfd;
	int ret;
	struct timeval tv;
	fd_set readfds;
	
	memset(&myaddr,0,sizeof(myaddr));
	myaddr.sin_family=AF_INET;
	myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	myaddr.sin_port=htons(80);
	sin_len=sizeof(myaddr);
	
	ufd = socket(AF_INET, SOCK_DGRAM, 0);
	if(ufd<0)
	{
		custom_log("[E]create udp socket fail");
	}

	hfnet_set_udp_broadcast_port_valid(80,8899);
	bind(ufd, (struct sockaddr*)&myaddr, sin_len);
	
	while(1)
	{
		maxfd = ufd;
	
		FD_ZERO(&readfds);
		FD_SET(ufd, &readfds);
		
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		ret = select(maxfd +1, &readfds, NULL, NULL, &tv);
		if(ret <= 0)
			continue;
		
		if(FD_ISSET(ufd,&readfds))
		{	
			int rLength = 0;
			memset(rxMessage,sizeof(rxMessage),0);
			if((rLength=recvfrom(ufd, rxMessage,sizeof(rxMessage),0,(struct sockaddr *)&myaddr, (socklen_t*)&sin_len))>=0)
				app_search_device(rxMessage,rLength, ufd, myaddr);
			else
				break;
		}
	}
}
#endif


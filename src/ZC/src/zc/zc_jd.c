#if 0

/**
******************************************************************************
* @file     zc_bc.c
* @authors  cxy
* @version  V1.0.0
* @date     10-Sep-2014
* @brief    broadcast
******************************************************************************
*/
#include <zc_jd.h>
#include <zc_protocol_controller.h>
int g_Jdfd = 0;
//jdNVargs_t jdArgs; 
static cJSON* ZC_JDPacketAnalyse(cmd_header* *pCmd, char *pBuf, int length);
static int ZC_JDPacketBuild(u8* pBuf, int enctype, int type, cJSON *root);
u8 JD_PROUUID[7] = {'0'};
/*************************************************
* Function: ZC_SendJdQueryReq
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
static int ZC_JDPacketBuild(u8* pBuf, int enctype, int type, cJSON *root)
{
	common_header_t* pCommon = (common_header_t*)pBuf;
	pCommon->magic = 0x55AA;
	pCommon->enctype = 0;	// 加密类型,如果是0表示调试阶段不加密

	pBuf += sizeof(common_header_t);
	cmd_header* pCmd = (cmd_header*)(pBuf);
	pCmd->type = type;
	memcpy(pCmd->cmd, "OK", 2);

	u8* pData = pBuf+sizeof(cmd_header);

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

/*************************************************
* Function: ZC_SendJdQueryReq
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
static cJSON* ZC_JDPacketAnalyse(cmd_header* *pCmd, char *pBuf, int length)
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
		return NULL;
	}
	cJSON* pRet = cJSON_Parse(pBuf);
	return pRet;
}


/*************************************************
* Function: ZC_SendJdQueryReq
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void ZC_SendJDQueryReq(u8 *pu8Msg, u32 u32RecvLen,struct sockaddr_in addr)
{
    int len_udp_back;
	cmd_header* pCmd = NULL;
	ZC_SendParam struParam;
    
	cJSON* jDevice = ZC_JDPacketAnalyse(&pCmd, (char *)pu8Msg, u32RecvLen);
	if(jDevice == NULL)
	{
		ZC_Printf("PacketAnalyse error\n\r");
		return;
	}
	cJSON *root = cJSON_CreateObject();							    //注意用完必须释放内存
	if(root == NULL)
	{
		ZC_Printf("cJSON_CreateObject failed");
		cJSON_Delete(jDevice);
		return;
	}
				
	switch(pCmd->type)
	{
		case 1:
			{
				//int smtlk = 0;
                u8 DeviceId[ZC_HS_DEVICE_ID_LEN+1];
                u8 *pu8DeviceId;
                #if 0
				hfsys_nvm_read(0,(char *)&smtlk,4);
				if(!((hfsys_get_time() < 2000*60) && (smtlk == 1)))
				{
					smtlk = 0;
					hfsys_nvm_write(0,(char *)&smtlk,4);
					break;
				}
				#endif
				cJSON *pItem = cJSON_GetObjectItem(jDevice, "productuuid");
				if( (strcmp((const char *)JD_PROUUID, (const char *)pItem->valuestring) )||(pItem->valuestring[0]==0)||(strcmp("0", (const char *)pItem->valuestring) ))
				{
					if(ZC_MAGIC_FLAG!=g_struZcConfigDb.struJdInfo.u32MagicFlag)
					{
						cJSON_AddStringToObject(root,"feedid", "0");
						ZC_Printf("Configuration file not exists");
					}
					else
					{
						cJSON_AddStringToObject(root,"feedid", (const char *)g_struZcConfigDb.struJdInfo.u8Feedid);
						ZC_Printf("Configuration file exists");
					}

                    ZC_GetStoreInfor(ZC_GET_TYPE_DEVICEID, &pu8DeviceId);
                    memcpy(DeviceId, pu8DeviceId, ZC_HS_DEVICE_ID_LEN);
                    DeviceId[ZC_HS_DEVICE_ID_LEN] = 0;
					cJSON_AddStringToObject(root,"mac", (const char *)DeviceId);
					cJSON_AddStringToObject(root,(const char *)"productuuid", (const char *)JD_PROUUID);
								
					int length = ZC_JDPacketBuild(pu8Msg, 2, 2, root);
                    struParam.u8NeedPoll = 0;
                    struParam.pu8AddrPara = (u8 *)&addr;
                    g_struProtocolController.pstruMoudleFun->pfunSendUdpData(g_Jdfd, pu8Msg, length, &struParam); 
                    ZC_Printf("HeartBeat_OK\n");
				}				
				break;
			}
		case 3:
			{		
				cJSON *pItem = cJSON_GetObjectItem(jDevice,"feedid");
                ZC_StoreFeedInfo((u8 *)pItem->valuestring);
				
				cJSON_AddNumberToObject(root,"code",0);
				cJSON_AddStringToObject(root,"msg","write feed_id and accesskey successfully!");
				len_udp_back = ZC_JDPacketBuild(pu8Msg, 2, 4, root);
                struParam.u8NeedPoll = 0;
                struParam.pu8AddrPara = (u8*)&addr;
                g_struProtocolController.pstruMoudleFun->pfunSendUdpData(g_Jdfd, pu8Msg, len_udp_back, &struParam); 
				char* p = cJSON_PrintUnformatted(jDevice);
				ZC_Printf("WriteOK_%s",p);
				hfmem_free(p);							
				break;
			}	             
		default:
				break;
	}		
	cJSON_Delete(root);
	cJSON_Delete(jDevice);  
}
#endif
/******************************* FILE END ***********************************/



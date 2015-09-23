/**
******************************************************************************
* @file     zc_hf_adpter.c
* @authors  cxy
* @version  V1.0.0
* @date     10-Sep-2014
* @brief    Event
******************************************************************************
*/
#include <zc_protocol_controller.h>
#include <zc_timer.h>
#include <zc_module_interface.h>
#include <hsf.h>
#include <zc_hf_adpter.h>
#include <stdlib.h>
//#ifdef WIFI_ON_MCU
#include <Ac_cfg.h>
#include <ac_api.h>
u8  g_u8EqVersion[]={0,0,0,0};      
u8  g_u8ModuleKey[ZC_MODULE_KEY_LEN] =DEFAULT_IOT_PRIVATE_KEY;
u64  g_u64Domain;
u8  g_u8DeviceId[ZC_HS_DEVICE_ID_LEN];
//#endif
u16 SUB_DOMAIN_ID	= 0xFFFF;//����id

extern PTC_ProtocolCon  g_struProtocolController;
PTC_ModuleAdapter g_struHfAdapter;

MSG_Buffer g_struRecvBuffer;
MSG_Buffer g_struRetxBuffer;
MSG_Buffer g_struClientBuffer;


MSG_Queue  g_struRecvQueue;
//MSG_Buffer g_struSendBuffer[MSG_BUFFER_SEND_MAX_NUM];
MSG_Buffer g_struSendBuffer[1];

MSG_Queue  g_struSendQueue;

u8 g_u8MsgBuildBuffer[MSG_BULID_BUFFER_MAXLEN];

u8 g_u8ClientSendLen = 0;


u16 g_u16TcpMss;
u16 g_u16LocalPort;


u8 g_u8recvbuffer[HF_MAX_SOCKET_LEN];

ZC_UartBuffer g_struUartBuffer;
HF_TimerInfo g_struHfTimer[ZC_TIMER_MAX_NUM];
hfthread_mutex_t g_struTimermutex;
u8  g_u8BcSendBuffer[100];
u8  g_u8JdRcvBuffer[256];
u32 g_u32BcSleepCount = 800;
struct sockaddr_in struRemoteAddr;
u8 JD_PROUUID[7];
int g_Jdfd;

/*************************************************
* Function: HF_ReadDataFormFlash
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_ReadDataFormFlash(void) 
{
    u32 u32MagicFlag = 0xFFFFFFFF;
#ifdef __LPT200__
    hffile_userbin_read(0, (char *)(&u32MagicFlag), 4);
    if (ZC_MAGIC_FLAG == u32MagicFlag)
    {   
        hffile_userbin_read(0, (char *)(&g_struZcConfigDb), sizeof(ZC_ConfigDB));
    }
    else
    {
        ZC_Printf("no para, use default\n");
    }
#else
    hfuflash_read(0, (char *)(&u32MagicFlag), 4);
    if (ZC_MAGIC_FLAG == u32MagicFlag)
    {   
        hfuflash_read(0, (char *)(&g_struZcConfigDb), sizeof(ZC_ConfigDB));
    }
    else
    {
        ZC_Printf("no para, use default\n");
    }
#endif    

}

/*************************************************
* Function: HF_WriteDataToFlash
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_WriteDataToFlash(u8 *pu8Data, u16 u16Len)
{
#ifdef __LPT200__
    hffile_userbin_write(0, (char*)pu8Data, u16Len);
#else
    hfuflash_erase_page(0,1); 
    hfuflash_write(0, (char*)pu8Data, u16Len);
#endif
}

/*************************************************
* Function: HF_timer_callback
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void USER_FUNC HF_timer_callback(hftimer_handle_t htimer) 
{
    u8 u8TimeId;
    hfthread_mutext_lock(g_struTimermutex);
    u8TimeId = hftimer_get_timer_id(htimer);
    if (1 == g_struHfTimer[u8TimeId].u32FirstFlag)
    {
        g_struHfTimer[u8TimeId].u32FirstFlag = 0;
        hftimer_start(htimer);
    }
    else
    {
        TIMER_TimeoutAction(u8TimeId);
        TIMER_StopTimer(u8TimeId);
    }
    
    hfthread_mutext_unlock(g_struTimermutex);
}
/*************************************************
* Function: HF_StopTimer
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_StopTimer(u8 u8TimerIndex)
{
    hftimer_stop(g_struHfTimer[u8TimerIndex].struHandle);
    hftimer_delete(g_struHfTimer[u8TimerIndex].struHandle);
}

/*************************************************
* Function: HF_SetTimer
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_SetTimer(u8 u8Type, u32 u32Interval, u8 *pu8TimeIndex)
{
    u8 u8TimerIndex;
    u32 u32Retval;

    u32Retval = TIMER_FindIdleTimer(&u8TimerIndex);
    if (ZC_RET_OK == u32Retval)
    {
        TIMER_AllocateTimer(u8Type, u8TimerIndex, (u8*)&g_struHfTimer[u8TimerIndex]);
        g_struHfTimer[u8TimerIndex].struHandle = hftimer_create(NULL,u32Interval,false,u8TimerIndex,
             HF_timer_callback,0);
        g_struHfTimer[u8TimerIndex].u32FirstFlag = 1;
        hftimer_start(g_struHfTimer[u8TimerIndex].struHandle);  
        
        *pu8TimeIndex = u8TimerIndex;
    }
    
    return u32Retval;
}
/*************************************************
* Function: HF_FirmwareUpdateFinish
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_FirmwareUpdateFinish(u32 u32TotalLen)
{
    int retval;
    retval = hfupdate_complete(HFUPDATE_SW, u32TotalLen);
    if (HF_SUCCESS == retval)
    {
        return ZC_RET_OK;
    }
    else
    {
        //hfdbg_set_level(10);
        ZC_Printf("hfupdate_complete error is %d\n", retval);
        //sleep(3);
        //hfdbg_set_level(0);
        return ZC_RET_ERROR;    
    }
}


/*************************************************
* Function: HF_FirmwareUpdate
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_FirmwareUpdate(u8 *pu8FileData, u32 u32Offset, u32 u32DataLen)
{
    int retval;
    if (0 == u32Offset)
    {
        hfupdate_start(HFUPDATE_SW);
    }
    
    retval = hfupdate_write_file(HFUPDATE_SW, u32Offset, (char *)pu8FileData, u32DataLen); 
    if (retval < 0)
    {
        return ZC_RET_ERROR;
    }
    
    return ZC_RET_OK;
}
/*************************************************
* Function: HF_SendDataToMoudle
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_SendDataToMoudle(u8 *pu8Data, u16 u16DataLen)
{
//#ifdef WIFI_ON_MCU
    AC_RecvMessage((ZC_MessageHead *)pu8Data);
/*
#else
    u8 u8MagicFlag[4] = {0x02,0x03,0x04,0x05};
    hfuart_send(HFUART0,(char*)u8MagicFlag,4,1000); 
    hfuart_send(HFUART0,(char*)pu8Data,u16DataLen,1000);
#endif
*/
    return ZC_RET_OK;
}

/*************************************************
* Function: HF_Rest
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Rest(void)
{
 #if 0   
    g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig = 0;            
    HF_WriteDataToFlash((u8 *)&g_struZcConfigDb, sizeof(ZC_ConfigDB));
    msleep(500);
    hfsmtlk_start();
#endif
}
/*************************************************
* Function: HF_SendTcpData
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SendTcpData(u32 u32Fd, u8 *pu8Data, u16 u16DataLen, ZC_SendParam *pstruParam)
{
    send(u32Fd, pu8Data, u16DataLen, 0);
}
/*************************************************
* Function: HF_SendUdpData
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SendUdpData(u32 u32Fd, u8 *pu8Data, u16 u16DataLen, ZC_SendParam *pstruParam)
{
    sendto(u32Fd,(char*)pu8Data,u16DataLen,0,
        (struct sockaddr *)pstruParam->pu8AddrPara,
        sizeof(struct sockaddr_in)); 
}

/*************************************************
* Function: HF_CloudRecvfunc
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
USER_FUNC static void HF_CloudRecvfunc(void* arg) 
{
    s32 s32RecvLen=0; 
    fd_set fdread;
    u32 u32Index;
    u32 u32Len=0; 
    u32 u32ActiveFlag = 0;
    struct sockaddr_in cliaddr;
    int connfd;
    extern u8 g_u8ClientStart;
    u32 u32MaxFd = 0;
    struct timeval timeout; 
    struct sockaddr_in addr;
    int tmp=1;    
    s32 s32ret = 0;
    //u32 u32i;
    
    while(1) 
    {
        //ZC_StartClientListen();

        u32ActiveFlag = 0;
        
        timeout.tv_sec= 0; 
        timeout.tv_usec= 1000; 
        
        FD_ZERO(&fdread);

        FD_SET(g_Bcfd, &fdread);
        u32MaxFd = u32MaxFd > g_Bcfd ? u32MaxFd : g_Bcfd;

        FD_SET(g_Jdfd, &fdread);
        u32MaxFd = u32MaxFd > g_Jdfd ? u32MaxFd : g_Jdfd;
#if 0
        if (PCT_INVAILD_SOCKET != g_struProtocolController.struClientConnection.u32Socket)
        {
            FD_SET(g_struProtocolController.struClientConnection.u32Socket, &fdread);
            u32MaxFd = u32MaxFd > g_struProtocolController.struClientConnection.u32Socket ? u32MaxFd : g_struProtocolController.struClientConnection.u32Socket;
            u32ActiveFlag = 1;
        }
#endif        
        if ((g_struProtocolController.u8MainState >= PCT_STATE_WAIT_ACCESSRSP) 
        && (g_struProtocolController.u8MainState < PCT_STATE_DISCONNECT_CLOUD))
        {
            FD_SET(g_struProtocolController.struCloudConnection.u32Socket, &fdread);
            u32MaxFd = u32MaxFd > g_struProtocolController.struCloudConnection.u32Socket ? u32MaxFd : g_struProtocolController.struCloudConnection.u32Socket;
            u32ActiveFlag = 1;
        }

#if 0

        for (u32Index = 0; u32Index < ZC_MAX_CLIENT_NUM; u32Index++)
        {
            if (0 == g_struClientInfo.u32ClientVaildFlag[u32Index])
            {
                FD_SET(g_struClientInfo.u32ClientFd[u32Index], &fdread);
                u32MaxFd = u32MaxFd > g_struClientInfo.u32ClientFd[u32Index] ? u32MaxFd : g_struClientInfo.u32ClientFd[u32Index];
                u32ActiveFlag = 1;            
            }
        }

#endif 

        if (0 == u32ActiveFlag)
        {
            continue;
        }
        
        s32ret = select(u32MaxFd + 1, &fdread, NULL, NULL, &timeout);
        if(s32ret<=0)
        {
           continue; 
        }
        if ((g_struProtocolController.u8MainState >= PCT_STATE_WAIT_ACCESSRSP) 
        && (g_struProtocolController.u8MainState < PCT_STATE_DISCONNECT_CLOUD))
        {
            if (FD_ISSET(g_struProtocolController.struCloudConnection.u32Socket, &fdread))
            {
                s32RecvLen = recv(g_struProtocolController.struCloudConnection.u32Socket, g_u8recvbuffer, HF_MAX_SOCKET_LEN, 0); 
                
                if(s32RecvLen > 0) 
                {
                    //ZC_Printf("recv data len = %d\n", s32RecvLen);
                    
                    MSG_RecvDataFromCloud(g_u8recvbuffer, s32RecvLen);
                }
                else
                {
                    ZC_Printf("recv error, len = %d\n",s32RecvLen);
                    PCT_DisConnectCloud(&g_struProtocolController);
                    
                    g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
                    g_struUartBuffer.u32RecvLen = 0;
                }
            }
            
        }

#if 0
        for (u32Index = 0; u32Index < ZC_MAX_CLIENT_NUM; u32Index++)
        {
            if (0 == g_struClientInfo.u32ClientVaildFlag[u32Index])
            {
                if (FD_ISSET(g_struClientInfo.u32ClientFd[u32Index], &fdread))
                {
                    s32RecvLen = recv(g_struClientInfo.u32ClientFd[u32Index], g_u8recvbuffer, HF_MAX_SOCKET_LEN, 0); 
                    if (s32RecvLen > 0)
                    {
                        ZC_RecvDataFromClient(g_struClientInfo.u32ClientFd[u32Index], g_u8recvbuffer, s32RecvLen);
                    }
                    else
                    {   
                        ZC_ClientDisconnect(g_struClientInfo.u32ClientFd[u32Index]);
                        close(g_struClientInfo.u32ClientFd[u32Index]);
                    }
                    
                }
            }
            
        }

        if (PCT_INVAILD_SOCKET != g_struProtocolController.struClientConnection.u32Socket)
        {
            if (FD_ISSET(g_struProtocolController.struClientConnection.u32Socket, &fdread))
            {
                connfd = accept(g_struProtocolController.struClientConnection.u32Socket,(struct sockaddr *)&cliaddr,&u32Len);

                if (ZC_RET_ERROR == ZC_ClientConnect((u32)connfd))
                {
                    close(connfd);
                }
                else
                {
                    ZC_Printf("accept client = %d\n", connfd);
                }
            }
        }

        if (FD_ISSET(g_Bcfd, &fdread))
        {
            tmp = sizeof(addr); 
            s32RecvLen = recvfrom(g_Bcfd, g_u8BcSendBuffer, 100, 0, (struct sockaddr *)&addr, (socklen_t*)&tmp); 
            if(s32RecvLen > 0) 
            {
                ZC_SendClientQueryReq(g_u8BcSendBuffer, (u16)s32RecvLen);
            } 
        }
#endif

        if(FD_ISSET(g_Jdfd, &fdread))
        {
            tmp = sizeof(addr); 
            s32RecvLen = recvfrom(g_Jdfd, g_u8JdRcvBuffer, sizeof(g_u8JdRcvBuffer), 0, (struct sockaddr *)&addr, (socklen_t*)&tmp); 
            if(s32RecvLen >=0) 
            {
                app_search_device(g_u8JdRcvBuffer, s32RecvLen, g_Jdfd, addr);
                //ZC_SendJDQueryReq(g_u8JdRcvBuffer, s32RecvLen, addr);
            } 
        }
        msleep(100);
    } 
}
/*************************************************
* Function: HF_GetMac
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_GetMac(u8 *pu8Mac)
{
#if 0
    char rsp[64]={0};
    char *mac[3]={0};
    memset(rsp, 0, sizeof(rsp));
    hfat_send_cmd("AT+WSMAC\r\n", sizeof("AT+WSMC\r\n"), rsp, 64);
    ZC_Printf("AT+WSMAC's response:%s\n",rsp);
    hfat_get_words(rsp, mac, 3);
    memcpy(pu8Mac,mac[1],ZC_SERVER_MAC_LEN);
#endif
}

/*************************************************
* Function: HF_Reboot
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Reboot(void)
{
    msleep(500);
    hfsys_reset();
}

/*************************************************
* Function: HF_ConnectToCloud
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ConnectToCloud(PTC_Connection *pstruConnection)
{
    int fd; 
    struct sockaddr_in addr;
    struct ip_addr struIp;
    int retval;
    u16 port;
    memset((char*)&addr,0,sizeof(addr));
    if (1 == g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig)
    {
        port = g_struZcConfigDb.struSwitchInfo.u16ServerPort;
        struIp.addr = htonl(g_struZcConfigDb.struSwitchInfo.u32ServerIp);
        retval = HF_SUCCESS;
    }
    else
    {
        port = ZC_CLOUD_PORT;
        retval = hfnet_gethostbyname((const char *)g_struZcConfigDb.struCloudInfo.u8CloudAddr, &struIp);
    }


    if (HF_SUCCESS != retval)
    {
        ZC_Printf("can't get ip, ret is %d\n", retval);
        return ZC_RET_ERROR;
    }
    
    ZC_Printf("connect ip = 0x%x!\n",struIp.addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr=struIp.addr;
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd<0)
        return ZC_RET_ERROR;
    
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))< 0)
    {
        close(fd);
        if(g_struProtocolController.struCloudConnection.u32ConnectionTimes++>20)
        {
           g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig = 0;
        }

        return ZC_RET_ERROR;
    }
    g_struProtocolController.struCloudConnection.u32ConnectionTimes = 0;

    ZC_Printf("connect ok!\n");
    g_struProtocolController.struCloudConnection.u32Socket = fd;

    
    ZC_Rand(g_struProtocolController.RandMsg);

    return ZC_RET_OK;
}
/*************************************************
* Function: HF_ListenClient
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ListenClient(PTC_Connection *pstruConnection)
{
#if 0
    int fd; 
    struct sockaddr_in servaddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd<0)
        return ZC_RET_ERROR;

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port = htons(pstruConnection->u16Port);
    if(bind(fd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
    {
        close(fd);
        return ZC_RET_ERROR;
    }
    
    if (listen(fd, TCP_DEFAULT_LISTEN_BACKLOG)< 0)
    {
        close(fd);
        return ZC_RET_ERROR;
    }

    ZC_Printf("Tcp Listen Port = %d\n", pstruConnection->u16Port);
    g_struProtocolController.struClientConnection.u32Socket = fd;

    return ZC_RET_OK;
#endif
}

/*************************************************
* Function: HF_BcInit
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_BcInit()
{
    int tmp=1;
    struct sockaddr_in addr; 

    addr.sin_family = AF_INET; 
    addr.sin_port = htons(ZC_MOUDLE_PORT); 
    addr.sin_addr.s_addr=htonl(INADDR_ANY);

    g_Bcfd = socket(AF_INET, SOCK_DGRAM, 0); 

    tmp=1; 
    setsockopt(g_Bcfd, SOL_SOCKET,SO_BROADCAST,&tmp,sizeof(tmp)); 

    hfnet_set_udp_broadcast_port_valid(ZC_JD_PORT,  ZC_MOUDLE_PORT + 1);

    bind(g_Bcfd, (struct sockaddr*)&addr, sizeof(addr)); 
    g_struProtocolController.u16SendBcNum = 0;

    memset((char*)&struRemoteAddr,0,sizeof(struRemoteAddr));
    struRemoteAddr.sin_family = AF_INET; 
    struRemoteAddr.sin_port = htons(ZC_MOUDLE_BROADCAST_PORT); 
    struRemoteAddr.sin_addr.s_addr=inet_addr("255.255.255.255"); 
    g_pu8RemoteAddr = (u8*)&struRemoteAddr;
    g_u32BcSleepCount = 250000;

    return;
}
/*************************************************
* Function: HF_JdInit
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_JdInit()
{
    struct sockaddr_in addr; 
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET; 
    addr.sin_port = htons(ZC_JD_PORT); 
    addr.sin_addr.s_addr=htonl(INADDR_ANY);

    g_Jdfd = socket(AF_INET, SOCK_DGRAM, 0); 

    bind(g_Jdfd, (struct sockaddr*)&addr, sizeof(addr)); 
    return;
}

/*************************************************
* Function: HF_Cloudfunc
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
USER_FUNC static void HF_Cloudfunc(void* arg) 
{
    int fd;
    u32 u32Timer = 0;

    HF_BcInit();
    HF_JdInit();

    while(1) 
    {
        //ZC_Printf("enter task1\n");
        fd = g_struProtocolController.struCloudConnection.u32Socket;
        PCT_Run();
        
        if (PCT_STATE_DISCONNECT_CLOUD == g_struProtocolController.u8MainState)
        {
            close(fd);
            if (0 == g_struProtocolController.struCloudConnection.u32ConnectionTimes)
            {
                u32Timer = 1000;
            }
            else
            {
                u32Timer = rand();
                u32Timer = (PCT_TIMER_INTERVAL_RECONNECT) * (u32Timer % 10 + 1);
            }
            PCT_ReconnectCloud(&g_struProtocolController, u32Timer);
            g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
            g_struUartBuffer.u32RecvLen = 0;
        }
        else
        {
            MSG_SendDataToCloud((u8*)&g_struProtocolController.struCloudConnection);
        }
        ZC_SendBc();
        msleep(100);
    } 
}

/*************************************************
* Function: HF_Init
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Init(void *arg)
{
    u32 u32MagicFlag = 0xFFFFFFFF;
#if 0
    while(1)
    {
        hffile_userbin_read(300, (char *)(&u32MagicFlag), 4);
        if (ZC_MAGIC_FLAG == u32MagicFlag)
        {   
            hffile_userbin_read(304, (char *)(&SUB_DOMAIN_ID), sizeof(u64));
            break;       
        }
        msleep(200);
    }
#endif
    SUB_DOMAIN_ID = 287;
    if (287 == SUB_DOMAIN_ID)
    {
        memcpy(JD_PROUUID, "5IUP6T", 7);
    }
    else
    {
        memcpy(JD_PROUUID, "EGYHST", 7);
    }
	g_u64Domain= ((((u64)((SUB_DOMAIN_ID & 0xff00) >> 8)) << 48) + (((u64)(SUB_DOMAIN_ID & 0xff)) << 56) + (((u64)MAJOR_DOMAIN_ID & 0xff) << 40) + ((((u64)MAJOR_DOMAIN_ID & 0xff00) >> 8) << 32)
	+ ((((u64)MAJOR_DOMAIN_ID & 0xff0000) >> 16) << 24)
	+ ((((u64)MAJOR_DOMAIN_ID & 0xff000000) >> 24) << 16)
	+ ((((u64)MAJOR_DOMAIN_ID & 0xff00000000) >> 32) << 8)
	+ ((((u64)MAJOR_DOMAIN_ID & 0xff0000000000) >> 40) << 0));
	char mac_buf[16];
    ZC_Printf("MT Init\n");
    g_struHfAdapter.pfunConnectToCloud = HF_ConnectToCloud;
    g_struHfAdapter.pfunListenClient = HF_ListenClient;
    g_struHfAdapter.pfunSendTcpData = HF_SendTcpData;   
    g_struHfAdapter.pfunUpdate = HF_FirmwareUpdate;     
    g_struHfAdapter.pfunUpdateFinish = HF_FirmwareUpdateFinish;
    g_struHfAdapter.pfunSendToMoudle = HF_SendDataToMoudle;  
    g_struHfAdapter.pfunSetTimer = HF_SetTimer;   
    g_struHfAdapter.pfunStopTimer = HF_StopTimer;
    
    g_struHfAdapter.pfunRest = HF_Rest;
    g_struHfAdapter.pfunWriteFlash = HF_WriteDataToFlash;
    g_struHfAdapter.pfunSendUdpData = HF_SendUdpData;   
    g_struHfAdapter.pfunGetMac = HF_GetMac;
    g_struHfAdapter.pfunReboot = HF_Reboot;
    
    g_u16TcpMss = 1000;
    PCT_Init(&g_struHfAdapter);

    g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
    g_struUartBuffer.u32RecvLen = 0;
    HF_ReadDataFormFlash();
    hfthread_create(HF_Cloudfunc,"HF_Cloudfunc",256,NULL,HFTHREAD_PRIORITIES_LOW,NULL,NULL); 
    hfthread_create(HF_CloudRecvfunc,"HF_CloudRecvfunc",256,NULL,HFTHREAD_PRIORITIES_LOW,NULL,NULL); 
    hfthread_mutext_new(&g_struTimermutex);
    memset(g_u8DeviceId,'0',16);
    memset(mac_buf,'\0',16);
    hfat_send_cmd("AT+WSMAC\r\n",strlen("AT+WSMAC\r\n"),mac_buf,16);
    memcpy(g_u8DeviceId,mac_buf+4,12);
    memcpy(g_struRegisterInfo.u8PrivateKey, g_u8ModuleKey, ZC_MODULE_KEY_LEN);
    memcpy(g_struRegisterInfo.u8DeviciId, g_u8DeviceId, ZC_HS_DEVICE_ID_LEN);
    memcpy(g_struRegisterInfo.u8DeviciId + ZC_HS_DEVICE_ID_LEN, &g_u64Domain, ZC_DOMAIN_LEN);
    memcpy(g_struRegisterInfo.u8EqVersion, g_u8EqVersion, ZC_EQVERSION_LEN);
//for test 
    g_struZcConfigDb.struSwitchInfo.u32SecSwitch = 1;
    g_struZcConfigDb.struSwitchInfo.u32TraceSwitch = 0;
    g_struZcConfigDb.struSwitchInfo.u32WifiConfig = 0;
    g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig = 0;
    memcpy(g_struZcConfigDb.struCloudInfo.u8CloudAddr, "device.ablecloud.cn", ZC_CLOUD_ADDR_MAX_LEN);
    //ZC_Printf("MT Init end1\n");
    hfthread_destroy(NULL);
    //ZC_Printf("MT Init end2\n");
    return;
//#endif

}

/*************************************************
* Function: HF_WakeUp
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_WakeUp()
{
    PCT_WakeUp();
}
/*************************************************
* Function: HF_Sleep
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Sleep()
{
    u32 u32Index;
    
    close(g_Bcfd);

    if (PCT_INVAILD_SOCKET != g_struProtocolController.struClientConnection.u32Socket)
    {
        close(g_struProtocolController.struClientConnection.u32Socket);
        g_struProtocolController.struClientConnection.u32Socket = PCT_INVAILD_SOCKET;
    }

    if (PCT_INVAILD_SOCKET != g_struProtocolController.struCloudConnection.u32Socket)
    {
        close(g_struProtocolController.struCloudConnection.u32Socket);
        g_struProtocolController.struCloudConnection.u32Socket = PCT_INVAILD_SOCKET;
    }
#if 0
    for (u32Index = 0; u32Index < ZC_MAX_CLIENT_NUM; u32Index++)
    {
        if (0 == g_struClientInfo.u32ClientVaildFlag[u32Index])
        {
            close(g_struClientInfo.u32ClientFd[u32Index]);
            g_struClientInfo.u32ClientFd[u32Index] = PCT_INVAILD_SOCKET;
        }
    }
#endif
    PCT_Sleep();
    
    g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
    g_struUartBuffer.u32RecvLen = 0;
}

/******************************* FILE END ***********************************/



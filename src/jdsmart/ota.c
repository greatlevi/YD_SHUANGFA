#if 1

#include <stdio.h>
#include <hsf.h>
#include "common.h"
#include "jdsmart.h"
#include <httpc/httpc.h>
#include <md5.h>

void USER_FUNC update_timer_callback( hftimer_handle_t htimer )
{
	if(hfgpio_fpin_is_high(HFGPIO_F_NREADY))
		hfgpio_fset_out_low(HFGPIO_F_NREADY);
	else
		hfgpio_fset_out_high(HFGPIO_F_NREADY);
}

int URLEncode(const char* str, const int strSize, char* result, const int resultSize)
{
    int i;
    int j = 0;//for result index
    char ch;

    if ((str==NULL) || (result==NULL) || (strSize<=0) || (resultSize<=0)) {
        return 0;
    }

    for ( i=0; (i<strSize)&&(j<resultSize); ++i) {
        ch = str[i];
        if (((ch>='A') && (ch<'Z')) ||
            ((ch>='a') && (ch<'z')) ||
            ((ch>='0') && (ch<'9'))) {
            result[j++] = ch;
        } else if (ch == ' ') {
            result[j++] = '+';
        } else if (ch == '.' || ch == '-' || ch == '_' || ch == '*') {
            result[j++] = ch;
        } else {
            if (j+3 < resultSize) {
                sprintf(result+j, "%%%02X", (unsigned char)ch);
                j += 3;
            } else {
                return 0;
            }
        }
    }

    result[j] = '\0';
    return j;
}

int encode_signature(char *url)
{
	char *head = url;
	char *sig = strstr(url, "Signature");
	if(NULL == sig)
		return -1;

	int len = sig-head + 10;
	memset(Jd_GlobalVar.ssl_tx, '\0', sizeof(Jd_GlobalVar.ssl_tx));
	MEMCPY(Jd_GlobalVar.ssl_tx, head, len);

	sig +=10;
	char signature[50]={0};
	URLEncode(sig, strlen(sig), signature, sizeof(signature));
	strcat((char *)Jd_GlobalVar.ssl_tx, signature);
	return 1;
}
int Httpc_get(char *purl, int *dl_status)
{
	httpc_req_t  http_req;
	char *content_data=NULL;
	char *temp_buf=NULL;
	parsed_url_t url={0};
	http_session_t hhttp=0;
	int total_size,read_size=0;
	int rv=0;
	tls_init_config_t  *tls_cfg=NULL;
	
	rv = encode_signature(purl);
	if(rv == -1)
	{
		*dl_status = DOWNLOAD_FAIL;
		custom_log("Signature urlencode failed\r\n");
		return -1;
	}

	memset(Jd_GlobalVar.ssl_tx, 0, sizeof(Jd_GlobalVar.ssl_tx));
	uint8_t * test_url = Jd_GlobalVar.ssl_tx;
	custom_log("Download url:%s\n", Jd_GlobalVar.ssl_tx);
	
	hftimer_handle_t upg_timer=NULL;
	struct MD5Context md5_ctx;
	uint8_t digest[16]={0};
	
	bzero(&http_req,sizeof(http_req));
	http_req.type = HTTP_GET;
	http_req.version=HTTP_VER_1_1;
	
	if((temp_buf = (char*)hfmem_malloc(256))==NULL)
	{
		custom_log("no memory\n");
		//rv= -HF_E_NOMEM;
		rv = DOWNLOAD_FAIL;
		goto exit;
	}	
	bzero(temp_buf,sizeof(temp_buf));
	
	if((rv=hfhttp_parse_URL((const char *)test_url,temp_buf , 256, &url))!=HF_SUCCESS)
	{
		rv = DOWNLOAD_FAIL;
		goto exit;
	}

	if((rv=hfhttp_open_session(&hhttp,(const char *)test_url,0,tls_cfg,3))!=HF_SUCCESS)
	{
		custom_log("http open fail\n");
		rv = DOWNLOAD_FAIL;
		goto exit;
	}

	hfsys_disable_all_soft_watchdogs();
	hfupdate_start(HFUPDATE_SW);
	http_req.resource = url.resource;
	hfhttp_prepare_req(hhttp,&http_req,HDR_ADD_CONN_CLOSE);
	//hfhttp_add_header(hhttp,"Range","bytes=0");
	if((rv=hfhttp_send_request(hhttp,&http_req))!=HF_SUCCESS)
	{
		custom_log("http send request fail\n");
		rv = DOWNLOAD_FAIL;
		goto exit;
	}
	
	content_data = (char*)hfmem_malloc(512);
	if(content_data==NULL)
	{
		//rv= -HF_E_NOMEM;
		rv = DOWNLOAD_FAIL;
		goto exit;
	}
	total_size=0;
	bzero(content_data,512);

	if((upg_timer = hftimer_create("UPG-TIMER",100,true,1,update_timer_callback,0))==NULL)
	{
		custom_log("create timer 1 fail\n");
		rv = DOWNLOAD_FAIL;
		goto exit;
	}
	
	hftimer_start(upg_timer);
	MD5Init(&md5_ctx);
	while((read_size=hfhttp_read_content(hhttp,content_data,512))>0)
	{
		//hfthread_reset_softwatchdog(NULL);
		hfupdate_write_file(HFUPDATE_SW, total_size,content_data, read_size);
		MD5Update(&md5_ctx,(uint8_t*)content_data,read_size);
		total_size+=read_size;
		//custom_log("download file:[%d] [%d]\r",total_size,read_size);
	}
	MD5Final(digest,&md5_ctx);
	custom_log("read_size:%d digest is ",total_size);
	custom_log("%02x%02x%02x%02x",digest[0],digest[1],digest[2],digest[3]);
	custom_log("%02x%02x%02x%02x",digest[4],digest[5],digest[6],digest[7]);
	custom_log("%02x%02x%02x%02x",digest[8],digest[9],digest[10],digest[11]);
	custom_log("%02x%02x%02x%02x\n",digest[12],digest[13],digest[14],digest[15]);

	if(total_size <= 0)
		*dl_status = DOWNLOAD_FAIL;
	else
		*dl_status = DOWNLOAD_OK;
	
	if(hfupdate_complete(HFUPDATE_SW,total_size)!=HF_SUCCESS)
	{
		rv = UPGRADE_FAIL;
		custom_log("update software failed\n");
	}
	else
	{
		rv = UPGRADE_OK;
		custom_log("update software successfully\n");
	}
exit:
	if(upg_timer!=NULL)
	{
		hftimer_delete(upg_timer);
		hftimer_delete(upg_timer);
	}
	if(temp_buf!=NULL)	
		hfmem_free(temp_buf);
	if(content_data!=NULL)
		hfmem_free(content_data);
	if(hhttp!=0)
		hfhttp_close_session(&hhttp);
	hfgpio_fset_out_low(HFGPIO_F_NREADY);
	return rv;
}

#endif

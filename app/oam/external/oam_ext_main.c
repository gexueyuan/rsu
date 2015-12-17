/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_ext_main.c
 @brief  : oam externel main fuction
 @author : wanglei
 @history:
           2015-12-7    wanglei    Created file
           ...
******************************************************************************/
#include "oam_cmn.h"
#include "oam_sdk_cmn.h"
#include "oam_ext_sdk.h"
#include "oam_ext_raw.h"
#include "oam_ext_main.h"
#include "test_sdk_msg.h"

#define CLI_OAM_SERVER_IP "127.0.0.1"
oam_client_t g_oam_client;

extern cv_sdk_data_order_func_t   g_sdk_xlat_table[];
extern cv_sdk_data_handler_func_t g_sdk_handler_table[];
extern cv_sdk_data_order_func_t   g_evt_xlat_table[];

void oam_buf_print(uint8_t *pbuf , uint32_t len)
{
    uint32_t i , j=0 ,k;

    printf("================================================");
    for(i = 0 ; i < len ; i++) {
        if(i%16 == 0) {
            j = 0;
            printf("\r\n");
        }
        printf("%02x ",*((uint8_t *)(pbuf+i)));

        if(++j == 8)
            printf(" ");
    }

    printf("\r\n\n");
    return;
}


void cv_oam_proc(uint8_t* pdu, uint32_t len)
{
    cv_oam_frame_t * frame = (cv_oam_frame_t*) pdu;
    cv_sdk_msg_t * sdk_msg = (cv_sdk_msg_t*) frame->data;
    cv_sdk_evt_msg_t *evt_msg = (cv_sdk_evt_msg_t*) frame->data;
    cv_raw_pkt_msg_t * pkt_msg = (cv_raw_pkt_msg_t*) frame->data; 

    switch (oam_ntohs(frame->frame_type))
    {
        case CV_OAM_FRAME_TYPE_CMD_MSG:
            switch(oam_ntohs(frame->layer2_type.cmd_type)){
                case CV_OAM_L2_MSG_CMD_RSP:
                    cv_oam_sdk_api_rsp(sdk_msg);
                    break;
                default:
                    break;    
            }
            break;
            
        case CV_OAM_FRAME_TYPE_EVENT_MSG:
            //cv_oam_sdk_evt_proc(evt_msg);            
            break;
        case CV_OAM_FRAME_TYPE_RAW_DATA:
            //cv_oam_raw_pkt_proc(pkt_msg);
            break;

        case CV_OAM_FRAME_TYPE_KEEPALIVE:

            break;
            
        default:
            break;
    }
}


oam_status cv_oam_client_init(oam_client_t *p_client, char * srv_ip, int srv_port)
{
    OAM_ASSERT_RET(srv_ip != NULL, OAM_E_PARAM);
    
    int sockfd = -1;
	int len = 0;
	struct sockaddr_in srv_addr;
	int ret;
    
	/* tcp server info */
    bzero(&srv_addr,sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(srv_ip);
    srv_addr.sin_port = htons(srv_port);
                      
    /* create TCPsocket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
		fprintf(stderr, "Failed to create socket: %s.\n", strerror(errno));
        return OAM_E_SOCKET_ERROR;
    }
 
	len = sizeof(srv_addr);

    /* connect to server */
	ret = connect(sockfd, (struct sockaddr*)&srv_addr, len);
	if(ret < 0){
		fprintf(stderr, "Failed to connect server: %s.\n", strerror(errno));
        close(sockfd);
        return OAM_E_SOCKET_ERROR;
	}

    p_client->sock_fd = sockfd;

    return ret;
}
        


void *cv_oam_proc_thread(void *param)
{
    uint8_t buf[CV_OAM_PKT_MAX_LEN];
    int len;
    oam_client_t *p_client = (oam_client_t *)param;

    while(1)
    {
        memset(buf, 0x00, CV_OAM_PKT_MAX_LEN);
        len = recv(p_client->sock_fd, buf, CV_OAM_PKT_MAX_LEN, 0);

        if(len <= 0 && errno != EINTR) {
            OAM_LOG("server is break down. errno=%d\n", errno);
            close(p_client->sock_fd);
#if 0
            sleep(10);
            /* reconnect to server */
            cv_oam_client_init(&g_oam_client, CLI_OAM_SERVER_IP, OAM_SERVER_PORT);
#endif
        }
        else if(len > 0){
            cv_oam_proc(buf, len);
        }
        else {
            /* len = 0 && errno != EINTR,  peer tcp is down */
            OAM_LOG("oam recv failed, len %d, errno %d\n", len, errno);
       }
        
    }
}


int cv_oam_client_send_msg(uint16_t frame_type, uint16_t msg_type, uint8_t * msg, uint16_t msg_len)
{
    uint8_t            pkt[CV_OAM_PKT_MAX_LEN] = {0};
    cv_oam_frame_t *  frame = (cv_oam_frame_t *)pkt;
    int ret = OAM_OK;

    OAM_ASSERT_RET(msg != NULL, OAM_E_PARAM);

    if(msg_len > CV_OAM_PKT_MAX_LEN - CV_FRAME_HEADER_LEN){
        return OAM_E_PARAM;
    }

    frame->frame_type = oam_htons(frame_type);
    frame->layer2_type.cmd_type = oam_htons(msg_type);

    memcpy(frame->data, msg, msg_len);

    ret = send(g_oam_client.sock_fd, pkt, msg_len + CV_FRAME_HEADER_LEN, 0);
    if (ret < 0) {
        fprintf(stderr, "send to server failed: %s\r\n", strerror(errno));
        return -1;
    }
   
    return OAM_E_OK;
}

int cv_oam_init()
{
    cv_oam_init_info_t info;
    int ret = OAM_E_OK;
    int  thread_id;
   
    ret = cv_oam_client_init(&g_oam_client, CLI_OAM_SERVER_IP, OAM_SERVER_PORT);
    if(ret != OAM_E_OK){
        OAM_LOG("init port failed \n");
        return ret;
    }

    info.sdk_xlats = g_sdk_xlat_table;
    info.sdk_num = CV_SDK_REQ_TYPE_END;
    info.evt_num = 0;
    info.evt_xlats = NULL;

    cv_oam_sdk_init(info.sdk_xlats, info.sdk_num);
    //cv_oam_sdk_evt_init(info.evt_xlats,  info.evt_num);

    ret = rc_pthread_create(
            &thread_id, 
            CV_OAM_PROC_THREAD_NAME, 
            CV_OAM_PROC_THREAD_PRIORITY,
            cv_oam_proc_thread, (void *)&g_oam_client);
    if(ret != 0){
        OAM_LOG("create thread failed, ret = %d \n", ret);
        return OAM_E_CREATE_THREAD;
    }

    return OAM_E_OK;

}

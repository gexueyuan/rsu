/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : oam_int_main.c
 @brief  : rsu  oam main
 @author : wanglei
 @history:
           2015-12-3    wanglei    Created file
           ...
******************************************************************************/
#include "oam_cmn.h"
#include "oam_sdk_cmn.h"
//#include "oam_int_evt.h"
#include "oam_int_main.h"
#include "oam_int_sdk.h"

#include "test_sdk_msg.h"


extern cv_sdk_data_order_func_t   g_sdk_xlat_table[];
extern cv_sdk_data_handler_func_t g_sdk_handler_table[];
extern cv_sdk_data_order_func_t   g_evt_xlat_table[];

oam_server_t g_oam_srv;
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

void cv_oam_server_init(oam_server_t *srv)
{
    OAM_ASSERT(srv);
    memset(srv, 0x0, sizeof(oam_server_t));
    srv->client_cnt = 0;
    srv->srv_fd = -1;
    srv->srv_port = OAM_SERVER_PORT;
}

void cv_oam_server_free_client(int clientfd)
{
    int i = 0;
    oam_server_t *p_srv = &g_oam_srv;
    for (i=0; i<OAM_MAX_CLIENT_NUM; i++) {
        if (p_srv->client[i].valid == 1 && p_srv->client[i].sock_fd == clientfd) {
            p_srv->client[i].sock_fd = -1;
            p_srv->client[i].valid = 0;
            p_srv->client_cnt -= 1;
        }
    }
}

void cv_oam_server_add_client(int clientfd, 	struct sockaddr_in client_addr)
{
    int i = 0;
    oam_server_t *p_srv = &g_oam_srv;
    for (i=0; i<OAM_MAX_CLIENT_NUM; i++) {
        if(p_srv->client[i].sock_fd == clientfd){
            break;
        }
        else if (p_srv->client[i].valid == 0) {
            p_srv->client[i].sock_fd = clientfd;
            memcpy(&p_srv->client[i].client_addr, &client_addr, sizeof(client_addr));
            p_srv->client_cnt += 1;
            break;
        }
    }
}
void *cv_oam_proc_thread(void * param)
{
    int ret = 0;
    oam_server_t * p_srv = (oam_server_t *)param;
    int server_sockfd = -1;
	int client_sockfd = -1;
	int client_len = 0;
    pthread_t thread_id;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
    char name[20] = {0};
    int yes = 1;
    
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        (void) fprintf(stderr, "create srv socket failed: %s\r\n", strerror(errno));
    }

	/* 设置服务器接收的连接地址和监听的端口 */
    bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = oam_htonl(INADDR_ANY); //接受所有IP地址的连接
	server_addr.sin_port = htons(p_srv->srv_port);

    //设置为可重复使用
    if(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))== -1){
        perror("setsockopt SO_REUSEADDR failed.");
    }
    //绑定（命名）套接字
	ret = bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        (void) fprintf(stderr, "srv socket bind failed: %s\r\n", strerror(errno));
    }
    
	//创建套接字队列，监听套接字 
	ret = listen(server_sockfd, 20);
    if (ret < 0) {
        perror("call to listen");
    }
#if 0
	//忽略子进程停止或退出信号
	signal(SIGCHLD, SIG_IGN);
#endif
	
	while(1)
	{
		client_len = sizeof(client_addr);
		printf("Server waiting\n");
		//接受连接，创建新的套接字
		if (p_srv->client_cnt > OAM_MAX_CLIENT_NUM) {
            OAM_LOG("The number of clients has reached the maximum\r\n");
            sleep(10);
		    continue;
		}
		client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);

	    //新线程，读取客户端发过来的信息，处理信息，再发送应答给客户端
        if (client_sockfd > 0) {
            memset(name, 0, sizeof(name));
            sprintf(name, "tk_oam_rx_%d", client_sockfd);
            os_pthread_create(&thread_id, 
                                name, 
                                CV_OAM_RX_THREAD_PRIORITY, 
                                CV_OAM_RX_THREAD_STACK_SIZE,
                                cv_oam_rx_thread,
                                &client_sockfd);
            cv_oam_server_add_client(client_sockfd, client_addr);
        }
	}
}


void *cv_oam_rx_thread(void *arg)
{
    int sockfd = *(int *)arg;
    uint8_t buf[CV_OAM_PKT_MAX_LEN] = {0};
    int32_t len;
    /* the main loop */
    while (1) {
        memset(buf, 0x00, sizeof(buf));          
        len = recv(sockfd, buf, CV_OAM_PKT_MAX_LEN, 0);
        
        if(len <= 0 && errno != EINTR){
            OAM_LOG("peer socket is break down\n");
            cv_oam_server_free_client(sockfd);
            close(sockfd);
            return;
        }
        else if (len > 0) {
            //oam_buf_print(buf, len);
            cv_oam_frame_proc(arg, buf, len);        
        }
        else {
            continue;            
        }
    }
}


void cv_oam_frame_proc(void *src, uint8_t * pdu, uint32_t len)
{
    cv_oam_frame_t * frame = (cv_oam_frame_t*) pdu;
    cv_sdk_msg_t * sdk_msg = (cv_sdk_msg_t*) frame->data;
    cv_sdk_evt_ack_t * ack = (cv_sdk_evt_ack_t*) frame->data;
    uint16_t type = oam_ntohs(frame->layer2_type.raw_data_type);

    switch (oam_htons(frame->frame_type))
    {
        case CV_OAM_FRAME_TYPE_CMD_MSG:
            if(CV_OAM_L2_MSG_CMD_REQ == oam_ntohs(frame->layer2_type.cmd_type)){
                cv_oam_sdk_msg_handler(src, sdk_msg);
            }
            else {
                /* should not receive command response message */
                return OAM_E_NOT_SUPPORT;
            }
            break;
        case CV_OAM_FRAME_TYPE_EVENT_MSG:
            if(CV_OAM_L2_MSG_EVENT_ACK == oam_ntohs(frame->layer2_type.evt_type)){
                /* event ack  */
                //cv_oam_sdk_evt_ack_proc(ack);
            }
            break;
       case CV_OAM_FRAME_TYPE_RAW_DATA:
            cv_oam_raw_data_proc(type, frame->data, len-CV_FRAME_HEADER_LEN);
            break;
        case CV_OAM_FRAME_TYPE_KEEPALIVE:
            //cv_oam_hello_proc(pdu, len);
            break;
        default:
            break;
    }
}



void cv_oam_init()
{
    int ret = 0; 
    oam_server_t *p_srv = &g_oam_srv;

    cv_oam_server_init(p_srv);
    
    cv_oam_sdk_init(g_sdk_xlat_table, g_sdk_handler_table, CV_SDK_REQ_TYPE_END);

    int  thread_id;
    
    ret = os_pthread_create(
            &thread_id, 
            CV_OAM_PROC_THREAD_NAME, 
            CV_OAM_PROC_THREAD_PRIORITY,
            CV_OAM_PROC_THREAD_STACK_SIZE,
            cv_oam_proc_thread, (void *)p_srv);

    if(ret != 0){
        OAM_LOG("start thread failed \n");
        return OAM_E_CREATE_THREAD;
    }
}

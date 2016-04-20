/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_wnet.h
 @brief  : The definition of wireless network layer
 @author : wangyf
 @history:
           2014-12-8    wangyf    Created file
           ...
******************************************************************************/
#ifndef __CV_WNET_H__
#define __CV_WNET_H__

#include "list.h"
#include "cv_osal.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#define WNET_TEST_MODE         (1)

#define LLC_ETHERTYPE_IPv4    0x0800
#define LLC_ETHERTYPE_IPv6    0x86DD
#define LLC_ETHERTYPE_WSMPv1  0x88DC
#define LLC_ETHERTYPE_DSMPv1  0x88E2
#define LLC_ETHERTYPE_DSAv1   0x88E4

#define LLC_SAP 0xAA

#define MACADDR_LENGTH 6
#define IPADDR_LENGTH  4

typedef union {
    struct {
        uint8_t addr[MACADDR_LENGTH];
    } mac;
    struct {
        uint8_t addr[MACADDR_LENGTH];
        uint32_t aid;
    } dsmp;
    struct {
        uint8_t addr[IPADDR_LENGTH];
        uint16_t port;
    } udp;
} wnet_addr_t;


#define WNET_TRANS_PROT_DSMP 0
#define WNET_TRANS_PROT_UDP  1

#define WNET_TRANS_ENCRYPT_NONE 0
#define WNET_TRANS_ENCRYPT_ENABLE 1

#define WNET_TRANS_RRORITY_NORMAL 0
#define WNET_TRANS_RRORITY_EMERGENCY 1


/**
 * The infomation that are necessary for transmitting message, all contents should be 
 * filled by VAM and consumed by WNET.
 */ 
typedef struct _wnet_txinfo {
    wnet_addr_t dest;

    uint8_t protocol; /* 0 - DSMP, 1 - UDP */
    uint8_t encryption; /* 0 - none, 1 - encpypted */
    uint8_t prority;  /* 0 - NORMAL, 1 - EMERGENCY */
    uint32_t timestamp; /* time of generated message */

    void *extension; /* reserved for future */
} wnet_txinfo_t;

/**
 * The infomation that is getted from received message, all contents should be 
 * filled by WNET and consumed by VAM.
 */ 
typedef struct _wnet_rxinfo {
    wnet_addr_t src;

    uint8_t protocol; /* 0 - DSMP, 1 - UDP */
    uint8_t encryption; /* 0 - none, 1 - encpypted */
    uint8_t rssi;  
    uint32_t timestamp; /* time of received message */

    void *extension; /* reserved for future */
} wnet_rxinfo_t;

#define TXBUF_LENGTH     512//256
#define RXBUF_LENGTH     512//256               

#define TXBUF_NUM        1
#define RXBUF_NUM        1

#define TXBUF_FLAG_NONE        0x0000
#define TXBUF_FLAG_PROCESSING  0x0001

#define TXBUF_RESERVE_LENGTH (20)  /* LLC, SEC, DSMP */

typedef struct _wnet_txbuf {
    /**
     * DO NOT MODIFY IT
     */
    list_head_t list;
    wnet_txinfo_t info;
    /**
     * END
     */

    uint32_t flag;   /* buffer's status */
    
    uint8_t *data_ptr;
    int32_t data_len;

    uint8_t buffer[TXBUF_LENGTH];
}wnet_txbuf_t;


typedef struct _wnet_rxbuf {
    /**
     * DO NOT MODIFY IT
     */
    #define RXBUF_PTR(txinfo) (struct _wnet_rxbuf *)((uint32_t)rxinfo - sizeof(list_head_t))
    list_head_t list;
    wnet_rxinfo_t info;
    /**
     * END
     */

    uint8_t *data_ptr;
    int32_t data_len;

    uint8_t buffer[RXBUF_LENGTH];
}wnet_rxbuf_t;

#define WNET_TXBUF_DATA_PTR(txbuf) txbuf->data_ptr
#define WNET_TXBUF_INFO_PTR(txbuf) &txbuf->info
#define WNET_TXBUF_PTR(info) (struct _wnet_txbuf *)((uint32_t)info - sizeof(list_head_t))
#define WNET_RXBUF_PTR(info) (struct _wnet_rxbuf *)((uint32_t)info - sizeof(list_head_t))

typedef struct _wnet_llc_header {
    uint8_t dsap;
    uint8_t ssap;
    uint8_t control;
    uint8_t ouiid[3];
    uint16_t ether_type;
} wnet_llc_header_t;

#define WNET_LLC_HEADER_LEN  sizeof(wnet_llc_header_t)


typedef struct _wnet_config {
    uint8_t channel;
    uint8_t txrate;
    uint8_t mode;  /* 0:normal, 1:qc test */
} wnet_config_t;


/**
 * The Envar struct of Wireless Network Layer  
 *
 */
typedef struct _wnet_envar {
    void *global;
     //��

    /* working_param */
    wnet_config_t working_param;

    /**
     * Frame process
     */
    osal_task_t *task_wnet_tx;
    osal_task_t *task_wnet_rx;
#if 0
    osal_sem_t *sem_wnet_tx;
    osal_sem_t *sem_wnet_rx;
    list_head_t txbuf_free_list;
    list_head_t txbuf_waiting_list;
    list_head_t rxbuf_free_list;
    list_head_t rxbuf_waiting_list;
    wnet_txbuf_t txbuf[TXBUF_NUM];
    wnet_rxbuf_t rxbuf[RXBUF_NUM];
#endif
     //to be added ��
     osal_queue_t * queue_frame_rx;
     osal_queue_t * queue_frame_tx;

     /* �շ���ͳ��timer */
     osal_timer_t * timer_stat;
   
} wnet_envar_t;

/**
 * Declare the extern functions
 *
 */
wnet_txbuf_t *wnet_get_txbuf(void);
void wnet_release_txbuf(wnet_txbuf_t *txbuf);

wnet_rxbuf_t *wnet_get_rxbuf(void);
void wnet_release_rxbuf(wnet_rxbuf_t *rxbuf);

int wnet_recv(wnet_rxinfo_t *rxinfo, uint8_t *pdata, uint32_t length);
int wnet_send(wnet_txinfo_t *txinfo, uint8_t *pdata, uint32_t length);
void wnet_send_complete(void);

int dsmp_send(wnet_envar_t *p_wnet, wnet_txinfo_t *txinfo, uint8_t *pdata, uint32_t length);
int dsmp_recv(wnet_envar_t *p_wnet, wnet_rxinfo_t *rxinfo, uint8_t *pdata, uint32_t length);

int llc_send(wnet_envar_t *p_wnet, wnet_txinfo_t *txinfo, uint8_t *pdata, uint32_t length);
int llc_recv(wnet_envar_t *p_wnet, wnet_rxinfo_t *rxinfo, uint8_t *pdata, uint32_t length);

int fp_send(wnet_envar_t *p_wnet, wnet_txinfo_t *txinfo, uint8_t *pdata, uint32_t length);
int fp_recv(wnet_envar_t *p_wnet, wnet_rxinfo_t *rxinfo, uint8_t *pdata, uint32_t length);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __CV_WNET_H__ */


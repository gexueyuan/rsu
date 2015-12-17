/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_wnet_fp.c
 @brief  : This file realizes the frame process
 @author : wangyf
 @history:
           2014-12-8    wangyf    Created file
           ...
******************************************************************************/
#include "cv_osal.h"
#define OSAL_MODULE_DEBUG
#define OSAL_MODULE_DEBUG_LEVEL OSAL_DEBUG_INFO
#define MODULE_NAME "fp"
#include "cv_osal_dbg.h"

#include "cv_vam.h"
#include "cv_cms_def.h"
#include "cv_wnet.h"
#include "cv_drv_vnet.h"

extern wnet_envar_t *p_wnet_envar;
extern float rcp_dbg_distance;

/*****************************************************************************
 * declaration of variables and functions                                    *
*****************************************************************************/
int wnet_dbg_trx_all = 0;
int wnet_dbg_rx_actual = 0;
int wnet_dbg_rx_fresh = 0;
int wnet_dbg_cacul_peroid = 10;  /* unit: second */
int wnet_dbg_cacul_cnt = 0;

/*****************************************************************************
 * implementation of functions                                               *
*****************************************************************************/
void test_comm(void)
{
    int rx_ratio;

	  if (++wnet_dbg_cacul_cnt >= wnet_dbg_cacul_peroid*10) {
        rx_ratio = wnet_dbg_rx_actual*100/wnet_dbg_cacul_cnt;
        osal_printf("\r\n[RX] Max=%d Act=%d Ratio=%d%% dis=%d\r\n\r\n", wnet_dbg_cacul_cnt,\
                    wnet_dbg_rx_actual, rx_ratio, (int)rcp_dbg_distance);

        wnet_dbg_cacul_cnt = 0;
        wnet_dbg_rx_actual = 0;
    }
		
    if (wnet_dbg_rx_fresh > 0) {
        if(wnet_dbg_rx_fresh > 1){
            osal_printf_unbuf("\b");
        }
        do {
            osal_printf_unbuf(".");
        } while ((--wnet_dbg_rx_fresh > 0));
        wnet_dbg_rx_fresh = 0;
    }
    else {
        osal_printf_unbuf(" ");
    }

}


/**
 * Put the TXBUF into waiting list only.
 * When the prority is EMERGENCY, the element is inserted to the front of 
 * the first NORMAL one.
 */
int fp_send(wnet_envar_t *p_wnet, wnet_txinfo_t *txinfo, uint8_t *pdata, uint32_t length)
{
    int ret = 0;
    ret = drv_vnet_send(txinfo, pdata, length);
    if (ret < 0) {
        osal_printf("drv send failed\r\n");
        wnet_release_txbuf(WNET_TXBUF_PTR(txinfo));
        return -1;
    }
    
    wnet_release_txbuf(WNET_TXBUF_PTR(txinfo));
    return 0;
}


/**
 * Allocate a RXBUF and put it into waiting list.
 */
int fp_recv(wnet_envar_t *p_wnet, wnet_rxinfo_t *rxinfo, uint8_t *pdata, uint32_t length)
{
    wnet_rxbuf_t *rxbuf;

    rxbuf = wnet_get_rxbuf();
    if (!rxbuf){
        OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_WARN, "failed to get rxbuf\n");
        return -1;
    }

    memcpy(&rxbuf->info, rxinfo, sizeof(wnet_rxinfo_t));
    memcpy(rxbuf->buffer, pdata, length);
    rxbuf->data_ptr = rxbuf->buffer;
    rxbuf->data_len = length;

    llc_recv(p_wnet, rxinfo, rxbuf->data_ptr, rxbuf->data_len);
    return 0;
}

wnet_txbuf_t *wnet_get_txbuf(void)
{
    wnet_txbuf_t *txbuf = NULL;
    txbuf = (wnet_txbuf_t *)malloc(sizeof(wnet_txbuf_t));
    memset(txbuf, 0, sizeof(wnet_txbuf_t));
    txbuf->data_ptr = txbuf->buffer + TXBUF_RESERVE_LENGTH + drv_vnet_mac_header_len();
    txbuf->data_len = 0;
    return txbuf;   
}

wnet_rxbuf_t *wnet_get_rxbuf(void)
{
    wnet_rxbuf_t *rxbuf = NULL;
    rxbuf = (wnet_rxbuf_t *)malloc(sizeof(wnet_rxbuf_t));
    memset(rxbuf, 0, sizeof(wnet_rxbuf_t));
    rxbuf->data_ptr = rxbuf->buffer;
    rxbuf->data_len = 0;
    return rxbuf;   
}

void wnet_release_txbuf(wnet_txbuf_t *txbuf)
{
    free(txbuf);
}

void wnet_release_rxbuf(wnet_rxbuf_t *rxbuf)
{
    free(rxbuf);
}


int wnet_send(wnet_txinfo_t *txinfo, uint8_t *pdata, uint32_t length)
{
    int r; 
    wnet_envar_t *p_wnet = p_wnet_envar;
    
    switch (txinfo->protocol) {
    case WNET_TRANS_PROT_DSMP:
        r = dsmp_send(p_wnet, txinfo, pdata, length);
        break;
    
    default:
        r = -1;
        break;
    }

    return r;
}

int wnet_recv(wnet_rxinfo_t *rxinfo, uint8_t *pdata, uint32_t length)
{
    
    if (g_dbg_print_type){
        wnet_dbg_rx_fresh++;
        wnet_dbg_rx_actual++;
    }
    //osal_printf_unbuf("."); /* Indicate RX is in process, for debug only */

    return fp_recv(p_wnet_envar, rxinfo, pdata, length);
}


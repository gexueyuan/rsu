/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_drv_wifi.h
 @brief  : cv_drv_wifi.c header file
 @author : wanglei
 @history:
           2015-11-25    wanglei    Created file
           ...
******************************************************************************/
#ifndef __CV_DRV_WIFI_H__
#define __CV_DRV_WIFI_H__

#include <stdbool.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <endian.h>

#include "nl80211.h"
#include "ieee80211.h"

#include "cv_osal.h"
#include "cv_wnet.h"


/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
#  define nl_sock nl_handle
#endif


struct nl80211_state {
	struct nl_sock *nlsock;
	int nl80211_id;
};

typedef struct _drv_wifi_config {
    uint8_t channel;
    uint8_t txrate;
} drv_wifi_config_t;


typedef struct _drv_wifi_envar {
    drv_wifi_config_t working_param;
    struct nl80211_state nl_state;
	int dev_ifidx;
    uint8_t dev_flag;             /* 1: IFF_UP,  0:down */
    uint8_t mac[MACADDR_LENGTH];
	int monitor_sock;
	int monitor_refcount;    
} drv_wifi_envar_t;

enum command_identify_by {
	CIB_NONE,
	CIB_PHY,
	CIB_NETDEV,
	CIB_WDEV,
};

enum id_input {
	II_NONE,
	II_NETDEV,
	II_PHY_NAME,
	II_PHY_IDX,
	II_WDEV,
};

extern drv_wifi_envar_t g_wifi_envar; 

int drv_vnet_init(wnet_envar_t *p_wnet);
int drv_vnet_deinit(void);
int drv_vnet_send(wnet_txinfo_t *txinfo, uint8_t *pdata, int32_t length);
int drv_vnet_recv(wnet_rxinfo_t *rxinfo, uint8_t *pdata, int32_t *length);
int drv_vnet_mac_header_len(void);


int nl80211_init(struct nl80211_state *state);
int drv_wifi_dev_config();
int drv_wifi_create_monitor_socket(drv_wifi_envar_t *drv);

int mac_addr_a2n(unsigned char *mac_addr, char *arg);
void mac_addr_n2a(char *mac_addr, unsigned char *arg);
int ieee80211_channel_to_frequency(int chan, enum nl80211_band band);
int ieee80211_frequency_to_channel(int freq);

#endif /* __CV_DRV_WIFI_H__ */

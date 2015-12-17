/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_drv_wifi.c
 @brief  : 
 @author : wanglei
 @history:
           2015-11-20    wanglei    Created file
           ...
******************************************************************************/
#include "platform.h"
#include "ieee80211.h"
#include "cv_drv_wifi.h"
#include "radiotap_iter.h"

extern uint8_t zero_macaddr[6];
static uint8_t BroadcastAddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint8_t BssidForV2V[] = {0x00, 0x63, 0x73, 0x76, 0x32, 0x76};
static uint8_t BeaconFixedElement[] = 
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* timestamp */
    /* Reseverd */
    0x64, 0x00, /* beacon interval */
    0x22, 0x04, /* CapabilityInfo: IBSS, short preamble, short slot time */
    0x00, 0x00,
};
#define MAC_BEACON_FIX_LENGTH (sizeof(BeaconFixedElement))
#define COPY_MAC_ADDR(addr1, addr2) memcpy((addr1), (addr2), MACADDR_LENGTH)

drv_wifi_envar_t g_wifi_envar; 


int drv_vnet_mac_header_len(void)
{
    return  IEEE80211_HDRLEN + sizeof(BeaconFixedElement);
}

int drv_vnet_send(wnet_txinfo_t *txinfo, uint8_t *pdata, int32_t length)
{
    struct ieee80211_hdr *p_hdr_80211;
    uint8_t *pPayload;
    drv_wifi_envar_t *p_wifi = &g_wifi_envar;

    pPayload = pdata - MAC_BEACON_FIX_LENGTH;
    
    /* fill the beacon fixed element */
    memcpy(pPayload, BeaconFixedElement, MAC_BEACON_FIX_LENGTH);

    /* fill the 802.11 header */
    p_hdr_80211 = (struct ieee80211_hdr *)(pPayload - IEEE80211_HDRLEN);
    memset(p_hdr_80211, 0, IEEE80211_HDRLEN);
    
    p_hdr_80211->fc.Type = BTYPE_MGMT;
    p_hdr_80211->fc.SubType = SUBTYPE_BEACON;

    COPY_MAC_ADDR(p_hdr_80211->IEEE80211_DA_FROMDS, BroadcastAddr);
    COPY_MAC_ADDR(p_hdr_80211->IEEE80211_SA_FROMDS, p_wifi->mac);
    COPY_MAC_ADDR(p_hdr_80211->IEEE80211_BSSID_FROMDS, BssidForV2V);

    if (p_wifi->monitor_sock) {
        return nl80211_send_monitor(p_wifi, (uint8_t *)p_hdr_80211, 
                             length+MAC_BEACON_FIX_LENGTH+IEEE80211_HDRLEN, 1);
    }
    else {
        return -1;
    }
}

int drv_vnet_recv(wnet_rxinfo_t *rxinfo, uint8_t *pdata, int32_t *length)
{
    drv_wifi_envar_t *p_wifi = &g_wifi_envar;
	unsigned char buf[2048];
    int len;
	int datarate = 0, ssi_signal = 0;
	struct ieee80211_radiotap_iterator iter;
    int radiotap_len = 0;
    struct ieee80211_hdr *hdr;
    int hdr_len = sizeof(struct ieee80211_hdr);
    while (1){
    	len = recv(p_wifi->monitor_sock, buf, sizeof(buf), 0);
    	if (len < 0) {
    		printf("nl80211: Monitor socket recv failed: %s\r\n",
    			   strerror(errno));
    		return -1;
    	}
        
    	if (ieee80211_radiotap_iterator_init(&iter, (void *) buf, len, NULL)) {
    		osal_printf("nl80211: received invalid radiotap frame\r\n");
    		return -1;
    	}

        radiotap_len = iter._max_length;
        hdr = (struct ieee80211_hdr *)(buf + radiotap_len);
        if ((memcmp(hdr->addr3, BssidForV2V, sizeof(BssidForV2V))==0) && (memcmp(hdr->addr2, p_wifi->mac, MACADDR_LENGTH) != 0)){
            *length = len - radiotap_len;
#if 0
            monitor_buf_print(hdr, *length);  
#endif

            /* Parse the the element of V2V */
            #define SKIP_LEN 14  /* TimestampLLC  + Reserve */
            uint8_t * pPayload;
            uint8_t ElementID, ElementLen;

            pPayload = buf + radiotap_len + hdr_len;
            *length -= hdr_len;
            
            pPayload += SKIP_LEN; /* skip fixed element in the beacon */
            
            ElementID = *pPayload++;
            ElementLen = *pPayload++;
            /* obu frame */
            if (ElementID == 0xDD) {
               /* move LLC header */ 
               pPayload -= 8;
               memcpy(pPayload, buf + radiotap_len + hdr_len, 8); 
               /* reduce 'reserved' length */
               *length -= 8;
            }
            /* rsu frame |80211header|Timestrmp|Reserved|LLC+Dsmp+rcp| */
            if(ElementID == 0xaa){
                pPayload -= 2;
                *length -= SKIP_LEN;
            }

            memcpy(pdata, pPayload, *length);
            memcpy(rxinfo->src.mac.addr, hdr->addr2, MACADDR_LENGTH);
            
            return 0;
        }
    }

}

/*****************************************************************************
 @funcname: drv_vnet_init
 @brief   : wifi init
 @param   : None
 @return  : 
*****************************************************************************/
int drv_vnet_init(wnet_envar_t *p_wnet)
{
    drv_wifi_envar_t *p_wifi = &g_wifi_envar;
	char buf[IFNAMSIZ];
	snprintf(buf, IFNAMSIZ, "%s", IF_NAME);
	buf[IFNAMSIZ - 1] = '\0';
	p_wifi->dev_ifidx = if_nametoindex(buf);
    p_wifi->working_param.channel = 13;
        
    nl80211_init(&p_wifi->nl_state);
    drv_wifi_dev_config(p_wifi);
    
    /* creater monitor socket to receive frame */
    drv_wifi_create_monitor_socket(p_wifi);
    
}

int drv_vnet_deinit(  )
{
    drv_wifi_envar_t *p_wifi = &g_wifi_envar;
    p_wifi->dev_ifidx = -1;
    nl80211_cleanup(&p_wifi->nl_state);    
}


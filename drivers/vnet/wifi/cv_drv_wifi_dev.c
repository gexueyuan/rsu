/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : test.c
 @brief  : test iw monitor
 @author : wanglei
 @history:
           2015-11-20    wanglei    Created file
           ...
******************************************************************************/
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <endian.h>

#include "platform.h"
#include "radiotap.h"
#include "radiotap_iter.h"
#include "nl80211.h"
#include "ieee80211.h"

#include "cv_drv_wifi.h"

int iw_debug = 0;
#define MAX_TXPOWER_MBM  (30*100)

/*
 * we post-process the filter code later and rewrite
 * this to the offset to the last instruction
 */
#define PASS	0xFF
#define FAIL	0xFE

static struct sock_filter msock_filter_insns[] = {
	/*
	 * do a little-endian load of the radiotap length field
	 */
	/* load lower byte into A */
	BPF_STMT(BPF_LD  | BPF_B | BPF_ABS, 2),
	/* put it into X (== index register) */
	BPF_STMT(BPF_MISC| BPF_TAX, 0),
	/* load upper byte into A */
	BPF_STMT(BPF_LD  | BPF_B | BPF_ABS, 3),
	/* left-shift it by 8 */
	BPF_STMT(BPF_ALU | BPF_LSH | BPF_K, 8),
	/* or with X */
	BPF_STMT(BPF_ALU | BPF_OR | BPF_X, 0),
	/* put result into X */
	BPF_STMT(BPF_MISC| BPF_TAX, 0),

	/*
	 * Allow management frames through, this also gives us those
	 * management frames that we sent ourselves with status
	 */
	/* load the lower byte of the IEEE 802.11 frame control field */
	BPF_STMT(BPF_LD  | BPF_B | BPF_IND, 0),
	/* mask off frame type and version */
	BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0xF),
	/* accept frame if it's both 0, fall through otherwise */
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, PASS, 0),

	/*
	 * TODO: add a bit to radiotap RX flags that indicates
	 * that the sending station is not associated, then
	 * add a filter here that filters on our DA and that flag
	 * to allow us to deauth frames to that bad station.
	 *
	 * For now allow all To DS data frames through.
	 */
	/* load the IEEE 802.11 frame control field */
	BPF_STMT(BPF_LD  | BPF_H | BPF_IND, 0),
	/* mask off frame type, version and DS status */
	BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0x0F03),
	/* accept frame if version 0, type 2 and To DS, fall through otherwise
	 */
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0801, PASS, 0),

#if 0
	/*
	 * drop non-data frames
	 */
	/* load the lower byte of the frame control field */
	BPF_STMT(BPF_LD   | BPF_B | BPF_IND, 0),
	/* mask off QoS bit */
	BPF_STMT(BPF_ALU  | BPF_AND | BPF_K, 0x0c),
	/* drop non-data frames */
	BPF_JUMP(BPF_JMP  | BPF_JEQ | BPF_K, 8, 0, FAIL),
#endif
	/* load the upper byte of the frame control field */
	BPF_STMT(BPF_LD   | BPF_B | BPF_IND, 1),
	/* mask off toDS/fromDS */
	BPF_STMT(BPF_ALU  | BPF_AND | BPF_K, 0x03),
	/* accept WDS frames */
	BPF_JUMP(BPF_JMP  | BPF_JEQ | BPF_K, 3, PASS, 0),

	/*
	 * add header length to index
	 */
	/* load the lower byte of the frame control field */
	BPF_STMT(BPF_LD   | BPF_B | BPF_IND, 0),
	/* mask off QoS bit */
	BPF_STMT(BPF_ALU  | BPF_AND | BPF_K, 0x80),
	/* right shift it by 6 to give 0 or 2 */
	BPF_STMT(BPF_ALU  | BPF_RSH | BPF_K, 6),
	/* add data frame header length */
	BPF_STMT(BPF_ALU  | BPF_ADD | BPF_K, 24),
	/* add index, was start of 802.11 header */
	BPF_STMT(BPF_ALU  | BPF_ADD | BPF_X, 0),
	/* move to index, now start of LL header */
	BPF_STMT(BPF_MISC | BPF_TAX, 0),

	/*
	 * Accept empty data frames, we use those for
	 * polling activity.
	 */
	BPF_STMT(BPF_LD  | BPF_W | BPF_LEN, 0),
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_X, 0, PASS, 0),

	/*
	 * Accept EAPOL frames
	 */
	BPF_STMT(BPF_LD  | BPF_W | BPF_IND, 0),
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0xAAAA0300, 0, FAIL),
	BPF_STMT(BPF_LD  | BPF_W | BPF_IND, 4),
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0000888E, PASS, FAIL),

	/* keep these last two statements or change the code below */
	/* return 0 == "DROP" */
	BPF_STMT(BPF_RET | BPF_K, 0),
	/* return ~0 == "keep all" */
	BPF_STMT(BPF_RET | BPF_K, ~0),
};

static struct sock_fprog msock_filter = {
	.len = ARRAY_SIZE(msock_filter_insns),
	.filter = msock_filter_insns,
};

static int add_monitor_filter(int s)
{
	int idx;

	/* rewrite all PASS/FAIL jump offsets */
	for (idx = 0; idx < msock_filter.len; idx++) {
		struct sock_filter *insn = &msock_filter_insns[idx];

		if (BPF_CLASS(insn->code) == BPF_JMP) {
			if (insn->code == (BPF_JMP|BPF_JA)) {
				if (insn->k == PASS)
					insn->k = msock_filter.len - idx - 2;
				else if (insn->k == FAIL)
					insn->k = msock_filter.len - idx - 3;
			}

			if (insn->jt == PASS)
				insn->jt = msock_filter.len - idx - 2;
			else if (insn->jt == FAIL)
				insn->jt = msock_filter.len - idx - 3;

			if (insn->jf == PASS)
				insn->jf = msock_filter.len - idx - 2;
			else if (insn->jf == FAIL)
				insn->jf = msock_filter.len - idx - 3;
		}
	}

	if (setsockopt(s, SOL_SOCKET, SO_ATTACH_FILTER,
		       &msock_filter, sizeof(msock_filter))) {
		printf("nl80211: setsockopt(SO_ATTACH_FILTER) failed: %s\r\n",
			   strerror(errno));
		return -1;
	}

	return 0;
}


void drv_wifi_remove_monitor_socket(drv_wifi_envar_t *drv)
{
	if (drv->monitor_refcount > 0)
		drv->monitor_refcount--;

	if (drv->monitor_refcount > 0)
		return;

	if (drv->dev_ifidx >= 0) {
		drv->dev_ifidx = -1;
	}
	if (drv->monitor_sock >= 0) {
		close(drv->monitor_sock);
		drv->monitor_sock = -1;
	}
}

int drv_wifi_create_monitor_socket(drv_wifi_envar_t *drv)
{
	struct sockaddr_ll ll;
	int optval;
	socklen_t optlen;
//    int ret;

    /* create monitor socket */
	memset(&ll, 0, sizeof(ll));
	ll.sll_family = AF_PACKET;
	ll.sll_ifindex = drv->dev_ifidx;
	drv->monitor_sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (drv->monitor_sock < 0) {
		printf("nl80211: socket[PF_PACKET,SOCK_RAW] failed: %s\r\n",
			   strerror(errno));
		goto error;
	}

	if (add_monitor_filter(drv->monitor_sock)) {
		printf("Failed to set socket filter for monitor "
			   "interface; do filtering in user space\r\n");
		/* This works, but will cost in performance. */
	}

	if (bind(drv->monitor_sock, (struct sockaddr *) &ll, sizeof(ll)) < 0) {
		printf("nl80211: monitor socket bind failed: %s\r\n",
			   strerror(errno));
		goto error;
	}

	optlen = sizeof(optval);
	optval = 20;
	if (setsockopt(drv->monitor_sock, SOL_SOCKET, SO_PRIORITY, &optval, optlen)) {
		printf("nl80211: Failed to set socket priority: %s\r\n",
			   strerror(errno));
		goto error;
	}

	drv->monitor_refcount++;

	return 0;
 error:
	drv_wifi_remove_monitor_socket(drv);
	return -1;
}


static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int (*registered_handler)(struct nl_msg *, void *);
static void *registered_handler_data;

void register_handler(int (*handler)(struct nl_msg *, void *), void *data)
{
	registered_handler = handler;
	registered_handler_data = data;
}

int valid_handler(struct nl_msg *msg, void *arg)
{
	if (registered_handler)
		return registered_handler(msg, registered_handler_data);

	return NL_OK;
}
int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nlsock = nl_socket_alloc();
	if (!state->nlsock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	nl_socket_set_buffer_size(state->nlsock, 8192, 8192);

	if (genl_connect(state->nlsock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	state->nl80211_id = genl_ctrl_resolve(state->nlsock, "nl80211");
	if (state->nl80211_id < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

 out_handle_destroy:
	nl_socket_free(state->nlsock);
	return err;
}

void nl80211_cleanup(struct nl80211_state *state)
{
	nl_socket_free(state->nlsock);
}


int nl80211_set_interface_monitor(drv_wifi_envar_t *drv)
{
	struct nl_cb *cb;
	struct nl_cb *s_cb;
	struct nl_msg *msg;
//	signed long long devidx = 0;
	int err;
//	enum command_identify_by command_idby = CIB_NONE;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	if (!cb || !s_cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out;
	}

	genlmsg_put(msg, 0, 0, drv->nl_state.nl80211_id, 0,
		    0, NL80211_CMD_SET_INTERFACE, 0);
    
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, drv->dev_ifidx);

    /* set wlan0 to monitor type */
    NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);
    

	nl_socket_set_cb(drv->nl_state.nlsock, s_cb);
	err = nl_send_auto_complete(drv->nl_state.nlsock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	while (err > 0)
		nl_recvmsgs(drv->nl_state.nlsock, cb);
 out:
	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");

    return 2;

}

int nl80211_set_wiphy_txpower(drv_wifi_envar_t *drv, int mbm)
{
	struct nl_cb *cb;
	struct nl_cb *s_cb;
	struct nl_msg *msg;
//	signed long long devidx = 0;
	int err;
//	enum command_identify_by command_idby = CIB_NONE;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	if (!cb || !s_cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out;
	}

	genlmsg_put(msg, 0, 0, drv->nl_state.nl80211_id, 0,
		    0, NL80211_CMD_SET_WIPHY, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, drv->dev_ifidx);

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_TX_POWER_SETTING, NL80211_TX_POWER_FIXED);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, mbm);
    
	nl_socket_set_cb(drv->nl_state.nlsock, s_cb);

	err = nl_send_auto_complete(drv->nl_state.nlsock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	while (err > 0)
		nl_recvmsgs(drv->nl_state.nlsock, cb);
 out:
	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");

    return 2;

}

int nl80211_set_wiphy_channel(drv_wifi_envar_t *drv, int ch)
{
	struct nl_cb *cb;
	struct nl_cb *s_cb;
	struct nl_msg *msg;
//	signed long long devidx = 0;
	int err;
//	enum command_identify_by command_idby = CIB_NONE;
	unsigned int freq;

    /* set wlan0 to monitor type */
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	if (!cb || !s_cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out;
	}

	genlmsg_put(msg, 0, 0, drv->nl_state.nl80211_id, 0,
		    0, NL80211_CMD_SET_WIPHY, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, drv->dev_ifidx);

	enum nl80211_band band;
	band = ch <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
	freq = ieee80211_channel_to_frequency(ch, band);

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);   
    
	nl_socket_set_cb(drv->nl_state.nlsock, s_cb);

	err = nl_send_auto_complete(drv->nl_state.nlsock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	while (err > 0)
		nl_recvmsgs(drv->nl_state.nlsock, cb);
 out:
	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");

    return 2;

}


int nl80211_send_monitor(drv_wifi_envar_t *drv,
			 const void *data, size_t len,
			 int encrypt, int noack)
{
	uint8_t rtap_hdr[] = {
		0x00, 0x00, /* radiotap version */
		0x0e, 0x00, /* radiotap length */
		0x02, 0xc0, 0x00, 0x00, /* bmap: flags, tx and rx flags */
		IEEE80211_RADIOTAP_F_FRAG, /* F_FRAG (fragment if required) */
		0x00,       /* padding */
		0x00, 0x00, /* RX and TX flags to indicate that */
		0x00, 0x00, /* this is the injected frame directly */
	};

	struct iovec iov[2] = {
		{
			.iov_base = &rtap_hdr,
			.iov_len = sizeof(rtap_hdr),
		},
		{
			.iov_base = (void *) data,
			.iov_len = len,
		}
	};
	struct msghdr msg = {
		.msg_name = NULL,
		.msg_namelen = 0,
		.msg_iov = iov,
		.msg_iovlen = 2,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0,
	};
	int res;
	uint16_t txflags = 0;

	if (encrypt)
		rtap_hdr[8] |= IEEE80211_RADIOTAP_F_WEP;

	if (drv->monitor_sock < 0) {
		printf("nl80211: No monitor socket available \r\n");
		return -1;
	}

	if (noack)
		txflags |= IEEE80211_RADIOTAP_F_TX_NOACK;
	WPA_PUT_LE16(&rtap_hdr[12], txflags);

	res = sendmsg(drv->monitor_sock, &msg, 0);
	if (res < 0) {
		printf("nl80211: sendmsg: %s\r\n", strerror(errno));
		return -1;
	}
	return 0;
}

int drv_wifi_set_interface_flag(char *ifname, short flag)  
{  
        struct ifreq ifr;  
        int fd,skfd;  

        /* Create a channel to the NET kernel. */  
        if ((skfd = socket(AF_INET,SOCK_DGRAM ,0)) < 0) {  
                perror("socket");  
                exit(1);  
        }  

        fd = skfd;  

        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);  
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {  
                fprintf(stderr, "%s: unknown interface: %s\n",   
                                ifname, strerror(errno));  
                return -1;  
        }  
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);  
        if(flag == 0)
                ifr.ifr_flags &= ~IFF_UP;
        else if(flag == 1)
                ifr.ifr_flags |= IFF_UP;   
        if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {  
                perror("SIOCSIFFLAGS");  
                return -1;  
        } 
        return (0);  
}

/* get wifi local macaddr. none '\0' */
int drv_wifi_get_macaddr(uint8_t * mac)
{
    struct ifreq ifreq;
    int sock;

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, IF_NAME);

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        return -1;
    }
    memcpy(mac, ifreq.ifr_hwaddr.sa_data, MACADDR_LENGTH);
    
    return 0;
}

/* iw dev wlan0  
  1. set type monitor
  2. set channel
  3. set txpower
*/
int drv_wifi_dev_config(drv_wifi_envar_t *drv)
{
    int ret = 0;

    drv_wifi_get_macaddr(drv->mac);
    
    ret = drv_wifi_set_interface_flag(IF_NAME, 0);
    /* 1. set wlan0 to monitor type */
    ret = nl80211_set_interface_monitor(drv);
    /* 2. ifconfig wlan0 up */
    ret = drv_wifi_set_interface_flag(IF_NAME, 1);

    /* 3. set channel */
    ret = nl80211_set_wiphy_channel(drv, drv->working_param.channel);

    /* 4. set txpower 3000 */
    ret = nl80211_set_wiphy_txpower(drv, MAX_TXPOWER_MBM);
    
    return ret;
}

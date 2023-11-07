/*
 * Driver interaction with extended Linux CFG8021
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#include "includes.h"
#include <sys/types.h>
#include <fcntl.h>
#include <netlink/genl/genl.h>
#include <net/if.h>

#include "common.h"
#include "utils/common.h"
#include "linux_ioctl.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/qca-vendor.h"
#include "common/qca-vendor-attr.h"
#include "driver_nl80211.h"
#include "../../wpa_supplicant/wpa_supplicant_i.h"
#include "../../wpa_supplicant/config.h"
#ifdef ANDROID
#include "android_drv.h"
#include <cutils/properties.h>
#if defined(__BIONIC_FORTIFY)
#include <sys/system_properties.h>
#endif
#endif
#include "linux_ioctl.h"

#define WPA_PS_ENABLED        0
#define WPA_PS_DISABLED        1

#define MAX_WPSP2PIE_CMD_SIZE        512

typedef struct android_wifi_priv_cmd_nxp {
    u64 buf;
    int used_len;
    int total_len;
} android_wifi_priv_cmd_nxp;

typedef struct android_wifi_priv_cmd {
#ifdef BCMDHD_64_BIT_IPC
	u64 bufaddr;
#else
	char *bufaddr;
#endif
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

static int drv_errors = 0;

static void wpa_driver_send_hang_msg(struct wpa_driver_nl80211_data *drv)
{
	drv_errors++;
	if (drv_errors > DRV_NUMBER_SEQUENTIAL_ERRORS) {
		drv_errors = 0;
		wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
	}
}
#if 0
static int get_power_mode_handler(struct nl_msg *msg, void *arg)
{
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    int *state = (int *)arg;

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
          genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[NL80211_ATTR_PS_STATE])
        return NL_SKIP;

    if (state) {
        *state = (int)nla_get_u32(tb[NL80211_ATTR_PS_STATE]);
        wpa_printf(MSG_DEBUG, "nl80211: Get power mode = %d", *state);
        *state = (*state == NL80211_PS_ENABLED) ?
                WPA_PS_ENABLED : WPA_PS_DISABLED;
    }

    return NL_SKIP;
}
#endif

#ifndef HOSTAPD
#define NL80211_BGSCAN_HEADER           "BGSCAN-CONFIG "
#define NL80211_BGSCAN_HEADER_SIZE      14
#define NL80211_SSID_AMOUNT             16
#define NL80211_BGCAN_BUF_LEN           720
#define NL80211_CSCAN_SSID_SECTION      'S'
#define NL80211_SSID_MAX_SIZE           32
#define NL80211_BGSCAN_RSSI_SECTION     'R'
#define NL80211_BGSCAN_INTERVAL_SECTION 'T'
#define NL80211_BGSCAN_INTERVAL_DEF     30
#define NL80211_BGSCAN_REPEAT_SECTION   'E'
#define NL80211_BGSCAN_REPEAT_DEF       5

static char *getop(char *s, int *first_time)
{
    const char delim[] = " \t\n";
    char *p;
    if (*first_time) {
        p = strtok(s, delim);
        *first_time = 0;//FALSE
    }
    else {
        p = strtok(NULL, delim);
    }
    return (p);
}

static int wpa_driver_set_backgroundscan_params(void *priv, char *cmd)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    struct ifreq ifr;
    android_wifi_priv_cmd_nxp priv_cmd;
    struct wpa_supplicant *wpa_s;
    int ret = 0, i = 0, bp;
    char buf[NL80211_BGCAN_BUF_LEN];
    struct wpa_ssid *ssid_conf;
    int first_time = 1;//TRUE
    char    *opstr = NULL;
    char      *ptr = NULL;
    int find_ssid = 0;
    int find_interval = 0;
    int find_repeat = 0;

    if (drv == NULL) {
        wpa_printf(MSG_ERROR, "%s: drv is NULL. Exiting", __func__);
        return -1;
    }
    if (drv->ctx == NULL) {
        wpa_printf(MSG_ERROR, "%s: drv->ctx is NULL. Exiting", __func__);
        return -1;
    }
    wpa_s = (struct wpa_supplicant *)(drv->ctx);
    if (wpa_s->conf == NULL) {
        wpa_printf(MSG_ERROR, "%s: wpa_s->conf is NULL. Exiting", __func__);
        return -1;
    }
    ssid_conf = wpa_s->conf->ssid;

    bp = NL80211_BGSCAN_HEADER_SIZE;
    os_memcpy(buf, NL80211_BGSCAN_HEADER, bp);

    opstr = getop(cmd, &first_time);
    while ((opstr = getop(cmd, &first_time)) != NULL) {
        if ((ptr = strstr(opstr, "SSID=")) != NULL) {
            find_ssid = 1;
            ptr = ptr + strlen("SSID=");
            buf[bp++] = NL80211_CSCAN_SSID_SECTION;
            buf[bp++] = strlen(ptr);
            os_memcpy(&buf[bp], ptr, strlen(ptr));
            bp += strlen(ptr);
            i++;
        }
        else if ((ptr = strstr(opstr, "RSSI=")) != NULL) {
            ptr = ptr + strlen("RSSI=");
            buf[bp++] = NL80211_BGSCAN_RSSI_SECTION;
            buf[bp++] = atoi(ptr);
        }
        else if ((ptr = strstr(opstr, "INTERVAL=")) != NULL) {
            find_interval = 1;
            ptr = ptr + strlen("INTERVAL=");
            buf[bp++] = NL80211_BGSCAN_INTERVAL_SECTION;
            buf[bp++] = (u8)atoi(ptr);
            buf[bp++] = (u8)(atoi(ptr) >> 8);
        }
        else if ((ptr = strstr(opstr, "REPEAT=")) != NULL) {
            find_repeat = 1;
            ptr = ptr + strlen("REPEAT=");
            buf[bp++] = NL80211_BGSCAN_REPEAT_SECTION;
            buf[bp++] = (u8)atoi(ptr);
        }
    }

    if (!find_ssid) {
        while ((i < NL80211_SSID_AMOUNT) && (ssid_conf != NULL)) {
            if ((!ssid_conf->disabled) && (ssid_conf->ssid_len <= NL80211_SSID_MAX_SIZE)) {
                wpa_printf(MSG_DEBUG, "For BG Scan: %s", ssid_conf->ssid);
                buf[bp++] = NL80211_CSCAN_SSID_SECTION;
                buf[bp++] = ssid_conf->ssid_len;
                os_memcpy(&buf[bp], ssid_conf->ssid, ssid_conf->ssid_len);
                bp += ssid_conf->ssid_len;
                i++;
            }
            ssid_conf = ssid_conf->next;
        }
    }

    if (!find_interval) {
        buf[bp++] = NL80211_BGSCAN_INTERVAL_SECTION;
        buf[bp++] = NL80211_BGSCAN_INTERVAL_DEF;
        buf[bp++] = (NL80211_BGSCAN_INTERVAL_DEF >> 8);
    }

    if (!find_repeat) {
        buf[bp++] = NL80211_BGSCAN_REPEAT_SECTION;
        buf[bp++] = NL80211_BGSCAN_REPEAT_DEF;
    }

    memset(&ifr, 0, sizeof(ifr));
    memset(&priv_cmd, 0, sizeof(priv_cmd));
    os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);
#if defined(__LP64__)
    //driver only support 32bit user space, we fake one
    priv_cmd.buf = (u64)buf & 0x00000000ffffffff;
#else
    u32 tmp = (u32)buf;
    priv_cmd.buf = tmp;
#endif
    priv_cmd.used_len = NL80211_BGCAN_BUF_LEN;
    priv_cmd.total_len = NL80211_BGCAN_BUF_LEN;
    ifr.ifr_data = &priv_cmd;

    if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 14, &ifr)) < 0) {
        wpa_printf(MSG_ERROR, "ioctl[SIOCSIWPRIV] (bgscan config): %d", ret);
    } else {
        wpa_printf(MSG_DEBUG, "%s %s len = %d, %zu", __func__, buf, ret, strlen(buf));
    }
    return ret;
}
#endif

int wpa_driver_nl80211_driver_cmd_nxp(void *priv, char *cmd, char *buf,
                  size_t buf_len )
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    struct ifreq ifr;
    android_wifi_priv_cmd_nxp priv_cmd;
    int ret = 0;
    wpa_printf(MSG_INFO, "the nl80211 driver cmd is %s\n", cmd);
    wpa_printf(MSG_INFO, "the nl80211 driver cmd len is %zu\n", strlen(cmd));
    if (os_strcasecmp(cmd, "STOP") == 0) {
        linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 0);
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED");
    } else if (os_strcasecmp(cmd, "START") == 0) {
        linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 1);
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED");
    } else if(os_strncasecmp(cmd, "COUNTRY", strlen("COUNTRY")) == 0 && ((strlen(cmd) - strlen("COUNTRY") - 1) >= 2)) {
        char alpha2[3];
    struct nl_msg *msg;

    msg = nlmsg_alloc();
    if (!msg)
        return -ENOMEM;

    memcpy(alpha2, cmd + strlen("COUNTRY") + 1, strlen(cmd) - strlen("COUNTRY") - 1);
    alpha2[2] = '\0';
	if (!nl80211_cmd(drv, msg, 0, NL80211_CMD_RELOAD_REGDB))
	{
			    wpa_printf(MSG_ERROR, "debug for fw reload\n");
		        nlmsg_free(msg);
				        return -EINVAL;
	}
	    if (send_and_recv_msgs(drv, msg, NULL, NULL, NULL, NULL))
			        return -EINVAL;
	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;
    if (!nl80211_cmd(drv, msg, 0, NL80211_CMD_REQ_SET_REG) ||
    nla_put_string(msg, NL80211_ATTR_REG_ALPHA2, alpha2)) {
        nlmsg_free(msg);
        return -EINVAL;
    }
    if (send_and_recv_msgs(drv, msg, NULL, NULL, NULL, NULL))
        return -EINVAL;
    } else if (os_strcasecmp(cmd, "MACADDR") == 0) {
        u8 macaddr[ETH_ALEN] = {};

        ret = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
        if (!ret)
            ret = os_snprintf(buf, buf_len,
                      "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
    } else if (os_strcasecmp(cmd, "RELOAD") == 0) {
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
    } else { /* Use private command */
#ifndef HOSTAPD
        if (os_strncasecmp(cmd, "BGSCAN-START", 12) == 0) {
            /* Issue a command 'BGSCAN-CONFIG' to driver */
            ret = wpa_driver_set_backgroundscan_params(priv, cmd);
            if (ret < 0){
                wpa_printf(MSG_ERROR,
                    "%s: failed to issue private command: BGSCAN-START\n", __func__);
            }
            return ret;
        } else
#endif
        {
            os_memcpy(buf, cmd, strlen(cmd) + 1);
        }
        memset(&ifr, 0, sizeof(ifr));
        memset(&priv_cmd, 0, sizeof(priv_cmd));
        os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

#if defined(__LP64__)
        priv_cmd.buf = (u64)buf;
#else
        u32 tmp = (u32)buf;
        wpa_printf(MSG_ERROR, "%s: Buffer len = <%zu>\n", __func__, buf_len);
        priv_cmd.buf = tmp;
#endif
        priv_cmd.used_len = buf_len;
        priv_cmd.total_len = buf_len;
        ifr.ifr_data = &priv_cmd;

        if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 14, &ifr)) < 0) {
            wpa_printf(MSG_ERROR, "%s: failed to issue private commands\n", __func__);
            wpa_printf(MSG_ERROR, "the cmd is %s\n", cmd);
            wpa_printf(MSG_ERROR, "the error is [%s] \n", strerror(errno));
            //wpa_driver_send_hang_msg(drv);
        } else {
            drv_errors = 0;
            ret = 0;
            if ((os_strcasecmp(cmd, "LINKSPEED") == 0) ||
                (os_strcasecmp(cmd, "RSSI") == 0) ||
                (os_strcasecmp(cmd, "GETBAND") == 0) ||
                (os_strncasecmp(cmd, "WLS_BATCHING", 12) == 0))
                ret = strlen(buf);
            else if ((os_strncasecmp(cmd, "COUNTRY", 7) == 0) ||
                 (os_strncasecmp(cmd, "SETBAND", 7) == 0))

                //wpa_supplicant_event(drv->ctx,
                //    EVENT_CHANNEL_LIST_CHANGED, NULL);
             //wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret, strlen(buf));
             wpa_printf(MSG_DEBUG, "%s %d %d", __func__, __LINE__, ret);
        }
    }
    return ret;
}

int wpa_driver_set_p2p_noa_nxp(void *priv, u8 count, int start, int duration)
{
    char buf[MAX_DRV_CMD_SIZE];

    memset(buf, 0, sizeof(buf));
    wpa_printf(MSG_DEBUG, "%s: Entry", __func__);
    snprintf(buf, sizeof(buf), "P2P_SET_NOA %d %d %d", count, start, duration);
    return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf)+1);
}

int wpa_driver_get_p2p_noa_nxp(void *priv, u8 *buf, size_t len)
{
    /* Return 0 till we handle p2p_presence request completely in the driver */
    return 0;
}

int wpa_driver_set_p2p_ps_nxp(void *priv, int legacy_ps, int opp_ps, int ctwindow)
{
    char buf[MAX_DRV_CMD_SIZE];

    memset(buf, 0, sizeof(buf));
    wpa_printf(MSG_DEBUG, "%s: Entry", __func__);
    snprintf(buf, sizeof(buf), "P2P_SET_PS %d %d %d", legacy_ps, opp_ps, ctwindow);
    return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf) + 1);
}

int wpa_driver_set_ap_wps_p2p_ie_nxp(void *priv, const struct wpabuf *beacon,
                 const struct wpabuf *proberesp,
                 const struct wpabuf *assocresp)
{
    char *buf = NULL;
    struct wpabuf *ap_wps_p2p_ie = NULL;
    char *_cmd = "SET_AP_WPS_P2P_IE";
    char *pbuf;
    char *tb = NULL;
    int ret = 0;
    int i;
    size_t buf_size, offset;
    struct cmd_desc {
        int cmd;
        const struct wpabuf *src;
    } cmd_arr[] = {
        {0x1, beacon},
        {0x2, proberesp},
        {0x4, assocresp},
        {-1, NULL}
    };

    buf = os_malloc(MAX_WPSP2PIE_CMD_SIZE);
    if (buf == NULL) {
        wpa_printf(MSG_DEBUG, "%s: %s", __func__, strerror(errno));
        return errno;
    }
    buf_size = MAX_WPSP2PIE_CMD_SIZE;
    wpa_printf(MSG_DEBUG, "%s: Entry", __func__);
    for (i = 0; cmd_arr[i].cmd != -1; i++) {
        os_memset(buf, 0, buf_size);
        pbuf = buf;
        pbuf += sprintf(pbuf, "%s %d", _cmd, cmd_arr[i].cmd);
        *pbuf++ = '\0';
        ap_wps_p2p_ie = cmd_arr[i].src ?
            wpabuf_dup(cmd_arr[i].src) : NULL;
        if (ap_wps_p2p_ie) {
            if ((size_t)(buf + buf_size - pbuf) < wpabuf_len(ap_wps_p2p_ie)) {
                offset = pbuf - buf;
                tb = os_realloc(buf, buf_size + wpabuf_len(ap_wps_p2p_ie) + 1);
                if (tb == NULL) {
                    wpa_printf(MSG_DEBUG, "%s: %s", __func__, strerror(errno));
                    os_free(buf);
                    return errno;
                }
                buf = tb;
                pbuf = buf + offset;
                buf_size += wpabuf_len(ap_wps_p2p_ie) + 1;
                os_memset(pbuf, 0, (buf_size - offset));
                wpa_printf(MSG_DEBUG, "Realloc WPS P2P IE buffer size to %zu bytes", buf_size);
            }
            os_memcpy(pbuf, wpabuf_head(ap_wps_p2p_ie), wpabuf_len(ap_wps_p2p_ie));
            ret = wpa_driver_nl80211_driver_cmd(priv, buf, buf,
                strlen(_cmd) + 3 + wpabuf_len(ap_wps_p2p_ie));
            wpabuf_free(ap_wps_p2p_ie);
            if (ret < 0)
                break;
        }
    }

    if (buf) os_free(buf);
    return ret;
}

static void wpa_driver_notify_country_change(void *ctx, char *cmd)
{
	if ((os_strncasecmp(cmd, "COUNTRY", 7) == 0) ||
	    (os_strncasecmp(cmd, "SETBAND", 7) == 0)) {
		union wpa_event_data event;

		os_memset(&event, 0, sizeof(event));
		event.channel_list_changed.initiator = REGDOM_SET_BY_USER;
		if (os_strncasecmp(cmd, "COUNTRY", 7) == 0) {
			event.channel_list_changed.type = REGDOM_TYPE_COUNTRY;
			if (os_strlen(cmd) > 9) {
				event.channel_list_changed.alpha2[0] = cmd[8];
				event.channel_list_changed.alpha2[1] = cmd[9];
			}
		} else {
			event.channel_list_changed.type = REGDOM_TYPE_UNKNOWN;
		}
		wpa_supplicant_event(ctx, EVENT_CHANNEL_LIST_CHANGED, &event);
	}
}

int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
				  size_t buf_len )
{
	struct i802_bss *bss = priv;
	struct wpa_driver_nl80211_data *drv = bss->drv;
	struct ifreq ifr;
	android_wifi_priv_cmd priv_cmd;
	int ret = 0;
	char wifi_status[PROPERTY_VALUE_MAX] = {'\0'};

	property_get("vendor.wifi_name", wifi_status, NULL);
	wpa_printf(MSG_INFO, "%s: wifi vendor: %s", __func__, wifi_status);
	wpa_printf(MSG_INFO, "%s: %s: private command: %s", __func__, bss->ifname, cmd);

	if (os_strncasecmp(wifi_status, "nxp", 3) == 0)
		return wpa_driver_nl80211_driver_cmd_nxp(priv, cmd, buf, buf_len);

	do {
		if (bss->ifindex <= 0 && bss->wdev_id > 0) {
		/* DRIVER CMD received on the DEDICATED P2P Interface which doesn't
		* have an NETDEVICE associated with it. So we have to re-route the
		* command to the parent NETDEVICE
		*/
		struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(drv->ctx);

		if (os_strncmp(bss->ifname, "p2p-dev-wlan", 12) == 0 &&
			(os_strncasecmp(wifi_status, "bcm", 3) == 0)) {
			 wpa_printf(MSG_WARNING, "%s:%d bcmwifi"
			"ignore re-route cmd when ifindex was wrong", __func__, __LINE__);
			break;
		}

		if (wpa_s && wpa_s->parent && wpa_s->parent->drv_priv) {
			/* Update the nl80211 pointers corresponding to parent iface */
			bss = wpa_s->parent->drv_priv;
			drv = bss->drv;
			wpa_printf(MSG_DEBUG, "Re-routing command to iface: %s"
				" cmd (%s)", bss->ifname, cmd);
			}
		}
	} while(0);

	if (os_strncasecmp(cmd, "COUNTRY", 7) == 0) {
		char alpha2[3];
		struct nl_msg *msg;
		msg = nlmsg_alloc();
		if (!msg)
			return -ENOMEM;

	memcpy(alpha2, cmd + strlen("COUNTRY") + 1, strlen(cmd) - strlen("COUNTRY") - 1);
	alpha2[2] = '\0';
	if (!nl80211_cmd(drv, msg, 0, NL80211_CMD_RELOAD_REGDB))
	{
		wpa_printf(MSG_ERROR, "debug for fw reload\n");
		nlmsg_free(msg);
		return -EINVAL;
	}
	if (send_and_recv_msgs(drv, msg, NULL, NULL, NULL, NULL))
		return -EINVAL;
	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;
	if (!nl80211_cmd(drv, msg, 0, NL80211_CMD_REQ_SET_REG) ||
	nla_put_string(msg, NL80211_ATTR_REG_ALPHA2, alpha2)) {
		nlmsg_free(msg);
		return -EINVAL;
	}
	if (send_and_recv_msgs(drv, msg, NULL, NULL, NULL, NULL))
		return -EINVAL;
	}

	if (os_strncasecmp(cmd, "BTCOEXMODE", 10) == 0 || os_strncasecmp(cmd, "MIRACAST", 8) == 0 ||
		os_strncasecmp(cmd, "WLS_BATCHING", 12) == 0 || os_strcasecmp(cmd, "BTCOEXSCAN-STOP") == 0 ||
		os_strncasecmp(cmd, "RXFILTER", 8) == 0 || os_strncasecmp(cmd, "SETSUSPENDMODE", 14) == 0 ||
		os_strncasecmp(cmd, "SETBAND", 7) == 0)
		return 0;

	if (os_strncasecmp(cmd, "SET_AP_WPS_P2P_IE", 17) == 0) {
		if ((os_strncasecmp(wifi_status, "mtk", 3) == 0) ||
			(os_strncasecmp(wifi_status, "rtl8852bs", 9) == 0) ||
			(os_strncasecmp(wifi_status, "uwe", 3) ==0 )) {
			return 0;
		}
	}

	if (os_strcasecmp(cmd, "STOP") == 0) {
		linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 0);
		wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED");
	} else if (os_strcasecmp(cmd, "START") == 0) {
		linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 1);
		wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED");
	} else if (os_strcasecmp(cmd, "P2P_DISABLE") == 0) {
		os_memcpy(buf, "P2P_DISABLE", 12);
		wpa_printf(MSG_DEBUG, "P2P_DISABLE");
	} else if (os_strcasecmp(cmd, "MACADDR") == 0) {
		u8 macaddr[ETH_ALEN] = {};

		ret = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
		if (!ret)
			ret = os_snprintf(buf, buf_len,
					  "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
	} else { /* Use private command */
		os_memcpy(buf, cmd, strlen(cmd) + 1);
		memset(&ifr, 0, sizeof(ifr));
		memset(&priv_cmd, 0, sizeof(priv_cmd));
		if (os_strncmp(bss->ifname, "p2p-dev-wlan", 12) == 0)
			os_strlcpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
		else
			os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

#ifdef BCMDHD_64_BIT_IPC
		priv_cmd.bufaddr = (u64)(uintptr_t)buf;
#else
		priv_cmd.bufaddr = buf;
#endif
		priv_cmd.used_len = buf_len;
		priv_cmd.total_len = buf_len;
		ifr.ifr_data = &priv_cmd;
		if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr)) < 0) {
			wpa_printf(MSG_ERROR, "%s: failed to issue private command: %s", __func__, cmd);
			wpa_driver_send_hang_msg(drv);
		} else {
			drv_errors = 0;
			ret = 0;
			if ((os_strcasecmp(cmd, "LINKSPEED") == 0) ||
			    (os_strcasecmp(cmd, "RSSI") == 0) ||
			    (os_strcasecmp(cmd, "GETBAND") == 0) ||
			    (os_strncasecmp(cmd, "WLS_BATCHING", 12) == 0))
				ret = strlen(buf);
			wpa_driver_notify_country_change(drv->ctx, cmd);
			wpa_printf(MSG_DEBUG, "%s %s len = %d, %zu", __func__, buf, ret, strlen(buf));
		}
	}
	return ret;
}

int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration)
{
	char buf[MAX_DRV_CMD_SIZE];

	memset(buf, 0, sizeof(buf));
	wpa_printf(MSG_DEBUG, "%s: Entry", __func__);

	char wifi_status[PROPERTY_VALUE_MAX] = {'\0'};
	property_get("vendor.wifi_name", wifi_status, NULL);

	if (os_strncasecmp(wifi_status, "nxp", 3) == 0)
		return wpa_driver_set_p2p_noa_nxp(priv, count, start, duration);

	snprintf(buf, sizeof(buf), "P2P_SET_NOA %d %d %d", count, start, duration);
	return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf)+1);
}

int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len)
{
	char wifi_status[PROPERTY_VALUE_MAX] = {'\0'};
	property_get("vendor.wifi_name", wifi_status, NULL);

	if (os_strncasecmp(wifi_status, "nxp", 3) == 0)
		return wpa_driver_get_p2p_noa_nxp(priv, buf, len);

	/* Return 0 till we handle p2p_presence request completely in the driver */
	return 0;
}

int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow)
{
	char buf[MAX_DRV_CMD_SIZE];

	memset(buf, 0, sizeof(buf));
	wpa_printf(MSG_DEBUG, "%s: Entry", __func__);

	char wifi_status[PROPERTY_VALUE_MAX] = {'\0'};
	property_get("vendor.wifi_name", wifi_status, NULL);

	if (os_strncasecmp(wifi_status, "nxp", 3) == 0)
		return wpa_driver_set_p2p_ps_nxp(priv, legacy_ps, opp_ps, ctwindow);

	snprintf(buf, sizeof(buf), "P2P_SET_PS %d %d %d", legacy_ps, opp_ps, ctwindow);
	return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf) + 1);
}

int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
				 const struct wpabuf *proberesp,
				 const struct wpabuf *assocresp)
{
	char *buf;
	const struct wpabuf *ap_wps_p2p_ie = NULL;

	char *_cmd = "SET_AP_WPS_P2P_IE";
	char *pbuf;
	int ret = 0;
	int i, buf_len;
	struct cmd_desc {
		int cmd;
		const struct wpabuf *src;
	} cmd_arr[] = {
		{0x1, beacon},
		{0x2, proberesp},
		{0x4, assocresp},
		{-1, NULL}
	};

	wpa_printf(MSG_DEBUG, "%s: Entry", __func__);

	char wifi_status[PROPERTY_VALUE_MAX] = {'\0'};
	property_get("vendor.wifi_name", wifi_status, NULL);

	if (os_strncasecmp(wifi_status, "nxp", 3) == 0)
		return wpa_driver_set_ap_wps_p2p_ie_nxp(priv, beacon, proberesp, assocresp);

	for (i = 0; cmd_arr[i].cmd != -1; i++) {
		ap_wps_p2p_ie = cmd_arr[i].src;
		if (ap_wps_p2p_ie) {
			buf_len = strlen(_cmd) + 3 + wpabuf_len(ap_wps_p2p_ie);
			buf = os_zalloc(buf_len);
			if (NULL == buf) {
				wpa_printf(MSG_ERROR, "%s: Out of memory",
					   __func__);
				ret = -1;
				break;
			}
		} else {
			continue;
		}
		pbuf = buf;
		pbuf += snprintf(pbuf, buf_len - wpabuf_len(ap_wps_p2p_ie),
				 "%s %d",_cmd, cmd_arr[i].cmd);
		*pbuf++ = '\0';
		os_memcpy(pbuf, wpabuf_head(ap_wps_p2p_ie), wpabuf_len(ap_wps_p2p_ie));
		ret = wpa_driver_nl80211_driver_cmd(priv, buf, buf, buf_len);
		os_free(buf);
		if (ret < 0)
			break;
	}

	return ret;
}

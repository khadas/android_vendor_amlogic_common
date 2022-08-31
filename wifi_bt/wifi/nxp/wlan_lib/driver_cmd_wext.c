/*
 * Copyright 2015-2020 NXP
 * Driver interaction with extended Linux Wireless Extensions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#include "includes.h"
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net/if.h>

#include "linux_wext.h"
#include "common.h"
#include "driver.h"
#include "eloop.h"
#include "priv_netlink.h"
#include "driver_wext.h"
#include "ieee802_11_defs.h"
#include "wpa_common.h"
#include "wpa_ctrl.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#include "linux_ioctl.h"
#include "scan.h"

#include "driver_cmd_wext.h"
#ifdef ANDROID
#include "android_drv.h"
#endif /* ANDROID */

#define RSSI_CMD			"RSSI"
#define LINKSPEED_CMD			"LINKSPEED"

/**
 * wpa_driver_wext_set_scan_timeout - Set scan timeout to report scan completion
 * @priv:  Pointer to private wext data from wpa_driver_wext_init()
 *
 * This function can be used to set registered timeout when starting a scan to
 * generate a scan completed event if the driver does not report this.
 */
static void wpa_driver_wext_set_scan_timeout(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	int timeout = 10; /* In case scan A and B bands it can be long */

	/* Not all drivers generate "scan completed" wireless event, so try to
	 * read results after a timeout. */
	if (drv->scan_complete_events) {
	/*
	 * The driver seems to deliver SIOCGIWSCAN events to notify
	 * when scan is complete, so use longer timeout to avoid race
	 * conditions with scanning and following association request.
	 */
		timeout = 30;
	}
	wpa_printf(MSG_DEBUG, "Scan requested - scan timeout %d seconds",
		   timeout);
	eloop_cancel_timeout(wpa_driver_wext_scan_timeout, drv, drv->ctx);
	eloop_register_timeout(timeout, 0, wpa_driver_wext_scan_timeout, drv,
			       drv->ctx);
}

/**
 * wpa_driver_wext_combo_scan - Request the driver to initiate combo scan
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @params: Scan parameters
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_combo_scan(void *priv, struct wpa_driver_scan_params *params)
{
	char buf[WEXT_CSCAN_BUF_LEN];
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret, bp;
	unsigned i;

	if (!drv->driver_is_started) {
		wpa_printf(MSG_DEBUG, "%s: Driver stopped", __func__);
		return 0;
	}

	wpa_printf(MSG_DEBUG, "%s: Start", __func__);

	/* Set list of SSIDs */
	bp = WEXT_CSCAN_HEADER_SIZE;
	os_memcpy(buf, WEXT_CSCAN_HEADER, bp);
	for(i=0; i < params->num_ssids; i++) {
		if ((bp + IW_ESSID_MAX_SIZE + 10) >= (int)sizeof(buf))
			break;
		wpa_printf(MSG_DEBUG, "For Scan: %s", params->ssids[i].ssid);
		buf[bp++] = WEXT_CSCAN_SSID_SECTION;
		buf[bp++] = params->ssids[i].ssid_len;
		os_memcpy(&buf[bp], params->ssids[i].ssid, params->ssids[i].ssid_len);
		bp += params->ssids[i].ssid_len;
	}

	/* Set list of channels */
	buf[bp++] = WEXT_CSCAN_CHANNEL_SECTION;
	buf[bp++] = 0;

	/* Set passive dwell time (default is 250) */
	buf[bp++] = WEXT_CSCAN_PASV_DWELL_SECTION;
	buf[bp++] = (u8)WEXT_CSCAN_PASV_DWELL_TIME;
	buf[bp++] = (u8)(WEXT_CSCAN_PASV_DWELL_TIME >> 8);

	/* Set home dwell time (default is 40) */
	buf[bp++] = WEXT_CSCAN_HOME_DWELL_SECTION;
	buf[bp++] = (u8)WEXT_CSCAN_HOME_DWELL_TIME;
	buf[bp++] = (u8)(WEXT_CSCAN_HOME_DWELL_TIME >> 8);

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.pointer = buf;
	iwr.u.data.length = bp;

	if ((ret = ioctl(drv->ioctl_sock, SIOCSIWPRIV, &iwr)) < 0) {
		if (!drv->bgscan_enabled)
			wpa_printf(MSG_ERROR, "ioctl[SIOCSIWPRIV] (cscan): %d", ret);
		else
			ret = 0;	/* Hide error in case of bg scan */
	}
	return ret;
}

static int wpa_driver_wext_set_cscan_params(char *buf, size_t buf_len, char *cmd)
{
	char *pasv_ptr;
	int bp, i;
	u16 pasv_dwell = WEXT_CSCAN_PASV_DWELL_TIME_DEF;
	u8 channel;

	wpa_printf(MSG_DEBUG, "%s: %s", __func__, cmd);

	/* Get command parameters */
	pasv_ptr = os_strstr(cmd, ",TIME=");
	if (pasv_ptr) {
		*pasv_ptr = '\0';
		pasv_ptr += 6;
		pasv_dwell = (u16)atoi(pasv_ptr);
		if (pasv_dwell == 0)
			pasv_dwell = WEXT_CSCAN_PASV_DWELL_TIME_DEF;
	}
	channel = (u8)atoi(cmd + 5);

	bp = WEXT_CSCAN_HEADER_SIZE;
	os_memcpy(buf, WEXT_CSCAN_HEADER, bp);

	/* Set list of channels */
	buf[bp++] = WEXT_CSCAN_CHANNEL_SECTION;
	buf[bp++] = channel;
	if (channel != 0) {
		i = (pasv_dwell - 1) / WEXT_CSCAN_PASV_DWELL_TIME_DEF;
		for (; i > 0; i--) {
			if ((size_t)(bp + 12) >= buf_len)
				break;
			buf[bp++] = WEXT_CSCAN_CHANNEL_SECTION;
			buf[bp++] = channel;
		}
	} else {
		if (pasv_dwell > WEXT_CSCAN_PASV_DWELL_TIME_MAX)
			pasv_dwell = WEXT_CSCAN_PASV_DWELL_TIME_MAX;
	}

	/* Set passive dwell time (default is 250) */
	buf[bp++] = WEXT_CSCAN_PASV_DWELL_SECTION;
	if (channel != 0) {
		buf[bp++] = (u8)WEXT_CSCAN_PASV_DWELL_TIME_DEF;
		buf[bp++] = (u8)(WEXT_CSCAN_PASV_DWELL_TIME_DEF >> 8);
	} else {
		buf[bp++] = (u8)pasv_dwell;
		buf[bp++] = (u8)(pasv_dwell >> 8);
	}

	/* Set home dwell time (default is 40) */
	buf[bp++] = WEXT_CSCAN_HOME_DWELL_SECTION;
	buf[bp++] = (u8)WEXT_CSCAN_HOME_DWELL_TIME;
	buf[bp++] = (u8)(WEXT_CSCAN_HOME_DWELL_TIME >> 8);

	/* Set cscan type */
	buf[bp++] = WEXT_CSCAN_TYPE_SECTION;
	buf[bp++] = WEXT_CSCAN_TYPE_PASSIVE;
	return bp;
}

static char *wpa_driver_get_country_code(int channels)
{
	char *country = "US"; /* WEXT_NUMBER_SCAN_CHANNELS_FCC */

	if (channels == WEXT_NUMBER_SCAN_CHANNELS_ETSI)
		country = "EU";
	else if( channels == WEXT_NUMBER_SCAN_CHANNELS_MKK1)
		country = "JP";
	return country;
}

#ifndef HOSTAPD
#define WEXT_BGSCAN_HEADER           "BGSCAN-CONFIG "
#define WEXT_BGSCAN_HEADER_SIZE      14
#define WEXT_SSID_AMOUNT             16
#define WEXT_BGCAN_BUF_LEN           720
#define WEXT_CSCAN_SSID_SECTION      'S'
#define SSID_MAX_SIZE                32
#define WEXT_BGSCAN_RSSI_SECTION     'R'
#define WEXT_BGSCAN_INTERVAL_SECTION 'T'
#define WEXT_BGSCAN_INTERVAL_DEF     30
#define WEXT_BGSCAN_REPEAT_SECTION   'E'
#define WEXT_BGSCAN_REPEAT_DEF       5

static char *getop(char *s, int *first_time)
{
	const char delim[] = " \t\n";
	char *p;
	if (*first_time){
		p = strtok(s, delim);
		*first_time = FALSE;
	}
	else{
		p = strtok(NULL, delim);
	}
	return (p);
}

static int wpa_driver_set_backgroundscan_params(void *priv, char *cmd)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_nl80211_data *drv = bss->drv;
	struct ifreq ifr;
	android_wifi_priv_cmd priv_cmd;
	struct wpa_supplicant *wpa_s;
	int ret = 0, i = 0, bp;
	char buf[WEXT_BGCAN_BUF_LEN];
	struct wpa_ssid *ssid_conf;
	int first_time = TRUE;
	char *opstr = NULL;
	char *ptr = NULL;
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

	bp = WEXT_BGSCAN_HEADER_SIZE;
	os_memcpy(buf, WEXT_BGSCAN_HEADER, bp);

	opstr = getop(cmd, &first_time);
	while ((opstr = getop(cmd, &first_time)) != NULL) {
		if((ptr = strstr(opstr, "SSID=")) != NULL) {
			find_ssid = 1;
			ptr = ptr + strlen("SSID=");
			buf[bp++] = WEXT_CSCAN_SSID_SECTION;
			buf[bp++] = strlen(ptr);
			os_memcpy(&buf[bp], ptr, strlen(ptr));
			bp += strlen(ptr);
			i++;
		}
		else if((ptr = strstr(opstr, "RSSI=")) != NULL) {
			ptr = ptr + strlen("RSSI=");
			buf[bp++] = WEXT_BGSCAN_RSSI_SECTION;
			buf[bp++] = atoi(ptr);
		}
		else if((ptr = strstr(opstr, "INTERVAL=")) != NULL) {
			find_interval = 1;
			ptr = ptr + strlen("INTERVAL=");
			buf[bp++] = WEXT_BGSCAN_INTERVAL_SECTION;
			buf[bp++] = (u8)atoi(ptr);
			buf[bp++] = (u8)(atoi(ptr) >> 8);
		}
		else if((ptr = strstr(opstr, "REPEAT=")) != NULL) {
			find_repeat = 1;
			ptr = ptr + strlen("REPEAT=");
			buf[bp++] = WEXT_BGSCAN_REPEAT_SECTION;
			buf[bp++] = (u8)atoi(ptr);
		}
	}

	if(!find_ssid) {
		while ((i < WEXT_SSID_AMOUNT) && (ssid_conf != NULL)) {
			if ((!ssid_conf->disabled) && (ssid_conf->ssid_len <= SSID_MAX_SIZE)){
				wpa_printf(MSG_DEBUG, "For BG Scan: %s", ssid_conf->ssid);
				buf[bp++] = WEXT_CSCAN_SSID_SECTION;
				buf[bp++] = ssid_conf->ssid_len;
				os_memcpy(&buf[bp], ssid_conf->ssid, ssid_conf->ssid_len);
				bp += ssid_conf->ssid_len;
				i++;
			}
			ssid_conf = ssid_conf->next;
		}
	}

	if(!find_interval){
		buf[bp++] = WEXT_BGSCAN_INTERVAL_SECTION;
		buf[bp++] = WEXT_BGSCAN_INTERVAL_DEF;
		buf[bp++] = (WEXT_BGSCAN_INTERVAL_DEF >> 8);
	}

	if(!find_repeat){
		buf[bp++] = WEXT_BGSCAN_REPEAT_SECTION;
		buf[bp++] = WEXT_BGSCAN_REPEAT_DEF;
	}

	memset(&ifr, 0, sizeof(ifr));
	memset(&priv_cmd, 0, sizeof(priv_cmd));
	os_strncpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

	priv_cmd.buf = buf;
	priv_cmd.used_len = WEXT_BGCAN_BUF_LEN;
	priv_cmd.total_len = WEXT_BGCAN_BUF_LEN;
	ifr.ifr_data = &priv_cmd;

	if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 14, &ifr)) < 0) {
		wpa_printf(MSG_ERROR, "ioctl[SIOCSIWPRIV] (bgscan config): %d", ret);
	} else {
		wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret, strlen(buf));
	}
	return ret;
}
#endif

int wpa_driver_wext_driver_cmd( void *priv, char *cmd, char *buf, size_t buf_len )
{
	struct wpa_driver_wext_data *drv = priv;
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(drv->ctx);
	struct iwreq iwr;
	int ret = 0, flags;

	wpa_printf(MSG_DEBUG, "%s %s len = %d", __func__, cmd, buf_len);

	if (!drv->driver_is_started && (os_strcasecmp(cmd, "START") != 0)) {
		wpa_printf(MSG_ERROR,"WEXT: Driver not initialized yet");
		return -1;
	}

	if (os_strcasecmp(cmd, "RSSI-APPROX") == 0) {
		os_strncpy(cmd, RSSI_CMD, MAX_DRV_CMD_SIZE);
	} else if( os_strncasecmp(cmd, "SCAN-CHANNELS", 13) == 0 ) {
		int no_of_chan;

		no_of_chan = atoi(cmd + 13);
		os_snprintf(cmd, MAX_DRV_CMD_SIZE, "COUNTRY %s",
			wpa_driver_get_country_code(no_of_chan));
	} else if (os_strcasecmp(cmd, "STOP") == 0) {
		linux_set_iface_flags(drv->ioctl_sock, drv->ifname, 0);
	} else if( os_strcasecmp(cmd, "RELOAD") == 0 ) {
		wpa_printf(MSG_DEBUG,"Reload command");
		wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
		return ret;
#ifndef HOSTAPD
	} else if( os_strncasecmp(cmd, "BGSCAN-START") == 0 ) {
		/* Issue a command 'BGSCAN-CONFIG' to driver */
		ret = wpa_driver_set_backgroundscan_params(priv);
		if (ret < 0) {
			wpa_printf(MSG_ERROR, "%s: failed to issue command: BGSCAN-START\n", __func__);
		}
		drv->bgscan_enabled = 1;
		return ret;
	} else if( os_strcasecmp(cmd, "BGSCAN-STOP") == 0 ) {
		drv->bgscan_enabled = 0;
		return 0;
#endif
	}

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	os_memcpy(buf, cmd, strlen(cmd) + 1);
	iwr.u.data.pointer = buf;
	iwr.u.data.length = buf_len;

	if( os_strncasecmp(cmd, "CSCAN", 5) == 0 ) {
		if (!wpa_s->scanning && ((wpa_s->wpa_state <= WPA_SCANNING) ||
					(wpa_s->wpa_state >= WPA_COMPLETED))) {
			iwr.u.data.length = wpa_driver_wext_set_cscan_params(buf, buf_len, cmd);
		} else {
			wpa_printf(MSG_ERROR, "Ongoing Scan action...");
			return ret;
		}
	}

	ret = ioctl(drv->ioctl_sock, SIOCSIWPRIV, &iwr);

	if (ret < 0) {
		wpa_printf(MSG_ERROR, "%s failed (%d): %s", __func__, ret, cmd);
		drv->errors++;
		if (drv->errors > DRV_NUMBER_SEQUENTIAL_ERRORS) {
			drv->errors = 0;
			wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
		}
	} else {
		drv->errors = 0;
		ret = 0;
		if ((os_strcasecmp(cmd, RSSI_CMD) == 0) ||
		    (os_strcasecmp(cmd, LINKSPEED_CMD) == 0) ||
		    (os_strcasecmp(cmd, "MACADDR") == 0) ||
		    (os_strcasecmp(cmd, "GETPOWER") == 0) ||
		    (os_strcasecmp(cmd, "GETBAND") == 0)) {
			ret = strlen(buf);
		} else if (os_strcasecmp(cmd, "START") == 0) {
			drv->driver_is_started = TRUE;
			linux_set_iface_flags(drv->ioctl_sock, drv->ifname, 1);
			/* os_sleep(0, WPA_DRIVER_WEXT_WAIT_US);
			wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED"); */
		} else if (os_strcasecmp(cmd, "STOP") == 0) {
			drv->driver_is_started = FALSE;
			/* wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED"); */
		} else if (os_strncasecmp(cmd, "CSCAN", 5) == 0) {
			wpa_driver_wext_set_scan_timeout(priv);
			wpa_supplicant_notify_scanning(wpa_s, 1);
		}
		wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret, strlen(buf));
	}
	return ret;
}

int wpa_driver_signal_poll(void *priv, struct wpa_signal_info *si)
{
	char buf[MAX_DRV_CMD_SIZE];
	struct wpa_driver_wext_data *drv = priv;
	char *prssi;
	int res;

	os_memset(si, 0, sizeof(*si));
	res = wpa_driver_wext_driver_cmd(priv, RSSI_CMD, buf, sizeof(buf));
	/* Answer: SSID rssi -Val */
	if (res < 0)
		return res;
	prssi = strcasestr(buf, RSSI_CMD);
	if (!prssi)
		return -1;
	si->current_signal = atoi(prssi + strlen(RSSI_CMD) + 1);

	res = wpa_driver_wext_driver_cmd(priv, LINKSPEED_CMD, buf, sizeof(buf));
	/* Answer: LinkSpeed Val */
	if (res < 0)
		return res;
	si->current_txrate = atoi(buf + strlen(LINKSPEED_CMD) + 1) * 1000;

	return 0;
}

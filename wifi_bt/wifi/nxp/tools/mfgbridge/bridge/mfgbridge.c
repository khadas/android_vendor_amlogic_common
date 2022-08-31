/** @file mfgbridge.c
 *
 *  @brief This file contains MFG bridge code.
 *
 * (C) Copyright 2011-2017 Marvell International Ltd. All Rights Reserved
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include "mfgbridge.h"
#include "../drvwrapper/drv_wrapper.h"
#include "mfgdebug.h"

#include <fcntl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
//#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#define BRIDGE_VERSION "0.3.0.06"

#define RETRIES_FOR_X  3
#define RETRIES  12
#define CRCTRIES 2
#define PADCHAR  0x1a
#define SOH      1
#define EOT      4
#define ACK      6
#define NAK      0x15
#define CAN      0x18
#define CRC      'C'

#define TRUE 1
#define FALSE 0

// Prototypes:

enum
{
    kNumRetries = 3
};

// Hold the original termios attributes so we can reset them
static struct termios ttyas;

#ifdef NONPLUG_SUPPORT
static unsigned char uart_buf[2048] = { 0 };    // uart buffer

#ifndef MFG_UPDATE
static int uart_fd = -1;
#endif
static char HCI_PORT[256] = "hci0";     /* Default BT interface hci0 */
static char UART_PORT[256] = "/dev/ttyS0";      /* Default host Uart port */
speed_t BAUDRATE = B38400;      // default UART speed
unsigned int crc32_table[256];
#define CRC32_POLY 0x04c11db7
#endif /* NONPLUG_SUPPORT */

#define FD_MAX(x,y)   ((x)> (y) ? (x) : (y))
#define UART_BUF_SIZE    2048
//static char        WLAN_PORT[256] = "mlan0"; /* Default WiFi interface mlan0 */
static bridge_cb Bridge;
drv_config Drv_Config;
//char               LOAD_SCRIPT[256], UNLOAD_SCRIPT[256];

static char TFTP_CMD[256] = "tftp 192.168.1.100 -c get ";       /* Default TFTP
                                                                   command */
static char FW_LOCATION[256] = "/lib/firmware/mrvl/usb8782.bin";        /* Default
                                                                           firmware
                                                                           location
                                                                         */
char load_script[256], unload_script[256];
int proto = UDP;                /* Default protocol UDP */
int mode = MODE_AUTO;           /* Default mode Auto */

char RAW_UART_PORT[256] = {0};      /* default hci Uart port */

static char *usage[] = {
    "Usage: ",
#ifndef NONPLUG_SUPPORT
    "   mfgbridge [-hvB] ",
#else
    "   mfgbridge [-shvB] ",
    "   -s = ",                 /* TODO */
#endif
    "   -h = help",
    "   -v = version",
    "   -B = run the process in background.",
};
extern int  WiFidevicecnt;
extern struct _new_drv_cb *Driver1, *MultiDevPtr;
char multi_wlan_ifname[MAXBUF]="\n";
/********************************************************
        Local Variables
********************************************************/
int s_port_num;
int c_port_num;

int ethio_flag=1;
/********************************************************
        Local Functions
********************************************************/
static void
display_usage(void)
{
    unsigned int i;
    for (i = 0; i < sizeof(usage) / sizeof(usage[0]); i++)
        printf("%s\n", usage[i]);
}

void *
get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int get_string(const char *pos, char *buf)
{
    const char *pos1, *pos2;
    int len;

    pos1 = strchr(pos, '"');
    if (!pos1)
        return -1;
    pos1++;

    pos2 = strchr(pos1, '"');
    if (!pos2)
        return -1;
    len = pos2 - pos1;
    memcpy(buf, pos1, len);

    return len;
}

#ifdef NONPLUG_SUPPORT
void
init_crc32()
{
    int i, j;
    unsigned int c;
    for (i = 0; i < 256; ++i) {
        for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
        crc32_table[i] = c;
    }
}

unsigned int
get_crc32(int len, unsigned char *buf)
{
    unsigned char *p;
    unsigned int crc;
    crc = 0xffffffff;
    for (p = buf; len > 0; ++p, --len)
        crc = (crc << 8) ^ crc32_table[(crc >> 24) ^ *p];
    return ~crc;
}

void
get_bridge_chk_sum(int byte_count, unsigned int *p_bridge_chksum)
{
    int i;
    unsigned int sum;
    sum = 0;
    for (i = 0; i < byte_count; i++) {
        sum += uart_buf[4 + i++];
        sum += (uart_buf[4 + i] << 8);
    }
    *p_bridge_chksum = sum;
}
#endif /* NONPLUG_SUPPORT */

#ifdef NONPLUG_SUPPORT

void
uart_init_crc32(uart_cb * uart)
{
    int i, j;
    unsigned int c;
    for (i = 0; i < 256; ++i) {
        for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
        uart->crc32_table[i] = c;
    }
}

unsigned int
uart_get_crc32(uart_cb * uart, int len, unsigned char *buf)
{
    unsigned int *crc32_table = uart->crc32_table;
    unsigned char *p;
    unsigned int crc;
    crc = 0xffffffff;
    for (p = buf; len > 0; ++p, --len)
        crc = (crc << 8) ^ (crc32_table[(crc >> 24) ^ *p]);
    return ~crc;
}

// Wait x seconds for char in the com. port, return -1 if none, char otherwise
static int
uart_rd_char_timeout(uart_cb * uart, int nSeconds)
{
    struct termios options;
    char value;
    int k = 0;
    // Get the current options and save them so we can restore the default
    // settings later.
    if (tcgetattr(uart->uart_fd, &ttyas) == -1) {
        printf("Error getting tty attributes d).\n");
    }
    options = ttyas;

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = nSeconds;

    if (tcsetattr(uart->uart_fd, TCSANOW, &options) == -1) {
        printf("Error setting tty attributes\n");
    }

    k = read(uart->uart_fd, &value, 1);

    if (k == -1)
        return k;
    else
        return value;
}

// Write a character to the port specified by nPortID
// returns TRUE if success, FALSE if failure
int
uart_wr_char(uart_cb * uart, char towrite)
{
    char local;
    local = towrite;
    return write(uart->uart_fd, &local, 1);
}

// this is modified Xmodem downloader that use the size parameter to
// determine the end of the file transfer
int
uart_download_image(uart_cb * uart, char *imag_file_name, int file_size)
{
    int nCurrentBlock = 0, nSOH = 0, nBlock, nNotBlock, i;
    int bCRC = TRUE, bFirst = TRUE;
    unsigned short wOurChecksum, wChecksumSent;
    int gcTries;                // retry counter
    unsigned char gachBuffer[130];      // I/O buffer

    int progress;
    FILE *fd;
    fd = fopen(imag_file_name, "wba");

    progress = file_size;
    // Send Cs then NAKs until the sender starts sending
    gcTries = 0;
    while (nSOH != SOH) {
        bCRC = (gcTries++ < CRCTRIES);
        uart_wr_char(uart, NAK);
        nSOH = uart_rd_char_timeout(uart, 6);
        printf("nSOH = %X  \n", nSOH);
        if (nSOH != -1 && nSOH != SOH)
            sleep(6);
    }
    if (bCRC)
        printf("XMODEM Download (CRC) \n");

    while ((gcTries < RETRIES)) {
        // get the block to write file
        if (!bFirst) {
            nSOH = uart_rd_char_timeout(uart, 10);
            if (nSOH == -1)     // TimeOut
                continue;
            else if (nSOH == CAN)
                break;
            else if (nSOH == EOT) {
                uart_wr_char(uart, ACK);
                break;
            }
        }
        bFirst = FALSE;
        nBlock = uart_rd_char_timeout(uart, 1); // block number
        nNotBlock = uart_rd_char_timeout(uart, 1);      // 1's complement
        // get data block
        for (i = 0, wOurChecksum = 0; i < 128; i++) {
            int nValue;
            nValue = uart_rd_char_timeout(uart, 1);
            // printf("0x%2.2x ",nValue);
            // if (nValue < 0) // Time out?
            // break;
            gachBuffer[i] = (char) nValue;
            wOurChecksum = (wOurChecksum + (gachBuffer[i] & 255)) & 255;
        }
        if (i != 128)
            continue;
        // checksum or crc from sender
        wChecksumSent = uart_rd_char_timeout(uart, 1);
        if (bCRC)
            wChecksumSent =
                (wChecksumSent << 8) + uart_rd_char_timeout(uart, 1);
        if (nSOH != SOH)        // Check the nSOH
        {
            printf("receive invalid SOH nSOH = %X  \n", nSOH);
            continue;
        }
        if ((0xff & nBlock) == (0xff & nCurrentBlock))
            fseek(fd, -128, SEEK_CUR);
        else if ((0xff & nBlock) != (0xff & (nCurrentBlock + 1))) {
            uart_wr_char(uart, CAN);
            printf("receive invalid block \n");
            break;
        } else
            nCurrentBlock++;
        // Test the block # 1s complement
        if ((0xff & nNotBlock) != ((~nCurrentBlock) & 0xff)) {
            printf("receive invalid block \n");
            continue;
        }
        nSOH = nBlock = nNotBlock = wChecksumSent = gcTries = 0;
        if (progress > 128)
            fwrite(gachBuffer, 128, 1, fd);
        else {
            if (progress > 0)
                fwrite(gachBuffer, progress, 1, fd);
        }
        progress = progress - 128;
        uart_wr_char(uart, ACK);
    }
    fclose(fd);
    return (nSOH == EOT);       // return TRUE if transfer was succesfull
}

int
uart_init(uart_cb * uart)
{
    struct termios ti;
    uart->uart_fd = -1;

    mfg_dprint(DBG_MINFO, "UART: initialize ...\n");

    uart->uart_fd = open(UART_PORT, O_RDWR | O_NOCTTY);

    if (uart->uart_fd < 0) {
        perror("Can't open serial port");
        return -1;
    }

    tcflush(uart->uart_fd, TCIOFLUSH);

    if (tcgetattr(uart->uart_fd, &ti) < 0) {
        perror("Can't get port settings");
        return -1;
    }

    cfmakeraw(&ti);
    ti.c_cflag |= CLOCAL;

    // Set 1 stop bit & no parity (8-bit data already handled by cfmakeraw)
    ti.c_cflag &= ~(CSTOPB | PARENB);

    ti.c_cflag |= CRTSCTS;

    // FOR READS: set timeout time w/ no minimum characters needed (since we
    // read only 1 at at time...)
    ti.c_cc[VMIN] = 0;
    ti.c_cc[VTIME] = 60; /* deciseconds */
    cfsetispeed(&ti, BAUDRATE);
    cfsetospeed(&ti, BAUDRATE);

    if (tcsetattr(uart->uart_fd, TCSANOW, &ti) < 0) {
        perror("Can't set port settings");
        return -1;
    }
    tcflush(uart->uart_fd, TCIOFLUSH);

    uart_init_crc32(uart);

    // register callback
    uart->uart_download = uart_download_image;

    mfg_dprint(DBG_MINFO, "UART: initialization is completed.\n");

    return 0;

}

int
uart_recv_msg(uart_cb * uart, unsigned char *buf, int buflen)
{
    int i, index;
    int k = 0;
    int header_flag = 0;
    unsigned char *p_real, rd_buf[64];
    unsigned int host_chksum, bridge_chksum, cc = 0, nn = 0;
//    unsigned char  uart_buf[UART_BUF_SIZE];      // uart buffer

    // NOTE: uart_buf may not be needed in this function.
    unsigned char *uart_buf = uart->uart_buf;
    int msglen = 0;

    memset(rd_buf, 0, sizeof(rd_buf));
    memset(uart_buf, 0, UART_BUF_SIZE);
    p_real = uart_buf;

    while (1) {
        if (header_flag == 1) {
            k = read(uart->uart_fd, rd_buf, sizeof(rd_buf));
        }

        unsigned char temp_buf[4];      // for beginning header
        if (header_flag == 0) {
            for (i = 0; i < 4; i++) {
                k = read(uart->uart_fd, &temp_buf[i], 1);
            }
            mfg_dprint(DBG_MSG,
                       "UART:  header: 0x%02x 0x%02x 0x%02x 0x%02x \n",
                       temp_buf[0], temp_buf[1], temp_buf[2], temp_buf[3]);

            memcpy(rd_buf, temp_buf, 4);
            if (rd_buf[0] == 0x55) {
                nn = (int) (rd_buf[2] | (rd_buf[3] << 8));
                header_flag = 1;
            }

            k = 4;
        }
        memcpy(p_real, rd_buf, k);
        p_real += k;
        cc += k;
        if (cc >= (nn + sizeof(uart_header) + sizeof(host_chksum))) {
            header_flag = 0;
            index = sizeof(uart_header) + nn;
            host_chksum = (uart_buf[index] | (uart_buf[index + 1] << 8)
                           | (uart_buf[index + 2] << 16)
                           | (uart_buf[index + 3] << 24));

            mfg_dprint(DBG_GINFO,
                       "UART: receive a msg: number = %d, rx crc =  0x%8.8x\n",
                       p_real - uart_buf, host_chksum);

            mfg_hexdump(DBG_MSG, "RX Msg:", uart_buf, index + 4);

            bridge_chksum =
                uart_get_crc32(uart, nn, uart_buf + sizeof(uart_header));
            // get_bridge_chk_sum(nn,&bridge_chksum);
            if (host_chksum != bridge_chksum) {
                printf("check sum error\n");
                msglen = 0;
                break;
            }
            msglen = nn;
            if (buflen < msglen) {
                printf
                    ("ERROR: (%s) buffer is small (buflen = %d, msglen = %d.\n",
                     __FUNCTION__, buflen, msglen);
                msglen = 0;
                break;
            }

            memcpy(buf, uart_buf + sizeof(uart_header), msglen);
            break;
        }
    }
    return msglen;
}

int
uart_send_msg(uart_cb * uart, unsigned char *msg, int msglen)
{
//    unsigned char  uart_buf[UART_BUF_SIZE];      // uart buffer
    unsigned char *uart_buf = uart->uart_buf;
    int index;
    int num_bytes = 0;
    unsigned int bridge_chksum = 0;
//    unsigned int   i;

    memset(uart_buf, 0, UART_BUF_SIZE);

    /* Copy the p_uart_info back to the receive buffer */
    memcpy((uart_buf + sizeof(uart_header)), msg, msglen);

    mfg_dprint(DBG_GINFO, "UART: send a msg.\n");

    /* caculate CRC */
    bridge_chksum =
        uart_get_crc32(uart, msglen, uart_buf + sizeof(uart_header));
    index = sizeof(uart_header) + msglen;

    mfg_dprint(DBG_GINFO, "UART: tx crc= 0x%8.8x\n", bridge_chksum);
    uart_buf[index] = bridge_chksum & 0xff;
    uart_buf[index + 1] = (bridge_chksum & 0xff00) >> 8;
    uart_buf[index + 2] = (bridge_chksum & 0xff0000) >> 16;
    uart_buf[index + 3] = (bridge_chksum & 0xff000000) >> 24;

    /* fill in packet header */
    uart_buf[0] = 0x55;
    uart_buf[1] = 0x55;

    /* fill in packet length */
    uart_buf[2] = msglen & 0xff;
    uart_buf[3] = (msglen & 0xff00) >> 8;

    mfg_hexdump(DBG_MSG, "TX Msg:", uart_buf,
                sizeof(uart_header) + sizeof(bridge_chksum) + msglen);

    num_bytes = write(uart->uart_fd, uart_buf, 8 + msglen);
    mfg_dprint(DBG_GINFO, "UART: bytes sent = %d\n", num_bytes);

    return 0;
}

#endif /* NONPLUG_SUPPORT */

int
net_download_image(net_cb * net, char *image_file, int file_size)
{
    system(strcat(strcat(TFTP_CMD, image_file), FW_LOCATION));
    return 0;
}

int
net_send_msg(net_cb * net, struct sockaddr_in *client, unsigned char *buf,
             int buflen)
{
    int returnStatus = -1;

    printf("NET:  send a msg.\n");
    /* Send response */
    client->sin_port = htons(net->client_port);
    returnStatus = sendto(net->currfd, buf, buflen, 0,
                          (const struct sockaddr *) client,
                          sizeof(struct sockaddr_in));

    if (returnStatus == -1) {
        fprintf(stderr, "Could not send response to remote!\n");
        return -1;
    } else {
        printf("NET:  the msg is sent.\n");
    }
    return 0;

}

int
net_init(net_cb * net)
{
    int returnStatus = -1;
    struct sockaddr_in server;

    // register callback
    net->net_download = net_download_image;

    net->skfd = -1;

    mfg_dprint(DBG_MINFO, "NET:  initialize ...\n");

    /* create a socket */
    if (net->proto == UDP) {
        net->skfd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        net->skfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (net->skfd == -1) {
        mfg_dprint(DBG_ERROR, "NET:  could not create a socket!\n");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(net->server_port);

    /* bind to the socket */
    returnStatus = bind(net->skfd, (struct sockaddr *) &server, sizeof(server));

    if (returnStatus == 0) {
        mfg_dprint(DBG_GINFO, "NET:  socket bind is completed!\n");
    } else {
        mfg_dprint(DBG_ERROR, "NET:  could not bind to address!\n");
        close(net->skfd);
        return -1;
    }

#define BACKLOG 5
    if (net->proto == TCP) {
        if (listen(net->skfd, BACKLOG)) {
            printf("Listen failed\n");
            close(net->skfd);
            return -1;
        }
    }

    mfg_dprint(DBG_MINFO, "NET:  initialization is completed.\n");

    return 0;
}

int brdg_parse_config(bridge_cb * bridge, char *input_conf_file)
{
    char input_line[200];
    FILE *input;
    char *p, temp[5];
    int n;
    net_cb *net = &bridge->net;
    drv_config *drv_conf = &Drv_Config;
#ifdef NONPLUG_SUPPORT
    int baud_in_num;
#endif
    int multi_wifi_device_no=0;
    
    if (!(input = fopen(input_conf_file, "rw"))) {
        printf("Failed to open file\n");
        goto error;
    }

    while (fgets(input_line, 100, input)) {
        if (input_line[0] == '#')
            continue;

        if (input_line[0] == '\n')
            continue;

        if (input_line[0] == '%') {
            /* break if the line starts with '%' */
            break;
        }

        if (strstr(input_line, "ethio_flag")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            ethio_flag = atoi(++p);
            continue;
        }
        if (strstr(input_line, "Server_port")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            net->server_port = atoi(++p);
            continue;
        }        
        if (strstr(input_line, "Client_port")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            net->client_port = atoi(++p);
            continue;
        }
#if 0
        if (strstr(input_line, "WLAN_interface_name1")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, drv_conf->wlan_port);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
        if (strstr(input_line, "WLAN_interface_name2")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, secondWlanif_name);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
#endif //#if 0
        //Get WiFi interface count
        if (strstr(input_line, "WLAN_interface_count")) {
             p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            WiFidevicecnt = atoi(++p);
             printf("WiFidevicecnt=%d\n", WiFidevicecnt);
           if(WiFidevicecnt)
        	{
        		  multi_wifi_device_no =0;
        			Driver1 = (new_drv_cb *)malloc(sizeof(new_drv_cb) * WiFidevicecnt);  
        			MultiDevPtr = Driver1;  		
							sprintf(multi_wlan_ifname, "WLAN_interface_name_%d",multi_wifi_device_no);
							printf("Serach %s\n",multi_wlan_ifname);  
        	}
            continue;
        } 

         if (strstr(input_line, multi_wlan_ifname)) {

            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            MultiDevPtr = Driver1; 
                MultiDevPtr +=multi_wifi_device_no;
            n = get_string(++p, &MultiDevPtr->wlan_ifname);
            if (n == -1)
                goto invalid_format_error;
            
            printf("Found %s\n", &MultiDevPtr->wlan_ifname); 
            multi_wifi_device_no++;
            if(multi_wifi_device_no < WiFidevicecnt)
            	{
            		sprintf(multi_wlan_ifname, "WLAN_interface_name_%d",multi_wifi_device_no); 
            		printf("DEBUG>>Serach %s\n",multi_wlan_ifname); 
            	}
            continue;
        }
       
#ifdef NONPLUG_SUPPORT
        if (strstr(input_line, "BAUD")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            baud_in_num = atoi(++p);

            if (baud_in_num == 9600)
                BAUDRATE = B9600;
            else if (baud_in_num == 19200)
                BAUDRATE = B19200;
            else if (baud_in_num == 38400)
                BAUDRATE = B38400;
            else if (baud_in_num == 115200)
                BAUDRATE = B115200;
            else {
                printf("Fail parser !!! Invalid baud rate %d\n", baud_in_num);
                goto error;
            }
            continue;
        }

        if (strstr(input_line, "Serial")) {
            memset(UART_PORT, 0, 256);
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, UART_PORT);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
        if (strstr(input_line, "tftp_command")) {
            memset(TFTP_CMD, 0, 256);
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, TFTP_CMD);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
        if (strstr(input_line, "Rawur_port")) {
            memset(RAW_UART_PORT, 0, 256);
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, RAW_UART_PORT);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
        if (strstr(input_line, "fw")) {
            memset(FW_LOCATION, 0, 256);
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, FW_LOCATION);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
	   if (strstr(input_line,  "BT_interface_name_Master")) {	
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, drv_conf->hci_port[0]);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
     if (strstr(input_line,  "BT_interface_name_Slave")) {	
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, drv_conf->hci_port[1]);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
#endif /* NONPLUG_SUPPORT */

        if (strstr(input_line, "Mode")) {
            memset(temp, 0, sizeof(temp));
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, temp);
            if (n == -1)
                goto invalid_format_error;
            if (!strcmp(temp, "Ethr")) {
                bridge->mode = MODE_ETHERNET;
            } else if (!strcmp(temp, "Auto")) {
                bridge->mode = MODE_AUTO;
            } else {
                printf("Fail parser !!! Invalid mode\"%s\"\n", temp);
                goto error;
            }
            continue;
        }
        if (strstr(input_line, "Protocol")) {
            memset(temp, 0, sizeof(temp));
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, temp);
            if (n == -1)
                goto invalid_format_error;
            if (!strcmp(temp, "TCP")) {
                net->proto = TCP;
            } else if (!strcmp(temp, "UDP")) {
                net->proto = UDP;
            } else {
                printf("Fail parser !!! Invalid protocol \"%s\"\n", temp);
                goto error;
            }
            continue;
        }
        if (strstr(input_line, "Load_script")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, drv_conf->load_script);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
        if (strstr(input_line, "Unload_script")) {
            p = strchr(input_line, '=');
            if (!p)
                goto invalid_format_error;
            n = get_string(++p, drv_conf->unload_script);
            if (n == -1)
                goto invalid_format_error;
            continue;
        }
    }

    fclose(input);
    return 0;

invalid_format_error:
    printf("Fail parser !!! Invalid line format\n");
error:
    fclose(input);
    return -1;
}

//The function may overwrite the request (input) msg buffer and msg length with a response message as follows:
// paramters: msgbuf: input:  request cmd
//                    output: response cmd
//            msglen:
//                    output: size of response cmd
//            buflen: input:  buffer size for storing request/response cmd
//            port:   input:  uart or net
//
int
brdg_process_msg(bridge_cb * bridge, unsigned char *msgbuf, int *msglen,
                 int buflen, int port)
{

//    char           *script = NULL;
    char *ifname = NULL;
    unsigned char *cmd_payload = NULL;
    int cmd_payload_len = 0;
    cmd_header *cmd_hd = (cmd_header *) msgbuf;
    int cmdlen = cmd_hd->length;
    int ret = -1;
	int HCIDeviceID =0;

    *msglen = cmdlen;

    if (cmdlen > buflen) {
        printf
            ("ERROR: (%s) buffer is small (buflen = %d, cmdlen = %d.\n",
             __FUNCTION__, buflen, cmdlen);
        return -1;
    }
    mfg_dprint(DBG_MINFO, "BRDG: process Rx msg ... \n");

#define BRDG_CMD_HDR_LEN          12    // sizeof(cmd_hd)
#define UPDT_CMD_PROJ_NAME_LEN    32
#define UPDT_CMD_FW_NAME_LEN      32

    switch (cmd_hd->type) {
    case TYPE_LOCAL:
        switch (cmd_hd->sub_type) {
        case SUB_TYPE_LOAD_DRV:
            if ( bridge->drv_load(bridge->drv, DRV_IF_WLAN) == -1)
		cmd_hd->status = 0xffff;	
            break;
        case SUB_TYPE_UNLOAD_DRV:
            if ( bridge->drv_unload(bridge->drv, DRV_IF_WLAN) == -1)
		cmd_hd->status = 0xffff;	
            break;
        default:
            printf("Unknown local command subtype: %d\n", cmd_hd->sub_type);
        }                       // end of switch subtype
        break;
    case TYPE_WLAN:
        mfg_dprint(DBG_MINFO, "NET:  WLAN command.\n");

        cmd_payload = msgbuf + sizeof(cmd_header);
        cmd_payload_len = cmdlen - sizeof(cmd_header);
        ret =
            bridge->drv_proc_wlan_cmd(bridge->drv, cmd_payload,
                                      &cmd_payload_len, buflen);
        /* Update command header information */
        cmd_hd->status = ret;
        cmd_hd->length = cmd_payload_len;
        *msglen = cmd_payload_len + sizeof(cmd_header);
        break;
#ifdef NONPLUG_SUPPORT
    case TYPE_HCI:
        printf("HCI Command\n");
        cmd_payload = msgbuf + sizeof(cmd_header);
//            cmd_payload_len = *msglen - sizeof(cmd_header);
        cmd_payload_len = cmdlen - sizeof(cmd_header);
        ifname = HCI_PORT;

        HCIDeviceID = cmd_hd->Demux.DeviceID;
        printf("HCIDeviceID=0x%x\n",HCIDeviceID);
        ret =
            bridge->drv_proc_hci_cmd(bridge->drv, cmd_payload,
                                     &cmd_payload_len, buflen, HCIDeviceID);

        /* Update command header information */
        cmd_hd->status = ret;
        cmd_hd->length = cmd_payload_len + sizeof(cmd_header);
        *msglen = cmd_payload_len + sizeof(cmd_header);
        break;
#endif
    default:
        printf("Unknown command type: %d\n", cmd_hd->type);
    }

    return 0;
}

int
brdg_init(bridge_cb * bridge)
{
    memset(bridge, 0, sizeof(bridge_cb));

#ifdef NONPLUG_SUPPORT
    bridge->mode = MODE_AUTO;
#else
    bridge->mode = MODE_ETHERNET;
#endif
    // register driver callback
    bridge->drv_load = drv_load_driver;
    bridge->drv_unload = drv_unload_driver;
    bridge->drv_proc_wlan_cmd = drv_proc_wlan_command;
#ifdef NONPLUG_SUPPORT
    bridge->drv_proc_hci_cmd = drv_proc_hci_command;
#endif

    return 0;
}

int
main(int argc, char *argv[])
{
    int new_skfd, j, bytes;
//    unsigned char            buf[MAXBUF];
    unsigned char *buf = NULL;
    int c, fdmax, daemonize = FALSE;
    fd_set master, read_fds;
    struct sockaddr_storage new_client;
    char new_client_ip[INET6_ADDRSTRLEN];
    struct sockaddr_in client;
    socklen_t addrlen;
    net_cb *net = NULL;
#ifdef NONPLUG_SUPPORT
    uart_cb *uart = NULL;
#endif
    bridge_cb *bridge = NULL;
    int msglen = 0;
    int a=0;

    drv_config_default(&Drv_Config);

    bridge = &Bridge;
    brdg_init(bridge);

#ifdef NONPLUG_SUPPORT
    uart = &bridge->uart;
#endif
    net = &bridge->net;

    for (;;) {
#ifdef NONPLUG_SUPPORT
        c = getopt(argc, argv, "Bshv");
#else
        c = getopt(argc, argv, "Bhv");
#endif
        /* check if all command-line options have been parsed */
        if (c == -1)
            break;

        switch (c) {
        case 'B':
            daemonize = TRUE;
            break;
#ifdef NONPLUG_SUPPORT
        case 's':
            uart_init(uart);
            break;
#endif
        case 'h':
            display_usage();
            return 0;
        case 'v':
            printf("Marvell MFG Tool Bridge Version: %s\n", BRIDGE_VERSION);
            return 0;
        default:
            printf("Invalid argument\n");
            display_usage();
            return 0;
        }
    }

    /** Make the process background-process */
    if (daemonize)
        daemon(1, 0);

    if (brdg_parse_config(&Bridge, "bridge_init.conf") != 0) {
        /* Failed to parse */
        return 0;
    }
    //Debug
    {
    	MultiDevPtr = Driver1; 
      for (a=0; a<WiFidevicecnt; a++)
      {     
    		printf("DEBUG>>%s\n", &MultiDevPtr->wlan_ifname);
    		MultiDevPtr++;
    	}
    }
#ifdef NONPLUG_SUPPORT
    if (bridge->mode == MODE_AUTO) {
        uart_init(uart);
    }
#endif

    if (net_init(net) < 0) {
        return 0;
    }

    mfg_dprint(DBG_GINFO, "NET:  server port: %d\n", net->server_port);
    mfg_dprint(DBG_GINFO, "NET:  client port: %d\n", net->client_port);

    buf = (unsigned char *) malloc(MAXBUF);

    /* Clear and set the FD set */
    FD_ZERO(&read_fds);

    // initialize master set
    FD_ZERO(&master);
    FD_SET(net->skfd, &master);
    fdmax = net->skfd;

#ifdef NONPLUG_SUPPORT
    if (bridge->mode == MODE_AUTO) {
        if (uart->uart_fd >= 0) {
            FD_SET(uart->uart_fd, &master);
            fdmax = FD_MAX(fdmax, uart->uart_fd);
        }
    }
#endif
    while (1) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            printf("ERROR: select error \n");
            continue;
        }
        for (j = 0; j <= fdmax + 1; j++) {
            if (FD_ISSET(j, &read_fds)) {
                mfg_dprint(DBG_MINFO, "NET:  socket FD = %d\n", j);
#ifdef NONPLUG_SUPPORT
                if (bridge->mode == MODE_AUTO) {
                    if (j == uart->uart_fd) {
                        bytes = uart_recv_msg(uart, buf, MAXBUF);
                        if (bytes > 0) {
                            brdg_process_msg(bridge,
                                             buf, &msglen, MAXBUF, UART);
                            // send a response
                            uart_send_msg(uart, buf, msglen);

                        }
                    }
                }
#endif /* NONPLUG_SUPPORT */

                if ((j == net->skfd) && (net->proto == TCP)) {
                    addrlen = sizeof(struct sockaddr_storage);
                    /* New client requect/reject */
                    new_skfd = accept(net->skfd, (struct sockaddr *)
                                      &new_client, &addrlen);
                    if (new_skfd == -1) {
                        printf("ERROR: accept error\n");
                    } else {
                        // update the master set
                        FD_SET(new_skfd, &master);
                        if (new_skfd > fdmax)
                            fdmax = new_skfd;
                        mfg_dprint(DBG_MINFO,
                                   "NET:  new connection from %s\n",
                                   inet_ntop(new_client.ss_family,
                                             get_in_addr((struct sockaddr *)
                                                         &new_client),
                                             new_client_ip, INET6_ADDRSTRLEN));
                        drv_init(bridge, &Drv_Config);
                    }
                } else {
                    addrlen = sizeof(struct sockaddr_in);
                    /* Data received */
                    bytes =
                        recvfrom(j, buf, MAXBUF, 0,
                                 (struct sockaddr *) &client, &addrlen);

                    mfg_dprint(DBG_MINFO,
                               "NET:  receive a packet (bytes = %d\n", bytes);
                    if (bytes == 0) {
                        mfg_dprint(DBG_MINFO, "NET:  close client socket\n");
                        close(j);
                        FD_CLR(j, &master);
#ifdef NONPLUG_SUPPORT
                        drv_wrapper_deinit_hci();
#endif
                        drv_wrapper_deinit();
                        // ;mfg_dprint(DBG_MINFO,"Test\n");
                    }

                    if (bytes > 0) {
                /** Initialize drvwrapper, if driver already loadded */
                        // ;drv_init(bridge, &Drv_Config);

                        brdg_process_msg(bridge, buf, &msglen, MAXBUF, NET);
                        // send a response messge
                        net->currfd = j;
                        net_send_msg(net, &client, buf, msglen);
                    }
                }
            }
        }
        // may update fmax here later
    }

    /* cleanup */
    close(net->skfd);
    free(buf);
    if(Driver1)
    	free(Driver1);
    return 0;
}

/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/un.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define UNIX_DOMAIN "@/data/misc/bluedroid/rtkbt_service.sock"
#define RTKBT_CONF_MAC_HDL      "/data/vendor/bluetooth/rtkbt_mac_hdl.conf"

#define CHECK_MALLOC_FAILED(condition) \
    do { \
        if (!(condition)) { \
            printf("check fail: source file \"%s\", line %d, in function %s", \
                   __FILE__, __LINE__, __func__); \
            abort(); \
        } \
    } while (0)

typedef struct Rtk_Socket_Data
{
    unsigned char           type;      //hci,other,inner
    unsigned char           opcodel;
    unsigned char           opcodeh;
    unsigned char           parameter_len;
    unsigned char           parameter[0];
} Rtk_Socket_Data;

/*typedef struct
{
    unsigned short          event;
    unsigned short          len;
    unsigned short          offset;
    unsigned short          layer_specific;
    unsigned char           data[];
} HC_BT_HDR;
*/
const char shortOptions[] = "f:r:s:h";
const struct option longOptions[] =
{
    {"fullhcicmd",  required_argument,      NULL,   'f'},
    {"read",        required_argument,      NULL,   'r'},
    {"read_rssi",   required_argument,      NULL,   's'},
    {"help",        no_argument,            NULL,   'h'},
    {0, 0, 0, 0}
};

static void usage(void)
{
    fprintf(stderr, "Usage: rtkcmd [options]\n\n"
            "Options:\n"
            "-f | --fullhcicmd [opcode,parameter_len,parameter]    send hci cmd\n"
            "-r | --read       [address]                           read register address \n"
            "-s | --read_rssi     [bd_address]                     read device rssi \n"
            "-h | --help                                           Print this message\n\n");
}

int Rtkbt_Sendcmd(int socketfd, char *p)
{
    char *token = NULL;
    int i = 0;
    unsigned short OpCode = 0;
    unsigned char ParamLen = 0;
    unsigned char ParamLen_1 = 0;
    int sendlen = 0;
    int params_count = 0;
    int ret = 0;
    Rtk_Socket_Data *p_buf = NULL;

    token = strtok(p, ",");
    if (token != NULL)
    {
        OpCode = strtol(token, NULL, 0);
        //printf("OpCode = %x\n",OpCode);
        params_count++;
    }
    else
    {
        //ret = FUNCTION_PARAMETER_ERROR;
        printf("parameter error\n");
        return -1;
    }

    token = strtok(NULL, ",");
    if (token != NULL)
    {
        ParamLen = strtol(token, NULL, 0);
        //printf("ParamLen = %d\n",ParamLen);
        params_count++;
    }
    else
    {
        printf("parameter error\n");
        return -1;
    }

    p_buf = (Rtk_Socket_Data *)malloc(sizeof(Rtk_Socket_Data) + sizeof(char) * ParamLen);
    CHECK_MALLOC_FAILED(p_buf);
    p_buf->type = 0x01;
    p_buf->opcodeh = OpCode >> 8;
    p_buf->opcodel = OpCode & 0xff;
    p_buf->parameter_len = ParamLen;

    ParamLen_1 = ParamLen;
    while (ParamLen_1--)
    {
        token = strtok(NULL, ",");
        if (token != NULL)
        {
            p_buf->parameter[i++] = strtol(token, NULL, 0);
            params_count++;
        }
        else
        {
            printf("parameter error\n");
            free(p_buf);
            return -1;
        }
    }

    if (params_count != ParamLen + 2)
    {
        printf("parameter error\n");
        free(p_buf);
        return -1;
    }

    sendlen = sizeof(Rtk_Socket_Data) + sizeof(char) * p_buf->parameter_len;
    ret = write(socketfd, p_buf, sendlen);
    if (ret != sendlen)
    {
        free(p_buf);
        return -1;
    }


    free(p_buf);
    return 0;
}

int Rtkbt_Getevent(int sock_fd, char *rssi)
{
    unsigned char type = 0;
    unsigned char event = 0;
    unsigned char len = 0;
    unsigned char *recvbuf = NULL;
    unsigned char tot_len = 0;
    int ret = 0;
    int i;

    ret = read(sock_fd, &type, 1);
    if (ret <= 0)
    {
        return -1;
    }

    ret = read(sock_fd, &event, 1);
    if (ret <= 0)
    {
        return -1;
    }

    ret = read(sock_fd, &len, 1);
    if (ret <= 0)
    {
        return -1;
    }

    tot_len = len + 2;
    recvbuf = (unsigned char *)malloc(sizeof(char) * tot_len);
    CHECK_MALLOC_FAILED(recvbuf);
    recvbuf[0] = event;
    recvbuf[1] = len;
    ret = read(sock_fd, recvbuf + 2, len);
    if (ret < len)
    {
        free(recvbuf);
        return -1;
    }

    printf("Event: ");
    for (i = 0; i < tot_len - 1; i++)
    {
        printf("0x%02x ", recvbuf[i]);
    }
    printf("0x%02x\n", recvbuf[tot_len - 1]);

    if (rssi)
    {
        *rssi = recvbuf[tot_len - 1];
    }
    free(recvbuf);
    return 0;
}

int socketinit()
{
    int sock_fd;
    struct sockaddr_un un;
    int len;
    memset(&un, 0, sizeof(un));            /* fill socket address structure with our address */
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, UNIX_DOMAIN);
    un.sun_path[0] = 0;
    len = offsetof(struct sockaddr_un, sun_path) + strlen(UNIX_DOMAIN);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        printf("socket failed %s\n", strerror(errno));
        return -1;
    }
    if (connect(sock_fd, (struct sockaddr *)&un, len) < 0)
    {
        printf("connect failed %s\n", strerror(errno));
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

static uint16_t rtkbt_find_mac_hdl_conf(uint8_t *mac)
{
    char *token = NULL;
    int c = 4, fd = -1, i = 0;
    uint8_t *tr = NULL, t[6] = {0};
    off_t off_s, off_e;
    uint16_t rh = 0;
    token = strtok(mac, ":");
    if (token != NULL)
    {
        t[5] = strtoul(token, NULL, 16);
    }
    while (c >= 0)
    {
        token = strtok(NULL, ":");
        if (token != NULL)
        {
            t[c--] = strtoul(token, NULL, 16);
        }
    }
    fd = open(RTKBT_CONF_MAC_HDL,  O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd == -1)
    {
        printf("%s unable to open '%s': %s", __func__, RTKBT_CONF_MAC_HDL, strerror(errno));
        return 0;
    }
    off_s = lseek(fd, 0, SEEK_SET);
    off_e = lseek(fd, 0, SEEK_END);
    if (off_s == off_e)
    {
        return 0;
    }
    else
    {
        tr = (uint8_t *)malloc(off_e - off_s);
        CHECK_MALLOC_FAILED(tr);
        lseek(fd, 0, SEEK_SET);
        read(fd, tr, off_e - off_s);
        do
        {
            if (memcmp(tr + i * 8, t, 6) == 0) //found
            {
                close(fd);
                rh = (*(uint16_t *)(tr + i * 8 + 6));
                free(tr);
                return rh;
            }
            else
            {
                i++;
            }
        }
        while ((i * 8) < (off_e - off_s));
        close(fd);
        free(tr);
        return 0;
    }
}

int main(int argc, char *argv[])
{
    int index;
    int c;
    int ret;
    int socketfd;

    socketfd = socketinit();
    if (socketfd < 0)
    {
        printf("socketinit failed\n");
        exit(0);
    }

    c = getopt_long(argc, argv, shortOptions, longOptions, &index);

    if (c == -1)
    {
        usage();
    }
    else
    {
        switch (c)
        {
        case 'f':
            {
                printf("Hcicmd %s\n", optarg);
                ret = Rtkbt_Sendcmd(socketfd, optarg);
                if (ret >= 0)
                {
                    if (Rtkbt_Getevent(socketfd, NULL) < 0)
                    {
                        printf("Getevent fail\n");
                    }
                }
                break;
            }
        case 's':
            {
                printf("rssi addr %s\n", optarg);
                char _c [20] = {0}, _rssi = 0;
                uint16_t hl = rtkbt_find_mac_hdl_conf(optarg);
                if (hl == 0)
                {
                    printf("can not get rssi! \n");
                    break;
                }
                sprintf(_c, "0x%02x%02x,%d,0x%02x,0x%02x", 0x14, 0x05, 2, (uint8_t)hl, (uint8_t)(hl >> 8));
                printf("Hcicmd %s\n", _c);
                ret = Rtkbt_Sendcmd(socketfd, _c);
                if (ret >= 0)
                {
                    if (Rtkbt_Getevent(socketfd, &_rssi) < 0)
                    {
                        printf("Getevent fail\n");
                    }
                    printf("RSSI=%c%d dB(br/edr),dBm(le)\n", (_rssi & 0x80) ? '-' : '+',
                           (_rssi & 0x80) ? (128 - (_rssi & 0x7F)) : (_rssi & 0x7F));
                }
                break;
            }
        case 'r':
            {
                printf("read register %s\n", optarg);
                //Rtkbt_Readreg(socketfd,optarg);
                break;
            }
        case 'h':
            {
                usage();
                break;
            }
        }
    }

    close(socketfd);

}

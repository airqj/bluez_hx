#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/ioctl.h>
/*
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <signal.h>

#include "textfile.h"
#include "csr.h"
#include "../sha1.h"

#define STOP_ADV 0x00
#define START_ADV 0x01

/* Unofficial value, might still change */

//#define LE_LINK		0x03

//#define FLAGS_AD_TYPE 0x01
//#define FLAGS_LIMITED_MODE_BIT 0x01
//#define FLAGS_GENERAL_MODE_BIT 0x02

//#define EIR_FLAGS                   0x01  /* flags */
//#define EIR_UUID16_SOME             0x02  /* 16-bit UUID, more available */
//#define EIR_UUID16_ALL              0x03  /* 16-bit UUID, all listed */
//#define EIR_UUID32_SOME             0x04  /* 32-bit UUID, more available */
//#define EIR_UUID32_ALL              0x05  /* 32-bit UUID, all listed */
//#define EIR_UUID128_SOME            0x06  /* 128-bit UUID, more available */
//#define EIR_UUID128_ALL             0x07  /* 128-bit UUID, all listed */
//#define EIR_NAME_SHORT              0x08  /* shortened local name */
//#define EIR_NAME_COMPLETE           0x09  /* complete local name */
//#define EIR_TX_POWER                0x0A  /* transmit power level */
//#define EIR_DEVICE_ID               0x10  /* device ID */
/*
#define KEY "abcdef"

typedef struct _MYDATA {
    uint8_t length;
    uint16_t magic_number;
    uint64_t device_id;
    uint8_t checksum[20];
}__attribute__((packed)) MYDATA;

*/

#include "hx_ble.h"

static volatile int signal_received = 0;
void sigint_handler(int sig)
{
    signal_received = sig;
}

void start_le_adv(int ctl, int hdev)
{
   struct hci_request rq;
   le_set_advertise_enable_cp advertise_cp;
   uint8_t status;
   int dd, ret;

   if (hdev < 0)
       hdev = hci_get_route(NULL);

   dd = hci_open_dev(hdev);
   if (dd < 0) {
       perror("Could not open device");
       exit(1);
   }

   memset(&advertise_cp, 0, sizeof(advertise_cp));
/*
   if (strcmp(opt, "noleadv") == 0)
       advertise_cp.enable = 0x00;
   else
*/
   advertise_cp.enable = START_ADV;

   memset(&rq, 0, sizeof(rq));
   rq.ogf = OGF_LE_CTL;
   rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
   rq.cparam = &advertise_cp;
   rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
   rq.rparam = &status;
   rq.rlen = 1;

   ret = hci_send_req(dd, &rq, 1000);

   hci_close_dev(dd);

   if (ret < 0) {
       fprintf(stderr, "Can't set advertise mode on hci%d: %s (%d)\n",
                       hdev, strerror(errno), errno);
       exit(1);
   }

   if (status) {
       fprintf(stderr, "LE set advertise enable on hci%d returned status %d\n",
                       hdev, status);
       exit(1);
   }
}

void stop_le_adv(int ctl,int hdev)
{
    struct hci_request rq;
    le_set_advertise_enable_cp advertise_cp;
    uint8_t status;
    int dd,ret;

    if(hdev<0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if(dd < 0)
    {
        printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
        perror("open device error\n");
        exit(2);
    }

    memset(&advertise_cp,0,sizeof(advertise_cp));
    advertise_cp.enable = STOP_ADV;

    memset(&rq,0,sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    rq.cparam = &advertise_cp;
    rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    rq.rparam = &status;
    rq.clen = 1;

    ret = hci_send_req(dd,&rq,1000);
    hci_close_dev(dd);

    if (ret < 0) {
        fprintf(stderr, "Can't set advertise mode on hci%d: %s (%d)\n",
                           hdev, strerror(errno), errno);
        exit(1);
    }

    if (status) {
        fprintf(stderr, "LE set advertise enable on hci%d returned status %d\n",
                           hdev, status);
        exit(1);
    }
}

int read_flags(uint8_t *flags, const uint8_t *data, size_t size)
{
    size_t offset;

    if (!flags || !data)
        return -EINVAL;

    offset = 0;
    while (offset < size) {
        uint8_t len = data[offset];
        uint8_t type;

        /* Check if it is the end of the significant part */
        if (len == 0)
            break;

        if (len + offset > size)
            break;

        type = data[offset + 1];

        if (type == FLAGS_AD_TYPE) {
            *flags = data[offset + 2];
            return 0;
        }

        offset += 1 + len;
    }

    return -ENOENT;
}

int check_report_filter(uint8_t procedure, le_advertising_info *info)
{
   uint8_t flags;

   /* If no discovery procedure is set, all reports are treat as valid */
   if (procedure == 0)
       return 1;

   /* Read flags AD type value from the advertising report if it exists */
   if (read_flags(&flags, info->data, info->length))
       return 0;

   switch (procedure) {
   case 'l': /* Limited Discovery Procedure */
       if (flags & FLAGS_LIMITED_MODE_BIT)
           return 1;
       break;
   case 'g': /* General Discovery Procedure */
       if (flags & (FLAGS_LIMITED_MODE_BIT | FLAGS_GENERAL_MODE_BIT))
           return 1;
       break;
   default:
       fprintf(stderr, "Unknown discovery procedure\n");
   }

   return 0;
}

int print_advertising_devices(int dd, uint8_t filter_type,uint8_t scan_flag)
{
    unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
    HX_REPORT *hx_report=NULL;
    struct hci_filter nf, of;
    struct sigaction sa;
    socklen_t olen;
    int len;

    olen = sizeof(of);
    if (getsockopt(dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
        printf("Could not get socket options\n");
        return -1;
    }

    hci_filter_clear(&nf);
    hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
    hci_filter_set_event(EVT_LE_META_EVENT, &nf);


    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        printf("Could not set socket options\n");
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_NOCLDSTOP;
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        evt_le_meta_event *meta;
        le_advertising_info *info;

        char addr[18];

        while ((len = read(dd, buf, sizeof(buf))) < 0) {
            if (errno == EINTR && signal_received == SIGINT) {
                len = 0;
                goto done;
            }

            if (errno == EAGAIN || errno == EINTR)
                continue;
            goto done;
        }

        ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
        len -= (1 + HCI_EVENT_HDR_SIZE);

        meta = (void *) ptr;

        if (meta->subevent != 0x02)
            goto done;

        /* Ignoring multiple reports */
        info = (le_advertising_info *) (meta->data + 1);
        if (check_report_filter(filter_type, info)) {
  //          mydata = (MYDATA *)(info->data);
            hx_report = (HX_REPORT *)(info->data);
            if(hx_report->indicator == INDICATOR_HX_REPORT)
            {
                uint8_t digest[20];
                sha1_hmac(KEY,strlen(KEY),(uint8_t *)hx_report->data,8,digest);
                if(memcmp(digest,hx_report->checksum,sizeof(digest)) == 0)
                {
                    printf("found a hx report\n");
                }
            }
            else
            {
                printf("found BLE package\n");
            }
/*
            if(scan_flag == CENTRAL)
            {
                //for PERIPHERAL_TO_CENTRAL,central need to reply peripheral
                if(htons(mydata->magic_number) == MAGIC_NUMBER_PERIPHERAL_TO_CENTRAL)
                {
                    uint8_t digest[20];
                    uint8_t cmd_data[32]= {0x1F};
                    uint64_t device_id = (mydata->device_id);
                    sha1_hmac(KEY,strlen(KEY),(unsigned char *)&device_id,8,digest);

                    int dev_id = hci_get_route(NULL);
                    if(dev_id < 0)
                    {
                        printf("%s hci_get_route error\n",__FILE__);
                    }
                    if(0 == memcmp(digest,mydata->checksum,sizeof(digest)))
                    {
                        printf("check sum Ok\n");
                        MYDATA to_peripheral;
                        to_peripheral.device_id = mydata->device_id;
                        to_peripheral.length = 0x1E;
                        to_peripheral.magic_number = MAGIC_NUMBER_CENTRAL_TO_PERIPHERAL;
                        memcpy(cmd_data+1,(uint8_t *)&to_peripheral,sizeof(MYDATA));

                        set_adv_data(dd,cmd_data,sizeof(cmd_data));
                        start_le_adv(-1,0);
                        sleep(10);
                        stop_le_adv(-1,0);
                    }
                }
            }

            else if(scan_flag == PERIPHERAL)//for scan_flag == PERIPHERAL
            {
                printf("magic_number is %u\n",htons(mydata->magic_number));
                if(htons(mydata->magic_number == MAGIC_NUMBER_CENTRAL_TO_PERIPHERAL))
                {
                    printf("stop_le_adv()\n");
                    stop_le_adv(-1,0);
                }
            }
*/
        }
    }

done:
    setsockopt(dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));

    if (len < 0)
        return -1;

    return 0;
}

//void lescan(int dev_id, int argc, char **argv)
void lescan(int dev_id,uint8_t scan_flag)
{
    int err, opt, dd;
    uint8_t own_type = 0x00;
    uint8_t scan_type = 0x01;
    uint8_t filter_type = 0;
    uint16_t interval = htobs(0x0010);
    uint16_t window = htobs(0x0010);
    uint8_t filter_dup = 1;

    if (dev_id < 0)
        dev_id = hci_get_route(NULL);

    dd = hci_open_dev(dev_id);
    if (dd < 0) {
        perror("Could not open device");
        exit(1);
    }

    err = hci_le_set_scan_parameters(dd, scan_type, interval, window,
                            own_type, 0x00, 1000);
    if (err < 0) {
        perror("Set scan parameters failed");
        exit(1);
    }

    err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 1000);
    if (err < 0) {
        perror("Enable scan failed");
        exit(1);
    }

    printf("LE Scan ...\n");

    err = print_advertising_devices(dd, filter_type,scan_flag);
    if (err < 0) {
        perror("Could not receive advertising events");
        exit(1);
    }

    err = hci_le_set_scan_enable(dd, 0x00, filter_dup, 1000);
    if (err < 0) {
        perror("Disable scan failed");
        exit(1);
    }

    hci_close_dev(dd);
}

 void hex_dump(char *pref, int width, unsigned char *buf, int len)
{
    register int i,n;

    for (i = 0, n = 1; i < len; i++, n++) {
        if (n == 1)
            printf("%s", pref);
        printf("%2.2X ", buf[i]);
        if (n == width) {
            printf("\n");
            n = 0;
        }
    }
    if (i && n!=1)
        printf("\n");
}

//void cmd_cmd(int dev_id, int argc, char **argv)
void set_adv_data(int dd,uint8_t *cmd_data ,uint8_t cmd_len)
{
    unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr = buf;
    struct hci_filter flt;
    hci_event_hdr *hdr;
    int i, opt, len;
    uint16_t ocf;
    uint8_t ogf;

/*
    for_each_opt(opt, cmd_options, NULL) {
        switch (opt) {
        default:
            printf("%s", cmd_help);
            return;
        }
    }
    helper_arg(2, -1, &argc, &argv, cmd_help);
*/
/*
    if (dev_id < 0)
        dev_id = hci_get_route(NULL);

*/
    errno = 0;
/*
    ogf = strtol(argv[0], NULL, 16);
    ocf = strtol(argv[1], NULL, 16);
*/
    ogf = 0x08;
    ocf = 0x0008;
    if (errno == ERANGE || (ogf > 0x3f) || (ocf > 0x3ff)) {
        printf("something error\n");
        return;
    }
/*
    for (i = 2, len = 0; i < argc && len < (int) sizeof(buf); i++, len++)
        *ptr++ = (uint8_t) strtol(argv[i], NULL, 16);
*/
/*
    dd = hci_open_dev(dev_id);
    if (dd < 0) {
        perror("Device open failed");
        exit(EXIT_FAILURE);
    }
*/
    /* Setup filter */
    hci_filter_clear(&flt);
    hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
    hci_filter_all_events(&flt);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        perror("HCI filter setup failed");
        exit(EXIT_FAILURE);
    }

    printf("< HCI Command: ogf 0x%02x, ocf 0x%04x, plen %d\n", ogf, ocf,cmd_len);
    hex_dump("  ", 20,cmd_data,cmd_len); fflush(stdout);

//    if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
    if (hci_send_cmd(dd, ogf, ocf,32, cmd_data) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    len = read(dd, buf, sizeof(buf));
    if (len < 0) {
        perror("Read failed");
        exit(EXIT_FAILURE);
    }

    hdr = (void *)(buf + 1);
    ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
    len -= (1 + HCI_EVENT_HDR_SIZE);

    printf("> HCI Event: 0x%02x plen %d\n", hdr->evt, hdr->plen);
    hex_dump("  ", 20, ptr, len); fflush(stdout);

    hci_close_dev(dd);
}

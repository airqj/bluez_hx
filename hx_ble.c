#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "textfile.h"
#include "csr.h"
#include "../sha1.h"

#define STOP_ADV 0x00
#define START_ADV 0x01

#define KEY "abcdef"

typedef struct _MYDATA {
    uint8_t length;
    uint16_t magic_number;
    uint64_t device_id;
    uint8_t checksum[20];
}__attribute__((packed)) MYDATA;

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

int print_advertising_devices(int dd, uint8_t filter_type)
{
    unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
    MYDATA *mydata=NULL;
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
            mydata = (MYDATA *)(info->data);
            uint8_t digest[20];
            sha1_hmac(KEY,strlen(KEY),mydata->device_id,8,digest);
            if(memcmp(digest,mydata->checksum,sizeof(digest)) == 0)
            {
                printf("device: %llu is a valid device\n",mydata->device_id);
            }

/*
                    if(strncmp("00:02:5B:00:A5:A5",addr,strlen("00:02:5B:00:A5:A5"))==0)
                    {
                         printf("%s %s %u\n", addr, name,time(NULL));
                            u_int8_t index=0;
                            for(index=0;index < info->length;index++)
                            {
                                printf("%02X",info->data[index]);
                            }
                            printf("\n");
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

void lescan(int dev_id, int argc, char **argv)
{
    int err, opt, dd;
    uint8_t own_type = 0x00;
    uint8_t scan_type = 0x01;
    uint8_t filter_type = 0;
    uint16_t interval = htobs(0x0010);
    uint16_t window = htobs(0x0010);
    uint8_t filter_dup = 1;

    printf("%d\n",argc);
    printf("%s\n",argv[0]);
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

    err = print_advertising_devices(dd, filter_type);
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


int main(int argc,char **argv)
{
    uint8_t cmd_data[32]= {0x1F,0x1F,0x01,0x1A,0x1A,0xFF,0x4C,0x00,0x02,0x15,0xFD,0xA5,0x06,0x93,0xA4,0xE2,0x4F,0xB1,0xAF,0xCF,0xC6,0xEB,0x07,0x64,0x78,0x25,0x27,0x19,0x34,0x64,0xC9,0x00};
/*
    start_le_adv(0,-1);
    printf("le_adv started successful\n");
    printf("now try to stop adv\n");
    stop_le_adv(0,-1);
*/
    int dev_id = hci_get_route(NULL);
    if(dev_id < 0)
    {
        printf("hci_get_route faild\n");
        return 1;
    }

    int dd = hci_open_dev(dev_id);
    hci_send_cmd(dd,0x08,0x0008,sizeof(cmd_data),cmd_data);

    return 0;
}

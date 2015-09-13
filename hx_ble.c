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

#define STOP_ADV 0x00
#define START_ADV 0x01

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

int main(int argc,char **argv)
{
    start_le_adv(0,-1);
    printf("le_adv started successful\n");
    printf("now try to stop adv\n");
    stop_le_adv(0,-1);

    return 0;
}

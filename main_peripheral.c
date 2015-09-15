#include "hx_ble.h"

int main(int argc,char **argv)
{
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
//    stop_le_adv(0,-1);

    uint8_t cmd_data[32]= {0x1F};
    MYDATA mydata;
    memset(&mydata,0,sizeof(MYDATA));
    mydata.device_id = htobe64(123456789);
    mydata.length = 0x1E;
    mydata.magic_number = MAGIC_NUMBER_PERIPHERAL_TO_CENTRAL;

    sha1_hmac(KEY,strlen(KEY),(unsigned char *)&mydata.device_id,8,(unsigned char *)&mydata.checksum);

    memcpy(cmd_data+1,&mydata,sizeof(MYDATA));
    set_adv_data(dev_id,cmd_data,sizeof(cmd_data));

    start_le_adv(0,-1);
    lescan(dev_id,PERIPHERAL);
    return 0;
}

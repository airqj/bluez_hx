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

//    int dd = hci_open_dev(dev_id);

//    stop_le_adv(0,-1);

    lescan(dev_id,CENTRAL);
//    start_le_adv(0,-1);
    //waiting central reply
//    lescan(dev_id);
//    stop_le_adv(0,-1);
    return 0;
}

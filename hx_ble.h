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
#include <signal.h>
#include <endian.h>

#include "textfile.h"
#include "csr.h"
#include "../sha1.h"

#define STOP_ADV 0x00
#define START_ADV 0x01

/* Unofficial value, might still change */
#define LE_LINK		0x03

#define FLAGS_AD_TYPE 0x01
#define FLAGS_LIMITED_MODE_BIT 0x01
#define FLAGS_GENERAL_MODE_BIT 0x02

#define EIR_FLAGS                   0x01  /* flags */
#define EIR_UUID16_SOME             0x02  /* 16-bit UUID, more available */
#define EIR_UUID16_ALL              0x03  /* 16-bit UUID, all listed */
#define EIR_UUID32_SOME             0x04  /* 32-bit UUID, more available */
#define EIR_UUID32_ALL              0x05  /* 32-bit UUID, all listed */
#define EIR_UUID128_SOME            0x06  /* 128-bit UUID, more available */
#define EIR_UUID128_ALL             0x07  /* 128-bit UUID, all listed */
#define EIR_NAME_SHORT              0x08  /* shortened local name */
#define EIR_NAME_COMPLETE           0x09  /* complete local name */
#define EIR_TX_POWER                0x0A  /* transmit power level */
#define EIR_DEVICE_ID               0x10  /* device ID */

#define KEY "abcdef"
#define MAGIC_NUMBER_CENTRAL_TO_PERIPHERAL 0x3531
#define MAGIC_NUMBER_PERIPHERAL_TO_CENTRAL 0x3530

#define CENTRAL 0x01
#define PERIPHERAL 0x02
#define INDICATOR_HX_REPORT 0x2719
#define CHECKSUM_LENGTH 20

/*
typedef struct _MYDATA {
    uint8_t length;
    uint16_t magic_number;
    uint64_t device_id;
    uint8_t checksum[20];
}__attribute__((packed)) MYDATA;
*/

typedef struct _HX_REPORT {
    uint16_t indicator;
    uint8_t  length;
    uint64_t data;
    uint8_t  checksum[20];
}__attribute__((packed)) HX_REPORT;

/*
typedef struct _USER_FLAG {
    uint64_t hx_id;
    timer_t  timestamp;
} __attribute__((packed)) USER_FLAG;
*/

void start_le_adv(int ctl,int hdev);
void stop_le_adv(int ctl,int hdev);
//int print_advertising_devices(int dd,uint8_t filter_type);
void lescan(int dev_id,uint8_t scan_flag);
void set_adv_data(int dd,uint8_t *cmd_data,uint8_t cmd_len);

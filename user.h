#include <time.h>
#include <string.h>
#include <stdint.h>
#include <curl/curl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <endian.h>
#include <openssl/md5.h>

#define TIMEOUT 10
#define EXPIRE_TIME 30
#define HX_CURL_FORMAT "http://www.tongrenbao.com:8080/an/alarmingReceive.do?routerid=%llu&deviceid=%llu&timestamp=%lu&md5=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
#define HX_KEY "aGFuZ3h1bmtlamk="

typedef struct _USER_FLAG {
    uint64_t hx_id;
    timer_t  timestamp;
} __attribute__((packed)) USER_FLAG;

uint8_t user_init(void);
uint8_t init_timer(void);
uint8_t find_user(uint64_t device_id);
//uint8_t delete_user(uint64_t device_id);
uint8_t add_user(uint64_t device_id);


uint8_t info_server(uint64_t device_id,uint64_t router_id);
uint64_t get_routerId();

#include "user.h"

#define MAX_USER 100
static uint64_t gdevice_id=0;

USER_FLAG user[MAX_USER];
static uint8_t user_counter=0;

uint8_t user_init(void)
{
    memset(&user,0,sizeof(user));
    return 0;
}

uint8_t find_user(uint64_t device_id)
{
    uint8_t index=0;
    for(index=0;index<MAX_USER;index++)
    {
        if(user[index].hx_id == device_id)
        {
            return 1;
        }
    }
    return 0;
}

uint8_t add_user(uint64_t device_id)
{
    uint8_t index=0;
    for(index=0;index<MAX_USER;index++)
    {
        if(user[index].hx_id == 0)
        {
            user[index].hx_id = device_id;
            user[index].timestamp = time(NULL);
            printf("add user %llu\n",device_id);
            return 0;
        }
    }

    syslog(LOG_INFO,"user array full,add user faild\n");
    return 1;
}

void delete_expire_user(void) //use for timeout handle,find expire user and delete it
{
    printf("timeout\n");
    uint8_t index=0;
    time_t current_time = time(NULL);
    for(index=0;index<MAX_USER;index++)
    {
        if(user[index].hx_id != 0)
        {
            if((uint32_t )current_time - (uint32_t)user[index].timestamp >= EXPIRE_TIME)
            {
                printf("found a expire user: %llu,delete it\n",user[index].hx_id);
                user[index].hx_id = 0;
            }
        }
    }
}

uint64_t get_routerId()
{
    int router_id_fd =open("/etc/routerId",O_RDONLY);
    if(!router_id_fd)
    {
        syslog(LOG_ERR,"open /etc/routerId faild\n");
        return -1;
    }

    uint8_t routerId[32];
    size_t read_size = read(router_id_fd,routerId,32);
    if(!read_size)
    {
        syslog(LOG_ERR,"read /etc/routerId faild\n");
        return 2;
    }

    return atoll(routerId);
}

/*
uint8_t info_server(uint64_t device_id, uint64_t router_id)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if(curl == NULL)
    {
        syslog(LOG_INFO,"curl_easy_init() error\n");
        return 1;
    }

    uint8_t str[100];
    sprintf(str,"routerId=%llu&deviceId=%llu",router_id,device_id);

    curl_easy_setopt(curl,CURLOPT_URL,HX_SERVER);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,str);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        syslog(LOG_ERR,curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
*/
uint8_t recv_result(void *pre,size_t size,size_t nmnb,void *stream)
{
    printf("res is %d\n",atoi((char *)pre));
    if(1 == atoi((char *)pre))
    {
        add_user(gdevice_id);
    }
    return 1;
}

uint8_t info_server(uint64_t device_id, uint64_t router_id)
{
    gdevice_id = device_id;
    uint8_t url[1024];
    uint8_t md5[16];
    uint8_t timestamp[36];
    time_t timestamp0=time(NULL);
    sprintf(timestamp,"%luaGFuZ3h1bmtlamk=",timestamp0);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx,timestamp,strlen(timestamp));
    MD5_Final(md5,&ctx);
    sprintf(url,HX_CURL_FORMAT,router_id,device_id,timestamp0,md5[0],md5[1],md5[2],md5[3],md5[4],md5[5],md5[6],md5[7],md5[8],md5[9],md5[10],md5[11],md5[12],md5[13],md5[14],md5[15]);

    printf("%s\n",url);

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl)
    {
        curl_easy_setopt(curl,CURLOPT_URL,url);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,recv_result);
        curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            syslog(LOG_ERR,curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return 1;
        }
        curl_easy_cleanup(curl);
    }

    return 0;
}
uint8_t init_timer(void)
{
    signal(SIGALRM,delete_expire_user);
    struct itimerval interval;
    interval.it_value.tv_sec= 2;
    interval.it_value.tv_usec=0;
    interval.it_interval.tv_sec=TIMEOUT;
    interval.it_interval.tv_usec=0;

    if(setitimer(ITIMER_REAL,&interval,NULL))
    {
        syslog(LOG_INFO,"%s: setitimer faild\n",__FUNCTION__);
        return 1;
    }
    return 0;
}

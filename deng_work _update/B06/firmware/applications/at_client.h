#ifndef __AT_CLIENT_H
#define __AT_CLIENT_H

#include <rtthread.h>

enum at_status
{
    AT_STATUS_UNINITIALIZED = 0,
    AT_STATUS_INITIALIZED,			
    AT_STATUS_BUSY,
};
typedef enum at_status at_status_t;

enum at_resp_status
{
     AT_RESP_OK = 0,                  
     AT_RESP_ERROR = -1,              
     AT_RESP_TIMEOUT = -2,           
     AT_RESP_BUFF_FULL= -3,           
};
typedef enum at_resp_status at_resp_status_t;

struct at_response
{
    char *buf;
    int buf_size;
	int recv_len;
    int line_num;
    int line_counts;
    int timeout;
};
typedef struct at_response *at_response_t;



struct at_client
{
    rt_device_t device;
    at_status_t status;
		char enable_recv;
    char end_sign;		 

    char *recv_buffer;
    int recv_bufsz;
    int cur_recv_len;
    rt_mutex_t lock;

    at_response_t resp;
    rt_sem_t resp_notice;
    at_resp_status_t resp_status;

    rt_thread_t parser;
		void *user_data;
};
typedef struct at_client *at_client_t;





at_response_t at_create_resp(int buf_size, int line_num, int timeout);
void at_delect_resp(at_response_t resp);
at_response_t at_resp_set_info(at_response_t resp, int buf_size, int line_num, int timeout);
const char *at_resp_get_line(at_response_t resp, int resp_line);
const char *at_resp_get_line_by_kw(at_response_t resp, const char *keyword);
int at_client_send(at_client_t client, const char *buf, int size);
int at_client_recv(at_client_t client, char *buf, int size);
int at_exec_cmd(at_client_t client, at_response_t resp,char *exec_cmd);
int at_client_wait_connect(at_client_t client, rt_uint32_t timeout,char* conn_cmd);
void at_set_end_sign(at_client_t client, char ch);
int at_client_init(at_client_t client,const char *dev_name,  int recv_bufsz);

#endif

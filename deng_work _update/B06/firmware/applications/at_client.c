
#include "at_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "param.h"

#define AT_RESP_END_OK        	"OK"
#define AT_RESP_END_BAD     	"bad"
#define AT_RESP_END_ERROR     	"error"
#define AT_RESP_END_FAIL       	"not"
#define AT_RESP_END_FAILED		"failed"

#define AT_CMD_HAVE_SUFFIX 
#define AT_END_CR_LF            "\n"

/*
创建指令响应接收对象
buf_size:接收buf大小
line_num:需要接收的行数
timeout:接收超时时间
*/
at_response_t at_create_resp(int buf_size, int line_num, int timeout)
{
    at_response_t resp = RT_NULL;
	if(buf_size <= 0 || line_num < 0)
	{
		rt_kprintf("input args [%d,%d,%d] error!\r\n",buf_size,line_num,timeout);
		return RT_NULL;
	}
	
    resp = (at_response_t) rt_malloc(sizeof(struct at_response));
    if (resp == RT_NULL)
    {
        rt_kprintf("create response object failed! No memory for response object!\r\n");
        return RT_NULL;
    }

    resp->buf = (char *) rt_malloc(buf_size);
    if (resp->buf == RT_NULL)
    {
        rt_kprintf("create response object failed! No memory for response buffer!\r\n");
        rt_free(resp);
        return RT_NULL;
    }

    resp->buf_size = buf_size;
    resp->line_num = line_num;
    resp->line_counts = 0;
	resp->recv_len = 0;
    resp->timeout = timeout;

    return resp;
}


//删除响应接收对象
void at_delect_resp(at_response_t resp)
{
	if(resp == RT_NULL)
	{
		rt_kprintf("input response object error!\r\n");
		return;
	}
	if(resp->buf != RT_NULL)
	{
		rt_free(resp->buf);
	}
	rt_free(resp);
	return;
}

//设置响应接收对象参数
at_response_t at_resp_set_info(at_response_t resp, int buf_size, int line_num, int timeout)
{
	if(resp == RT_NULL || buf_size <= 0 || line_num < 0)
	{
		rt_kprintf("input response object error!\r\n");
		return RT_NULL;
	}

    if (resp->buf_size != buf_size)
    {
		if(resp->buf != RT_NULL)
		{
			rt_free(resp->buf);
		}
				
        resp->buf_size = buf_size;
		
        resp->buf = (char *) rt_malloc(resp->buf_size);
        if (!resp->buf)
        {
            rt_kprintf("No memory for malloc response buffer size(%d).", buf_size);
            return RT_NULL;
        }
    }
    resp->line_num = line_num;
    resp->timeout = timeout;

    return resp;
}

//通过行号查找接收数据
const char *at_resp_get_line(at_response_t resp, int resp_line)
{
    char *resp_buf = resp->buf;
    char *resp_line_buf = RT_NULL;
    int line_num = 1;
	
	if(resp == RT_NULL)
	{
		rt_kprintf("input response object error!\r\n");
		return RT_NULL;
	}

    if (resp_line > resp->line_counts || resp_line <= 0)
    {
        rt_kprintf("response get line failed! Input response line(%d) error!\r\n", resp_line);
        return RT_NULL;
    }

    for (line_num = 1; line_num <= resp->line_counts; line_num++)
    {
        if (resp_line == line_num)
        {
            resp_line_buf = resp_buf;

            return resp_line_buf;
        }

        resp_buf += strlen(resp_buf) + 1;
    }

    return RT_NULL;
}

//通过关键字查找接收数据
const char *at_resp_get_line_by_kw(at_response_t resp, const char *keyword)
{
    char *resp_buf = resp->buf;
    char *resp_line_buf = RT_NULL;
    int line_num = 1;

   	if(resp == RT_NULL || keyword == RT_NULL)
	{
		rt_kprintf("input response object or  keyword error!\r\n");
		return RT_NULL;
	}

    for (line_num = 1; line_num <= resp->line_counts; line_num++)
    {
        if (strstr(resp_buf, keyword))
        {
            resp_line_buf = resp_buf;

            return resp_line_buf;
        }

        resp_buf += strlen(resp_buf) + 1;
    }

    return RT_NULL;
}



int at_client_send(at_client_t client, const char *buf, int size)
{
    if (client == RT_NULL || buf == RT_NULL)
    {
        rt_kprintf("input Client object or buf is NULL, please create or get AT Client object!\r\n");
        return -RT_ERROR;
    }
    return rt_device_write(client->device, 0, buf, size);
}

int at_client_recv(at_client_t client, char *buf, int size)
{
    int read_idx = 0;
    char ch;

    if (client == RT_NULL || buf == RT_NULL)
    {
        rt_kprintf("input Client or buf is NULL\r\n");
        return -RT_ERROR;
    }

    while (1)
    {
        if (read_idx < size)
        {
            while(rt_device_read(client->device,0,&ch,1) == 0)
			{
				rt_thread_delay(10);
			}
            buf[read_idx++] = ch;
        }
        else
        {
            break;
        }
    }
    return read_idx;
}

static void at_enable_cmd_data_recv(at_client_t client)
{
	client->enable_recv = 1;
}

static void at_disable_cmd_data_recv(at_client_t client)
{
	client->enable_recv = 0;
}

static char at_get_cmd_recv_status(at_client_t client)
{
	return client->enable_recv;
}


int at_exec_cmd(at_client_t client, at_response_t resp, char *exec_cmd)
{
    rt_err_t result = RT_EOK;
    if (client == RT_NULL || exec_cmd == RT_NULL)
    {
        rt_kprintf("input Client or exec_cmd is NULL\r\n");
		return -RT_ERROR;
    }

    rt_mutex_take(client->lock, RT_WAITING_FOREVER);

    client->resp_status = AT_RESP_OK;
    client->resp = resp;
	at_enable_cmd_data_recv(client);
	if(at_client_send(client,exec_cmd,strlen(exec_cmd)) !=strlen(exec_cmd))
	{
		rt_kprintf("[5.8G_wifi] send cmd failed! ---->%s",exec_cmd);
	}
	//rt_kprintf("[5.8G_wifi] send cmd success! ---->%s",exec_cmd);
    if (resp != RT_NULL)
    {
        resp->line_counts = 0;
        if (rt_sem_take(client->resp_notice,resp->timeout) != RT_EOK)
        {
            client->resp_status = AT_RESP_TIMEOUT;
        }
        if (client->resp_status != AT_RESP_OK)
        {
//            rt_kprintf("at client execute command failed!,cmd--->%s", exec_cmd);
            result = -RT_ERROR;
            goto exit;
        }
    }
exit:
    client->resp = RT_NULL;
	at_disable_cmd_data_recv(client);
    rt_mutex_release(client->lock);
    return result;
}

	
void at_set_end_sign(at_client_t client, char ch)
{
    if (client == RT_NULL)
    {
        rt_kprintf("input Client object is NULL, please create or get AT Client object!");
        return;
    }
    client->end_sign = ch;
}


static int at_recv_readline(at_client_t client)
{
    int  read_len = 0;
    char ch = 0;
    bool is_full = RT_FALSE;

    memset(client->recv_buffer, 0x00, client->recv_bufsz);
    client->cur_recv_len = 0;

    while (1)
    {
		while(rt_device_read(client->device,0,&ch,1) == 0)
		{
			rt_thread_delay(10);
		}
//		rt_kprintf("recv----->%d,%d,%c\r\n",read_len,ch,ch);
		if(at_get_cmd_recv_status(client) == 1)
		{
			if (read_len < client->recv_bufsz)
			{
				client->recv_buffer[read_len++] = ch;
				client->cur_recv_len = read_len;
			}
			else
			{
				is_full = RT_TRUE;
			}
//			rt_kprintf("recv----->%d,%d,%c\r\n",read_len,ch,ch);
			if ((client->end_sign != 0 && ch == client->end_sign)) 
			{
				if (is_full)
				{
//					rt_kprintf("read line failed. The line data length is out of buffer size(%d)!", client->recv_bufsz);
					memset(client->recv_buffer, 0x00, client->recv_bufsz);
					client->cur_recv_len = 0;
					return -RT_EFULL;
				}
				break;
			}
		}
    }
    return read_len;
}

//数据接收解析线程
static void client_parser(at_client_t client)
{
    while(1)
    {
        if (at_recv_readline(client) > 0)
        {
           
            if (client->resp != RT_NULL)
            {

                client->recv_buffer[client->cur_recv_len -1] = '\0';
                if (client->resp->recv_len + client->cur_recv_len < client->resp->buf_size)
                {
                    /* 拷贝一行数据到响应结束buf */
                    memcpy(client->resp->buf + client->resp->recv_len, client->recv_buffer, client->cur_recv_len);
                    client->resp->recv_len += client->cur_recv_len;
                    client->resp->line_counts++;
                }
                else
                {
                    client->resp_status = AT_RESP_BUFF_FULL;
//                    rt_kprintf("Read response buffer failed. The Response buffer size is out of buffer size(%d)!", client->resp->buf_size);
                }
                /* 校验接收结果 */
                if (memcmp(client->recv_buffer, AT_RESP_END_OK, strlen(AT_RESP_END_OK)) == 0 && client->resp->line_num == 0)
                {
                    /* 成功接收到OK */
										
                    client->resp_status = AT_RESP_OK;
                }
                else if (strstr(client->recv_buffer, AT_RESP_END_ERROR) || strstr(client->recv_buffer, AT_RESP_END_BAD) \
                        || strstr(client->recv_buffer, AT_RESP_END_FAIL) || strstr(client->recv_buffer, AT_RESP_END_FAILED))
                {
                    client->resp_status = AT_RESP_ERROR;
                }
                else if (client->resp->line_counts == client->resp->line_num && client->resp->line_num)
                {
                    client->resp_status = AT_RESP_OK;
                }
                else
                {
                    continue;
                }
                client->resp = RT_NULL;
                rt_sem_release(client->resp_notice);
            }
        }
    }
}


int at_client_init(at_client_t client,const char *dev_name,  int recv_bufsz)
{
    int result = RT_EOK;
	static int at_client_num = 0;
	char name[64];
	//输入参数判断
	if(client == RT_NULL || dev_name == RT_NULL || (recv_bufsz <= 0))
	{
		rt_kprintf("input arg error\r\n");
		return -RT_ERROR;
	}
	//找到设备
    client->device = rt_device_find(dev_name);
    if (client->device)
    {
			//
		rt_kprintf("\r\n[5.8G_WIFI] client initialize success! find the device(%s)\r\n", dev_name);
				//DMA发送，中断接收
        result = rt_device_open(client->device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX);
        
        if (result != RT_EOK)
        {
			rt_kprintf("client initialize failed! not open the device(%s)\r\n", dev_name);
			return -RT_ERROR;
        }
				
    }
    else
    {
			
        rt_kprintf("client initialize failed! not find the device(%s).", dev_name);	
    }
		
	client->status = AT_STATUS_UNINITIALIZED;
    client->cur_recv_len = 0;
    client->recv_bufsz = recv_bufsz;
	client->enable_recv = 0;
		
	//申请接收空间
	client->recv_buffer = rt_malloc(client->recv_bufsz);
	if(client->recv_buffer == RT_NULL)
	{
		rt_kprintf("at client malloc mem failed !\r\n");
		result = -RT_ENOMEM;
		goto exit;
	}
		
	//创建指令执行互斥信号量
	rt_snprintf(name,sizeof(name), "lock_%d", at_client_num);
    client->lock = rt_mutex_create(name, RT_IPC_FLAG_FIFO);
    if (client->lock == RT_NULL)
    {
        rt_kprintf("client initialize failed! at_client_recv_lock create failed!\r\n");
        result = -RT_ERROR;
        goto exit;
    }
		
	//创建指令响应同步信号量
    rt_snprintf(name,sizeof(name), "sem_%d", at_client_num);
    client->resp_notice = rt_sem_create(name, 0, RT_IPC_FLAG_FIFO);
    if (client->resp_notice == RT_NULL)
    {
        rt_kprintf("client initialize failed! at_client_resp semaphore create failed!\r\n");
        result = -RT_ERROR;
        goto exit;
    }


	//创建数据接收线程
    rt_snprintf(name, sizeof(name), "parser_%d",at_client_num);
    client->parser = rt_thread_create(name,
                                     (void (*)(void *parameter))client_parser,
                                     client,
                                     512,
                                     RT_THREAD_PRIORITY_MAX / 4 - 1,
   									 20);
	if(client->parser != RT_NULL)
	{
		rt_thread_startup(client->parser);
	}			
	else
    {
		rt_kprintf("client initialize failed! not creat paeser thread!\r\n");
        result = -RT_ENOMEM;
        goto exit;
    }
		client->status = AT_STATUS_INITIALIZED;
		at_client_num++;
		return RT_EOK;
exit:
		rt_sem_delete(client->resp_notice);
		rt_mutex_delete(client->lock);
		rt_free(client->recv_buffer);
		rt_device_close(client->device);
		
    return result;
}

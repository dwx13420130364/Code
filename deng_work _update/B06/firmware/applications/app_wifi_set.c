#include "app_wifi_set.h"
#include "stdbool.h"
#include "at_client.h"
#include "string.h"
#include "param.h"
#include "stmflash.h"


#define AT_MAX(a,b) (a)>(b)?(a):(b)

#define EXIT_TRANS_CMD    "+++"
//读取操作的指令在末尾添加\r\n
#define READ_MODULE_VER  								"AT+VER?\r\n"//模块版本的指令
#define READ_WIFI_OPMODE 								"AT+OPMODE?\r\n"//获取模块的工作模式：1STA，2AP
#define READ_LINK_STATUS 								"AT+LINKSTATUS?\r\n"//读取link状态
#define DISCONNECT_WIFI_ROUTER					"AT+DISCONNECT\r\n"
#define READ_STA_SETTING         				"AT+STASETTING?\r\n"
#define READ_AP_SETTING         				"AT+APSETTING?\r\n"
#define READ_WIFI_WIRELESSMODE      		"AT+WIRELESSMODE?\r\n"
#define READ_STA_IP             				"AT+STAIP?\r\n"
#define READ_AP_IP              				"AT+APIP?\r\n"
#define READ_AP_LSIT              			"AT+APLIST?\r\n"
#define REBOOT                   				"AT+REBOOT\r\n"
#define ENTER_TRANS_CMD									"AT+TCPC_TRAN\r\n"
#define READ_TCP_CLIENT  								"AT+TCPCLIENT?\r\n"
#define READ_UDP_CLIENT									"AT+TCPCLIENT?\r\n"
#define SET_WIFI_OPMODE           			"AT+OPMODE="
#define SET_WIFI_WIRELESSMODE       		"AT+WIRELESSMODE="
#define SET_AP_INFO 										"AT+APSETTING="
#define SET_AP_IP_INFO              		"AT+APIP="
#define SET_TCP_AUTO_MODE								"AT+TCPS_AUTOTRAN="
#define ENTER_TRANS_MODE								"AT+TCPC_TRAN="
#define TCPC_CONNECT_CREATE       			"AT+TCPCLIENT="
#define TCPC_CONNECT_CLOSE       				"AT+TCPCLIENT="
#define UDPC_CONNECT_CREATE       			"AT+UDPCLIENT="
#define UDPC_CONNECT_CLOSE       				"AT+UDPCLIENT="
#define TCPC_AUTO_TRANS									"AT+TCPC_AUTOTRAN="
#define CONN_WIFI_ROUTER								"AT+STASETTING="
#define SET_STA_IP											"AT+STAIP="
#define PING_IP               					"AT+PING="


rt_thread_t wifi_init_thread;
extern rt_uint8_t wifi_dev_sta;

static online_status wifi_onlien_status = online_init;

void set_wifi_online_status(online_status status){
	wifi_onlien_status = status;
}

online_status get_wifi_online_status(void){
	return wifi_onlien_status;
}

struct wifi_dev wifi_5G;//用于存储wifi信息数据

char Get_Trans_status(void){
	return wifi_5G.trans_status;
}
void Set_Trans_status(char status){
	wifi_5G.trans_status = status;
}

//wifi退出透传模式,调用次函数之前应挂起透传数据接收线程
int wifi_exit_trans_mode(at_client_t wifi_client){
	int ret = RT_EOK;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	Set_Trans_status(0);
	at_exec_cmd(wifi_client,RT_NULL,EXIT_TRANS_CMD);
	rt_thread_resume(wifi_client->parser);
	return ret;
}

//wifi进入透传模式，挂起at指令解析线程，进入数据接收函数
int wifi_enter_trans_mode(at_client_t wifi_client,char socket_num){
	char cmd[64];
	int ret =RT_EOK;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d\r\n",ENTER_TRANS_MODE,socket_num);
	rt_exit_critical();
	
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	if(ret == RT_EOK){
		if(rt_thread_suspend(wifi_client->parser) != RT_EOK){
			rt_kprintf("[5.8G_wifi] suspend parser thread fialed\r\n");
			ret = -RT_ERROR;
		}
		//表示进入透传模式
		Set_Trans_status(1);
	}
	return ret;
}

//tcp 客户端信息获取
int tcp_get_link_info(at_client_t wifi_client,struct tcp_info* tcp){
	const char *buf;
	const char *pchBegin;
	int ret = RT_EOK;
	int i;
	if(wifi_client == RT_NULL || tcp == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	at_response_t resp = at_create_resp(128,4,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,READ_TCP_CLIENT) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	rt_enter_critical();
	for(i = 0; i< (int)(sizeof(tcp)/sizeof(struct tcp_info)) ;i++){
		buf = at_resp_get_line(resp, i+1);
		if(buf == RT_NULL){
			rt_kprintf("[5.8G_wifi] get  tcp info data failed\r\n");
		}
		tcp[i].sock_num = *buf;
		if (NULL != (pchBegin = strchr(buf, ','))){
			memset(tcp[i].server_ip,0,sizeof(tcp[i].server_ip));
			memcpy(tcp[i].server_ip,buf+2, pchBegin -buf -2);
			tcp[i].server_ip[pchBegin -buf -2] = '\0';
		}
		memset(tcp[i].port,0,sizeof(tcp[i].port));
		memcpy(tcp[i].port, pchBegin+1,strlen(pchBegin+1));
		if(i >=2)break;
	}
	rt_exit_critical();
error:
	at_delect_resp(resp);
	return ret;
}

//创建套接字并连接目标ip及端口，socket：0，1；同时支持2个连接
int tcp_connect_start(at_client_t wifi_client, char socket, char *dest_ip, char *port){
	char cmd[64];
	int ret =RT_EOK;
	int tcp_status = 1;
	if(wifi_client == RT_NULL || dest_ip == RT_NULL || port == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d,%d,%s,%s\r\n",TCPC_CONNECT_CREATE,tcp_status,socket,dest_ip,port);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(10000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		if(wifi_client->resp_status != AT_RESP_TIMEOUT){
			ret = -RT_ERROR;
		}
	}
	at_delect_resp(resp);
	return ret;
}


//关闭连接
int tcp_connect_stop(at_client_t wifi_client, char socket){
	char cmd[64];
	int ret =RT_EOK;
	int tcp_status = 0;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d,%d\r\n",TCPC_CONNECT_CLOSE,tcp_status,socket);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(10000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}

//udp 客户端信息获取
int udp_get_link_info(at_client_t wifi_client,struct tcp_info* tcp){
	const char *buf;
	const char *pchBegin;
	int ret = RT_EOK;
	int i;
	if(wifi_client == RT_NULL || tcp == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	at_response_t resp = at_create_resp(128,4,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,READ_UDP_CLIENT) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	rt_enter_critical();
	for(i = 0; i< (int)(sizeof(tcp)/sizeof(struct tcp_info)) ;i++){
		buf = at_resp_get_line(resp, i+1);
		if(buf == RT_NULL){
			rt_kprintf("[5.8G_wifi] get  tcp info data failed\r\n");
		}
		tcp[i].sock_num = *buf;
		if (NULL != (pchBegin = strchr(buf, ','))){
			memset(tcp[i].server_ip,0,sizeof(tcp[i].server_ip));
			memcpy(tcp[i].server_ip,buf+2, pchBegin -buf -2);
			tcp[i].server_ip[pchBegin -buf -2] = '\0';
		}
		memset(tcp[i].port,0,sizeof(tcp[i].port));
		memcpy(tcp[i].port, pchBegin+1,strlen(pchBegin+1));
		if(i >=2)break;
	}
	rt_exit_critical();
error:
	at_delect_resp(resp);
	return ret;
}

//创建套接字并连接目标ip及端口，socket：0，1；同时支持2个连接
int udp_connect_start(at_client_t wifi_client, char socket, char *dest_ip, char *port){
	char cmd[64];
	int ret =RT_EOK;
	int udp_status = 1;
	if(wifi_client == RT_NULL || dest_ip == RT_NULL || port == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d,%d,%s,%s\r\n",UDPC_CONNECT_CREATE,udp_status,socket,dest_ip,port);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		if(wifi_client->resp_status != AT_RESP_TIMEOUT){
			rt_kprintf("[5.8G_wifi] exec cmd  failed!!!  cmd is : %s",cmd);
			ret = -RT_ERROR;
		}
	}
	at_delect_resp(resp);
	return ret;
}


//关闭连接
int udp_connect_stop(at_client_t wifi_client, char socket){
	char cmd[64];
	int ret =RT_EOK;
	int udp_status = 0;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d,%d\n",UDPC_CONNECT_CLOSE,udp_status,socket);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}

//udp 发送数据,功能未完善，函数内部实现需要修改
int udp_send_data(at_client_t wifi_client, char socket,uint32_t len,char *data){
	char cmd[64];
	int ret =RT_EOK;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d,%d,%s\r\n",UDPC_CONNECT_CLOSE,socket,len,data);
	rt_exit_critical();
	
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}

//创建连接并下次启动自动进入透传模式
int wifi_auto_enter_trans(at_client_t wifi_client, char socket, char *dest_ip, char *port){
	char cmd[64];
	int ret =RT_EOK;
	int tcp_status = 1;
	if(wifi_client == RT_NULL || dest_ip == RT_NULL || port == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d,%d,%s,%s\r\n",TCPC_AUTO_TRANS,tcp_status,socket,dest_ip,port);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;

}
//关闭开机进入透传模式
int wifi_clsoe_enter_trans(at_client_t wifi_client){
	char cmd[64];
	int ret =RT_EOK;
	int tcp_status = 0;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	if(wifi_exit_trans_mode(wifi_client) != RT_EOK){
		rt_kprintf("[5.8G_wifi] exit trans mode\r\n");
		return -RT_ERROR;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d,%d,%s,%s\r\n",TCPC_AUTO_TRANS,tcp_status);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}


//wifi重启
int wifi_reboot(at_client_t wifi_client){
	int ret = RT_EOK;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,REBOOT) != RT_EOK){
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}

//获取连接状态
int wifi_get_link_status(at_client_t wifi_client, char *link_status){
	const char *buf;
	int ret = RT_EOK;
	if(wifi_client == RT_NULL || link_status == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	at_response_t resp = at_create_resp(64,2,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,READ_LINK_STATUS) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL){
		rt_kprintf("[5.8G_wifi] get  linkstatus data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}
	rt_enter_critical();
	*link_status = buf[12] - '0';
	rt_exit_critical();
error:
	at_delect_resp(resp);
	return ret;
}
//获取模块版本
int wifi_get_version(at_client_t wifi_client, char *version){
	const char *buf;
	int ret = RT_NULL;
	if(wifi_client == RT_NULL || version == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	at_response_t resp = at_create_resp(64,2,rt_tick_from_millisecond(1000));
	if(at_exec_cmd(wifi_client,resp,READ_MODULE_VER) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL){
		rt_kprintf("[5.8G_wifi] get  version data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}
	wifi_dev_sta = 1; 
	rt_enter_critical();
	memcpy(version,buf+5,strlen(buf) -4);
	rt_exit_critical();
	rt_kprintf("[5.8G_wifi] version is : %s\r\n",version);
error:
	at_delect_resp(resp);
	return ret;
}


//获取模块工作模式
int wifi_get_opmode(at_client_t wifi_client, char *opmode){
	const char *buf;
	int ret = RT_EOK;
	if(wifi_client == RT_NULL || opmode == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	at_response_t resp = at_create_resp(64,2,rt_tick_from_millisecond(1000));
	if(at_exec_cmd(wifi_client,resp,READ_WIFI_OPMODE) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL){
		rt_kprintf("[5.8G_wifi] get  opmode data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}
	rt_enter_critical();
	*opmode = buf[strlen(buf) - 1] - '0';
	rt_exit_critical();
error:
	at_delect_resp(resp);
	return ret;
}

//获取模块支持频道模式
int wifi_get_wirelessmode(at_client_t wifi_client, char *wirelessmode){
	const char *buf;
	int ret = RT_EOK;
	if(wifi_client == RT_NULL || wirelessmode == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	at_response_t resp = at_create_resp(128,2,rt_tick_from_millisecond(1000));
	if(at_exec_cmd(wifi_client,resp,READ_WIFI_WIRELESSMODE) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL){
		rt_kprintf("[5.8G_wifi] get  wirelessmode data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}
	rt_enter_critical();
	*wirelessmode = buf[strlen(buf) - 2] - '0';
	rt_exit_critical();
error:
	at_delect_resp(resp);
	return ret;
}


//设置模块工作模式
int wifi_set_opmode(at_client_t wifi_client,char opmode){
	char cmd[64];
	char read_mode = 0xff;
	int ret =RT_EOK;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	wifi_get_opmode(wifi_client,&read_mode);
	//rt_kprintf("[opmode]read_mode = %d\r\n",read_mode);
	if(read_mode == opmode){
		return RT_EOK;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d\r\n",SET_WIFI_OPMODE,opmode);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	//再次读取比对
	wifi_get_opmode(wifi_client,&read_mode);
	if(read_mode != opmode){
		rt_kprintf("[5.8G_wifi] set opmode  failed!!!\r\n");
		ret = -RT_ERROR;
		goto error;
	}
error:
	at_delect_resp(resp);
	return ret;
}

//设置模块处于哪个频道工作模式
int wifi_set_wirelessmode(at_client_t wifi_client,char wirelessmode){
	char cmd[64];
	char read_mode = 0xff;
	int ret =RT_EOK;
	if(wifi_client == RT_NULL){
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	wifi_get_wirelessmode(wifi_client,&read_mode);
	//rt_kprintf("[wirelessmode]read_mode = %d\r\n",read_mode);
	if(read_mode == wirelessmode){
		return RT_EOK;
	}
	rt_enter_critical();
	rt_sprintf(cmd,"%s%d\r\n",SET_WIFI_WIRELESSMODE,wirelessmode);
	rt_exit_critical();
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK){
		ret = -RT_ERROR;
		goto error;
	}
	//再次读取比对
	wifi_get_wirelessmode(wifi_client,&read_mode);
	if(read_mode != wirelessmode){
		rt_kprintf("[5.8G_wifi] set wirelessmode  failed!!!\r\n");
		ret = -RT_ERROR;
		goto error;
	}
error:
	at_delect_resp(resp);
	return ret;
}


/*断开wifi连接*/
int  wifi_disconnect(at_client_t wifi_client)
{
	int  ret = RT_EOK;
	if(wifi_client == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,DISCONNECT_WIFI_ROUTER) != RT_EOK)
	{
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}

//获取模块保存或连接的ssid
int wifi_get_sta_setting(at_client_t wifi_client,char *ssid, char *password)
{
	const char *buf;
	int ret = RT_EOK;
	char *pchBegin,*pchEnd;
	if(wifi_client == RT_NULL || ssid == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	//发送命令，接收数据
	at_response_t resp = at_create_resp(64,2,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,READ_STA_SETTING) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}
	
	//获取第一行数据
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi] get  sta setting data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}

	if (NULL != (pchBegin = strchr(buf, ':'))) 
	{
		if (NULL != (pchEnd = strchr(pchBegin + 1, ','))) 
		{
			rt_enter_critical();
			memcpy(ssid, pchBegin+1, pchEnd - pchBegin);
			ssid[pchEnd - pchBegin -1] = '\0';
			rt_exit_critical();
		}
	}
	
	pchBegin = pchEnd;
	if (NULL != (pchBegin = strchr(buf, ','))) 
	{
		if (NULL != (pchEnd = strchr(pchBegin + 1, ','))) 
		{
//			rt_enter_critical();
//			memcpy(authormode, pchBegin+1, pchEnd - pchBegin);
//			authormode[pchEnd - pchBegin -1] = '\0';
//			rt_exit_critical();
		}
	}
	pchBegin = pchEnd;
	rt_enter_critical();
	memcpy(password, pchBegin+1, strlen(pchBegin+1));
	password[strlen(pchBegin+1)] = '\0';
	rt_exit_critical();	
	
//	//数据拷贝
//	rt_enter_critical();
//	memcpy(ssid,buf+12,strlen(buf)- 11);
//	rt_exit_critical();	
error:
	at_delect_resp(resp);
	return ret;
}

//wifi模块连接路由
int wifi_connect_router(at_client_t wifi_client, char *ssid, char *authormode, char *password)
{
	char  cmd[128];
	char old_ssid[32];
	char old_password[32];
	int ret = RT_EOK;
	int  max_len;
	if(wifi_client == RT_NULL || ssid== RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	rt_kprintf("[5.8G_wifi] join router ssid:%s,password:%s\r\n",ssid,password);
	//获取连接ssid
	if(wifi_get_sta_setting(wifi_client, old_ssid,old_password) != RT_EOK)
	{
		rt_kprintf("[5.8G_wifi]get old ssid  error\r\n");
	}
	
	max_len = AT_MAX(strlen(old_ssid), strlen(ssid));
	if(memcmp(old_ssid,ssid,max_len) == 0)
	{
		max_len = AT_MAX(strlen(old_password), strlen(password));
		if(memcmp(old_password,password,max_len) == 0)
		{
			return RT_EOK;
		}
	}
	
	at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(5000));
	//断开旧路由连接
	if(at_exec_cmd(wifi_client,resp,DISCONNECT_WIFI_ROUTER) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}
	//加入新的路由
	rt_enter_critical();
	rt_sprintf(cmd,"%s%s,%s,%s\n",CONN_WIFI_ROUTER,ssid,authormode,password);
	rt_exit_critical();
	
	if(at_exec_cmd(wifi_client, resp, cmd) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}

	if(wifi_reboot(wifi_client) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}
	rt_thread_delay(35000);

error:
	at_delect_resp(resp);
	return ret;
}

//获取模块STA IP 信息
int wifi_get_sta_ip(at_client_t wifi_client, char *status, char *ip, char *netmask, char *gateway)
{
	const char *buf;
	int ret = RT_EOK;
	const char* pchBegin;
	const char* pchEnd;
	if(wifi_client == RT_NULL || ip == RT_NULL || netmask == RT_NULL || gateway == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	at_response_t resp = at_create_resp(64,2,rt_tick_from_millisecond(20000));
	if(at_exec_cmd(wifi_client,resp,READ_STA_IP) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}
	
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi] get  sta setting data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}
	
	if (NULL != (pchBegin = strchr(buf, ':'))) 
	{
		if (NULL != (pchEnd = strchr(pchBegin + 1, ','))) 
		{
			rt_enter_critical();
			memcpy(status, pchBegin+1, pchEnd - pchBegin);
			status[pchEnd - pchBegin -1] = '\0';
			rt_exit_critical();
		}
	}
	
	pchBegin = pchEnd;
	if (NULL != (pchBegin = strchr(buf, ','))) 
	{
		if (NULL != (pchEnd = strchr(pchBegin + 1, ','))) 
		{
			rt_enter_critical();
			memcpy(ip, pchBegin+1, pchEnd - pchBegin);
			ip[pchEnd - pchBegin -1] = '\0';
			rt_exit_critical();
		}
	}
	
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin+ 1, ','))) 
	{
		rt_enter_critical();
		memcpy(netmask, pchBegin+1, pchEnd - pchBegin);
		netmask[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();
	}
	
	pchBegin = pchEnd;
	rt_enter_critical();
	memcpy(gateway, pchBegin+1, strlen(pchBegin+1));
	gateway[strlen(pchBegin+1)] = '\0';
	rt_exit_critical();	
error:
	at_delect_resp(resp);
	return ret;
}


//设置sta_ip
int wifi_set_sta_ip(at_client_t wifi_client, char *ip, char *netmask, char *gataway)
{
	char cmd[128];
	int ret = RT_EOK;
	if(wifi_client == RT_NULL || ip== RT_NULL || netmask== RT_NULL || gataway == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	rt_enter_critical();
	rt_sprintf(cmd,"%s%s,%s,%s\n",SET_STA_IP,ip,netmask,gataway);
	rt_exit_critical();
	
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	
	//设置新的sta_ip
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK)
	{
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}


//获取模块ap设置
int wifi_get_ap_setting(at_client_t wifi_client, char *ssid, char *password, char *channel, char* author_mode)
{
	const char *buf;
	int ret = RT_EOK;
	const char* pchBegin;
	const char* pchEnd;
	if(wifi_client == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	at_response_t resp = at_create_resp(128,2,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,READ_AP_SETTING) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}

	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi] get  sta setting data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}

	//获取数据
	if (NULL != (pchBegin = strchr(buf, ':'))) 
	{
		if (NULL != (pchEnd = strchr(buf, ','))) 
		{
			rt_enter_critical();
			memcpy(ssid, pchBegin+1, pchEnd - pchBegin);
			ssid[pchEnd - pchBegin -1] = '\0';
			rt_exit_critical();
		}
	}
	
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin + 1, ','))) 
	{
		rt_enter_critical();
		memcpy(password, pchBegin+1, pchEnd - pchBegin);
		password[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();
	}
	
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin+ 1, ','))) 
	{
		rt_enter_critical();
		memcpy(channel, pchBegin+1, pchEnd - pchBegin);
		channel[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();
	}
	
	pchBegin = pchEnd;
	rt_enter_critical();
	memcpy(author_mode, pchBegin+1, strlen(pchBegin+1));
	author_mode[strlen(pchBegin+1)] = '\0';
	rt_exit_critical();
error:
	at_delect_resp(resp);
	return ret;
}

//ap设置
int wifi_set_ap_info(at_client_t wifi_client, char *ssid, char *password ,char *channel ,char *author)
{
	char cmd[128];
	int ret =RT_EOK;
	if(wifi_client == RT_NULL || ssid== RT_NULL || password== RT_NULL || channel == RT_NULL || author== RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}

	rt_enter_critical();
	rt_sprintf(cmd,"%s%s,%s,%s,%s\n",SET_AP_INFO,ssid,password,channel,author);
	rt_exit_critical();
	
	at_response_t resp = at_create_resp(64,0,rt_tick_from_millisecond(5000));
	
	//设置新的AP
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}
//	if(wifi_reboot(wifi_client) != RT_EOK)
//	{
//		rt_kprintf("[5.8G_wifi] wifi mode reboot  failed!!!\r\n");
//		ret = -RT_ERROR;
//		goto error;
//	}
//	
//	rt_thread_delay(rt_tick_from_millisecond(35000));
error:
	at_delect_resp(resp);
	return ret;
}


//获取模块ap IP 信息
int wifi_get_ap_ip_info(at_client_t wifi_client, char *ip, char *netmask,char *gateway,char *dns1,char *dns2, char *start_ip,char* end_ip)
{
	const char *buf;
	int ret = RT_EOK;
	const char* pchBegin;
	const char* pchEnd;
	if(wifi_client == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	at_response_t resp = at_create_resp(128,2,rt_tick_from_millisecond(5000));
	if(at_exec_cmd(wifi_client,resp,READ_AP_IP) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}
	
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi] get  sta setting data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}

	//获取数据
	if (NULL != (pchBegin = strchr(buf, ':'))) 
	{
		if (NULL != (pchEnd = strchr(buf, ','))) 
		{
			rt_enter_critical();
			memcpy(ip, pchBegin+1, pchEnd - pchBegin);
			ip[pchEnd - pchBegin -1] = '\0';
			rt_exit_critical();
		}
	}
	
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin + 1, ','))) 
	{
		rt_enter_critical();
		memcpy(netmask, pchBegin+1, pchEnd - pchBegin);
		netmask[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();	
		
		
	}
	
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin + 1, ','))) 
	{
		rt_enter_critical();
		memcpy(gateway, pchBegin+1, pchEnd - pchBegin);
		gateway[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();	
		
		
	}
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin+ 1, ','))) 
	{
		rt_enter_critical();	
		memcpy(dns1, pchBegin+1, pchEnd - pchBegin);
		dns1[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();	
	} 
	
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin+ 1, ','))) 
	{
		rt_enter_critical();	
		memcpy(dns2, pchBegin+1, pchEnd - pchBegin);
		dns2[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();	
	} 
		
	pchBegin = pchEnd;
	if (NULL != (pchEnd = strchr(pchBegin+ 1, ','))) 
	{
		rt_enter_critical();	
		memcpy(start_ip, pchBegin+1, pchEnd - pchBegin);
		start_ip[pchEnd - pchBegin -1] = '\0';
		rt_exit_critical();	
	}
	
 
	  
	pchBegin = pchEnd;
	rt_enter_critical();	
	memcpy(end_ip, pchBegin+1, strlen(pchBegin+1));
	end_ip[strlen(pchBegin+1)] = '\0';
	rt_exit_critical();	

error:
	at_delect_resp(resp);
	return ret;
}

//设置ap信息
int wifi_set_ap_ip_info(at_client_t wifi_client, char *ip, char *netmask,char *gateway,char *dns1,char *dns2, char *start_ip,char* end_ip)
{
	char cmd[128];
	int ret =RT_EOK;
	if(wifi_client == RT_NULL || ip== RT_NULL || netmask== RT_NULL || start_ip == RT_NULL || end_ip== RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	//生成at指令
	rt_enter_critical();
	rt_sprintf(cmd,"%s%s,%s,%s,%s,%s,%s,%s\r\n",SET_AP_IP_INFO,ip,netmask,gateway,dns1,dns2,start_ip,end_ip);
	rt_exit_critical();
	
	//设置新的AP
	at_response_t resp = at_create_resp(128,0,rt_tick_from_millisecond(5000));
	//执行指令
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK)
	{
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}


//设置ap信息
int wifi_set_ap_auto_server(at_client_t wifi_client, char *mode ,char *port_num)
{
	char cmd[128];
	int ret =RT_EOK;
	if(wifi_client == RT_NULL ||mode == RT_NULL || port_num == RT_NULL )
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	//生成at指令
	rt_enter_critical();
	rt_sprintf(cmd,"%s%s,%s\r\n",SET_TCP_AUTO_MODE,mode,port_num);
	rt_exit_critical();
	
	//设置新的AP
	at_response_t resp = at_create_resp(128,0,rt_tick_from_millisecond(5000));
	//执行指令
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK)
	{
		ret = -RT_ERROR;
	}
	at_delect_resp(resp);
	return ret;
}



int wifi_ping_ip(at_client_t wifi_client,char *ip)
{
	char cmd[128];
	const char *buf;
	int ret =RT_EOK;
	if(wifi_client == RT_NULL || ip== RT_NULL )
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	rt_enter_critical();
	rt_sprintf(cmd,"%s%s\n",PING_IP,ip);
	rt_exit_critical();
	
	//共接收4行数据
	at_response_t resp = at_create_resp(128,3,rt_tick_from_millisecond(10000));
	if(at_exec_cmd(wifi_client,resp,cmd) != RT_EOK)
	{
		ret = -RT_ERROR;
		goto error;
	}
	//与PING结果比对
	buf = at_resp_get_line(resp,1);
	if(buf == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi] get  ping data failed\r\n");
		ret = -RT_ERROR;
		goto error;
	}
	if(strstr(buf,"0% packet loss"))
	{
		rt_kprintf("[5.8G_wifi] ping IP:%s successed\r\n",ip);
	}
	else
	{
		rt_kprintf("[5.8G_wifi] ping IP:%s failed\r\n",ip);
		ret = -RT_ERROR;
	}
error:
	at_delect_resp(resp);
	return ret;
}

//获取附近AP,传入的buf必须大于等于1000,有问题
int wifi_get_ap_list(at_client_t wifi_client,char *buf)
{
	int ret =RT_EOK;
	if(wifi_client == RT_NULL)
	{
		rt_kprintf("[5.8G_wifi]input wifi object error\r\n");
		return -RT_ERROR;
	}
	
	//共接收10行数据
	at_response_t resp = at_create_resp(1000,10,rt_tick_from_millisecond(15000));
	if(at_exec_cmd(wifi_client,resp,READ_AP_LSIT) != RT_EOK)
	{
		ret = -RT_ERROR;
	}
	rt_memcpy(buf,resp->buf,1000);
	at_delect_resp(resp);
	return ret;
}

//当进入透传模式后，退出透传，再次执行指令，需先发送一次指令，用于唤醒模块指令响应
void wifi_at_cmd_wakeup(void)
{
	at_client_send(&wifi_5G.client,"AT+VER?\n",8);
}

static void wifi_hw_reset_enable(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET);
	rt_thread_delay(50);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET);
	rt_thread_delay(50);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET);
}

static void wifi_hw_reset_init(void)
{
	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_GPIOA_CLK_ENABLE();
	
	GPIO_Initure.Pin = GPIO_PIN_6;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_PULLUP;
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA,&GPIO_Initure);
	wifi_hw_reset_enable();
}

void wifi_device_init(wifi_dev_t wifi,int recv_size,char *dev_name)
{
	uint32_t ret = RT_EOK;
	uint8_t count = 0;
	do
	{
		ret = at_client_init(&wifi->client,dev_name,recv_size);
		if(count ++ >5)
		{
			count = 0;
			rt_kprintf("\r\n[5.8G_wifi] init failed\r\n");
			rt_thread_delay(2000);
		}
	}while(ret != RT_EOK);
	at_set_end_sign(&wifi->client,'\n');
	wifi->client.user_data = wifi;
	wifi->trans_status = 0;
	
	wifi_hw_reset_init();
}


uint8_t get_set_opmode(wifi_dev_t wifi)
{
	
	return wifi->mode;
}

void wifi_opmode_set(wifi_dev_t wifi,uint8_t mode)
{
	wifi->mode = mode;
	return;
}

static bool reconn_falg = false;
void set_reconnet_flag(bool flag)
{
	reconn_falg = flag;
}

bool get_reconnet_flag(void)
{
	return reconn_falg;
}

static bool ap_trans_flag = false;
void set_ap_trans_falg(bool flag)
{
	ap_trans_flag = flag;
}

bool get_ap_trans_flag(void)
{
	return ap_trans_flag;
}

#define CONN_ERROR_COUNT 5
rt_uint8_t exit_trans_count = 0;//用于记录发送+++退出透传模式，避免重复发送进行指令解析错误
extern bool wifi_running;
static char ap_ssid[32] = "Fylo_mini-xxxxxxxx";
static char ap_password[32] = "12345678";
static char ap_channel[32] = "149";
static char ap_author_mode[20] = "WPA_PSK_WPA2_PSK";

static char ap_ip[16] = "192.168.1.1";
static char ap_netmask[16] = "255.255.255.0";
static char ap_gateway[16] = "192.168.1.1";
static char ap_dns1[16] = "192.168.1.1";
static char ap_dns2[16] = "0";
static char ap_dhcp_start_ip[16] = "192.168.1.100";
static char ap_dhcp_end_ip[16] = "192.168.1.120";
static char ap_auto_server_mode[16] = "1";
static char ap_auto_server_port[16] = "1234";

extern rt_uint8_t connect_indicator_flag;

bool ap_init_flag = false;


void app_wifi_set_entry(void *arg){
	rt_uint8_t count = 0;
	rt_uint32_t uid[3] = {0};
	rt_uint32_t ssid_tail = 0;
	struct save_param s_param;
	//读取写入的ssid和密码
	read_param(0, (int8_t *)&s_param, sizeof(struct save_param));
	if(s_param.wifi_ap.have_ssid_flag == 0x55aa55aa){
		rt_kprintf("[5.8G_wifi] storage find the ssid\r\n");
		rt_memcpy(ap_ssid,s_param.wifi_ap.ssid,sizeof(s_param.wifi_ap.ssid));
	}
	else{
		rt_kprintf("[5.8G_wifi] storage not find ssid,user default\r\n");
		//获取芯片的uid;
		uid[0] = HAL_GetUIDw0();
		uid[1] = HAL_GetUIDw1();
		uid[2] = HAL_GetUIDw2();
		ssid_tail = uid[0]+uid[1]+uid[2];
		//设备芯片的ssid
		rt_snprintf(ap_ssid, sizeof(ap_ssid), "Fylo_mini-%08X",ssid_tail);
		//rt_kprintf("ap_ssid = %s\r\n",ap_ssid);
	}
	if(s_param.wifi_ap.have_password_flag == 0x55aa55aa){
		rt_kprintf("[5.8G_wifi] storage find the password\r\n");
		rt_memcpy(ap_password, s_param.wifi_ap.password, sizeof(s_param.wifi_ap.password));
	}
	rt_kprintf("[5.8G_WIFI] ssid : %s, password : %s\r\n",ap_ssid,ap_password);

	if(s_param.bs_sn.have_sn_flag == 0x55aa55aa){
		rt_kprintf("[SN] sn: %s\r\n",s_param.bs_sn.sn);
	}
	else rt_kprintf("[SN] sn not found\r\n");

	char temp_data[4][32] = {0};
	char apip_data[7][32] = {0};

	static rt_uint8_t re_setting_ap = false;
	
	//记录当前通道，用于改变进行切换
	wifi_opmode_set(&wifi_5G, AP_MODE);
	rt_uint8_t current_opmde = get_set_opmode(&wifi_5G);
	rt_thread_delay(1000);
	wifi_device_init(&wifi_5G, 128, "uart6");

init:
	 rt_thread_delay(1000);

	while(1){
		while(wifi_running){
			//ap模式操作
			if((get_set_opmode(&wifi_5G) == AP_MODE) &&(ap_init_flag == false)){
				current_opmde = wifi_5G.mode;
				//退出透传模式
				if(exit_trans_count == 0){
					wifi_exit_trans_mode(&wifi_5G.client);
				}
				else exit_trans_count --;

				rt_thread_delay(100);
				while(wifi_get_version(& wifi_5G.client,wifi_5G.module_ver) != RT_EOK){
					connect_indicator_flag = 0X01;
					if(count ++ > CONN_ERROR_COUNT){
						wifi_dev_sta = 2;
						count = 0;
						rt_kprintf("[wifi] mcu connect wifi devices failed，reset wifi module\r\n");
						wifi_hw_reset_enable();
						goto init;
					}
				}
			
				connect_indicator_flag = 0x02;
				//设置为AP模式
				if(wifi_set_opmode(&wifi_5G.client, AP_MODE) != RT_EOK){
					rt_kprintf("[wifi] mcu set wifi ap mode failed\r\n");
					wifi_reboot(&wifi_5G.client);
					goto init;
				}
				//设置工作频段
				if(wifi_set_wirelessmode(&wifi_5G.client, A_B_G_N_MIXED_2_5) != RT_EOK){
					rt_kprintf("[wifi] mcu set wifi ap mode failed\r\n");
					wifi_reboot(&wifi_5G.client);
					goto init;
				}
				rt_memset(temp_data, 0, sizeof(temp_data));
				//获取AP信息
				if(wifi_get_ap_setting(&wifi_5G.client, temp_data[0], temp_data[1], temp_data[2], temp_data[3]) != RT_EOK){
					rt_kprintf("[wifi] mcu get wifi ap setting failed\r\n");
					//重启模块进入ap连接
					wifi_reboot(&wifi_5G.client);
					goto init;
				}
				else{
					if(rt_memcmp(temp_data[0], ap_ssid, sizeof(ap_ssid))){
					  rt_kprintf("[wifi] ssid not same:%s\r\n",temp_data[0]);
						re_setting_ap = true;
					}
					if(rt_memcmp(temp_data[1], ap_password, sizeof(ap_password))){
						rt_kprintf("[wifi] password not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(temp_data[2], ap_channel, sizeof(ap_channel))){
						rt_kprintf("[wifi] channel not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(temp_data[3], ap_author_mode, sizeof(ap_author_mode))){
						rt_kprintf("[wifi] autoor mode not same\r\n");
						re_setting_ap = true;
					}
					if(re_setting_ap){
						re_setting_ap = false;
						
						//设置wifi SSID信息
						if(wifi_set_ap_info(&wifi_5G.client,ap_ssid,ap_password,ap_channel,ap_author_mode) != RT_EOK){
							rt_kprintf("[wifi] mcu set wifi ap ssid failed\r\n");
							//重启模块进入ap连接
							wifi_reboot(&wifi_5G.client);
							goto init;
						}
					}
				}
				rt_memset(apip_data, 0, sizeof(apip_data));
		
				if(wifi_get_ap_ip_info(&wifi_5G.client, apip_data[0], apip_data[1], apip_data[2], apip_data[3],apip_data[4],apip_data[5],apip_data[6]) != RT_EOK){
				  	rt_kprintf("[wifi] mcu get wifi ip info failed\r\n");
					//重启模块进入ap链接
					goto init;
				}
				else{
					if(rt_memcmp(apip_data[0],ap_ip,sizeof(ap_ip))){						
					  	rt_kprintf("[wifi] ap ip not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(apip_data[1], ap_netmask, sizeof(ap_netmask)))
					{
						rt_kprintf("[wifi] ap netmask not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(apip_data[2], ap_gateway, sizeof(ap_gateway)))
					{
						rt_kprintf("[wifi] ap gateway not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(apip_data[3], ap_dns1, sizeof(ap_dns1)))
					{
						rt_kprintf("[wifi] ap dns1 not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(apip_data[4], ap_dns2, sizeof(ap_dns2)))
					{
						//rt_kprintf("[wifi] ap dns2 not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(apip_data[5], ap_dhcp_start_ip, sizeof(ap_dhcp_start_ip)))
					{
						//rt_kprintf("[wifi] ap dhcp start ip not same\r\n");
						re_setting_ap = true;
					}
					if(rt_memcmp(apip_data[6], ap_dhcp_end_ip, sizeof(ap_dhcp_end_ip)))
					{
						//rt_kprintf("[wifi] ap dhcp end ip not same\r\n");
						re_setting_ap = true;
					}
					if(re_setting_ap){
						re_setting_ap = false;
						
						if(wifi_set_ap_ip_info(&wifi_5G.client,ap_ip,ap_netmask,ap_gateway,ap_dns1,ap_dns2,ap_dhcp_start_ip,ap_dhcp_end_ip) != RT_EOK)
						{
							rt_kprintf("[wifi] mcu set wifi ip info failed\r\n");
							//重启模块进入ap连接
							wifi_reboot(&wifi_5G.client);
							goto init;
						}
					}
				}
				  
				if(wifi_set_ap_auto_server(&wifi_5G.client,ap_auto_server_mode,ap_auto_server_port) != RT_EOK){
					rt_kprintf("[wifi] mcu set auto server mode failed\r\n");
					wifi_reboot(&wifi_5G.client);
					goto init;
				}
				//输出提示信息，AP设置完成
				rt_kprintf("wifi ap mode set ok\r\n");
				set_ap_trans_falg(true);
				ap_init_flag = true;
				rt_kprintf("wifi enter ap trans mode\r\n");
        wifi_hw_reset_enable();
				goto init;
			}
			rt_thread_delay(100);
		}
		
		rt_thread_delay(10);
	}
}





#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <components.h>
#include "stdbool.h"
#include "stm32f4xx.h"
#include "board.h"

extern void app_key_entry(void *arg);
extern void app_bat_entry(void* arg);
extern void app_uwb_entry(void *arg);
extern void app_update_entry(void *arg);
extern void app_wifi_set_entry(void *arg);
extern void app_wifi_recv_entry(void *arg);
void rt_init_thread_entry(void * args)
{
//初始化系统组件
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_init();
#endif
    rt_thread_t tid = RT_NULL;
    tid = rt_thread_create("key",app_key_entry, RT_NULL,
                            512, 0x09, 10);
		if (tid != RT_NULL)
			rt_thread_startup(tid);
	
    tid = rt_thread_create("bat",app_bat_entry, RT_NULL,
                            512, 0x08, 10);
		if (tid != RT_NULL)
			rt_thread_startup(tid);

	tid = rt_thread_create("update",app_update_entry, RT_NULL,
                            2048, 0x07, 10);
	if (tid != RT_NULL)
		rt_thread_startup(tid);
	
		tid = rt_thread_create("wifi_set",app_wifi_set_entry, RT_NULL,
                            1024, 0x06, 10);
		if (tid != RT_NULL)
			rt_thread_startup(tid);
			
		tid = rt_thread_create("wifi_recv",app_wifi_recv_entry, RT_NULL,
                            1024, 0x06, 10);
		if (tid != RT_NULL)
			rt_thread_startup(tid);
		
		
    tid = rt_thread_create("uwb",app_uwb_entry, RT_NULL,
                            1024, 0x05, 10);
		if (tid != RT_NULL)
			rt_thread_startup(tid);
	//删除自身线程
	
	rt_thread_delete(rt_thread_self());
}

int rt_application_init()
{
	rt_thread_t tid;

    tid = rt_thread_create("init",
                            rt_init_thread_entry, RT_NULL,
                            1024, 0x02, 10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    return 0;
}



void rtthread_startup(void)
{
	rt_hw_interrupt_disable();

    rt_hw_board_init();
	
    /* timer system initialization */
    rt_system_timer_init();

    /* scheduler system initialization */
    rt_system_scheduler_init();

    /* create init_thread */
    rt_application_init();

    /* timer thread initialization */
    rt_system_timer_thread_init();

    /* idle thread initialization */
    rt_thread_idle_init();
	
    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */
    return;
}

void reboot(void)
{
    NVIC_SystemReset();
}
FINSH_FUNCTION_EXPORT(reboot, reboot system);

int main(void)
{
    rtthread_startup();
    return 0;
}

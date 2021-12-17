/**
 * MIT License
 *
 * Copyright (c) 2021 Klatu Networks
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// =============================================================================
// Includes

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "same54p20a.h"
#include "hal_gpio.h"
#include "hal_sleep.h"
#include "driver/include/m2m_wifi.h"

#include "app.h"
#include "trace.h"
#include "winc_mgr.h"


// =============================================================================
// Private types and definitions

#define APP_STATE 0

#define APP_THREAD_PRIORITY  (tskIDLE_PRIORITY +5 )

#define APP_THREAD_STACK_SIZE  (1256/ sizeof(portSTACK_TYPE))

#define APP_MSGQ_NO_OF_MESSAGES  30

#define APP_TIMER_VALUE_SEC 5  //5 seconds 

#define APP_HIBERNATE_TIME_SEC 60 //60 seconds 

#define APP_SENSOSR_DATA_SIZE 2048  


#define APP_LOCK_MSG_Q_MUTEX(p1,p2)                 \
do {                                            \
    if (p1 == FALSE) {                           \
        OSA_LockMutex(p2);  \
    }                                           \
} while (0);


#define APP_UNLOCK_MSG_Q_MUTEX(p1,p2)                 \
do {                                            \
    if (p1 == FALSE) {                           \
        OSA_UnLockMutex(p2);  \
    }                                           \
} while (0);


typedef enum APP_POWER_MODE
{
    IDLE_MODE = 2,
    APP_PS_RESERVED ,
    STANDBY_MODE,
    HIBERNATE_MODE,
    BACKUP_MODE,
    COMPLETE_OFF_MODE
}APP_POWER_MODE;



#define CONF_PL_CONFIG
#define CONF_DFLL_CONFIG hri_oscctrl_write_DFLLCTRLA_reg(OSCCTRL, 0)
#define CONF_BOD_CONFIG hri_supc_clear_BOD33_ENABLE_bit(SUPC)
#define CONF_VREG_CONFIG
#define CONF_NVM_CONFIG hri_nvmctrl_write_CTRLA_PRM_bf(NVMCTRL, 1)
#define CONF_PINS_CONFIG                                           \
gpio_set_pin_direction(GPIO(2, 21), GPIO_DIRECTION_OUT);           \
gpio_set_pin_level(GPIO(2, 21), false)

// =============================================================================
// Private (forward) declarations
static void app_thread_entry_func(void *arg);
#if 0
static void app_timer_handler(void *arg);
#endif
static BOOLEAN app_send_msg_req_to_q(APP_MSG_TYPE msg_id, void* buf, unsigned int len,BOOLEAN is_isr_context);
static void app_release_msg_req(App_MsgReq *pMsg );
static void app_do_hibernate();
static void app_callback();
#if APP_STATE
static void app_wifi_connect_socket_open();
void app_winc_cb(SOCKET socket, winc_mgr_state_t final_state);
#endif
// =============================================================================
// Private storage
static APP App;
static unsigned char App_sensor_data_buf[APP_SENSOSR_DATA_SIZE];
// =============================================================================
// Public code
extern int SENSOR_GetData(unsigned char* buf, int size);

void APP_Start(void)
{
    TRACE_Init();
	TRACE_DBG("%s() Entry\n", __FUNCTION__);
	
	//TRACE_DBG( " portTICK_RATE_MS = %d configMINIMAL_STACK_SIZE = %d\n",portTICK_RATE_MS,configMINIMAL_STACK_SIZE);	
	
	OSA_InitMsgQue(&App.app_thread_msg_q,(unsigned char*)"app_action_msg_q", APP_MSGQ_NO_OF_MESSAGES, sizeof(void *));
		
    OSA_InitMutex(&App.msg_q_mutex);

    RTC_Init(&App.Rtc,RTC_CALENDER);

	NW_WINC_Init(&App.Nw,(unsigned char*)"wdbsystems",(unsigned char*)"WDBNet123",M2M_WIFI_SEC_WPA_PSK);

	/* Create APP  thread */
    OSA_ThreadCreate(&App.h_app_thread,
       		         APP_THREAD_PRIORITY ,
					 app_thread_entry_func,
					 NULL,
                     "app_thread",
			     	 NULL,
					 APP_THREAD_STACK_SIZE,
					 0);
					 
#if APP_STATE
	winc_mgr_init();
#endif
		 
//	TRACE_DBG("Enable Interrupts\n");
//	NVIC_EnableIRQ(SVCall_IRQn | PendSV_IRQn | SysTick_IRQn);
	
	 /* Start FreeRTOS Kernel  */
	TRACE_DBG("Starting freeRTOS Scheduler\n");
    vTaskStartScheduler();	
	while(1);
}

void App_send_msg_req_to_q(APP_MSG_TYPE msg_id, void* buf, unsigned int len,BOOLEAN is_isr_context)
{
    app_send_msg_req_to_q(msg_id,  buf,  len,is_isr_context);
}


// =============================================================================
// Private (static) code
static void app_thread_entry_func(void *arg)
{
	App_MsgReq *pMsg = NULL;
    int socket_id;
    int size ;
   // unsigned char rx_buf[100];
    unsigned char datetime[30];
   // memset(rx_buf,0x00,100);
#if 0
	OSA_InitTimer(&App.h_timer, APP_TIMER_VALUE_SEC*1000, FALSE, app_timer_handler, NULL);

    OSA_StartTimer(&App.h_timer);
#endif
	
	TRACE_DBG("%s() Entry()\n",__FUNCTION__);

	//if (hri_rstc_get_RCAUSE_POR_bit(RSTC))
	{
		/* It is power on Reset , need to set RTC time after SNTP synch */
		TRACE_DBG("Power On Reset\n");
		NW_WINC_GetSysTime(datetime);
		TRACE_DBG("Current date time = %s\n",datetime);
		RTC_SetDateTime(datetime);
	}
	/* Program RTC to wake up after 60 seconds */
	RTC_SetAlaram(APP_HIBERNATE_TIME_SEC,app_callback, NULL);

    size = SENSOR_GetData(App_sensor_data_buf, APP_SENSOSR_DATA_SIZE);

    TRACE_DBG("Sensor Data(%d) :%.*s\n",size,size,(unsigned char*)App_sensor_data_buf);


    //NW_WINC_OpenSocket(NW_WIFI_TCP_CLIENT,"192.168.1.135", NULL,5000,&socket_id);
   // NW_WINC_OpenSocket(NW_WIFI_SSL_TCP_CLIENT,(unsigned char*)"192.168.1.135",NULL,443,&socket_id);
   // NW_WINC_OpenSocket(NW_WIFI_SSL_TCP_CLIENT,NULL,(unsigned char*)"localveggy.com",5000,&socket_id);
   // NW_WINC_OpenSocket(NW_WIFI_SSL_TCP_CLIENT,(unsigned char*)"52.163.206.207",NULL,5000,&socket_id);
    //NW_WINC_SendSocket(socket_id,"Hello World", sizeof("Hello World"));
    //NW_WINC_ReceiveSocket(socket_id,rx_buf, 10);
    //TRACE_DBG( "Received Buffer = %s\n",rx_buf);

    NW_WINC_OpenSocket(NW_WIFI_SSL_TCP_CLIENT,NULL,(unsigned char*)"api.traxxekg.com",443,&socket_id);
#if 1 
    NW_WINC_SendSocket(socket_id,App_sensor_data_buf, size);
	memset(App_sensor_data_buf,0x00,APP_SENSOSR_DATA_SIZE);
    NW_WINC_ReceiveSocket(socket_id,App_sensor_data_buf, APP_SENSOSR_DATA_SIZE);
    TRACE_DBG("Received Buffer = %s\n",App_sensor_data_buf);
	NW_WINC_CloseSocket(socket_id);
 	NW_WINC_Term();
	app_do_hibernate();
#endif
	while(1)
	{

		//TRACE_DBG( "WAIT ON APP MSG Q  configCPU_CLOCK_HZ = %d  configTICK_RATE_HZ= %d  portTICK_RATE_MS = %d \n",configCPU_CLOCK_HZ , configTICK_RATE_HZ,portTICK_RATE_MS);
			
		TRACE_DBG( "WAIT ON APP MSG Q\n");
					
		/* Blocking call till time out  */
		OSA_ReceiveMsgQue(App.app_thread_msg_q,(void**) &pMsg);
		
		TRACE_DBG("APP THREAD MESSAGE ID  = %d\n",pMsg->msg_type);
		
		switch(pMsg->msg_type)
        {
            case APP_TIMER_EXPIRY_MSG:  
				app_do_hibernate();            
                break;
			case APP_RTCTIMER_EXPIRY_MSG:
				TRACE_DBG("RTC ALARM  EXPIRY MSG \n");
				break;
				
			default:
				break;
		}
		app_release_msg_req(pMsg);
	}
}
#if 0
static void app_timer_handler(void *arg)
{
    TRACE_INFO("%s Entry \n", __FUNCTION__);
	app_send_msg_req_to_q(APP_TIMER_EXPIRY_MSG, NULL, 0,FALSE);
	TRACE_DBG("%s() Exit\n",__FUNCTION__);
}
#endif
static BOOLEAN app_send_msg_req_to_q(APP_MSG_TYPE msg_id, void* buf, unsigned int len,BOOLEAN is_isr_context)
{
	App_MsgReq *msg;

    APP_LOCK_MSG_Q_MUTEX(is_isr_context,&App.msg_q_mutex);

    msg  = (App_MsgReq *) malloc(sizeof(App_MsgReq));

    if(msg)
    {
        TRACE_INFO("%s Entry \n", __FUNCTION__);
        msg->msg_buf = NULL;
        msg->msg_len = 0;

        if(len != 0)
        {
            msg->msg_buf = malloc(len);
            if(msg->msg_buf)
            {
                memcpy((unsigned int *)msg->msg_buf,(unsigned int*)buf,len);
                msg->msg_len = len;
            }
            else
            {
                free(msg);
                APP_UNLOCK_MSG_Q_MUTEX(is_isr_context,&App.msg_q_mutex);
                return STATUS_ERR; 
            }
        }
        msg->msg_type = msg_id;

        if(is_isr_context)
        {
            TRACE_INFO("%s ISR Context \n", __FUNCTION__);
            if(!OSA_SendMsgQueIsr(App.app_thread_msg_q,msg)) 
            {
                TRACE_WARN("Failed to post to App queue IN ISR Context\n");
                APP_UNLOCK_MSG_Q_MUTEX(is_isr_context,&App.msg_q_mutex);
                return STATUS_ERR;
            }
        }
        else
        {
            TRACE_INFO("%s NON ISR Context \n", __FUNCTION__);
            if(!OSA_SendMsgQue(App.app_thread_msg_q,msg)) 
            {
                TRACE_WARN("Failed to post to App queue\n");
                APP_UNLOCK_MSG_Q_MUTEX(is_isr_context,&App.msg_q_mutex);
                return STATUS_ERR;
            }
        }
        APP_UNLOCK_MSG_Q_MUTEX(is_isr_context,&App.msg_q_mutex); 
        return STATUS_OK;  
    }
    APP_UNLOCK_MSG_Q_MUTEX(is_isr_context,&App.msg_q_mutex);
    return STATUS_ERR;
}

static void app_release_msg_req(App_MsgReq *pMsg )
{
    if(pMsg->msg_buf)
    {
        free(pMsg->msg_buf);
    }
    free(pMsg);
}
static void app_do_hibernate()
{
    unsigned int sleep_rdy_bit= 0x00;

	TRACE_INFO("%s Entry \n", __FUNCTION__);
	
	/* Switch off HW blocks gracefully TBD  */
	
	
	CONF_PL_CONFIG;
//	CONF_DFLL_CONFIG;
	CONF_BOD_CONFIG;
	CONF_VREG_CONFIG;
	CONF_NVM_CONFIG;
	CONF_PINS_CONFIG;
	
	/* put the device to hibernate mode by writing registers SLEEPCFG, MAINVREG */
    sleep_rdy_bit = hri_pm_get_INTFLAG_SLEEPRDY_bit(PM);	
	TRACE_INFO("%s Sleep Ready Bit Value = %d \n", __FUNCTION__,sleep_rdy_bit);	
	
	/* switch off System RAM as well as BACKUP RAM during hibernation */	
	hri_pm_write_HIBCFG_reg(PM, PM_HIBCFG_RAMCFG(0x2) | PM_HIBCFG_BRAMCFG(0x2));
	TRACE_INFO("%s COnfigured H1BCFG  register Value = %x \n", __FUNCTION__, hri_pm_get_HIBCFG_reg(PM, 0xff));	

	if(sleep_rdy_bit)
		sleep(HIBERNATE_MODE);
}

static void app_callback()
{
	TRACE_INFO("%s Entry \n", __FUNCTION__);
	app_send_msg_req_to_q(APP_RTCTIMER_EXPIRY_MSG, NULL, 0,TRUE);
}

#if APP_STATE
static void app_wifi_connect_socket_open() 
{
  static winc_mgr_params_t winc_params = {
      .wifi_ssid = "wdbsystems",
      .wifi_sectype = M2M_WIFI_SEC_WPA_PSK,
      .wifi_auth = (void *)"WDBNet123",
      .wifi_channels = M2M_WIFI_CH_ALL,
      .host_port = _htons(443),
      .host_name = "api.traxxekg.com",
      .use_ssl = true,
  };
  winc_mgr_open_socket(&winc_params, app_winc_cb);
}


void app_winc_cb(SOCKET socket, winc_mgr_state_t final_state)
{
  char buf[80];
  snprintf(buf,
           80,
           "WINC callback, socket = %d, final state = %s\n",
           socket,
           winc_mgr_get_state_name(final_state));
  printf("%s",buf);
}
#endif
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
//#include "hal_gpio.h"
//#include "hal_sleep.h"
//#include "driver/include/m2m_wifi.h"

#include "app.h"
#include "trace.h"
//#include "winc_mgr.h"

#include "definitions.h"  
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

#if 0
#define CONF_BOD_CONFIG hri_supc_clear_BOD33_ENABLE_bit(SUPC)
#else
#define SUPC_BOD33_ENABLE_Pos       1            /**< \brief (SUPC_BOD33) Enable */
#define SUPC_BOD33_ENABLE           (_U_(0x1) << SUPC_BOD33_ENABLE_Pos)
#define CONF_BOD_CONFIG  SUPC_REGS->SUPC_BOD33  &= ~SUPC_BOD33_ENABLE;
#endif


#define CONF_VREG_CONFIG
#if 0
#define CONF_NVM_CONFIG hri_nvmctrl_write_CTRLA_PRM_bf(NVMCTRL, 1)
#else

//#define NVMCTRL_CTRLA_PRM_Pos       6            /**< \brief (NVMCTRL_CTRLA) Power Reduction Mode during Sleep */
//#define NVMCTRL_CTRLA_PRM_Msk       (_U_(0x3) << NVMCTRL_CTRLA_PRM_Pos)
#define  CONF_NVM_CONFIG     \
do {\
	uint16_t tmp;  \
	tmp = NVMCTRL_REGS->NVMCTRL_CTRLA;\
	tmp &= ~NVMCTRL_CTRLA_PRM_Msk;\
	tmp |= NVMCTRL_CTRLA_PRM(1);\
	NVMCTRL_REGS->NVMCTRL_CTRLA = tmp;\
}while(0)
#endif

#if 0
#define CONF_PINS_CONFIG                                           \
gpio_set_pin_direction(GPIO(2, 21), GPIO_DIRECTION_OUT);           \
gpio_set_pin_level(GPIO(2, 21), false)
#else 
#define  CONF_PINS_CONFIG     \
do {\
	PORT_GroupOutputEnable(PORT_GROUP_2, 0x200000) ;\
	PORT_GroupClear(PORT_GROUP_2, 0x200000);\
}while(0)



#endif


#define SD_DEVICE_NAME "/dev/mmcblka1"
#define SD_MOUNT_NAME "/mnt/mydrive"
#define FILE_IMAGE_NAME "config.txt"
// =============================================================================
// Private (forward) declarations
static void app_thread_entry_func(void *arg);
static BOOLEAN app_send_msg_req_to_q(APP_MSG_TYPE msg_id, void* buf, unsigned int len,BOOLEAN is_isr_context);
static void app_release_msg_req(App_MsgReq *pMsg );
static void app_do_hibernate();
static void app_callback();
static int app_read_config_file();

// =============================================================================
// Private storage
static APP App;
static unsigned char App_sensor_data_buf[APP_SENSOSR_DATA_SIZE];
// =============================================================================
// Public code
extern int SENSOR_GetData(unsigned char* buf, int size);
extern TaskHandle_t xFsHandle ;
extern TaskHandle_t xSdHandle ;
extern TaskHandle_t xWincHandle ;
void APP_Start(void)
{
    TRACE_Init();
	TRACE_DBG("%s() Entry\n", __FUNCTION__);
	
	//TRACE_DBG( " portTICK_RATE_MS = %d configMINIMAL_STACK_SIZE = %d\n",portTICK_RATE_MS,configMINIMAL_STACK_SIZE);	

    RTC_Init(&App.Rtc,RTC_CALENDER);
	NW_WINC_Init(&App.Nw);

	/* Create APP  thread */
    OSA_ThreadCreate(&App.h_app_thread,
       		         APP_THREAD_PRIORITY ,
					 app_thread_entry_func,
					 NULL,
                     "app_thread",
			     	 NULL,
					 APP_THREAD_STACK_SIZE,
					 0);
}

// =============================================================================
// Private (static) code
static void app_thread_entry_func(void *arg)
{
	App_MsgReq *pMsg = NULL;
    int socket_id;
    int size ;
    unsigned char datetime[30];
	
	TRACE_DBG("%s() Entry() Priority = %d  portTICK_PERIOD_MS = %d \n",__FUNCTION__,uxTaskPriorityGet(NULL),portTICK_PERIOD_MS);
	app_read_config_file();

#if 1
    SYS_TIME_Deinitialize (  sysObj.sysTime  );
    NVIC_DisableIRQ(TC0_IRQn);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    vTaskDelete( xFsHandle );
    vTaskDelete( xSdHandle );
#endif

	NW_WINC_connect(App.ssid,App.passphrase,M2M_WIFI_SEC_WPA_PSK);
    //if (hri_rstc_get_RCAUSE_POR_bit(RSTC))
    {
        /* It is power on Reset , need to set RTC time after SNTP synch */
        TRACE_DBG("Power On Reset\n");
        NW_WINC_GetSysTime(datetime);
        TRACE_DBG("Current date time = %s\n",datetime);

        //RTC_GetTimeStamp();
        RTC_SetDateTime(datetime);

        //vTaskDelay(5000 / portTICK_PERIOD_MS);
        //RTC_GetTimeStamp();
    }
	TRACE_DBG("Programming Alaram \n");
    /* Program RTC to wake up after 60 seconds */
    RTC_SetAlaram(App.frequency,app_callback, NULL);

    size = SENSOR_GetData(App_sensor_data_buf, APP_SENSOSR_DATA_SIZE);
//	SYS_CONSOLE_Write( SYS_CONSOLE_DEFAULT_INSTANCE ,App_sensor_data_buf, size);
    NW_WINC_OpenSocket(NW_WIFI_SSL_TCP_CLIENT,NULL,(unsigned char*)"api.traxxekg.com",443,&socket_id);
    NW_WINC_SendSocket(socket_id,App_sensor_data_buf, size);    
    NW_WINC_ReceiveSocket(socket_id,App_sensor_data_buf, APP_SENSOSR_DATA_SIZE);
//	SYS_CONSOLE_Write( SYS_CONSOLE_DEFAULT_INSTANCE ,App_sensor_data_buf, size);
	NW_WINC_CloseSocket(socket_id);
    NW_WINC_Term();

  while (SERCOM2_USART_WriteCountGet() > 0) {
    asm("nop");
  }
    vTaskDelete( xWincHandle );
    app_do_hibernate();
 asm("nop");
  while (1) {
    LED_Toggle();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

}



void _go_to_sleep(void){
	__DSB();
	__WFI();
}

int32_t _set_sleep_mode(const uint8_t mode)
{
	uint8_t delay = 10;

	switch (mode) {
	case 2:
	case 4:
	case 5:
	case 6:
	case 7:
	//	hri_pm_write_SLEEPCFG_reg(PM, mode);
        PM_REGS->PM_SLEEPCFG = mode;
		/* A small latency happens between the store instruction and actual
		 * writing of the SLEEPCFG register due to bridges. Software has to make
		 * sure the SLEEPCFG register reads the wanted value before issuing WFI
		 * instruction.
		 */
		do {
		//	if (hri_pm_read_SLEEPCFG_reg(PM) == mode) {
			if (PM_REGS->PM_SLEEPCFG == mode) {
				break;
			}
		} while (--delay);
		break;
	default:
		return -1;
	}

	return 0;
}
int sleep(const uint8_t mode)
{
	if (0 != _set_sleep_mode(mode))
		return -1;

	_go_to_sleep();

	return 0;
}
static void app_do_hibernate()
{
    unsigned int sleep_rdy_bit= 0x00;

	TRACE_INFO("%s Entry \n", __FUNCTION__);
	

	CONF_BOD_CONFIG;	
	CONF_NVM_CONFIG;
	CONF_PINS_CONFIG;

	/* put the device to hibernate mode by writing registers SLEEPCFG, MAINVREG */
    sleep_rdy_bit = PM_REGS->PM_INTFLAG;	
	TRACE_INFO("%s Sleep Ready Bit Value = %d \n", __FUNCTION__,sleep_rdy_bit);	
	
	/* switch off System RAM as well as BACKUP RAM during hibernation */	
	PM_REGS->PM_HIBCFG = ( PM_HIBCFG_RAMCFG(0x2) | PM_HIBCFG_BRAMCFG(0x2));
//	TRACE_INFO("%s Configured H1BCFG  register Value = %x \n", __FUNCTION__, PM_REGS->PM_HIBCFG );
    // Allow printing to finish before hibernation
  
int x;
LABEL2:
taskENTER_CRITICAL();
  x =   SERCOM2_USART_WriteCountGet();
taskEXIT_CRITICAL();
	while (x > 0)
	{
		asm("nop");
goto LABEL2;
	}

	NVIC_DisableIRQ(EIC_EXTINT_7_IRQn|DMAC_0_IRQn| DMAC_1_IRQn|TC0_IRQn|SERCOM6_1_IRQn|SERCOM6_2_IRQn|SERCOM6_OTHER_IRQn|SERCOM2_1_IRQn|SERCOM2_2_IRQn|SERCOM2_OTHER_IRQn|SERCOM4_1_IRQn|SERCOM4_2_IRQn|SERCOM4_OTHER_IRQn);
    LED_On();	
	if(sleep_rdy_bit)
		sleep(HIBERNATE_MODE);

}

static void app_callback()
{
	TRACE_INFO("%s Entry \n", __FUNCTION__);
}

static int app_read_config_file()
{
    unsigned char line[100];
   
	unsigned int i = 0;

    unsigned char *found = NULL;

    unsigned char *str_param[]= {"SSID=" ,
                                 "PASSPHRASE=" ,
                                 "FREQUENCY=" ,
                                 };

    unsigned int no_of_elements;

    SYS_FS_HANDLE infile;
   
    no_of_elements = 3;

LABEL1:

    vTaskDelay(10 / portTICK_PERIOD_MS);

    if (SYS_FS_Mount(SD_DEVICE_NAME, SD_MOUNT_NAME, FAT, 0, NULL) == SYS_FS_RES_SUCCESS) 
    {
        TRACE_DBG("MOUNTED \n");
    }
    else 
    {
       // TRACE_DBG("NO SD CARD = %ld \n",i);
        goto LABEL1;
    }

    if (SYS_FS_CurrentDriveSet(SD_MOUNT_NAME) == SYS_FS_RES_FAILURE)
    {
        TRACE_DBG("UNABLE  to select drive, error %d\n",SYS_FS_Error());

    }
    else 
    {
        TRACE_DBG( " Selected mounted drive\n");
    }

    infile = SYS_FS_FileOpen(FILE_IMAGE_NAME, SYS_FS_FILE_OPEN_READ);

    if (infile != SYS_FS_HANDLE_INVALID) 
    {
      TRACE_DBG( "File Open Successful \n");
    }
    else 
    {
      TRACE_DBG( "File Open Failed \n");
    }

    while( SYS_FS_FileStringGet(infile, line,200) == SYS_FS_RES_SUCCESS ) 
    {
        //  TRACE_DBG("Line : %s",  line);

        for (i=0 ; i<no_of_elements; i++)
        {
            found = strstr(line,str_param[i]);

            if (found)
            {  
                found += strlen(str_param[i]);
                //TRACE_DBG("found=%s  i = %d\n",found,i); 
                switch(i)
                {
                case 0:   /* "SSID = " */
                    strncpy(App.ssid, found,strlen(found)-2);
                    TRACE_DBG("ssid =%s \n",App.ssid);
                    break;
                case 1:   /* "PASSPHRASE = " */
                    strncpy(App.passphrase, found,strlen(found)-2);
                    TRACE_DBG("pass phrase=%s \n",App.passphrase);
                    break;

                case 2:   /* "FREQUENCY = " */
                    App.frequency = atoi(found);
                    TRACE_DBG("Frequency =%ld \n",App.frequency);
                    break;  
                default: 
                    break;

                }  /* end of switch */
				break;
            } /* if(found) */

        } /* for loop*/
        memset(line,0x00,sizeof(line));
    } /* while */
    SYS_FS_FileClose(infile);  /* Close the file */
}
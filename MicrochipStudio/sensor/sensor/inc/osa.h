/*******************************************************************************
* (C) Copyright 2020;  WDB Systems India Pvt Ltd, Bangalore
* The attached material and the information contained therein is proprietary
* to WDB Systems India Pvt Ltd and is issued only under strict confidentiality
* arrangements.It shall not be used, reproduced, copied in whole or in part,
* adapted,modified, or disseminated without a written license of WDB Systems 
* India Pvt Ltd.It must be returned to WDB Systems India Pvt Ltd upon its first
* request.
*
*  File Name           : osa.h
*
*  Description         : This is main controller application code 
*
*  Change history      : 
*
*     Author        Date           Ver                 Description
*  ------------    --------        ---   --------------------------------------
*   venu kosuri  24th Aug 2020      1.1               Initial Creation
*  
*******************************************************************************/
#ifndef OSA_H
#define OSA_H

/*******************************************************************************
*                          Include Files
*******************************************************************************/
#ifdef LINUX
#include "osa_linux.h"
#endif

#ifdef WINDOWS_MINGW
#include "osa_windows.h"
#endif

#ifdef FREERTOS_KERNEL
#include "osa_freertos.h"
#endif


/*******************************************************************************
*                          C++ Declaration Wrapper
*******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*                          Type & Macro Declarations
*******************************************************************************/
#define OSA_THREAD_ENABLE_CLOSE_STATE  1

#define OSA_THREAD_DIABLE_CLOSE_STATE  0

#define OSA_PERIODIC_TIMER 1

#define OSA_NO_PERIODIC_TIMER 0


/*******************************************************************************
*                          Extern Data Declarations
*******************************************************************************/

/*******************************************************************************
*                          Extern Function Prototypes
*******************************************************************************/ 
#ifdef LINUX
void OSA_ThreadCreate(osa_thread_handle       *thread_handle,
                      osa_thread_priority     *thread_priority,
                      osa_thread_entry_func   thread_entry_func,
                      osa_thread_arguement    thread_arg,
                      osa_thread_name         thread_name,
                      osa_thread_stack        thread_stack,
                      osa_thread_stack_size   thread_stack_size,
                      osa_thread_storage_data thread_storage_data);

#else
void OSA_ThreadCreate(osa_thread_handle       *thread_handle,
                      osa_thread_priority     thread_priority,
                      osa_thread_entry_func   thread_entry_func,
                      osa_thread_arguement    thread_arg,
                      osa_thread_name         thread_name,
                      osa_thread_stack        thread_stack,
                      osa_thread_stack_size   thread_stack_size,
                      osa_thread_storage_data thread_storage_data);
#endif

void OSA_ThreadClose(osa_thread_handle thread_handle);

void OSA_ThreadSetCloseState(BOOLEAN state );

void OSA_InitSemaphore(osa_semaphore_type_t *sem, unsigned int init_value,unsigned int Max);

void OSA_TakeSemaphore(osa_semaphore_type_t *sem);

BOOLEAN OSA_TakeSemaphore_Timeout(osa_semaphore_type_t *sem, unsigned int timeout_ms);

void OSA_GiveSemaphore(osa_semaphore_type_t *sem);

void OSA_GiveSemaphoreIsr(osa_semaphore_type_t *sem);

void OSA_DeInitSemaphore(osa_semaphore_type_t *sem);

void OSA_CloseSemaphore(osa_semaphore_type_t *sem);

void OSA_InitMutex(osa_mutex_type_t *mutex);

void OSA_LockMutex(osa_mutex_type_t *mutex);

void OSA_UnLockMutex(osa_mutex_type_t *mutex);

void OSA_DeInitMutex(osa_mutex_type_t *mutex);

BOOLEAN OSA_InitMsgQue(osa_msgq_type_t* mq_handler,unsigned char *mq_name, unsigned int max_msgs, unsigned int msg_size);

#ifdef FREERTOS_KERNEL

BOOLEAN OSA_SendMsgQue(osa_msgq_type_t mq_handler,void *msg);

signed int OSA_SendMsgQueTimeout(osa_msgq_type_t mq_handler,void *msg,  unsigned int timeout_ms );

BOOLEAN OSA_SendMsgQueIsr(osa_msgq_type_t mq_handler,void *msg);

BOOLEAN OSA_ReceiveMsgQue(osa_msgq_type_t mq_handler,void **msg);

signed int  OSA_ReceiveMsgQueTimeout(osa_msgq_type_t mq_handler,void **msg, unsigned int timeout_ms);

unsigned int OSA_ReceiveMsgQueIsr(osa_msgq_type_t mq_handler,void **msg);


#else

BOOLEAN OSA_SendMsgQue(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size);

signed int OSA_SendMsgQueTimeout(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size, unsigned int time_sec);

BOOLEAN OSA_ReceiveMsgQue(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size);

signed int OSA_ReceiveMsgQueTimeout(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size, unsigned int time_sec);

#endif


BOOLEAN OSA_DeinitMsgQue(osa_msgq_type_t* mq_handler,unsigned char *mq_name);

void OSA_Sleep(unsigned int time_sec);

void OSA_Usleep(unsigned int time_usec);

unsigned int OSA_InitTimer(osa_timer_type_t *timer, osa_time_type_t period_ms, BOOLEAN is_periodic, Callback_1_Arg function, void *arg);

unsigned int OSA_StartTimer(osa_timer_type_t *timer);

unsigned int OSA_ResetTimer(osa_timer_type_t *timer);

unsigned int OSA_StopTimer(osa_timer_type_t *timer);

unsigned int OSA_DeInitTimer(osa_timer_type_t *timer);

BOOLEAN  OSA_IsTimerRunning(osa_timer_type_t *timer);

unsigned int OSA_get_time(void);




#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
*                          End of File
*******************************************************************************/

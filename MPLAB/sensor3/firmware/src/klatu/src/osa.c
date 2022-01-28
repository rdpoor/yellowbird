/*******************************************************************************
* (C) Copyright 2014;  WDBSystems, Bangalore
* The attached material and the information contained therein is proprietary
* to WDBSystems and is issued only under strict confidentiality arrangements.
* It shall not be used, reproduced, copied in whole or in part, adapted,
* modified, or disseminated without a written license of WDBSystems.           
* It must be returned to WDBSystems upon its first request.
*
*  File Name           : osa.c
*
*  Description         : It contains  OS abstraction functions
*
*  Change history      : $Id$
*
*     Author        Date           Ver                 Description
*  ------------    --------        ---   --------------------------------------
*   venu kosuri   16th Jan 2014     1.1               Initial Creation
*  
*******************************************************************************/


/*******************************************************************************
*                          Include Files
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>


#include "osa.h"


/*******************************************************************************
*                          Extern Data Declarations
*******************************************************************************/

/*******************************************************************************
*                          Extern Function Declarations
*******************************************************************************/

/*******************************************************************************
*                          Type & Macro Definitions
*******************************************************************************/

/*******************************************************************************
*                          Static Function Prototypes
*******************************************************************************/
#ifdef FREERTOS_KERNEL
static void osa_TimerCallback(xTimerHandle handle);
#endif

/*******************************************************************************
*                          Static Data Definitions
*******************************************************************************/

/*******************************************************************************
*                          Extern Data Definitions
*******************************************************************************/

/*******************************************************************************
*                          Extern Function Definitions
*******************************************************************************/

/*******************************************************************************
* Name       : OSA_ThreadCreate
* Description: This function creates the thread
* Remarks    : 
*******************************************************************************/
#ifdef LINUX
void OSA_ThreadCreate(osa_thread_handle       *thread_handle,
                      osa_thread_priority     *thread_priority,
                      osa_thread_entry_func   thread_entry_func,
                      osa_thread_arguement    thread_arg,
                      osa_thread_name         thread_name,
                      osa_thread_stack        thread_stack,
                      osa_thread_stack_size   thread_stack_size,
                      osa_thread_storage_data thread_storage_data)
#else
void OSA_ThreadCreate(osa_thread_handle       *thread_handle,
                      osa_thread_priority     thread_priority,
                      osa_thread_entry_func   thread_entry_func,
                      osa_thread_arguement    thread_arg,
                      osa_thread_name         thread_name,
                      osa_thread_stack        thread_stack,
                      osa_thread_stack_size   thread_stack_size,
                      osa_thread_storage_data thread_storage_data)
#endif
{
#ifdef LINUX
    pthread_create(thread_handle, thread_priority, thread_entry_func,thread_arg);   
#endif 

#ifdef WINDOWS_MINGW
    pthread_create(thread_handle, thread_priority, thread_entry_func,thread_arg);   
#endif 

#ifdef FREERTOS_KERNEL
    xTaskCreate((pdTASK_CODE)thread_entry_func, 
                (const char * const)thread_name, 
                thread_stack_size,
                thread_arg,
                thread_priority,
                thread_handle);
#endif 

}

/*******************************************************************************
* Name       : OSA_ThreadDestroy
* Description: This function terminates the thread
* Remarks    : 
*******************************************************************************/
void OSA_ThreadClose(osa_thread_handle  thread_handle)
{
#ifdef LINUX
    if (pthread_cancel(thread_handle) != 0 ) 
    {
        printf ("OSA_ThreadClose:- Error Cancelling Thread,Exiting \n");

        exit(0) ;
    }
#endif

#ifdef WINDOWS_MINGW
    if (pthread_cancel(thread_handle) != 0 ) 
    {
        printf ("OSA_ThreadClose:- Error Cancelling Thread,Exiting \n");

        exit(0) ;
    }
#endif

#ifdef FREERTOS_KERNEL

    if (thread_handle == NULL)
    {
        vTaskDelete(NULL);
    }
    else
    {
        vTaskDelete(thread_handle);
    }

#endif

}


/*******************************************************************************
* Name       : OSA_ThreadSetCloseState
* Description: This function terminates the thread
* Remarks    : Not applicable in freeRTOS case
*******************************************************************************/
void OSA_ThreadSetCloseState(BOOLEAN state )
{
#ifdef LINUX
    if(state ) 
    {
        printf ("OSA_ThreadSetCloseState:- Enabling Cancellation\n");
        pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
    }
    else
    {
        printf ("OSA_ThreadSetCloseState:- Disabling Cancellation\n");
        pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, NULL);
    }
#endif

#ifdef WINDOWS_MINGW
    if(state ) 
    {
        printf ("OSA_ThreadSetCloseState:- Enabling Cancellation\n");
        pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
    }
    else
    {
        printf ("OSA_ThreadSetCloseState:- Disabling Cancellation\n");
        pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, NULL);
    }
#endif

}

/*******************************************************************************
* Name       : OSA_InitSemaphore  
* Description: This function Creates/Initializes the semaphore
* Remarks    : 
*******************************************************************************/
void OSA_InitSemaphore(osa_semaphore_type_t *sem, unsigned int init_value,unsigned int max)
{

#ifdef LINUX
    if (sem_init(sem,0,init_value) != 0 ) 
    {
        printf ("OSA_InitSemaphore:- Error Initializing Semephore, Exiting\n");

        exit(0) ;
    }
#endif

#ifdef WINDOWS_MINGW
        if (sem_init(sem,0,init_value) != 0 ) 
    {
        printf ("OSA_InitSemaphore:- Error Initializing Semephore, Exiting\n");

        exit(0) ;
    }
#endif 


#ifdef FREERTOS_KERNEL
    *sem = xSemaphoreCreateCounting(max, init_value);
#endif
}

/*******************************************************************************
* Name       : OSA_TakeSemaphore 
* Description: This function acquires the semaphore 
* Remarks    : 
*******************************************************************************/
void OSA_TakeSemaphore(osa_semaphore_type_t *sem )
{
#ifdef LINUX

WAIT_ON_CONNECT_SEM:
    if (sem_wait(sem) != 0) 
    {
        if (errno == EINTR)
        {
            /* this error occurs when ISR interrupts system call 
               Need to ignore  restrat system  call once again */
            goto WAIT_ON_CONNECT_SEM;
        }
        else
        {
            printf ("OSA_TakeSemaphore:- Error waiting on Semephore, Exiting = %d\n",errno);
            exit(0) ; 
        }  
    }
#endif

#ifdef WINDOWS_MINGW

WAIT_ON_CONNECT_SEM:
    if (sem_wait(sem) != 0) 
    {
#if 0
        if (errno == EINTR)
        {
            /* this error occurs when ISR interrupts system call 
               Need to ignore  restrat system  call once again */
            goto WAIT_ON_CONNECT_SEM;
        }

        else
#endif
        {
            printf ("OSA_TakeSemaphore:- Error waiting on Semephore, Exiting = %d\n",errno);
            exit(0) ; 
        }  
    }
#endif

#ifdef FREERTOS_KERNEL
    if (xSemaphoreTake(*sem, OSA_MAX_DELAY_MS) != pdTRUE )
    {
        printf ("OSA_TakeSemaphore:- Error waiting on Semephore, Exiting \n");
    }
#endif

}

/*******************************************************************************
* Name       : OSA_TakeSemaphore_Timeout
* Description: This function acquires the semaphore, it waits to acquire semaphore
*              for particaular time period, if unable to acquire it times out
* Remarks    : 
*******************************************************************************/
BOOLEAN OSA_TakeSemaphore_Timeout(osa_semaphore_type_t *sem, unsigned int timeout_ms)
{


#ifdef LINUX

    time_t cur_time;
    struct timespec ts;

    cur_time = time(NULL);

    cur_time += (timeout_ms*1000);
    ts.tv_sec = cur_time;
    ts.tv_nsec = 0;

WAIT_ON_CONNECT_SEM:
    if (sem_timedwait(sem, &ts) != 0)
    {
        if (errno == EINTR)
        {
            /* this error occurs when ISR interrupts system call
               Need to ignore  restrat system  call once again */
            goto WAIT_ON_CONNECT_SEM;
        }
        else
        {
            printf ("OSA_TakeSemaphore:- Error waiting on Semephore, Exiting = %d\n",errno);
            return FALSE;
            //exit(0);
        }
    }

    return TRUE;
#endif

#ifdef WINDOWS_MINGW

    time_t cur_time;
    struct timespec ts;

    cur_time = time(NULL);

    cur_time += (timeout_ms*1000);
    ts.tv_sec = cur_time;
    ts.tv_nsec = 0;


WAIT_ON_CONNECT_SEM:
    if (sem_timedwait(sem, &ts) != 0)
    {
       if (errno == EINTR)
        {
            /* this error occurs when ISR interrupts system call
               Need to ignore  restrat system  call once again */
            goto WAIT_ON_CONNECT_SEM;
        }
        else
        {
            printf ("OSA_TakeSemaphore:- Semaphore Wait Timeout, Exiting = %d\n",errno);
            return FALSE;
           // exit(0);
        }
    }

    return TRUE;
#endif

#ifdef FREERTOS_KERNEL

    unsigned int  StartTime;
    unsigned int  EndTime;
    unsigned int  Elapsed;
    osa_tick_type_t ticks = timeout_ms;

    if (ticks != OSA_MAX_DELAY_MS) {
        ticks /= OSA_TICK_RATE_MS;
    }
    
    StartTime = OSA_get_time();

    if (xSemaphoreTake(*sem, ticks) == pdTRUE ) {
        EndTime = OSA_get_time();
        Elapsed = EndTime - StartTime;
            
        return Elapsed; // return time blocked TBD test 
    } else {
        return (BOOLEAN)OSA_MAX_DELAY_MS;
    }


#endif
}

/*******************************************************************************
* Name       : OSA_GiveSemaphore
* Description: This function releases the semaphore
* Remarks    : 
*******************************************************************************/
void OSA_GiveSemaphore(osa_semaphore_type_t *sem)
{
#ifdef LINUX
    if (sem_post(sem) != 0) 
    {
        printf ("OSA_GiveSemaphore:- Error post Semephore , Exiting = %d\n",errno);
        exit(0) ;
    }
#endif

#ifdef WINDOWS_MINGW
    if (sem_post(sem) != 0) 
    {
        printf ("OSA_GiveSemaphore:- Error post Semephore , Exiting = %d\n",errno);
        exit(0) ;
    }
#endif

#ifdef FREERTOS_KERNEL

    if (xSemaphoreGive(*sem) != pdPASS)
    {
        printf ("OSA_GiveSemaphore:- Error GIVE Semephore , Exiting \n");
    }

#endif
}

/*******************************************************************************
* Name       : OSA_GiveSemaphoreIsr
* Description: This function releases the semaphore from an ISR 
* Remarks    : Not applicable in Linux/windows Mingw
*******************************************************************************/
void OSA_GiveSemaphoreIsr(osa_semaphore_type_t *sem)
{

#ifdef FREERTOS_KERNEL

   // signed portBASE_TYPE result;

    signed int  xHigherPriorityTaskWoken;

    xSemaphoreGiveFromISR(*sem, &xHigherPriorityTaskWoken);

    /* If xSemaphoreGiveFromISR() unblocked a task, and the unblocked task has
    a higher priority than the currently executing task, then
    xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
    return directly to the higher priority unblocked task. */

    //portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

#endif
}



/*******************************************************************************
* Name       : OSA_DeInitSemaphore 
* Description: This function Closes  the semaphore
* Remarks    : 
*******************************************************************************/
void OSA_DeInitSemaphore(osa_semaphore_type_t *sem)
{
#ifdef LINUX
    sem_destroy(sem);
#endif

#ifdef WINDOWS_MINGW
    sem_destroy(sem);
#endif

#ifdef FREERTOS_KERNEL
    vQueueDelete(*sem);
    *sem = NULL;
#endif
}

/*******************************************************************************
* Name       : OSA_CloseSemaphore 
* Description: This function Closes  the semaphore
* Remarks    : Not applicable in freeRTOS
*******************************************************************************/
void OSA_CloseSemaphore(osa_semaphore_type_t *sem)
{
#ifdef LINUX
     sem_close(sem);  
#endif

#ifdef WINDOWS_MINGW
    sem_close(sem);
#endif

}
/*******************************************************************************
* Name       : OSA_InitMutex 
* Description: This function initializes the mutex
* Remarks    : 
*******************************************************************************/
void OSA_InitMutex(osa_mutex_type_t *mutex)
{
#ifdef LINUX
    pthread_mutex_init(mutex, NULL);
#endif

#ifdef WINDOWS_MINGW
    pthread_mutex_init(mutex, NULL);    
#endif 

#ifdef FREERTOS_KERNEL
    *mutex = xSemaphoreCreateMutex();
#endif

}

/*******************************************************************************
* Name       : OSA_LockMutex 
* Description: This function Lock  the mutex
* Remarks    : 
*******************************************************************************/
void OSA_LockMutex(osa_mutex_type_t *mutex)
{
#ifdef LINUX
    pthread_mutex_lock(mutex);
#endif

#ifdef WINDOWS_MINGW
    pthread_mutex_lock(mutex);    
#endif

#ifdef FREERTOS_KERNEL
    xSemaphoreTake(*mutex, OSA_MAX_DELAY_MS);
#endif

}

/*******************************************************************************
* Name       : OSA_UnLockMutex 
* Description: This function unlocks the mutex
* Remarks    : 
*******************************************************************************/
void OSA_UnLockMutex(osa_mutex_type_t *mutex)
{
#ifdef LINUX
    pthread_mutex_unlock(mutex);
#endif

#ifdef WINDOWS_MINGW
    pthread_mutex_unlock(mutex);    
#endif

#ifdef FREERTOS_KERNEL
    xSemaphoreGive( *mutex );
#endif
}
/*******************************************************************************
* Name       : OSA_DeInitMutex 
* Description: This function Deinitializes/Closes  the mutex
* Remarks    : 
*******************************************************************************/
void OSA_DeInitMutex(osa_mutex_type_t *mutex)
{
#ifdef LINUX
    pthread_mutex_destroy(mutex);
#endif

#ifdef WINDOWS_MINGW
    pthread_mutex_destroy(mutex);    
#endif

#ifdef FREERTOS_KERNEL
    vQueueDelete(*mutex);
    *mutex = NULL;
#endif

}

/*******************************************************************************
* Name       : OSA_Mq_Init
* Description: Creates the message queue
* Remarks    :
*******************************************************************************/
BOOLEAN OSA_InitMsgQue(osa_msgq_type_t* mq_handler,unsigned char *mq_name, unsigned int max_msgs, unsigned int msg_size)
{
#ifdef LINUX
    struct mq_attr attr;

    /* Attributes for message queue*/
    attr.mq_flags = 0;
    attr.mq_maxmsg = max_msgs;
    attr.mq_msgsize = msg_size;
    attr.mq_curmsgs = 0;

    /* delete the message,if existed*/
    mq_unlink(mq_name);

    /*Create the message queue*/
    *mq_handler = mq_open(mq_name,(O_CREAT|O_RDWR),0666,&attr);
    if(*mq_handler == -1) {
        perror("Unable to create message queue \n");
        return FALSE;
    }

    return TRUE;
#endif

#ifdef FREERTOS_KERNEL
    return (*mq_handler = xQueueCreate(max_msgs, msg_size)) != NULL ? TRUE : FALSE;
#endif
}
/*******************************************************************************
* Name       : OSA_SendMsgQue
* Description: Stores the data into message queue from given buffer
* Remarks    : Buffer and Size shall not be NULL
*******************************************************************************/
#ifdef FREERTOS_KERNEL
BOOLEAN OSA_SendMsgQue(osa_msgq_type_t mq_handler,void *msg)
#else
BOOLEAN OSA_SendMsgQue(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size)
#endif
{
#ifdef LINUX
    INT32 ret = -1;

    if(buffer == NULL || size == 0)
        return FALSE;

    /* NOTE: In linux message queue,while posting to mq data from given buffer is
       copied and pastes into kernel memory*/

    /* Post the data to messsage */
    ret = mq_send(*mq_handler,buffer, size,0);

    if(ret == -1){
        perror("Failed to Post \n");

        return FALSE;
    }

    return TRUE;
#endif

#ifdef FREERTOS_KERNEL
    if (pdTRUE != xQueueSend(mq_handler, &msg, OSA_MAX_DELAY_MS))
    {
        printf("Failed to Send tO message Q  \n");

        return FALSE;
    }
    else
    {
        return TRUE;
    } 
#endif
}


/*******************************************************************************
* Name       : OSA_SendMsgQueTimeout
* Description: Sends the data from message queue and stores it into given buffer
               It waits till the timeout
* Remarks    : Buffer and Size shall not be NULL
*******************************************************************************/
#ifdef FREERTOS_KERNEL
signed int OSA_SendMsgQueTimeout(osa_msgq_type_t mq_handler,void *msg,  unsigned int timeout_ms )
#else
signed int OSA_SendMsgQueTimeout(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size, unsigned int time_sec)
#endif
{
#ifdef LINUX
    INT32 len = -1;

    struct   timespec tm;

    if(buffer == NULL || size == 0)
        return len;

    clock_gettime(CLOCK_REALTIME, &tm);

    tm.tv_sec += time_sec;  
 
    /* Fetches data from message queue*/
    len = mq_timedsend(*mq_handler,buffer, size,0, &tm);

    if(len == -1)
        perror("Failed to Fetch \n");

    return len;
#endif

#ifdef FREERTOS_KERNEL

    unsigned int StartTime;
    unsigned int  EndTime;
    unsigned int  Elapsed;
    osa_tick_type_t ticks = timeout_ms;

    if (ticks != OSA_MAX_DELAY_MS) 
    {
        ticks /= OSA_TICK_RATE_MS;
    }

    StartTime = OSA_get_time();

    if (pdTRUE == xQueueSend(mq_handler, &msg, ticks))
    {
        EndTime = OSA_get_time();
        Elapsed = EndTime - StartTime;
            
        return Elapsed;
    }
    else 
    {
        // timed out blocking for message
        return OSA_MAX_DELAY_MS;
    }
#endif
}

/*******************************************************************************
* Name       : OSA_SendMsgQueIsr
* Description: This function posts message  from an ISR 
* Remarks    : Not applicable in Linux/windows Mingw
*******************************************************************************/
#ifdef FREERTOS_KERNEL
BOOLEAN OSA_SendMsgQueIsr(osa_msgq_type_t mq_handler,void *msg)
{

    signed portBASE_TYPE result;

    int32_t xHigherPriorityTaskWoken;
    result = xQueueSendFromISR(mq_handler, &msg, &xHigherPriorityTaskWoken);
    /* If xQueueSendFromISR() unblocked a task, and the unblocked task has
    a higher priority than the currently executing task, then
    xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
    return directly to the higher priority unblocked task. */
  // portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

    return ( result == pdPASS )? TRUE : FALSE;

}
#endif

/*******************************************************************************
* Name       : OSA_ReceiveMsgQue
* Description: Reads the data from message queue and stores it into given buffer
* Remarks    : Buffer and Size shall not be NULL
*******************************************************************************/
#ifdef FREERTOS_KERNEL
BOOLEAN OSA_ReceiveMsgQue(osa_msgq_type_t mq_handler,void **msg)
#else
BOOLEAN OSA_ReceiveMsgQue(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size)
#endif
{
#ifdef LINUX
    INT32 ret = -1;

    if(buffer == NULL || size == 0)
        return FALSE;

    /* NOTE: In linux message queue, while fetching data from mq is copied,
       and pastes into given buffer*/

    /* Fetches data from message queue*/
    ret = mq_receive(*mq_handler,buffer, size,0);

    if(ret == -1){
        perror("Failed to Fetch \n");
        return FALSE;
    }

    return TRUE;
#endif

#ifdef FREERTOS_KERNEL
    if (pdTRUE != xQueueReceive(mq_handler, msg, OSA_MAX_DELAY_MS))
    {
        printf("Failed to Receive message Q  \n");
		return FALSE;
    }
	else
		return TRUE; 
#endif
}


/*******************************************************************************
* Name       : OSA_ReceiveMsgQueTimeout
* Description: Reads the data from message queue and stores it into given buffer
               It waits till the timeout
* Remarks    : Buffer and Size shall not be NULL. in case of freeRTOS , time_sec
                is in milliseconds
*******************************************************************************/
#ifdef FREERTOS_KERNEL
signed int OSA_ReceiveMsgQueTimeout(osa_msgq_type_t mq_handler,void **msg, unsigned int timeout_ms)
#else
signed int OSA_ReceiveMsgQueTimeout(osa_msgq_type_t* mq_handler,unsigned char *buffer, unsigned int size, unsigned int time_sec)
#endif
{
#ifdef LINUX
    INT32 len = -1;

    struct   timespec tm;

    if(buffer == NULL || size == 0)
        return len;

    clock_gettime(CLOCK_REALTIME, &tm);

    tm.tv_sec += time_sec;  
 
    /* Fetches data from message queue*/
    len = mq_timedreceive(*mq_handler,buffer, size,0, &tm);

    if(len == -1)
        perror("Failed to Fetch \n");

    return len;
#endif

#ifdef FREERTOS_KERNEL

    void *dummyptr;
    unsigned int StartTime;
    unsigned int EndTime;
    unsigned int Elapsed;
    osa_tick_type_t ticks = timeout_ms;

#if 0
    if (ticks != OSA_MAX_DELAY_MS) {
        ticks /= OSA_TICK_RATE_MS;
    }
#endif

    StartTime = OSA_get_time();

    if (msg == NULL) {   msg = &dummyptr;  }
        
    if (pdTRUE == xQueueReceive(mq_handler, msg, ticks)) {
        EndTime = OSA_get_time();
        Elapsed = EndTime - StartTime;
        return Elapsed;
    } else {
        // timed out blocking for message
       // *msg = NULL;
        return OSA_MAX_DELAY_MS;
    }

#endif
}

/*******************************************************************************
* Name       : OSA_SendMsgQueIsr
* Description: This function posts message  from an ISR 
* Remarks    : Not applicable in Linux/windows Mingw
*******************************************************************************/
#ifdef FREERTOS_KERNEL
unsigned int OSA_ReceiveMsgQueIsr(osa_msgq_type_t mq_handler, void **msg)
{
    void *dummyptr;

    if (msg == NULL) {   msg = &dummyptr;  }
        
    signed portBASE_TYPE result;

    int32_t xHigherPriorityTaskWoken;
    result = xQueueReceiveFromISR(mq_handler, msg, &xHigherPriorityTaskWoken);
    /* If xQueueReceiveFromISR() unblocked a task, and the unblocked task has
    a higher priority than the currently executing task, then
    xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
    return directly to the higher priority unblocked task. */
 //   portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
 portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

    return ( result == pdPASS )? STATUS_OK : STATUS_ERR;
}
#endif

/*******************************************************************************
* Name       : OSA_DeinitMsgQue
* Description: Unlinks and close/deletes the message queue
* Remarks    : buffer and Size should not be NULL
*******************************************************************************/
BOOLEAN OSA_DeinitMsgQue(osa_msgq_type_t* mq_handler, unsigned char* mq_name)
{
#ifdef LINUX
    INT32 ret = -1;

    mq_unlink(mq_name);

    ret = mq_close(*mq_handler);

    if(ret == -1){
        perror("MQ Closing Failed \n");
        return FALSE;
    }

    return TRUE;
#endif

#ifdef FREERTOS_KERNEL
    if( uxQueueMessagesWaiting( *mq_handler ) )
    {
        /* Still has messages waiting? Ignored... */
        //TRACE_ASSERT();
    }

    vQueueDelete(*mq_handler);
    *mq_handler = NULL;
	return TRUE;
	
#endif
}
/*******************************************************************************
* Name       : OSA_Sleep
* Description: This function makes the system to go in sleep mode for designated 
*              time period in seconds
* Remarks    : 
*******************************************************************************/
void OSA_Sleep(unsigned int time_sec)
{
#ifdef LINUX
    sleep(time_sec);
#endif

#ifdef WINDOWS_MINGW
    //sleep(time_sec);
    usleep(time_sec*1000*1000);
#endif

#ifdef FREERTOS_KERNEL
    vTaskDelay(time_sec*1000/OSA_TICK_RATE_MS);
#endif
}

/*******************************************************************************
* Name       : OSA_Usleep
* Description: This function makes the system to go in sleep mode for designated 
*              time period in micro-seconds
* Remarks    : 
*******************************************************************************/
void OSA_Usleep(unsigned int time_usec)
{
#ifdef LINUX
    usleep(time_usec);
#endif

#ifdef WINDOWS_MINGW
    usleep(time_usec);
#endif
}


/*******************************************************************************
* Name       : OSA_InitTimer
* Description: This function creates timer handle
* Remarks    :
*******************************************************************************/
unsigned int OSA_InitTimer(osa_timer_type_t *timer, osa_time_type_t period_ms, BOOLEAN is_periodic, Callback_1_Arg function, void *arg)
{
#ifdef FREERTOS_KERNEL
	osa_tick_type_t ticks = period_ms / OSA_TICK_RATE_MS;
	timer->function = function;
	timer->arg = arg;

	timer->handle = (void *)xTimerCreate((const char *const)"timer", ticks, is_periodic, (void *)timer, osa_TimerCallback);

	return timer->handle == NULL ? STATUS_OK : STATUS_ERR;
#endif
}

/*******************************************************************************
* Name       : OSA_StartTimer
* Description: This function starts the timer
* Remarks    :
*******************************************************************************/
unsigned int OSA_StartTimer(osa_timer_type_t *timer)
{
#ifdef FREERTOS_KERNEL
	return xTimerStart(timer->handle, OSA_MAX_DELAY_MS) == pdPASS ? STATUS_OK : STATUS_ERR;
#endif
}

/*******************************************************************************
* Name       : OSA_ResetTimer
* Description: This function resets the timer
* Remarks    :
*******************************************************************************/
unsigned int OSA_ResetTimer(osa_timer_type_t *timer)
{
#ifdef FREERTOS_KERNEL
	return xTimerReset(timer->handle, 0) == pdPASS ? STATUS_OK : STATUS_ERR;
#endif
}

/*******************************************************************************
* Name       : OSA_StopTimer
* Description: This function stops the timer
* Remarks    :
*******************************************************************************/
unsigned int OSA_StopTimer(osa_timer_type_t *timer)
{
#ifdef FREERTOS_KERNEL
    if((timer->handle) && OSA_IsTimerRunning(timer->handle))
	{
        return xTimerStop(timer->handle, 0) == pdPASS ? STATUS_OK : STATUS_ERR;
    }
    else
    {
        return STATUS_OK;
    }
#endif
}
/*******************************************************************************
* Name       : OSA_IsTimerRunning
* Description: This function checks whether the timer is running or not
* Remarks    :
*******************************************************************************/
BOOLEAN OSA_IsTimerRunning(osa_timer_type_t *timer)
{
#ifdef FREERTOS_KERNEL
    return xTimerIsTimerActive(timer->handle) != 0 ? TRUE : FALSE;
#endif
}

/*******************************************************************************
* Name       : OSA_DeinitTimer
* Description: This function un initilizes timer
* Remarks    :
*******************************************************************************/
unsigned int OSA_DeInitTimer(osa_timer_type_t *timer)
{
#ifdef FREERTOS_KERNEL
    xTimerHandle   handle;

    handle = timer->handle;

//  timer->handle = 0x00;
//	timer->function = NULL;
//	timer->arg = 0x00;
	memset((unsigned char*)timer,0x00, sizeof(osa_timer_type_t));

    if(handle)
    {
	    return xTimerDelete(handle, 0) == pdPASS ? STATUS_OK : STATUS_ERR;
    }
    else
    {
        return STATUS_OK;
    }
#endif
}

/*******************************************************************************
* Name       : OSA_get_time
* Description:
* Remarks    : 
*******************************************************************************/
unsigned int OSA_get_time(void)
{
#ifdef FREERTOS_KERNEL
    return xTaskGetTickCount() * OSA_TICK_RATE_MS;
#endif
}

/*******************************************************************************
*                          Static Function Definitions
*******************************************************************************/

/*******************************************************************************
* Name       : OSA_ThreadCreate
* Description: This function creates the thread
* Remarks    :
*******************************************************************************/
#ifdef FREERTOS_KERNEL
static void osa_TimerCallback(xTimerHandle handle)
{

    osa_timer_type_t *timer = (osa_timer_type_t *)pvTimerGetTimerID(handle);

    if (timer->function) {
        timer->function(timer->arg);
    }

}
#endif

/*******************************************************************************
*                                   End of File
*******************************************************************************/

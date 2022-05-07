/*******************************************************************************
* (C) Copyright 2020;  WDB Systems India Pvt Ltd, Bangalore
* The attached material and the information contained therein is proprietary
* to WDB Systems India Pvt Ltd and is issued only under strict confidentiality
* arrangements.It shall not be used, reproduced, copied in whole or in part,
* adapted,modified, or disseminated without a written license of WDB Systems 
* India Pvt Ltd.It must be returned to WDB Systems India Pvt Ltd upon its first
* request.
*
*  File Name           : trace.h
*
*  Description         : This file defines trace functions
*
*  Change history      : 
*
*     Author        Date           Ver                 Description
*  ------------    --------        ---   --------------------------------------
*   Venu Kosuri  29th July 2020    1.1               Initial Creation
*  
*******************************************************************************/
#ifndef TRACE_H
#define TRACE_H

/*******************************************************************************
*                          Include Files
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "definitions.h"      
/*******************************************************************************
*                          C++ Declaration Wrapper
*******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*                          Type & Macro Declarations
*******************************************************************************/
#define TRACE_LEVEL_DBG          0
#define TRACE_LEVEL_INFO         1
#define TRACE_LEVEL_WARN         2
#define TRACE_LEVEL_ERR          3
#define TRACE_LEVEL_FATAL        4

#define TRACE_LEVEL_DBG_MSG         "DBG"
#define TRACE_LEVEL_INFO_MSG        "INFO"
#define TRACE_LEVEL_WARN_MSG        "WARN"
#define TRACE_LEVEL_ERR_MSG         "ERR"

#define TRACE_LEVEL_FATAL_MSG       "FATAL"

#define DBG_TRACE_LEVEL_DBG   /* Enable all types of prints during development */ 

#ifdef PS_USE_TRACE
#if defined(DBG_TRACE_LEVEL_DBG)
    #define TRACE_LEVEL     TRACE_LEVEL_DBG
#elif defined(DBG_TRACE_LEVEL_INFO)
    #define TRACE_LEVEL     TRACE_LEVEL_INFO
#elif defined(DBG_TRACE_LEVEL_WARN)
    #define TRACE_LEVEL     TRACE_LEVEL_WARN
#elif defined(DBG_TRACE_LEVEL_ERR)
    #define TRACE_LEVEL     TRACE_LEVEL_ERR
#elif defined(DBG_TRACE_LEVEL_FATAL)
    #define TRACE_LEVEL     TRACE_LEVEL_FATAL
#else
    #define TRACE_LEVEL     TRACE_LEVEL_INFO
#endif


#define TRACE_PRINTF               SYS_CONSOLE_PRINT  // printf


#define TRACE_PRINT(verbose, trace_level, ...) \
do {                                                        \
    TRACE_Lock();                                           \
   if (trace_level >= TRACE_LEVEL)                          \
   {                                                        \
     if (verbose)                                           \
     {                                                      \
       TRACE_PRINTF("(%s, %d) ",                            \
               __FILE__ ,                                   \
               __LINE__);                                   \
     }                                                      \
     TRACE_PRINTF("%s: ", trace_level##_MSG);               \
     TRACE_PRINTF(__VA_ARGS__);                             \
   }                                                        \
		TRACE_UnLock();                                    \
} while(0);

#define TRACE_DBG(...)  \
    TRACE_PRINT(0, TRACE_LEVEL_DBG, __VA_ARGS__)

#define TRACE_INFO(...) \
    TRACE_PRINT(0, TRACE_LEVEL_INFO, __VA_ARGS__)


#define TRACE_WARN(...) \
    TRACE_PRINT(0, TRACE_LEVEL_WARN, __VA_ARGS__)


#define TRACE_ERR(...)  \
    TRACE_PRINT(0, TRACE_LEVEL_ERR, __VA_ARGS__)

#define TRACE_FATAL(...)    \
    TRACE_PRINT(0, TRACE_LEVEL_FATAL, __VA_ARGS__)


#define TRACE_PRINT_ARRAY(lvl, array, size)     \
do {                                            \
    if (lvl >= TRACE_LEVEL)                     \
    {                                           \
        TRACE_printf_array(array, size);        \
    }                                           \
} while(0);

#else 

#define TRACE_PRINTF(...)                        ((void)0)
#define TRACE_PRINT(verbose, trace_level, ...) 
#define TRACE_DBG(...)
#define TRACE_INFO(...)
#define TRACE_WARN(...)
#define TRACE_ERR(...)
#define TRACE_FATAL(...)
#define TRACE_PRINT_ARRAY(lvl, array, size)

#endif  


/*******************************************************************************
*                          Extern Data Declarations
*******************************************************************************/
void TRACE_Init();

void TRACE_Lock();

void TRACE_UnLock();
/*******************************************************************************
*                          Extern Function Prototypes
*******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
*                          End of File
*******************************************************************************/

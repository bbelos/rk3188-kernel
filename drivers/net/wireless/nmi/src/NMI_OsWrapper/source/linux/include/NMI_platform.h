#ifndef __NMI_platfrom_H__
#define __NMI_platfrom_H__

/*!  
*  @file	NMI_platform.h
*  @brief	platform specific file for Linux port
*  @author	syounan
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	15 Dec 2010
*  @version	1.0
*/


/******************************************************************
        Feature support checks
*******************************************************************/

// CONFIG_NMI_THREAD_FEATURE is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_THREAD_SUSPEND_CONTROL
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_THREAD_STRICT_PRIORITY
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_SEMAPHORE_FEATURE is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SEMAPHORE_TIMEOUT
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_SLEEP_FEATURE is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SLEEP_HI_RES
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_TIMER_FEATURE is implemented

// CONFIG_NMI_TIMER_PERIODIC is implemented

// CONFIG_NMI_MEMORY_FEATURE is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MEMORY_POOLS
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MEMORY_DEBUG
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_ASSERTION_SUPPORT
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_STRING_UTILS is implemented

// CONFIG_NMI_MSG_QUEUE_FEATURE is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MSG_QUEUE_IPC_NAME
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MSG_QUEUE_TIMEOUT
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_FILE_OPERATIONS_FEATURE is implemented

// CONFIG_NMI_FILE_OPERATIONS_STRING_API is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_FILE_OPERATIONS_PATH_API
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_TIME_FEATURE is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_TIME_UTC_SINCE_1970
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_TIME_CALENDER
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_EVENT_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_EVENT_TIMEOUT
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_MATH_OPERATIONS_FEATURE is implemented

// CONFIG_NMI_EXTENDED_FILE_OPERATIONS is implemented

// CONFIG_NMI_EXTENDED_STRING_OPERATIONS is implemented

// CONFIG_NMI_EXTENDED_TIME_OPERATIONS is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SOCKET_FEATURE
#error This feature is not supported by this OS
#endif

/******************************************************************
	OS specific includes
*******************************************************************/
#define _XOPEN_SOURCE 600

#include <pthread.h>
#include <semaphore.h>

#include <stdio.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h> 
#include <time.h>

/******************************************************************
	OS specific types
*******************************************************************/

typedef pthread_t NMI_ThreadHandle;

typedef sem_t NMI_SemaphoreHandle;

typedef struct{

	/* the underlaying posix timer stored here */
	timer_t timerObject;
	
	/* semaphore for protection */
	NMI_SemaphoreHandle hAccessProtection;
	
	/* the callback is kept here */
	/* shoud be a proper pointer to function type
	insted of void*, but the timer callback type is
	defined later in its header */
	void* pfCallbackFunction;

	/* the invocation argument is kept here */
	void* pvArgument;

	/* if NMI_TRUE then the system expects a timer callback after a while
	if NMI_FALSE then the callback should be ignored */
	NMI_Bool bPendingTimer;

}NMI_TimerHandle;

/* change into OS specific type */
typedef void* NMI_MemoryPoolHandle;

/* Message Queue type is a structure */
typedef struct __Message_struct
{
	void* pvBuffer;
	NMI_Uint32 u32Length;
	struct __Message_struct *pstrNext;
} Message;

typedef struct __MessageQueue_struct
{	
	NMI_SemaphoreHandle hSem;
	NMI_Bool bExiting;
	NMI_Uint32 u32ReceiversCount;
	NMI_SemaphoreHandle strCriticalSection;
	Message * pstrMessageList;
} NMI_MsgQueueHandle;

typedef FILE* NMI_FileHandle;

/* change into OS specific type */
typedef void* NMI_EventHandle;

/*Time represented in 64 bit format*/
typedef time_t NMI_Time;

/*Time and date represented in a data structure*/
typedef struct tm NMI_tm;

/*******************************************************************
	others
********************************************************************/

/* Generic printf function */
#define NMI_PRINTF(...) printf(__VA_ARGS__)
#define __NMI_FILE__		__FILE__
#define __NMI_FUNCTION__	__FUNCTION__
#define __NMI_LINE__		__LINE__
#endif

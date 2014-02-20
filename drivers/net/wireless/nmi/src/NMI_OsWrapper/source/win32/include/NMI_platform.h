#ifndef __NMI_platfrom_H__
#define __NMI_platfrom_H__

/*!  
*  @file	NMI_platform.h
*  @brief	top level header for win32 implementation
*  @author	syounan
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	18 Aug 2010
*  @version	1.0
*/

/******************************************************************
        Feature support checks
*******************************************************************/

// CONFIG_NMI_THREAD_FEATURE is implemented

// CONFIG_NMI_THREAD_SUSPEND_CONTROL is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_THREAD_STRICT_PRIORITY
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_SEMAPHORE_FEATURE is implemented

// CONFIG_NMI_SEMAPHORE_TIMEOUT is implemented

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

// CONFIG_NMI_MSG_QUEUE_FEATURE  is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MSG_QUEUE_IPC_NAME
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_MSG_QUEUE_TIMEOUT is implemented

// CONFIG_NMI_FILE_OPERATIONS_FEATURE is implemented

// CONFIG_NMI_FILE_OPERATIONS_STRING_API is implemented

// CONFIG_NMI_TIME_FEATURE is implemented

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_TIME_UTC_SINCE_1970
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_TIME_CALENDER
#error This feature is not supported by this OS
#endif

// CONFIG_NMI_EVENT_FEATURE is implemented

// CONFIG_NMI_EVENT_TIMEOUT is implemented

/******************************************************************
	OS specific includes
*******************************************************************/
#include "stdio.h"
#define _WIN32_WINNT 0x0500
#include "Windows.h"

/******************************************************************
	OS specific types
*******************************************************************/

/* Thread Handle is a windows HANDLE */
typedef HANDLE NMI_ThreadHandle;

/* Semaphore Handle is a windows HANDLE */
typedef HANDLE NMI_SemaphoreHandle;

/* Timer Handle is a complex structure */
typedef struct{

	/* the underlaying windwos timer stored here */
	HANDLE hTimer;

	/* critical section for protection */
	CRITICAL_SECTION csProtectionCS;

	/* the callback is kept here */
	/* shoud be a proper pointer to function type
	insted of void*, but the timer callback type is
	defined later in its header */
	void* pfCallbackFunction;

	/* the invocation argument is kept here */
	void* pvArgument;

}NMI_TimerHandle;

/* change into OS specific type */
typedef void* NMI_MemoryPoolHandle;

/* Message Queue type is a structure */
typedef struct __Message_struct
{
	LPVOID pvBuffer;
	DWORD u32Length;
	struct __Message_struct *pstrNext;
} Message;

typedef struct __MessageQueue_struct
{	
	HANDLE hSem;
	BOOLEAN bExiting;
	DWORD ReceiversCount;
	CRITICAL_SECTION strCriticalSection;
	Message * pstrMessageList;
} NMI_MsgQueueHandle;

typedef FILE* NMI_FileHandle;

/* Event Handle is a windows HANDLE */
typedef HANDLE NMI_EventHandle;

/*******************************************************************
	others
********************************************************************/

/* Generic printf function */
#define NMI_PRINTF(...) printf(__VA_ARGS__)
#define __NMI_FILE__		__FILE__
#define __NMI_FUNCTION__	__FUNCTION__
#define __NMI_LINE__		__LINE__
#endif

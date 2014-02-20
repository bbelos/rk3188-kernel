#ifndef __NMI_platfrom_H__
#define __NMI_platfrom_H__

/*!  
*  @file	NMI_platform.h
*  @brief	platform specific file, when creating OS wrapper for a 
		new platform start wih this file
*  @author	syounan
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	10 Aug 2010
*  @version	1.0
*/


/******************************************************************
        Feature support checks
*******************************************************************/
/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_THREAD_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_THREAD_SUSPEND_CONTROL
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_THREAD_STRICT_PRIORITY
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SEMAPHORE_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SEMAPHORE_TIMEOUT
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SLEEP_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SLEEP_HI_RES
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_TIMER_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_TIMER_PERIODIC
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MEMORY_FEATURE
#error This feature is not supported by this OS
#endif

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

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_STRING_UTILS
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MSG_QUEUE_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MSG_QUEUE_IPC_NAME
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MSG_QUEUE_TIMEOUT
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_FILE_OPERATIONS_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_FILE_OPERATIONS_STRING_API
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_FILE_OPERATIONS_PATH_API
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_TIME_FEATURE
#error This feature is not supported by this OS
#endif

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

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_MATH_OPERATIONS_FEATURE
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_EXTENDED_FILE_OPERATIONS
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_EXTENDED_STRING_OPERATIONS
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_EXTENDED_TIME_OPERATIONS
#error This feature is not supported by this OS
#endif

/* remove the following block when implementing its feature */
#ifdef CONFIG_NMI_SOCKET_FEATURE
#error This feature is not supported by this OS
#endif

/******************************************************************
	OS specific includes
*******************************************************************/


/******************************************************************
	OS specific types
*******************************************************************/

/* change into OS specific type */
typedef void* NMI_ThreadHandle;

/* change into OS specific type */
typedef void* NMI_SemaphoreHandle;

/* change into OS specific type */
typedef void* NMI_TimerHandle;

/* change into OS specific type */
typedef void* NMI_MemoryPoolHandle;

/* change into OS specific type */
typedef void* NMI_MsgQueueHandle;

/* change into OS specific type */
typedef void* NMI_FileHandle;

/* change into OS specific type */
typedef void* NMI_EventHandle;

/*******************************************************************
	others
********************************************************************/

/* Generic printf function, map it to the standard printf */
#define NMI_PRINTF(...)
#define __NMI_FILE__
#define __NMI_FUNCTION__
#define __NMI_LINE__
#endif

#ifndef __NMI_OSWRAPPER_H__
#define __NMI_OSWRAPPER_H__

/*!  
*  @file	NMI_OSWrapper.h
*  @brief	Top level OS Wrapper, include this file and it will include all 
		other files as necessary
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/

/* OS Wrapper interface version */
#define NMI_OSW_INTERFACE_VER 2

/* Integer Types */
typedef unsigned char      NMI_Uint8;
typedef unsigned short     NMI_Uint16;
typedef unsigned int       NMI_Uint32;
typedef unsigned long long NMI_Uint64;
typedef signed char        NMI_Sint8;
typedef signed short       NMI_Sint16;
typedef signed int         NMI_Sint32;
typedef signed long long   NMI_Sint64;

/* Floating types */
typedef float NMI_Float;
typedef double NMI_Double;

/* Boolean type */
typedef enum {
	NMI_FALSE = 0,
	NMI_TRUE = 1	
}NMI_Bool;

/* Character types */
typedef char NMI_Char;
typedef NMI_Uint16 NMI_WideChar;

#define NMI_OS_INFINITY (~((NMI_Uint32)0))
#define NMI_NULL ((void*)0)

/* standard min and max macros */
#define NMI_MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define NMI_MAX(a, b)  (((a) > (b)) ? (a) : (b))

/* Os Configuration File */
#include "NMI_OsWrapper/OsConfig/NMI_OSConfig.h"

/* Platform specific include */
#if NMI_PLATFORM == NMI_WIN32
#include "NMI_OsWrapper/source/win32/include/NMI_platform.h"
#elif NMI_PLATFORM == NMI_NU
#include "NMI_OsWrapper/source/nucleus/include/NMI_platform.h"
#elif NMI_PLATFORM == NMI_MTK
#include "NMI_OsWrapper/source/mtk/include/NMI_platform.h"
#elif NMI_PLATFORM == NMI_LINUX
#include "NMI_OsWrapper/source/linux/include/NMI_platform.h"
#elif NMI_PLATFORM == NMI_LINUXKERNEL
#include "NMI_OsWrapper/source/linuxkernel/include/NMI_platform.h"
#else
#error "OS not supported"
#endif

/* Logging Functions */
#include "NMI_Log.h"

/* Error reporting and handling support */
#include "NMI_ErrorSupport.h"

/* Thread support */
#ifdef CONFIG_NMI_THREAD_FEATURE
#include "NMI_Thread.h"
#endif

/* Semaphore support */
#ifdef CONFIG_NMI_SEMAPHORE_FEATURE
#include "NMI_Semaphore.h"
#endif

/* Sleep support */
#ifdef CONFIG_NMI_SLEEP_FEATURE
#include "NMI_Sleep.h"
#endif

/* Timer support */
#ifdef CONFIG_NMI_TIMER_FEATURE
#include "NMI_Timer.h"
#endif

/* Memory support */
#ifdef CONFIG_NMI_MEMORY_FEATURE
#include "NMI_Memory.h"
#endif

/* String Utilities */
#ifdef CONFIG_NMI_STRING_UTILS
#include "NMI_StrUtils.h"
#endif

/* Message Queue */
#ifdef CONFIG_NMI_MSG_QUEUE_FEATURE
#include "NMI_MsgQueue.h"
#endif

/* File operations */
#ifdef CONFIG_NMI_FILE_OPERATIONS_FEATURE
#include "NMI_FileOps.h"
#endif

/* Time operations */
#ifdef CONFIG_NMI_TIME_FEATURE
#include "NMI_Time.h"
#endif

/* Event support */
#ifdef CONFIG_NMI_EVENT_FEATURE
#include "NMI_Event.h"
#endif

/* Socket operations */
#ifdef CONFIG_NMI_SOCKET_FEATURE
#include "NMI_Socket.h"
#endif

/* Math operations */
#ifdef CONFIG_NMI_MATH_OPERATIONS_FEATURE
#include "NMI_Math.h"
#endif



#endif

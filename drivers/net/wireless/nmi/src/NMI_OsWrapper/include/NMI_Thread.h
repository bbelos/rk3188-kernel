#ifndef __NMI_THREAD_H__
#define __NMI_THREAD_H__

/*!  
*  @file		NMI_Thread.h
*  @brief		Thread OS Wrapper functionality
*  @author		syounan
*  @sa			NMI_OSWrapper.h top level OS wrapper file
*  @date		10 Aug 2010
*  @version		1.0
*/

#ifndef CONFIG_NMI_THREAD_FEATURE
#error the feature NMI_OS_FEATURE_THREAD must be supported to include this file
#endif

typedef void (*tpfNMI_ThreadFunction)(void*);

typedef enum 
{
	#ifdef CONFIG_NMI_THREAD_STRICT_PRIORITY
	NMI_OS_THREAD_PIORITY_0 = 0,
	NMI_OS_THREAD_PIORITY_1 = 1,
	NMI_OS_THREAD_PIORITY_2 = 2,
	NMI_OS_THREAD_PIORITY_3 = 3,
	NMI_OS_THREAD_PIORITY_4 = 4,
	#endif

	NMI_OS_THREAD_PIORITY_HIGH = 0,
	NMI_OS_THREAD_PIORITY_NORMAL = 2,
	NMI_OS_THREAD_PIORITY_LOW = 4
}tenuNMI_ThreadPiority;

/*!
*  @struct 		NMI_ThreadAttrs
*  @brief		Thread API options 
*  @author		syounan
*  @date		10 Aug 2010
*  @version		1.0
*/
typedef struct
{
	/*!<
	stack size for use with NMI_ThreadCreate, default is NMI_OS_THREAD_DEFAULT_STACK
	*/
	NMI_Uint32 u32StackSize;

	/*!<
	piority for the thread, if NMI_OS_FEATURE_THREAD_STRICT_PIORITY is defined
	this value is strictly observed and can take a larger resolution
	*/
	tenuNMI_ThreadPiority enuPiority;
	
	#ifdef CONFIG_NMI_THREAD_SUSPEND_CONTROL
	/*!
	if true the thread will be created suspended
	*/
	NMI_Bool bStartSuspended;
	#endif
	
}tstrNMI_ThreadAttrs;

#define NMI_OS_THREAD_DEFAULT_STACK (10*1024)

/*!
*  @brief	Fills the NMI_ThreadAttrs with default parameters
*  @param[out]	pstrAttrs structure to be filled
*  @sa		NMI_ThreadAttrs
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/

static void NMI_ThreadFillDefault(tstrNMI_ThreadAttrs* pstrAttrs)
{
	pstrAttrs->u32StackSize = NMI_OS_THREAD_DEFAULT_STACK;
	pstrAttrs->enuPiority = NMI_OS_THREAD_PIORITY_NORMAL;
	
	#ifdef CONFIG_NMI_THREAD_SUSPEND_CONTROL
	pstrAttrs->bStartSuspended = NMI_FALSE;
	#endif
}

/*!
*  @brief	Creates a new thread
*  @details	if the feature NMI_OS_FEATURE_THREAD_SUSPEND_CONTROL is 
		defined and tstrNMI_ThreadAttrs.bStartSuspended is set to true 
		the new thread will be created in suspended state, otherwise
		it will start executing immeadiately
		if the feature NMI_OS_FEATURE_THREAD_STRICT_PIORITY is defined
		piorities are strictly observed, otherwise the underlaying OS 
		may not observe piorities
*  @param[out]	pHandle handle to the newly created thread object
*  @param[in]	pfEntry pointer to the entry point of the new thread
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return	Error code indicating sucess/failure
*  @sa		NMI_ThreadAttrs
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_ThreadCreate(NMI_ThreadHandle* pHandle, tpfNMI_ThreadFunction pfEntry,
	void* pvArg, tstrNMI_ThreadAttrs* pstrAttrs);

/*!
*  @brief	Destroys the Thread object
*  @details	This function is used for clean up and freeing any used resources
*		This function will block until the destroyed thread exits cleanely,
*		so, the thread code thould handle an exit case before this calling
*		this function
*  @param[in]	pHandle handle to the thread object
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return	Error code indicating sucess/failure
*  @sa		NMI_ThreadAttrs
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_ThreadDestroy(NMI_ThreadHandle* pHandle,
	tstrNMI_ThreadAttrs* pstrAttrs);

#ifdef CONFIG_NMI_THREAD_SUSPEND_CONTROL

/*!
*  @brief	Suspends an executing Thread object
*  @param[in]	pHandle handle to the thread object
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return	Error code indicating sucess/failure
*  @sa		NMI_ThreadAttrs
*  @note	Optional part, NMI_OS_FEATURE_THREAD_SUSPEND_CONTROL must be enabled
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_ThreadSuspend(NMI_ThreadHandle* pHandle,
	tstrNMI_ThreadAttrs* pstrAttrs);

/*!
*  @brief	Resumes a suspened Thread object
*  @param[in]	pHandle handle to the thread object
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return	Error code indicating sucess/failure
*  @sa		NMI_ThreadAttrs
*  @note	Optional part, NMI_OS_FEATURE_THREAD_SUSPEND_CONTROL must be enabled
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_ThreadResume(NMI_ThreadHandle* pHandle,
	tstrNMI_ThreadAttrs* pstrAttrs);

#endif


#endif

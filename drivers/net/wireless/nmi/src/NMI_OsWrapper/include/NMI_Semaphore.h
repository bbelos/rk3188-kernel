#ifndef __NMI_SEMAPHORE_H__
#define __NMI_SEMAPHORE_H__

/*!  
*  @file		NMI_Semaphore.h
*  @brief		Semaphore OS Wrapper functionality
*  @author		syounan
*  @sa			NMI_OSWrapper.h top level OS wrapper file
*  @date		10 Aug 2010
*  @version		1.0
*/


#ifndef CONFIG_NMI_SEMAPHORE_FEATURE
#error the feature NMI_OS_FEATURE_SEMAPHORE must be supported to include this file
#endif

/*!
*  @struct 		NMI_SemaphoreAttrs
*  @brief		Semaphore API options 
*  @author		syounan
*  @date		10 Aug 2010
*  @version		1.0
*/
typedef struct
{
	/*!<
	Initial count when the semaphore is created. default is 1
	*/
	NMI_Uint32 u32InitCount;

	#ifdef CONFIG_NMI_SEMAPHORE_TIMEOUT
	/*!<
	Timeout for use with NMI_SemaphoreAcquire, 0 to return immediately and 
	NMI_OS_INFINITY to wait forever. default is NMI_OS_INFINITY
	*/
	NMI_Uint32 u32TimeOut; 
	#endif
	
}tstrNMI_SemaphoreAttrs;


/*!
*  @brief	Fills the NMI_SemaphoreAttrs with default parameters
*  @param[out]	pstrAttrs structure to be filled
*  @sa		NMI_SemaphoreAttrs
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
static void NMI_SemaphoreFillDefault(tstrNMI_SemaphoreAttrs* pstrAttrs)
{
	pstrAttrs->u32InitCount = 1;
	#ifdef CONFIG_NMI_SEMAPHORE_TIMEOUT
	pstrAttrs->u32TimeOut = NMI_OS_INFINITY;
	#endif
}
/*!
*  @brief	Creates a new Semaphore object
*  @param[out]	pHandle handle to the newly created semaphore
*  @param[in]	pstrAttrs Optional attributes, NULL for defaults
				pstrAttrs->u32InitCount controls the initial count
*  @return	Error code indicating success/failure
*  @sa		NMI_SemaphoreAttrs
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_SemaphoreCreate(NMI_SemaphoreHandle* pHandle, 
	tstrNMI_SemaphoreAttrs* pstrAttrs);

/*!
*  @brief	Destroyes an existing Semaphore, releasing any resources
*  @param[in]	pHandle handle to the semaphore object
*  @param[in]	pstrAttrs Optional attributes, NULL for defaults
*  @return	Error code indicating success/failure
*  @sa		NMI_SemaphoreAttrs
*  @todo	need to define behaviour if the semaphore delayed while it is pending
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_SemaphoreDestroy(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs);

/*!
*  @brief	Acquire the Semaphore object
*  @details	This function will block until it can Acquire the given 
*		semaphore, if the feature NMI_OS_FEATURE_SEMAPHORE_TIMEOUT is
*		eanbled a timeout value can be passed in pstrAttrs
*  @param[in]	pHandle handle to the semaphore object
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return	Error code indicating success/failure
*  @sa		NMI_SemaphoreAttrs
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_SemaphoreAcquire(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs);

/*!
*  @brief	Release the Semaphore object
*  @param[in]	pHandle handle to the semaphore object
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return	Error code indicating sucess/failure
*  @sa		NMI_SemaphoreAttrs
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_SemaphoreRelease(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs);


#endif

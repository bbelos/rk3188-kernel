
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_EVENT_FEATURE


/*!
*  @author	syounan
*  @date	10 Oct 2010
*  @version	1.0
*/
NMI_ErrNo NMI_EventCreate(NMI_EventHandle* pHandle, tstrNMI_EventAttrs* pstrAttrs)
{
	*pHandle = CreateEvent(
		NULL, /* default security attributes */
		FALSE, /* auto reset */
		FALSE, /* initial state is non-signalled (UNTRIGGERED) */
		NULL /* unnamed object */);

	if(*pHandle != NULL)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}


/*!
*  @author	syounan
*  @date	10 Oct 2010
*  @version	1.0
*/
NMI_ErrNo NMI_EventDestroy(NMI_EventHandle* pHandle, 
	tstrNMI_EventAttrs* pstrAttrs)
{
	if(CloseHandle(*pHandle) != 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}


/*!
*  @author	syounan
*  @date	10 Oct 2010
*  @version	1.0
*/
NMI_ErrNo NMI_EventTrigger(NMI_EventHandle* pHandle, 
	tstrNMI_EventAttrs* pstrAttrs)
{
	if( SetEvent(*pHandle) != 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}


/*!
*  @author	syounan
*  @date	10 Oct 2010
*  @version	1.0
*/
NMI_ErrNo NMI_EventWait(NMI_EventHandle* pHandle, 
	tstrNMI_EventAttrs* pstrAttrs)
{

	DWORD dwMilliseconds = INFINITE;
	DWORD dwWaitResult;
	NMI_ErrNo s32RetStatus;

	#ifdef CONFIG_NMI_EVENT_TIMEOUT
	if((pstrAttrs != NMI_NULL) 
		&& (pstrAttrs->u32TimeOut != NMI_OS_INFINITY))
	{
		dwMilliseconds = pstrAttrs->u32TimeOut;
	}
	#endif

	dwWaitResult = WaitForSingleObject(*pHandle, dwMilliseconds);

	switch(dwWaitResult)
	{
	case WAIT_OBJECT_0:
		s32RetStatus = NMI_SUCCESS;
		break;

	case WAIT_TIMEOUT:
		s32RetStatus = NMI_TIMEOUT;
		break;

	default:
	case WAIT_FAILED:
		s32RetStatus = NMI_FAIL;
	}

	return s32RetStatus;

}


#endif
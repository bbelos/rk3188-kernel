
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"
#ifdef CONFIG_NMI_SEMAPHORE_FEATURE


NMI_ErrNo NMI_SemaphoreCreate(NMI_SemaphoreHandle* pHandle, 
	tstrNMI_SemaphoreAttrs* pstrAttrs)
{
	tstrNMI_SemaphoreAttrs strDefaultAttrs;
	if(pstrAttrs == NMI_NULL)
	{
		NMI_SemaphoreFillDefault(&strDefaultAttrs);
		pstrAttrs = &strDefaultAttrs;
	}

	sema_init(pHandle,pstrAttrs->u32InitCount);	
	return NMI_SUCCESS;
	
}


NMI_ErrNo NMI_SemaphoreDestroy(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs)
{
	/* nothing to be done ! */
	
	return NMI_SUCCESS;	

}


NMI_ErrNo NMI_SemaphoreAcquire(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs)
{
	NMI_ErrNo s32RetStatus = NMI_SUCCESS;

	#ifndef CONFIG_NMI_SEMAPHORE_TIMEOUT
	down(pHandle);

	#else
	if(pstrAttrs == NMI_NULL)
	{
		down(pHandle);
	}
	else
	{
		
		s32RetStatus = down_timeout(pHandle, msecs_to_jiffies(pstrAttrs->u32TimeOut));
	}
	#endif

	if(s32RetStatus == 0)
	{
		return NMI_SUCCESS;
	}
	else if (s32RetStatus == -ETIME)
	{
		return NMI_TIMEOUT;
	}
	else
	{
		return NMI_FAIL;
	}
	
	return NMI_SUCCESS;
	
}

NMI_ErrNo NMI_SemaphoreRelease(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs)
{

	up(pHandle);
	return NMI_SUCCESS;
	
}

#endif

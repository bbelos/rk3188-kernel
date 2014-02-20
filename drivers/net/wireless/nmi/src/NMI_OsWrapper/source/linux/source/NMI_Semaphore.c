
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

	if(sem_init(pHandle, 0, pstrAttrs->u32InitCount) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}


NMI_ErrNo NMI_SemaphoreDestroy(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs)
{
	if(sem_destroy(pHandle) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}


NMI_ErrNo NMI_SemaphoreAcquire(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs)
{
	if(sem_wait(pHandle) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

NMI_ErrNo NMI_SemaphoreRelease(NMI_SemaphoreHandle* pHandle,
	tstrNMI_SemaphoreAttrs* pstrAttrs)
{
	if(sem_post(pHandle) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

#endif

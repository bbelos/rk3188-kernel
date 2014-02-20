
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_THREAD_FEATURE

NMI_ErrNo NMI_ThreadCreate(NMI_ThreadHandle* pHandle, tpfNMI_ThreadFunction pfEntry,
	void* pvArg, tstrNMI_ThreadAttrs* pstrAttrs)
{
	pthread_attr_t attr;
	tstrNMI_ThreadAttrs strDefaultAttrs;
	NMI_ErrNo s32RetStatus = NMI_FAIL;

	if(pstrAttrs == NMI_NULL)
	{
		NMI_ThreadFillDefault(&strDefaultAttrs);
		pstrAttrs = &strDefaultAttrs;
	}

	if(pthread_attr_init(&attr) == 0)
	{
		/*if(pthread_attr_setstacksize(&attr, pstrAttrs->u32StackSize) != 0)
		{
			return NMI_FAIL;
		}*/
		
		if(pthread_create(pHandle, &attr, (void*(*)(void*))pfEntry, pvArg) == 0)
		{
			s32RetStatus = NMI_SUCCESS;
		}

		pthread_attr_destroy(&attr);
	}	

	return s32RetStatus;
}

NMI_ErrNo NMI_ThreadDestroy(NMI_ThreadHandle* pHandle,
	tstrNMI_ThreadAttrs* pstrAttrs)
{	
	if(pthread_join(*pHandle, NULL) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}



#endif

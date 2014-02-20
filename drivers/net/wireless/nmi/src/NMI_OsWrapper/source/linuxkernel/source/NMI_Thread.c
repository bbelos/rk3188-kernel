
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_THREAD_FEATURE



NMI_ErrNo NMI_ThreadCreate(NMI_ThreadHandle* pHandle, tpfNMI_ThreadFunction pfEntry,
	void* pvArg, tstrNMI_ThreadAttrs* pstrAttrs)
{


	*pHandle = kthread_run((int(*)(void *))pfEntry ,pvArg, "NMI_kthread");


	if(IS_ERR(*pHandle))
	{
		return NMI_FAIL;
	}
	else
	{
		return NMI_SUCCESS;
	}
	
}

NMI_ErrNo NMI_ThreadDestroy(NMI_ThreadHandle* pHandle,
	tstrNMI_ThreadAttrs* pstrAttrs)
{	
	NMI_ErrNo s32RetStatus = NMI_SUCCESS;

	kthread_stop(*pHandle);
	return s32RetStatus;
}



#endif

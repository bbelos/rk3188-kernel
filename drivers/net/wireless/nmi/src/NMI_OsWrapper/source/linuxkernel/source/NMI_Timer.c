
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_TIMER_FEATURE



NMI_ErrNo NMI_TimerCreate(NMI_TimerHandle* pHandle, 
	tpfNMI_TimerFunction pfCallback, tstrNMI_TimerAttrs* pstrAttrs)
{	
	NMI_ErrNo s32RetStatus = NMI_SUCCESS;
	setup_timer(pHandle,(void(*)(unsigned long))pfCallback,0);

	return s32RetStatus;
}

NMI_ErrNo NMI_TimerDestroy(NMI_TimerHandle* pHandle, 
	tstrNMI_TimerAttrs* pstrAttrs)
{
	NMI_ErrNo s32RetStatus = NMI_FAIL;
	if(pHandle != NULL){
		s32RetStatus = del_timer_sync(pHandle);
		pHandle = NULL;
	}

	return s32RetStatus;
}

	
NMI_ErrNo NMI_TimerStart(NMI_TimerHandle* pHandle, NMI_Uint32 u32Timeout, 
	void* pvArg, tstrNMI_TimerAttrs* pstrAttrs)
{
	NMI_ErrNo s32RetStatus = NMI_FAIL;
	if(pHandle != NULL){
		pHandle->data = (unsigned long)pvArg;
		s32RetStatus = mod_timer(pHandle, (jiffies + msecs_to_jiffies(u32Timeout)));
		}
	return s32RetStatus;	
}

NMI_ErrNo NMI_TimerStop(NMI_TimerHandle* pHandle, 
	tstrNMI_TimerAttrs* pstrAttrs)
{
	NMI_ErrNo s32RetStatus = NMI_FAIL;
	if(pHandle != NULL)
		s32RetStatus = del_timer(pHandle);

	return s32RetStatus;
}

#endif

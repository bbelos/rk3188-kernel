
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_TIMER_FEATURE

#include <signal.h>

static void TimerCallbackWrapper(union sigval val)
{
	NMI_TimerHandle* pHandle = (NMI_TimerHandle*)val.sival_ptr;

	if(NMI_SemaphoreAcquire(&(pHandle->hAccessProtection),
						NMI_NULL) == NMI_SUCCESS)
	{
		if(pHandle->bPendingTimer == NMI_TRUE)
		{
			((tpfNMI_TimerFunction)pHandle->pfCallbackFunction)
				(pHandle->pvArgument);
		}
		NMI_SemaphoreRelease(&(pHandle->hAccessProtection), NMI_NULL);
	}
}

NMI_ErrNo NMI_TimerCreate(NMI_TimerHandle* pHandle, 
	tpfNMI_TimerFunction pfCallback, tstrNMI_TimerAttrs* pstrAttrs)
{

	struct sigevent strSigEv;

	strSigEv.sigev_notify = SIGEV_THREAD;
	strSigEv.sigev_value.sival_ptr = pHandle;
	strSigEv.sigev_notify_function = TimerCallbackWrapper;
	strSigEv.sigev_notify_attributes = NULL;

	pHandle->pfCallbackFunction = pfCallback;
	pHandle->pvArgument = NMI_NULL;

	if((NMI_SemaphoreCreate(&(pHandle->hAccessProtection), NMI_NULL) == NMI_SUCCESS)
		&& (timer_create(CLOCK_REALTIME, &strSigEv, &(pHandle->timerObject)) == 0))
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

NMI_ErrNo NMI_TimerDestroy(NMI_TimerHandle* pHandle, 
	tstrNMI_TimerAttrs* pstrAttrs)
{
	NMI_ErrNo s32RetStatus = NMI_FAIL;

	s32RetStatus = NMI_TimerStop(pHandle, NMI_NULL);
	
	/* important : I use the AND operator '&' here instead of '&&' on purpose
	because I want both functions to be called anyway */
	if(	(timer_delete(pHandle->timerObject) == 0)
		& (NMI_SemaphoreDestroy(&(pHandle->hAccessProtection), NMI_NULL)
					== NMI_SUCCESS))
	{
		/* do nothing */
	}
	else
	{
		s32RetStatus =  NMI_FAIL;
	}

	return s32RetStatus;
}

static NMI_ErrNo SetupTimer(NMI_TimerHandle* pHandle, NMI_Uint32 u32Timeout, void* pvArg, NMI_Bool bPeriodic)
{
	struct itimerspec strTimeValue;
	NMI_ErrNo s32RetStatus = NMI_FAIL;

    strTimeValue.it_value.tv_sec = u32Timeout/1000L;
    strTimeValue.it_value.tv_nsec = (u32Timeout % 1000L) * 1000000L;

	if(bPeriodic == NMI_TRUE)
	{
		/* enable periodic timer */
		strTimeValue.it_interval.tv_sec = strTimeValue.it_value.tv_sec;
		strTimeValue.it_interval.tv_nsec = strTimeValue.it_value.tv_nsec;
	}
	else
	{
		/* disable periodic timer */
		strTimeValue.it_interval.tv_sec = 0;
		strTimeValue.it_interval.tv_nsec = 0;
	}

	s32RetStatus = NMI_SemaphoreAcquire(&(pHandle->hAccessProtection),
						NMI_NULL);
	NMI_ERRORCHECK(s32RetStatus);

	if(u32Timeout != 0)
	{
		pHandle->pvArgument = pvArg;
		pHandle->bPendingTimer = NMI_TRUE;
	}
	else
	{
		pHandle->bPendingTimer = NMI_FALSE;
	}
	if(timer_settime(pHandle->timerObject,
		0,
		&strTimeValue,
		NMI_NULL) != 0)
	{
		s32RetStatus = NMI_FAIL;
	}

	/* this part does not follow the usual error checking coventions, because :
	- I want to release the semaphore even if earlier function returned error
	- I don't want to overwrite s32RetStatus in case of an earlier failure
	*/
	if(NMI_SemaphoreRelease(&(pHandle->hAccessProtection), NMI_NULL)
		!= NMI_SUCCESS)
	{
		s32RetStatus = NMI_FAIL;
	}

	NMI_CATCH(s32RetStatus)
	{
	}

	return s32RetStatus;


}
	
NMI_ErrNo NMI_TimerStart(NMI_TimerHandle* pHandle, NMI_Uint32 u32Timeout, 
	void* pvArg, tstrNMI_TimerAttrs* pstrAttrs)
{
	NMI_Bool bPeriodic = NMI_FALSE;
	NMI_ErrNo s32RetStatus;

	#ifdef CONFIG_NMI_TIMER_PERIODIC
		if((pstrAttrs != NMI_NULL) 
			&& (pstrAttrs->bPeriodicTimer == NMI_TRUE))
		{
			/* enable periodic timer */
			bPeriodic = NMI_TRUE;
		}
	#endif

	if(u32Timeout == 0)
	{
		if(bPeriodic == NMI_TRUE)
		{
			/* a periodic timer with zero timeout does not make any sense,
			so we error on that */
			NMI_ERRORREPORT(s32RetStatus, NMI_INVALID_ARGUMENT);
		}
		else
		{
			/* in Linux timer API a timeout of zero means 'disable timer' 
			but in NMI API a timeout of zero means 'run it as soon as 
			possible' so here we workaround it by waiting just one msec */
			u32Timeout = 1;
		}
	}

	s32RetStatus = SetupTimer(pHandle, u32Timeout, pvArg, bPeriodic);

	NMI_CATCH(s32RetStatus)
	{
	}
	
	return s32RetStatus;	
}


NMI_ErrNo NMI_TimerStop(NMI_TimerHandle* pHandle, 
	tstrNMI_TimerAttrs* pstrAttrs)
{
	return SetupTimer(pHandle, 0, NMI_NULL, NMI_FALSE);
}

#endif


#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_SLEEP_FEATURE

/*
*  @author	mdaftedar
*  @date	10 Aug 2010
*  @version	1.0
*/
void NMI_Sleep(NMI_Uint32 u32TimeMilliSec)
{
	if(u32TimeMilliSec <= 4000000)
	{
		NMI_Uint32 u32Temp = u32TimeMilliSec * 1000;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		usleep_range(u32Temp, u32Temp);
#else
		udelay(u32Temp);
#endif
	}
	else
	{
    		msleep(u32TimeMilliSec);
	}
	
}
#endif

//#ifdef CONFIG_NMI_SLEEP_HI_RES
void NMI_SleepMicrosec(NMI_Uint32 u32TimeMicoSec)
{
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	usleep_range(u32TimeMicoSec,u32TimeMicoSec);
	#else
	udelay(u32TimeMicoSec);
	#endif
}
//#endif

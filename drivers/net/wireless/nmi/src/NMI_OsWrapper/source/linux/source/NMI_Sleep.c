
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_SLEEP_FEATURE

/*
*  @author	syounan
*  @date	10 Aug 2010
*  @version	1.0
*/
void NMI_Sleep(NMI_Uint32 u32TimeMilliSec)
{

    struct timespec tspec, trem;
    tspec.tv_sec = u32TimeMilliSec/1000L;
    u32TimeMilliSec %= 1000L;
    tspec.tv_nsec = u32TimeMilliSec * 1000000L;

    while(nanosleep(&tspec, &trem) != 0)
    {
        tspec.tv_sec = trem.tv_sec;
        tspec.tv_nsec = trem.tv_nsec;
    }
}


#endif
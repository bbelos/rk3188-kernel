
#define _CRT_SECURE_NO_DEPRECATE
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_TIME_FEATURE


NMI_Uint32 NMI_TimeMsec(void)
{
	NMI_Uint32 u32Time = 0;
	struct timespec current_time;

	current_time = current_kernel_time();
	u32Time = current_time.tv_sec * 1000;
	u32Time += current_time.tv_nsec / 1000000;
	
	
	return u32Time;
}


#ifdef CONFIG_NMI_EXTENDED_TIME_OPERATIONS

/**
*  @brief 		 
*  @details 	function returns the implementation's best approximation to the
				processor time used by the process since the beginning of an
				implementation-dependent time related only to the process invocation.
*  @return 		NMI_Uint32
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_Uint32 NMI_Clock()
{
	/*printk("Not implemented")*/
}


/**
*  @brief 		 
*  @details 	The difftime() function computes the difference between two calendar
				times (as returned by NMI_GetTime()): time1 - time0.
*  @param[in] 	NMI_Time time1
*  @param[in] 	NMI_Time time0
*  @return 		NMI_Double
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_DiffTime(NMI_Time time1, NMI_Time time0)
{
	/*printk("Not implemented")*/
}



/**
*  @brief 		 
*  @details 	The gmtime() function converts the time in seconds since
				the Epoch pointed to by timer into a broken-down time,
				expressed as Coordinated Universal Time (UTC).
*  @param[in] 	const NMI_Time* timer
*  @return 		NMI_tm*
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_tm* NMI_GmTime(const NMI_Time* timer)
{
	/*printk("Not implemented")*/
}


/**
*  @brief 		 
*  @details 	The localtime() function converts the time in seconds since
				the Epoch pointed to by timer into a broken-down time, expressed
				as a local time. The function corrects for the timezone and any 
				seasonal time adjustments. Local timezone information is used as 
				though localtime() calls tzset().
*  @param[in] 	const NMI_Time* timer
*  @return 		NMI_tm*
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_tm* NMI_LocalTime(const NMI_Time* timer)
{
	/*printk("Not implemented")*/
}


/**
*  @brief 		 
*  @details 	The mktime() function converts the broken-down time,
				expressed as local time, in the structure pointed to by timeptr, 
				into a time since the Epoch value with the same encoding as that 
				of the values returned by time(). The original values of the tm_wday
				and tm_yday components of the structure are ignored, and the original 
				values of the other components are not restricted to the ranges described 
				in the <time.h> entry.
*  @param[in] 	NMI_tm* timer
*  @return 		NMI_Time
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_Time NMI_MkTime(NMI_tm* timer)
{
	/*printk("Not implemented")*/
}


/**
*  @brief 		 
*  @details 	The strftime() function places bytes into the array 
				pointed to by s as controlled by the string pointed to by format.
*  @param[in] 	NMI_Char* s
*  @param[in]	NMI_Uint32 maxSize
*  @param[in]	const NMI_Char* format
*  @param[in]	const NMI_tm* timptr
*  @return 		NMI_Uint32
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_Uint32 NMI_StringFormatTime(NMI_Char* s,
								NMI_Uint32 maxSize,
								const NMI_Char* format,
								const NMI_tm* timptr)
{
	/*printk("Not implemented")*/
}


/**
*  @brief 		The NMI_GetTime() function returns the value of time in seconds since the Epoch.
*  @details 	The tloc argument points to an area where the return value is also stored.
				If tloc is a null pointer, no value is stored.
*  @param[in] 	NMI_Time* tloc
*  @return 		NMI_Time
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_Time NMI_GetTime(NMI_Time* tloc)
{
	/*printk("Not implemented")*/
}


#endif
#endif



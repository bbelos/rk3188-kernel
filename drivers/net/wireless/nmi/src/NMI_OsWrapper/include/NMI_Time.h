#ifndef __NMI_TIME_H__
#define __NMI_TIME_H__

/*!  
*  @file		NMI_Time.h
*  @brief		Time retrival functionality
*  @author		syounan
*  @sa			NMI_OSWrapper.h top level OS wrapper file
*  @date		2 Sep 2010
*  @version		1.0
*/

#ifndef CONFIG_NMI_TIME_FEATURE
#error the feature CONFIG_NMI_TIME_FEATURE must be supported to include this file
#endif

/*!
*  @struct 		NMI_ThreadAttrs
*  @brief		Thread API options 
*  @author		syounan
*  @date		2 Sep 2010
*  @version		1.0
*/
typedef struct
{
	/* a dummy type to prevent compile errors on empty structure*/
	NMI_Uint8 dummy;
}tstrNMI_TimeAttrs;

typedef struct
{
	/*!< current year */
	NMI_Uint16	u16Year;
	/*!< current month */
	NMI_Uint8	u8Month;
	/*!< current day */
	NMI_Uint8	u8Day;
	
	/*!< current hour (in 24H format) */
	NMI_Uint8	u8Hour;
	/*!< current minute */
	NMI_Uint8	u8Miute;
	/*!< current second */
	NMI_Uint8	u8Second;
	
}tstrNMI_TimeCalender;

/*!
*  @brief		returns the number of msec elapsed since system start up
*  @return		number of msec elapsed singe system start up
*  @note		since this returned value is 32 bit, the caller must handle 
				wraparounds in values every about 49 of continous operations
*  @author		syounan
*  @date		2 Sep 2010
*  @version		1.0
*/
NMI_Uint32 NMI_TimeMsec(void);



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
NMI_Uint32 NMI_Clock();

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
NMI_Double NMI_DiffTime(NMI_Time time1, NMI_Time time0);

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
NMI_tm* NMI_GmTime(const NMI_Time* timer);


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
NMI_tm* NMI_LocalTime(const NMI_Time* timer);


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
NMI_Time NMI_MkTime(NMI_tm* timer);


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
								const NMI_tm* timptr);


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
NMI_Time NMI_GetTime(NMI_Time* tloc);

#endif

#ifdef CONFIG_NMI_TIME_UTC_SINCE_1970

/*!
*  @brief		returns the number of seconds elapsed since 1970 (in UTC)
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return		number of seconds elapsed since 1970 (in UTC)
*  @sa			tstrNMI_TimeAttrs
*  @author		syounan
*  @date		2 Sep 2010
*  @version		1.0
*/
NMI_Uint32 NMI_TimeUtcSince1970(tstrNMI_TimeAttrs* pstrAttrs);

#endif

#ifdef CONFIG_NMI_TIME_CALENDER

/*!
*  @brief		gets the current calender time
*  @return		number of seconds elapsed since 1970 (in UTC)
*  @param[out]	ptstrCalender calender structure to be filled with time
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @sa			NMI_ThreadAttrs
*  @author		syounan
*  @date		2 Sep 2010
*  @version		1.0
*/
NMI_ErrNo NMI_TimeCalender(tstrNMI_TimeCalender* ptstrCalender, 
	tstrNMI_TimeAttrs* pstrAttrs);

#endif

#endif


#define _CRT_SECURE_NO_DEPRECATE

#include  "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_STRING_UTILS


/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
NMI_Sint32 NMI_memcmp(const void* pvArg1, const void* pvArg2, NMI_Uint32 u32Count)
{
	return memcmp(pvArg1, pvArg2, u32Count);
}


/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
void NMI_memcpy_INTERNAL(void* pvTarget, const void* pvSource, NMI_Uint32 u32Count)
{
	memcpy(pvTarget, pvSource, u32Count);
}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
void* NMI_memset(void* pvTarget, NMI_Uint8 u8SetValue, NMI_Uint32 u32Count)
{
	return memset(pvTarget, u8SetValue, u32Count);
}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
NMI_Char* NMI_strncat(NMI_Char* pcTarget, const NMI_Char* pcSource,
	NMI_Uint32 u32Count)
{
	return strncat(pcTarget, pcSource, u32Count);
}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
NMI_Char* NMI_strncpy(NMI_Char* pcTarget, const NMI_Char* pcSource,
	NMI_Uint32 u32Count)
{
	return strncpy(pcTarget, pcSource, u32Count);
}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
NMI_Sint32 NMI_strcmp(const NMI_Char* pcStr1, const NMI_Char* pcStr2)
{
	NMI_Sint32 s32Result;

	if(pcStr1 == NMI_NULL && pcStr2 == NMI_NULL)
	{
		s32Result = 0;
	}
	else if(pcStr1 == NMI_NULL)
	{
		s32Result = -1;
	}
	else if(pcStr2 == NMI_NULL)
	{
		s32Result = 1;
	}
	else
	{
		s32Result = strcmp(pcStr1, pcStr2);
		if(s32Result < 0)
		{
			s32Result = -1;
		}
		else if(s32Result > 0)
		{
			s32Result = 1;
		}
	}

	return s32Result;
}

NMI_Sint32 NMI_strncmp(const NMI_Char* pcStr1, const NMI_Char* pcStr2,
	NMI_Uint32 u32Count)
{
	NMI_Sint32 s32Result;

	if(pcStr1 == NMI_NULL && pcStr2 == NMI_NULL)
	{
		s32Result = 0;
	}
	else if(pcStr1 == NMI_NULL)
	{
		s32Result = -1;
	}
	else if(pcStr2 == NMI_NULL)
	{
		s32Result = 1;
	}
	else
	{
		s32Result = strncmp(pcStr1, pcStr2, u32Count);
		if(s32Result < 0)
		{
			s32Result = -1;
		}
		else if(s32Result > 0)
		{
			s32Result = 1;
		}
	}

	return s32Result;
}

/*
*  @author	syounan
*  @date	1 Nov 2010
*  @version	2.0
*/
NMI_Sint32 NMI_strcmp_IgnoreCase(const NMI_Char* pcStr1, const NMI_Char* pcStr2)
{
	NMI_Sint32 s32Result;

	if(pcStr1 == NMI_NULL && pcStr2 == NMI_NULL)
	{
		s32Result = 0;
	}
	else if(pcStr1 == NMI_NULL)
	{
		s32Result = -1;
	}
	else if(pcStr2 == NMI_NULL)
	{
		s32Result = 1;
	}
	else
	{
		NMI_Char cTestedChar1, cTestedChar2;
		do
		{
			cTestedChar1 = *pcStr1;
			if((*pcStr1 >= 'a') && (*pcStr1 <= 'z'))
			{
				/* turn a lower case character to an upper case one */
				cTestedChar1 -= 32;
			}

			cTestedChar2 = *pcStr2;
			if((*pcStr2 >= 'a') && (*pcStr2 <= 'z'))
			{
				/* turn a lower case character to an upper case one */
				cTestedChar2 -= 32;
			}

			pcStr1++;
			pcStr2++;

		}while((cTestedChar1 == cTestedChar2) 
			&& (cTestedChar1 != 0) 
			&& (cTestedChar2 != 0));

		if(cTestedChar1 > cTestedChar2)
		{
			s32Result = 1;
		}
		else if(cTestedChar1 < cTestedChar2)
		{
			s32Result = -1;
		}
		else
		{
			s32Result = 0;
		}
	}

	return s32Result;
}

/*!
*  @author	aabozaeid
*  @date	8 Dec 2010
*  @version	1.0
*/
NMI_Sint32 NMI_strncmp_IgnoreCase(const NMI_Char* pcStr1, const NMI_Char* pcStr2,
	NMI_Uint32 u32Count)
{
	NMI_Sint32 s32Result;

	if(pcStr1 == NMI_NULL && pcStr2 == NMI_NULL)
	{
		s32Result = 0;
	}
	else if(pcStr1 == NMI_NULL)
	{
		s32Result = -1;
	}
	else if(pcStr2 == NMI_NULL)
	{
		s32Result = 1;
	}
	else
	{
		NMI_Char cTestedChar1, cTestedChar2;
		do
		{
			cTestedChar1 = *pcStr1;
			if((*pcStr1 >= 'a') && (*pcStr1 <= 'z'))
			{
				/* turn a lower case character to an upper case one */
				cTestedChar1 -= 32;
			}

			cTestedChar2 = *pcStr2;
			if((*pcStr2 >= 'a') && (*pcStr2 <= 'z'))
			{
				/* turn a lower case character to an upper case one */
				cTestedChar2 -= 32;
			}

			pcStr1++;
			pcStr2++;
			u32Count--;

		}while( (u32Count > 0)
			&& (cTestedChar1 == cTestedChar2) 
			&& (cTestedChar1 != 0) 
			&& (cTestedChar2 != 0));

		if(cTestedChar1 > cTestedChar2)
		{
			s32Result = 1;
		}
		else if(cTestedChar1 < cTestedChar2)
		{
			s32Result = -1;
		}
		else
		{
			s32Result = 0;
		}
	}

	return s32Result;

}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
NMI_Uint32 NMI_strlen(const NMI_Char* pcStr)
{
	return (NMI_Uint32)strlen(pcStr);
}

/*!
*  @author	bfahmy
*  @date	28 Aug 2010
*  @version	1.0
*/
NMI_Sint32 NMI_strtoint(const NMI_Char* pcStr)
{
	return (NMI_Sint32)(simple_strtol(pcStr,NULL,10));
}

/*
*  @author	syounan
*  @date	1 Nov 2010
*  @version	2.0
*/
NMI_ErrNo NMI_snprintf(NMI_Char* pcTarget, NMI_Uint32 u32Size, 
	const NMI_Char* pcFormat, ...)
{
	va_list argptr;
	va_start(argptr, pcFormat);
	if(vsnprintf(pcTarget, u32Size, pcFormat, argptr) < 0)
	{
		/* if turncation happens windows does not properly terminate strings */
		pcTarget[u32Size - 1] = 0;
	}
	va_end(argptr);	

	/* I find no sane way of detecting errors in windows, so let it all succeed ! */
	return NMI_SUCCESS;
}

#ifdef CONFIG_NMI_EXTENDED_STRING_OPERATIONS

/**
*  @brief 		
*  @details 	Searches for the first occurrence of the character c in the first n bytes
				of the string pointed to by the argument str.
				Returns a pointer pointing to the first matching character,
				or null if no match was found.
*  @param[in] 	
*  @return 		 
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Char* NMI_memchr(const void *str, NMI_Char c, NMI_Sint32 n)
{
	return (NMI_Char*) memchr(str,c,(size_t)n);
}

/**
*  @brief 		
*  @details 	Searches for the first occurrence of the character c (an unsigned char)
				in the string pointed to by the argument str. 
				The terminating null character is considered to be part of the string.
				Returns a pointer pointing to the first matching character, 
				or null if no match was found.
*  @param[in] 	
*  @return 		 
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Char* NMI_strchr(const NMI_Char *str, NMI_Char c)
{
	return strchr(str,c);
}

/**
*  @brief 		
*  @details 	Appends the string pointed to by str2 to the end of the string pointed to by str1.
				The terminating null character of str1 is overwritten. 
				Copying stops once the terminating null character of str2 is copied. If overlapping occurs, the result is undefined.
				The argument str1 is returned.
*  @param[in] 	NMI_Char* str1,
*  @param[in] 	NMI_Char* str2,
*  @return 		NMI_Char* 
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Char* NMI_strcat(NMI_Char *str1, const NMI_Char *str2)
{
	return strcat(str1, str2);
}

/**
*  @brief 		
*  @details 	Copy pcSource to pcTarget
*  @param[in] 	NMI_Char* pcTarget
*  @param[in] 	const NMI_Char* pcSource
*  @return 		NMI_Char* 
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Char* NMI_strcpy(NMI_Char* pcTarget, const NMI_Char* pcSource)
{
	return strncpy(pcTarget, pcSource, strlen(pcSource));
}

/**
*  @brief 		
*  @details 	Finds the first sequence of characters in the string str1 that
				does not contain any character specified in str2.
				Returns the length of this first sequence of characters found that 
				do not match with str2.
*  @param[in] 	const NMI_Char *str1
*  @param[in] 	const NMI_Char *str2
*  @return 		NMI_Uint32
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Uint32 NMI_strcspn(const NMI_Char *str1, const NMI_Char *str2)
{
	return (NMI_Uint32)strcspn(str1, str2);
}
#if 0
/**
*  @brief 		
*  @details 	Searches an internal array for the error number errnum and returns a pointer
				to an error message string.
				Returns a pointer to an error message string.
*  @param[in] 	NMI_Sint32 errnum
*  @return 		NMI_Char*
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Char* NMI_strerror(NMI_Sint32 errnum)
{
	return strerror(errnum);
}
#endif

/**
*  @brief 		
*  @details 	Finds the first occurrence of the entire string str2 
				(not including the terminating null character) which appears in the string str1.
				Returns a pointer to the first occurrence of str2 in str1. 
				If no match was found, then a null pointer is returned. 
				If str2 points to a string of zero length, then the argument str1 is returned.
*  @param[in] 	const NMI_Char *str1
*  @param[in] 	const NMI_Char *str2
*  @return 		NMI_Char*
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Char* NMI_strstr(const NMI_Char *str1, const NMI_Char *str2)
{
	return strstr(str1, str2);
}
#if 0
/**
*  @brief 		
*  @details 	Parses the C string str interpreting its content as a floating point 
				number and returns its value as a double. 
				If endptr is not a null pointer, the function also sets the value pointed
				by endptr to point to the first character after the number.
*  @param[in] 	const NMI_Char* str
*  @param[in] 	NMI_Char** endptr
*  @return 		NMI_Double
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_StringToDouble(const NMI_Char* str, NMI_Char** endptr)
{
	return strtod (str,endptr);
}
#endif

/**
*  @brief 		Parses the C string str interpreting its content as an unsigned integral
				number of the specified base, which is returned as an unsigned long int value.
*  @details 	The function first discards as many whitespace characters as necessary
				until the first non-whitespace character is found. 
				Then, starting from this character, takes as many characters as possible
				that are valid following a syntax that depends on the base parameter,
				and interprets them as a numerical value. 
				Finally, a pointer to the first character following the integer
				representation in str is stored in the object pointed by endptr.
*  @param[in] 	const NMI_Char *str
*  @param[in]	NMI_Char **endptr
*  @param[in]	NMI_Sint32 base
*  @return 		NMI_Uint32
*  @note 		
*  @author		remil
*  @date		11 Nov 2010
*  @version		1.0
*/
NMI_Uint32 NMI_StringToUint32(const NMI_Char *str, NMI_Char **endptr, NMI_Sint32 base)
{
	return simple_strtoul(str, endptr, base);
}

#endif

#endif

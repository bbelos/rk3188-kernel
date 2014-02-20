
#define _CRT_SECURE_NO_DEPRECATE

#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#include <sys/types.h>
#include <sys/stat.h> 

#ifdef CONFIG_NMI_FILE_OPERATIONS_FEATURE

/**
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
NMI_ErrNo NMI_fopen(NMI_FileHandle* pHandle, const NMI_Char* pcPath, 
	tstrNMI_FileOpsAttrs* pstrAttrs)
{
	tstrNMI_FileOpsAttrs strFileDefaultAttrs;
	char* mode;

	if(pstrAttrs == NMI_NULL)
	{
		NMI_FileOpsFillDefault(&strFileDefaultAttrs);
		pstrAttrs = &strFileDefaultAttrs;
	}

	switch(pstrAttrs->enuAccess)
	{
	case NMI_FILE_READ_ONLY:
		mode = "rb";
		break;

	case NMI_FILE_READ_WRITE_NEW:
		mode = "wb+";
		break;

	default:
	case NMI_FILE_READ_WRITE_EXISTING:
		mode = "rb+";
		break;
	}

	*pHandle = fopen(pcPath, mode);

	if(*pHandle != NULL)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}

}

/**
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0 
*/
NMI_ErrNo NMI_fclose(NMI_FileHandle* pHandle, tstrNMI_FileOpsAttrs* pstrAttrs)
{
	if(fclose(*pHandle) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

/**
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0 
 */
NMI_Uint32 NMI_fread(NMI_FileHandle* pHandle, NMI_Uint8* pu8Buffer, 
				NMI_Uint32 u32Size, tstrNMI_FileOpsAttrs* pstrAttrs)
{
	return (NMI_Uint32)fread(pu8Buffer, 1, u32Size, *pHandle);
}

/**
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0  
 */
NMI_Uint32 NMI_fwrite(NMI_FileHandle* pHandle, NMI_Uint8* pu8Buffer, 
				NMI_Uint32 u32Size, tstrNMI_FileOpsAttrs* pstrAttrs)
{
	return (NMI_Uint32)fwrite(pu8Buffer, 1, u32Size, *pHandle);
}

/**
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0
 */
NMI_Bool NMI_feof(NMI_FileHandle* pHandle)
{
	if(feof(*pHandle) != 0)
	{
		return NMI_TRUE;
	}
	else
	{
		return NMI_FALSE;
	}
}

/**
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0
 */
NMI_ErrNo NMI_fseek(NMI_FileHandle* pHandle, NMI_Sint32 s32Offset, 
		tstrNMI_FileOpsAttrs* pstrAttrs)
{
	tstrNMI_FileOpsAttrs strFileDefaultAttrs;
	int mode;

	if(pstrAttrs == NMI_NULL)
	{
		NMI_FileOpsFillDefault(&strFileDefaultAttrs);
		pstrAttrs = &strFileDefaultAttrs;
	}

	switch(pstrAttrs->enuSeekOrigin)
	{
	case NMI_SEEK_FROM_START:
		mode = SEEK_SET; 
		break;
		
	case NMI_SEEK_FROM_END:
		mode = SEEK_END; 
		break;

	default:
	case NMI_SEEK_FROM_CURRENT:
		mode = SEEK_CUR;
	}

	if(fseek(* pHandle, s32Offset, mode) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

/**
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0
 */
NMI_Uint32 NMI_ftell(NMI_FileHandle* pHandle, tstrNMI_FileOpsAttrs* pstrAttrs)
{
	return ftell(*pHandle);
}


/**
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0
 */
NMI_ErrNo NMI_fflush(NMI_FileHandle* pHandle, tstrNMI_FileOpsAttrs* pstrAttrs)
{
	if(fflush(*pHandle) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

/**
 * @author		mabubakr
 * @date		10 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_fdelete(const NMI_Char* pcPath, tstrNMI_FileOpsAttrs* pstrAttrs)
{
	if(remove(pcPath) == 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

/**
 * @author		remil, syounan
 * @date		31 Oct 2010
 * @version		2.0
 */
NMI_ErrNo NMI_FileSize(NMI_FileHandle* pstrFileHandle, NMI_Uint32* pu32Size,
	tstrNMI_FileOpsAttrs* pstrAttrs)
{
	struct stat buffer;
	if(fstat( (*pstrFileHandle)->_fileno, &buffer) != 0)
	{
		return NMI_FAIL;
	}
	else
	{
		*pu32Size = (NMI_Uint32)buffer.st_size;
		return NMI_SUCCESS;
	}
}

#ifdef CONFIG_NMI_EXTENDED_FILE_OPERATIONS

/**
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_Sint32 NMI_getc(NMI_FileHandle* pHandle)
{
	return getc(*pHandle);
}

/**
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_Sint32 NMI_ungetc(NMI_Sint32 c,NMI_FileHandle* pHandle)
{
	return ungetc(c,*pHandle);
}


/**
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_freopen(NMI_FileHandle* pHandle, const NMI_Char* pcPath, 
	tstrNMI_FileOpsAttrs* pstrAttrs)
{

	tstrNMI_FileOpsAttrs strFileDefaultAttrs;
	char* mode;

	if(pstrAttrs == NMI_NULL)
	{
		NMI_FileOpsFillDefault(&strFileDefaultAttrs);
		pstrAttrs = &strFileDefaultAttrs;
	}

	switch(pstrAttrs->enuAccess)
	{
	case NMI_FILE_READ_ONLY:
		mode = "rb";
		break;

	case NMI_FILE_READ_WRITE_NEW:
		mode = "wb";
		break;

	default:
	case NMI_FILE_READ_WRITE_EXISTING:
		mode = "rb+";
		break;
	}

	*pHandle = freopen(pcPath, mode,*pHandle);

	if(*pHandle != NULL)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}


/**
*  @brief 		Translates character file open mode to the appropriate Enum Value
*  @details 	
*  @param[in] 	const NMI_Char* cpMode
*  @return 		tenuNMI_AccessMode
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
tenuNMI_AccessMode NMI_resolveFileMode(const NMI_Char* cpMode)
{
	tenuNMI_AccessMode enumAccessMode;
	NMI_Char* mode = (NMI_Char*) cpMode;

	while(*mode!= 0)
	{
		switch(*mode)
		{
		case 'r':
			enumAccessMode = NMI_FILE_READ_ONLY;
			break;

		case 'w':
			enumAccessMode = NMI_FILE_READ_WRITE_NEW;
			break;

		case 'a':
			enumAccessMode = NMI_FILE_READ_WRITE_EXISTING;
			break;

		case '+':
			enumAccessMode= NMI_FILE_READ_WRITE_EXISTING;
		}
		mode++;
	}

	return enumAccessMode;
}



/**
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_rename(const NMI_Char *old_filename, const NMI_Char *new_filename)
{
	 if (rename(old_filename,new_filename)== 0)
	 {
		 return NMI_SUCCESS;
	 }
	 else
	 {
		 return NMI_FAIL;
	 }
}


NMI_ErrNo NMI_setbuf(NMI_FileHandle *pHandle, NMI_Char* buffer)
{
	if(pHandle == NMI_NULL || buffer == NMI_NULL)
	{
		return NMI_FAIL;
	}else
	{
		setbuf (*pHandle,buffer);
		return NMI_SUCCESS;
	}
}


NMI_ErrNo NMI_setvbuf(NMI_FileHandle *pHandle, 
					  NMI_Char* buffer, 
					  NMI_Sint32 mode, 
					  NMI_Uint32 size)
{
	if(pHandle != NMI_NULL && buffer != NMI_NULL)
	{
		if(setvbuf(*pHandle,buffer,mode,(size_t)size)==0)
		{
			return NMI_SUCCESS;
		}
	}

	return NMI_FAIL;

}

/*
*Creates a temporary file in binary update mode (wb+). 
*The tempfile is removed when the program terminates or the stream is closed.
*On success a pointer to a file stream is returned. On error a null pointer is returned.
*/
NMI_ErrNo NMI_tmpfile(NMI_FileHandle *pHandle)
{
	*pHandle = tmpfile();
	if(*pHandle == NMI_NULL)
	{
		return NMI_FAIL;
	}
	else 
	{
		return NMI_SUCCESS;
	}
}



/*
 * Reads a line from the specified stream and stores it into the string pointed to by str.
 * It stops when either (n-1) characters are read,
 * the newline character is read, or the end-of-file is reached,
 * whichever comes first. The newline character is copied to the string.
 * A null character is appended to the end of the string.
 * On success a pointer to the string is returned. On error a null pointer is returned.
 * If the end-of-file occurs before any characters have been read, the string remains unchanged.
*/

//NMI_Char* NMI_fgets(NMI_Char* str, NMI_Sint32 n, NMI_FileHandle stream)
//{
//	return fgets(str,n,stream);
//}


/**
*  @brief 		Clears the end-of-file and error indicators for the given stream.  
*  @details 	As long as the error indicator is set, 
				all stream operations will return an error until NMI_clearerr is called.
*  @param[in] 	NMI_FileHandle* pHandle
*  @return 		N/A
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
void NMI_clearerr(NMI_FileHandle* pHandle)
{
	clearerr(*pHandle);
}


/**
*  @brief 		Tests the error indicator for the given stream. 
*  @details 	If the error indicator is set, then it returns NMI_FAIL.
				If the error indicator is not set, then it returns NMI_SUCCESS.
*  @param[in] 	NMI_FileHandle* pHandle
*  @return 		NMI_ErrNo
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_ErrNo NMI_ferror(NMI_FileHandle* pHandle)
{
	if(ferror(*pHandle)==0)
	{
		return NMI_SUCCESS;
	}else
	{
		return NMI_FAIL;
	}
}

/**
*  @brief 		Generates and returns a valid temporary filename which does not exist.
*  @details 	If the argument str is a null pointer, then the function returns 
				a pointer to a valid filename. 
				If the argument str is a valid pointer to an array,
				then the filename is written to the array and a pointer to the same array 
				is returned.The filename may be up to L_tmpnam characters long.
*  @param[in] 	NMI_Char* str
*  @return 		NMI_Char* 
*  @note 		
*  @author		remil
*  @date		3 Nov 2010
*  @version		1.0
*/
NMI_Char* NMI_tmpnam(NMI_Char* str)
{
 return tmpnam(str);
}

#endif


#ifdef CONFIG_NMI_FILE_OPERATIONS_STRING_API

/**
 * @author		syounan
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_fgets(NMI_FileHandle* pHandle, NMI_Char* pcBuffer
				, NMI_Uint32 u32Maxsize)
{
	if( fgets(pcBuffer, u32Maxsize, * pHandle) != NMI_NULL)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}

}

/**
 * @author		syounan
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_fprintf(NMI_FileHandle* pHandle, const NMI_Char* pcFormat, ...)
{
	va_list argptr;
	va_start(argptr, pcFormat);
	if(vfprintf(*pHandle, pcFormat, argptr) > 0)
	{
		va_end(argptr);
		return NMI_SUCCESS;
	}
	else
	{
		va_end(argptr);
		return NMI_FAIL;
	}
}

/**
 * @author		syounan
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_vfprintf(NMI_FileHandle* pHandle, const NMI_Char* pcFormat, 
				va_list argptr)
{
	if(vfprintf(*pHandle, pcFormat, argptr) > 0)
	{
		return NMI_SUCCESS;
	}
	else
	{
		return NMI_FAIL;
	}
}

#endif

#endif

#ifndef __NMI_FILEOPS_H__
#define __NMI_FILEOPS_H__

/*!  
*  @file	FileOps.h
*  @brief	File Operations OS wrapper functionality
*  @author	syounan
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	31 Aug 2010
*  @version	1.0
*/

#ifndef CONFIG_NMI_FILE_OPERATIONS_FEATURE
#error the feature CONFIG_NMI_FILE_OPS_FEATURE must be supported to include this file
#endif


typedef enum{
	NMI_FILE_READ_ONLY,
	NMI_FILE_READ_WRITE_EXISTING,
	NMI_FILE_READ_WRITE_NEW
}tenuNMI_AccessMode;

typedef enum{
	NMI_SEEK_FROM_START,
	NMI_SEEK_FROM_END,
	NMI_SEEK_FROM_CURRENT
}tenuNMI_SeekOrigin;

/*!
*  @struct 		tstrNMI_FileOpsAttrs
*  @brief		Message Queue API options
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
typedef struct
{
	/*!< access mode of the file, default is READ_WRITE_EXISTING */
	tenuNMI_AccessMode enuAccess;
	
	/*!< origin for seeking operations, default is NMI_FILE_SEEK_CUR */
	tenuNMI_SeekOrigin enuSeekOrigin;
	
}tstrNMI_FileOpsAttrs;

/*!
*  @brief		Fills the MsgQueueAttrs with default parameters
*  @param[out]	pstrAttrs structure to be filled
*  @sa			NMI_TimerAttrs
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
static void NMI_FileOpsFillDefault(tstrNMI_FileOpsAttrs* pstrAttrs)
{
	pstrAttrs->enuAccess = NMI_FILE_READ_WRITE_EXISTING;
	pstrAttrs->enuSeekOrigin = NMI_SEEK_FROM_CURRENT;
}


/**
*  @brief 		Opens a file
*  @details 	Opens a file, possibly creating a new file if write enabled and
 				pstrAttrs->bCreate is set to true
*  @param[in,out] pHandle Handle to the opened file
*  @param[in] 	pcPath path to the File
*  @param[in] 	pstrAttrs Optional attributes, NULL for defaults. bCreate will 
				create the file if does not exit, enuAccess will control access 
				mode
*  @return 		Error code indicating success/failure
*  @note 		Original Standard Function: 
				FILE *fopen( const char *filename, const char *mode );
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
NMI_ErrNo NMI_fopen(NMI_FileHandle* pHandle, const NMI_Char* pcPath, 
	tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		Closes a file.
 * @param[in] 	pHandle Handle to the file object
 * @param[in] 	pstrAttrs Optional attributes, NULL for default.
 * @return 		Error code indicating success/failure
 * @note 		Original Standard Function: int fclose( FILE *stream );
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0 
 */
NMI_ErrNo NMI_fclose(NMI_FileHandle* pHandle, tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		Reads data from a file.
 * @param[in] 	pHandle Handle to the file object
 * @param[out] 	pu8Buffer Storage location for data
 * @param[in] 	u32Size Number of bytes to be read
 * @param[in] 	pstrAttrs Optional attributes, NULL for default. 
 * @return 		Returns the number of bytes actually read, which may be less 
 				than count if an error occurs or if the end of the file is 
				encountered before reaching count.
				Use the NMI_feof function to distinguish a read error from an 
				end-of-file condition.
				If size or count is 0, NMI_fread returns 0 and the buffer 
				contents are unchanged.
 * @note 		Original Standard Function: 
 	size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0 
 */
NMI_Uint32 NMI_fread(NMI_FileHandle* pHandle, NMI_Uint8* pu8Buffer, 
				NMI_Uint32 u32Size, tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		Writes data to a FILE.
 * @param[in] 	pHandle Handle to the file object
 * @param[in] 	pu8Buffer Storage location for data
 * @param[in] 	u32Size Number of bytes to be write
 * @param[in] 	pstrAttrs Optional attributes, NULL for default. 
 * @return 		Returns the number of full items actually written, which may be
				less than count if an error occurs
				Also, if an error occurs, the file-position indicator cannot be 
				determined.
 * @note 		Original Standard Function: 
 	size_t fwrite( const void *buffer, size_t size, size_t count, FILE *stream );
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0  
 */
NMI_Uint32 NMI_fwrite(NMI_FileHandle* pHandle, NMI_Uint8* pu8Buffer, 
				NMI_Uint32 u32Size, tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		Tests for end-of-file
 * @param[in] 	pHandle Handle to the file object
 * @retval		NMI_FALSE if the current position is not end of file
 * @retval		NMI_TRUE after the first read operation that attempts to read past
				the end of the file.
 * @note 		Original Standard Function: int feof( FILE *stream );
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0
 */
NMI_Bool NMI_feof(NMI_FileHandle* pHandle);

/**
 * @brief 		Moves the file pointer to a specified location.
 * @param[in] 	pHandle Handle to the file object
 * @param[in] 	s32Offset Number of bytes to move, origin of seeking depends on
				pstrAttrs->enuSeekOrigin, if pstrAttrs is NULL then origin is the 
				current position at the file
 * @param[in] 	pstrAttrs Optional attributes, NULL for default. 
 				enuSeekOrigin defines the origin of seek operations 
 * @return 		Error code indicating success/failure
 * @note 		Original Standard Function:
				int fseek( FILE *stream, long offset, int origin );
 */
NMI_ErrNo NMI_fseek(NMI_FileHandle* pHandle, NMI_Sint32 s32Offset, 
		tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		Gets the current position of a file pointer.
 * @param[in] 	pHandle Handle to the file object
 * @param[in] 	pstrAttrs Optional attributes, NULL for default.
 * @return 		Returns the current file position. Or NMI_FAIL on error.
 * @note 		Original Standard Function: long ftell( FILE *stream );
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0 
 */
NMI_Uint32 NMI_ftell(NMI_FileHandle* pHandle, tstrNMI_FileOpsAttrs* pstrAttrs);


/**
 * @brief 		Flushes the file, writing all data that may be in buffers
 * @param[in] 	pHandle Handle to the file object
 * @param[in] 	pstrAttrs Optional attributes, NULL for default. 
 * @return 		Error code indicating success/failure
 * @note 		Original Standard Function: int fflush( FILE *stream );
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0 
 */
NMI_ErrNo NMI_fflush(NMI_FileHandle* pHandle, tstrNMI_FileOpsAttrs* pstrAttrs);



/**
 * @brief 		Gets the size of the file.
 * @param[in] 	pHandle Handle to the file object
 * @param[out] 	pu32Size returned size of file in question
 * @param[in] 	pstrAttrs Optional attributes, NULL for default.
 * @return 		Error code indicating success/failure
 * @note 		
 * @author		remil
 * @date		25 Oct 2010
 * @version		2.0 
 */
NMI_ErrNo NMI_FileSize(NMI_FileHandle* pstrFileHandle, NMI_Uint32* pu32Size,
	tstrNMI_FileOpsAttrs* pstrAttrs);



#ifdef CONFIG_NMI_EXTENDED_FILE_OPERATIONS
/**
 * @brief 		Returns the character currently pointed by the internal file position
 *				indicator of the specified stream. 
 *				The internal file position indicator is then advanced by one 
 *				character to point to the next character.
 * @param[in] 	NMI_FileHandle* pHandle
 * @return 		Returns the character currently pointed by the internal file position
 * @note 		Original Standard Function: int getc
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0 
 */
NMI_Sint32 NMI_getc(NMI_FileHandle* pHandle);


/**
 * @brief 		pushes the byte specified by c (converted to an unsigned char)
 *				back onto the input stream pointed to by stream.
 * @param[in]	NMI_Sint32 c
 * @param[in] 	NMI_FileHandle* pHandle
 * @return 		 returns the byte pushed back after conversion. Otherwise it returns EOF.
 * @note 		Original Standard Function:  ungetc
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0 
 */
NMI_Sint32 NMI_ungetc(NMI_Sint32 c,
					  NMI_FileHandle* pHandle);



/**
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_freopen(NMI_FileHandle* pHandle, const NMI_Char* pcPath, 
	tstrNMI_FileOpsAttrs* pstrAttrs);

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
tenuNMI_AccessMode NMI_resolveFileMode(const NMI_Char* cpMode);


/**
 * @author		remil
 * @date		31 Oct 2010
 * @version		1.0
 */
NMI_ErrNo NMI_rename(const NMI_Char *old_filename, const NMI_Char *new_filename);

NMI_ErrNo NMI_setbuf(NMI_FileHandle *pHandle, NMI_Char* buffer);


NMI_ErrNo NMI_setvbuf(NMI_FileHandle *pHandle, 
					  NMI_Char* buffer, 
					  NMI_Sint32 mode, 
					  NMI_Uint32 size);

/*
*Creates a temporary file in binary update mode (wb+). 
*The tempfile is removed when the program terminates or the stream is closed.
*On success a pointer to a file stream is returned. On error a null pointer is returned.
*/
NMI_ErrNo NMI_tmpfile(NMI_FileHandle *pHandle);


/*
 * Reads a line from the specified stream and stores it into the string pointed to by str.
 * It stops when either (n-1) characters are read,
 * the newline character is read, or the end-of-file is reached,
 * whichever comes first. The newline character is copied to the string.
 * A null character is appended to the end of the string.
 * On success a pointer to the string is returned. On error a null pointer is returned.
 * If the end-of-file occurs before any characters have been read, the string remains unchanged.
*/

//NMI_Char* NMI_fgets(NMI_Char* str, NMI_Sint32 n, NMI_FileHandle stream);


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
void NMI_clearerr(NMI_FileHandle* pHandle);


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
NMI_ErrNo NMI_ferror(NMI_FileHandle* pHandle);

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
NMI_Char* NMI_tmpnam(NMI_Char* str);


#endif


#ifdef CONFIG_NMI_FILE_OPERATIONS_STRING_API

/**
 * @brief 		Reads a line.
 * @param[in] 	pHandle Handle to the file object
 * @param[out] 	pcBuffer Storage location for data
 * @param[in] 	u32Maxsize Maximum number of data to be read including NULL terminator
 * @return 		Error code indicating success/failure
 * @retval 		NMI_FILE_EOF If the End-of-File is encountered, no characters 
 				have been read and the contents of buffer remain unchanged
 * @retval 		NMI_FAIL If an error occurs, no characters 
 				have been read and the contents of buffer remain unchanged
 * @note 		Original Standard Function: 
 				char *fgets(char *s,int n,FILE *stream);
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0
 */
NMI_ErrNo NMI_fgets(NMI_FileHandle* pHandle, NMI_Char* pcBuffer
				, NMI_Uint32 u32Maxsize);

/**
 * @brief 		Prints formatted data to a stream
 * @param[in] 	pHandle Handle to the file object
 * @param[in] 	pcFormat Format-control string
 * @param[in]	... Optional arguments
 * @return 		Error code indicating success/failure
 * @note Original Standard Function: 
 				int fprintf( FILE *stream, const char *format, ... );
 */
NMI_ErrNo NMI_fprintf(NMI_FileHandle* pHandle, const NMI_Char* pcFormat, ...);

/**
 * @brief 		Writes formatted output using a pointer to a list of arguments
 * @param[in] 	pHandle Handle to the file object
 * @param[in] 	pcFormat Format specification
 * @param[in] 	argptr Pointer to list of arguments
 * @return 		Error code indicating success/failure
 * @note 		riginal Standard Function: 
 			int vfprintf( FILE *stream, const char *format, va_list argptr );
 * @author		syounan
 * @date		30 Aug 2010
 * @version		1.0
 */
NMI_ErrNo NMI_vfprintf(NMI_FileHandle* pHandle, const NMI_Char* pcFormat, 
				va_list argptr);

#endif

#ifdef CONFIG_NMI_FILE_OPERATIONS_PATH_API

/**
 * @brief 		Renames/Moves a file or directory
 * @param[in] 	pcOldPath the old path
 * @param[in] 	pcNewPath the new path
 * @param[in] 	pstrAttrs Optional attributes, NULL for default.
 * @return 		Error code indicating success/failure
 * @note 		if there is a file or folder exsiting in the new path, the 
 *				operation fails
 * @author		syounan
 * @date		31 Oct 2010
 * @version		2.0 
 */
NMI_ErrNo NMI_PathMove(NMI_Char* pcOldPath, NMI_Char* pcNewPath,
	tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		Checks for the path exsitance
 * @details		Checks for the path exsitance, the path could be either a file or
				a directory, to test differentiate between files/directories 
				send a non-null poiter to the argument pbIsDirectory
 * @param[in] 	pcPath path of the examined file/directory
 * @param[out] 	pbIsDirectory set to NMI_TRUE if the path is directory, optional
				and could be NMI_NULL
 * @param[in] 	pstrAttrs Optional attributes, NULL for default.
 * @return 		NMI_TRUE if path exsits, NMI_FALSE if does not exist
 * @note 		if there is a file or folder exsiting in the new path, the 
 *				operation fails
 * @author		syounan
 * @date		31 Oct 2010
 * @version		2.0 
 */
NMI_Bool NMI_PathExsits(NMI_Char* pcPath, NMI_Bool* pbIsDirectory,
	tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		Deletes the given path
 * @details		Deletes the given path, wheather it is a file or a directory
 *				if it is a directory that contains other files and diretories, 
 *				they will be recusively deleted
 * @param[in] 	pcPath path to the File
 * @param[in] 	pstrAttrs Optional attributes, NULL for default. 
 * @return 		Error code indicating success/failure
 * @note 		if the path does not exist, the function returns an error
 * @author		mabubakr, syounan
 * @date		31 Oct 2010
 * @version		2.0 
 */
NMI_ErrNo NMI_PathDelete(NMI_Char* pcPath, tstrNMI_FileOpsAttrs* pstrAttrs);

/**
 * @brief 		creates a new directory
 * @details		creates a new directory in the given path, if a file or directory
				already exsits in this path the operation fails
 * @param[in] 	pcPath path to be created
 * @param[in] 	pstrAttrs Optional attributes, NULL for default. 
 * @return 		Error code indicating success/failure
 * @author		syounan
 * @date		31 Oct 2010
 * @version		2.0 
 */
NMI_ErrNo NMI_DirectoryCreate(NMI_Char* pcPath, tstrNMI_FileOpsAttrs* pstrAttrs);

#endif

#endif

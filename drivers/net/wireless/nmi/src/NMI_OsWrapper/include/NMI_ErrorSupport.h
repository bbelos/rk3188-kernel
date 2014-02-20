#ifndef __NMI_ERRORSUPPORT_H__
#define __NMI_ERRORSUPPORT_H__

/*!  
*  @file		NMI_ErrorSupport.h
*  @brief		Error reporting and handling support
*  @author		syounan
*  @sa			NMI_OSWrapper.h top level OS wrapper file
*  @date		10 Aug 2010
*  @version		1.0
*/

/* Psitive Numbers to indicate sucess with special status */
#define	NMI_ALREADY_EXSIT	+100	/** The requested object already exists */

/* Generic success will return 0 */
#define NMI_SUCCESS 		0	/** Generic success */

/* Negative numbers to indicate failures */
#define	NMI_FAIL                -100	/** Generic Fail */		
#define	NMI_BUSY                -101	/** Busy with another operation*/
#define	NMI_INVALID_ARGUMENT    -102	/** A given argument is invalid*/
#define	NMI_INVALID_STATE      	-103	/** An API request would violate the Driver state machine (i.e. to start PID while not camped)*/
#define	NMI_BUFFER_OVERFLOW     -104	/** In copy operations if the copied data is larger than the allocated buffer*/
#define NMI_NULL_PTR		-105	/** null pointer is passed or used */
#define	NMI_EMPTY               -107
#define NMI_FULL				-108
#define	NMI_TIMEOUT            	-109
#define NMI_CANCELED		-110	/** The required operation have been canceled by the user*/
#define NMI_INVALID_FILE	-112	/** The Loaded file is corruped or having an invalid format */
#define NMI_NOT_FOUND		-113	/** Cant find the file to load */
#define NMI_NO_MEM 		-114
#define NMI_UNSUPPORTED_VERSION -115
#define NMI_FILE_EOF			-116


/* Error type */
typedef NMI_Sint32 NMI_ErrNo;

#define NMI_IS_ERR(__status__) (__status__ < NMI_SUCCESS)

#define NMI_ERRORCHECK(__status__) do{\
	if(NMI_IS_ERR(__status__))\
	{\
		NMI_ERROR("NMI_ERRORCHECK(%d)\n", __status__);\
		goto ERRORHANDLER;\
	}\
}while(0)
	
#define NMI_ERRORREPORT(__status__, __err__) do{\
	NMI_ERROR("NMI_ERRORREPORT(%d)\n", __err__);\
	__status__ = __err__;\
	goto ERRORHANDLER;\
}while(0)

#define  NMI_NULLCHECK(__status__, __ptr__)	do{\
	if(__ptr__ == NMI_NULL)\
	{\
		NMI_ERRORREPORT(__status__, NMI_NULL_PTR);\
	}\
}while(0)

#define NMI_CATCH(__status__) \
ERRORHANDLER :\
if(NMI_IS_ERR(__status__)) \

#ifdef CONFIG_NMI_ASSERTION_SUPPORT

/**
 * @brief	prints a diagnostic message and aborts the program
 * @param[in] 	pcExpression The expression that triggered the assertion
 * @param[in] 	pcFileName The name of the current source file.
 * @param[in] 	u32LineNumber The line number in the current source file.
 * @warning DO NOT CALL DIRECTLY. USE EQUIVALENT MACRO FUNCTION INSTEAD.
 */
void NMI_assert_INTERNAL(NMI_Char* pcExpression, NMI_Char* pcFileName, NMI_Uint32 u32LineNumber);

#define NMI_assert(__expression__) (void)(!!(__expression__) || (NMI_assert_INTERNAL((#__expression__), __NMI_FILE__, __NMI_LINE__), 0))

#else
#define NMI_assert(__expression__) ((void)0)
#endif

#endif

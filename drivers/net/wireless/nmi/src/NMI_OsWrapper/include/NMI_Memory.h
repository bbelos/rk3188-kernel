#ifndef __NMI_MEMORY_H__
#define __NMI_MEMORY_H__

/*!  
*  @file	NMI_Memory.h
*  @brief	Memory OS wrapper functionality
*  @author	syounan
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	16 Aug 2010
*  @version	1.0
*/

#ifndef CONFIG_NMI_MEMORY_FEATURE
#error the feature CONFIG_NMI_MEMORY_FEATURE must be supported to include this file
#endif

/*!
*  @struct 		tstrNMI_MemoryAttrs
*  @brief		Memory API options 
*  @author		syounan
*  @date		16 Aug 2010
*  @version		1.0
*/
typedef struct {
	#ifdef CONFIG_NMI_MEMORY_POOLS
	/*!< the allocation pool to use for this memory, NULL for system 
	allocation. Default is NULL
	*/
	NMI_MemoryPoolHandle* pAllocationPool;
	#endif

	/* a dummy member to avoid compiler errors*/
	NMI_Uint8 dummy;
}tstrNMI_MemoryAttrs;

/*!
*  @brief	Fills the tstrNMI_MemoryAttrs with default parameters
*  @param[out]	pstrAttrs structure to be filled
*  @sa		tstrNMI_MemoryAttrs
*  @author	syounan
*  @date	16 Aug 2010
*  @version	1.0
*/
static void NMI_MemoryFillDefault(tstrNMI_MemoryAttrs* pstrAttrs)
{
	#ifdef CONFIG_NMI_MEMORY_POOLS
	pstrAttrs->pAllocationPool = NMI_NULL;
	#endif
}

/*!
*  @brief	Allocates a given size of bytes
*  @param[in]	u32Size size of memory in bytes to be allocated
*  @param[in]	strAttrs Optional attributes, NULL for default
		if not NULL, pAllocationPool should point to the pool to use for
		this allocation. if NULL memory will be allocated directly from
		the system
*  @param[in]	pcFileName file name of the calling code for debugging
*  @param[in]	u32LineNo line number of the calling code for debugging
*  @return	The new allocated block, NULL if allocation fails
*  @note	It is recommended to use of of the wrapper macros instead of 
		calling this function directly 
*  @sa		sttrNMI_MemoryAttrs
*  @sa		NMI_MALLOC
*  @sa		NMI_MALLOC_EX
*  @sa		NMI_NEW
*  @sa		NMI_NEW_EX
*  @author	syounan
*  @date	16 Aug 2010
*  @version	1.0
*/
void* NMI_MemoryAlloc(NMI_Uint32 u32Size, tstrNMI_MemoryAttrs* strAttrs,
	NMI_Char* pcFileName, NMI_Uint32 u32LineNo);

/*!
*  @brief	Allocates a given size of bytes and zero filling it
*  @param[in]	u32Size size of memory in bytes to be allocated
*  @param[in]	strAttrs Optional attributes, NULL for default
		if not NULL, pAllocationPool should point to the pool to use for
		this allocation. if NULL memory will be allocated directly from
		the system
*  @param[in]	pcFileName file name of the calling code for debugging
*  @param[in]	u32LineNo line number of the calling code for debugging
*  @return	The new allocated block, NULL if allocation fails
*  @note	It is recommended to use of of the wrapper macros instead of 
		calling this function directly 
*  @sa		sttrNMI_MemoryAttrs
*  @sa		NMI_CALLOC
*  @sa		NMI_CALLOC_EX
*  @sa		NMI_NEW_0
*  @sa		NMI_NEW_0_EX
*  @author	syounan
*  @date	16 Aug 2010
*  @version	1.0
*/
void* NMI_MemoryCalloc(NMI_Uint32 u32Size, tstrNMI_MemoryAttrs* strAttrs,
	NMI_Char* pcFileName, NMI_Uint32 u32LineNo);

/*!
*  @brief	Reallocates a given block to a new size
*  @param[in]	pvOldBlock the old memory block, if NULL then this function 
		behaves as a new allocation function
*  @param[in]	u32NewSize size of the new memory block in bytes, if zero then
		this function behaves as a free function
*  @param[in]	strAttrs Optional attributes, NULL for default
		if pAllocationPool!=NULL and pvOldBlock==NULL, pAllocationPool 
		should point to the pool to use for this allocation. 
		if pAllocationPool==NULL and pvOldBlock==NULL memory will be 
		allocated directly from	the system
		if and pvOldBlock!=NULL, pAllocationPool will not be inspected 
		and reallocation is done from the same pool as the original block
*  @param[in]	pcFileName file name of the calling code for debugging
*  @param[in]	u32LineNo line number of the calling code for debugging
*  @return	The new allocated block, possibly same as pvOldBlock
*  @note	It is recommended to use of of the wrapper macros instead of 
		calling this function directly 
*  @sa		sttrNMI_MemoryAttrs
*  @sa		NMI_REALLOC
*  @sa		NMI_REALLOC_EX
*  @author	syounan
*  @date	16 Aug 2010
*  @version	1.0
*/
void* NMI_MemoryRealloc(void* pvOldBlock, NMI_Uint32 u32NewSize, 
	tstrNMI_MemoryAttrs* strAttrs, NMI_Char* pcFileName, NMI_Uint32 u32LineNo);

/*!
*  @brief	Frees given block
*  @param[in]	pvBlock the memory block to be freed
*  @param[in]	strAttrs Optional attributes, NULL for default
*  @param[in]	pcFileName file name of the calling code for debugging
*  @param[in]	u32LineNo line number of the calling code for debugging
*  @note	It is recommended to use of of the wrapper macros instead of 
		calling this function directly 
*  @sa		sttrNMI_MemoryAttrs
*  @sa		NMI_FREE
*  @sa		NMI_FREE_EX
*  @sa		NMI_FREE_SET_NULL
*  @sa		NMI_FREE_IF_TRUE
*  @author	syounan
*  @date	16 Aug 2010
*  @version	1.0
*/
void NMI_MemoryFree(void* pvBlock, tstrNMI_MemoryAttrs* strAttrs,
	NMI_Char* pcFileName, NMI_Uint32 u32LineNo);

/*!
*  @brief	Creates a new memory pool
*  @param[out]	pHandle the handle to the new Pool
*  @param[in]	u32PoolSize The pool size in bytes
*  @param[in]	strAttrs Optional attributes, NULL for default
*  @return	Error code indicating sucess/failure
*  @sa		sttrNMI_MemoryAttrs
*  @author	syounan
*  @date	16 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_MemoryNewPool(NMI_MemoryPoolHandle* pHandle, NMI_Uint32 u32PoolSize,
	tstrNMI_MemoryAttrs* strAttrs);

/*!
*  @brief	Deletes a memory pool, freeing all memory allocated from it as well
*  @param[in]	pHandle the handle to the deleted Pool
*  @param[in]	strAttrs Optional attributes, NULL for default
*  @return	Error code indicating sucess/failure
*  @sa		sttrNMI_MemoryAttrs
*  @author	syounan
*  @date	16 Aug 2010
*  @version	1.0
*/
NMI_ErrNo NMI_MemoryDelPool(NMI_MemoryPoolHandle* pHandle, tstrNMI_MemoryAttrs* strAttrs);


#ifdef CONFIG_NMI_MEMORY_DEBUG

	/*!
	@brief	standrad malloc wrapper with custom attributes
	*/
	#define NMI_MALLOC_EX(__size__, __attrs__)\
		(NMI_MemoryAlloc(\
			(__size__), __attrs__,\
			(NMI_Char*)__NMI_FILE__, (NMI_Uint32)__NMI_LINE__))

	/*!
	@brief	standrad calloc wrapper with custom attributes
	*/
	#define NMI_CALLOC_EX(__size__, __attrs__)\
		(NMI_MemoryCalloc(\
			(__size__), __attrs__,\
			(NMI_Char*)__NMI_FILE__, (NMI_Uint32)__NMI_LINE__))
	
	/*!
	@brief	standrad realloc wrapper with custom attributes
	*/
	#define NMI_REALLOC_EX(__ptr__, __new_size__, __attrs__)\
		(NMI_MemoryRealloc(\
			(__ptr__), (__new_size__), __attrs__,\
			(NMI_Char*)__NMI_FILE__, (NMI_Uint32)__NMI_LINE__))

	/*!
	@brief	standrad free wrapper with custom attributes
	*/
	#define NMI_FREE_EX(__ptr__, __attrs__)\
		(NMI_MemoryFree(\
			(__ptr__), __attrs__,\
			(NMI_Char*)__NMI_FILE__, (NMI_Uint32)__NMI_LINE__))

#else

	/*!
	@brief	standrad malloc wrapper with custom attributes
	*/
	#define NMI_MALLOC_EX(__size__, __attrs__)\
		(NMI_MemoryAlloc(\
			(__size__), __attrs__, NMI_NULL, 0))

	/*!
	@brief	standrad calloc wrapper with custom attributes
	*/
	#define NMI_CALLOC_EX(__size__, __attrs__)\
		(NMI_MemoryCalloc(\
			(__size__), __attrs__, NMI_NULL, 0))

	/*!
	@brief	standrad realloc wrapper with custom attributes
	*/
	#define NMI_REALLOC_EX(__ptr__, __new_size__, __attrs__)\
		(NMI_MemoryRealloc(\
			(__ptr__), (__new_size__), __attrs__, NMI_NULL, 0))
	/*!
	@brief	standrad free wrapper with custom attributes
	*/
	#define NMI_FREE_EX(__ptr__, __attrs__)\
		(NMI_MemoryFree(\
			(__ptr__), __attrs__, NMI_NULL, 0))

#endif

/*!
@brief	Allocates a block (with custom attributes) of given type and number of 
elements 
*/
#define NMI_NEW_EX(__struct_type__, __n_structs__, __attrs__)\
	((__struct_type__*)NMI_MALLOC_EX(\
		sizeof(__struct_type__) * (NMI_Uint32)(__n_structs__), __attrs__))

/*!
@brief	Allocates a block (with custom attributes) of given type and number of 
elements and Zero-fills it
*/
#define NMI_NEW_0_EX(__struct_type__, __n_structs__, __attrs__)\
	((__struct_type__*)NMI_CALLOC_EX(\
		sizeof(__struct_type__) * (NMI_Uint32)(__n_structs__), __attrs__))

/*!
@brief	Frees a block (with custom attributes), also setting the original pointer
to NULL
*/
#define NMI_FREE_SET_NULL_EX(__ptr__, __attrs__) do{\
	if(__ptr__ != NMI_NULL){\
		NMI_FREE_EX(__ptr__, __attrs__);\
		__ptr__ = NMI_NULL ;\
	}\
}while(0)

/*!
@brief	Frees a block (with custom attributes) if the pointer expression evaluates 
to true
*/
#define NMI_FREE_IF_TRUE_EX(__ptr__, __attrs__) do{\
	if (__ptr__ != NMI_NULL){\
		NMI_FREE_EX(__ptr__, __attrs__);\
	}\
} while(0)

/*!
@brief	standrad malloc wrapper with default attributes
*/
#define NMI_MALLOC(__size__)\
	NMI_MALLOC_EX(__size__, NMI_NULL)

/*!
@brief	standrad calloc wrapper with default attributes
*/
#define NMI_CALLOC(__size__)\
	NMI_CALLOC_EX(__size__, NMI_NULL)
	
/*!
@brief	standrad realloc wrapper with default attributes
*/
#define NMI_REALLOC(__ptr__, __new_size__)\
	NMI_REALLOC_EX(__ptr__, __new_size__, NMI_NULL)
	
/*!
@brief	standrad free wrapper with default attributes
*/
#define NMI_FREE(__ptr__)\
	NMI_FREE_EX(__ptr__, NMI_NULL)

/*!
@brief	Allocates a block (with default attributes) of given type and number of 
elements 
*/
#define NMI_NEW(__struct_type__, __n_structs__)\
	NMI_NEW_EX(__struct_type__, __n_structs__, NMI_NULL)

/*!
@brief	Allocates a block (with default attributes) of given type and number of 
elements and Zero-fills it
*/
#define NMI_NEW_0(__struct_type__, __n_structs__)\
	NMI_NEW_O_EX(__struct_type__, __n_structs__, NMI_NULL)

/*!
@brief	Frees a block (with default attributes), also setting the original pointer
to NULL
*/
#define NMI_FREE_SET_NULL(__ptr__)\
	NMI_FREE_SET_NULL_EX(__ptr__, NMI_NULL)

/*!
@brief	Frees a block (with default attributes) if the pointer expression evaluates 
to true
*/
#define NMI_FREE_IF_TRUE(__ptr__)\
	NMI_FREE_IF_TRUE_EX(__ptr__, NMI_NULL)

	
#endif


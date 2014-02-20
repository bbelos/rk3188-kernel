
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_MEMORY_FEATURE


/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
void* NMI_MemoryAlloc(NMI_Uint32 u32Size, tstrNMI_MemoryAttrs* strAttrs,
	NMI_Char* pcFileName, NMI_Uint32 u32LineNo)
{
	if(u32Size > 0)
	{
		return kmalloc(u32Size, GFP_ATOMIC);
	}
	else
	{
		return NMI_NULL;
	}
}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
void* NMI_MemoryCalloc(NMI_Uint32 u32Size, tstrNMI_MemoryAttrs* strAttrs,
	NMI_Char* pcFileName, NMI_Uint32 u32LineNo)
{
	return kcalloc(u32Size, 1,GFP_KERNEL);
}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
void* NMI_MemoryRealloc(void* pvOldBlock, NMI_Uint32 u32NewSize, 
	tstrNMI_MemoryAttrs* strAttrs, NMI_Char* pcFileName, NMI_Uint32 u32LineNo)
{
	if(u32NewSize==0)
	{
		kfree(pvOldBlock);
		return NMI_NULL;
	}
	else if(pvOldBlock==NMI_NULL)
	{
		return kmalloc(u32NewSize, GFP_KERNEL);
	}
	else
	{
		return krealloc(pvOldBlock, u32NewSize, GFP_KERNEL);
	}

}

/*!
*  @author	syounan
*  @date	18 Aug 2010
*  @version	1.0
*/
void NMI_MemoryFree(void* pvBlock, tstrNMI_MemoryAttrs* strAttrs,
	NMI_Char* pcFileName, NMI_Uint32 u32LineNo)
{
	kfree(pvBlock);
}

#endif

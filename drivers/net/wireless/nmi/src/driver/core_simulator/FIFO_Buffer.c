

#include "NMI_OsWrapper/include/NMI_OSWrapper.h"
#include "driver/include/FIFO_Buffer.h"



NMI_Uint32 FIFO_InitBuffer(tHANDLE * hBuffer,NMI_Uint32 u32BufferLength)
{
	NMI_Uint32 u32Error = 0;
	tstrFifoHandler * pstrFifoHandler = NMI_MALLOC(sizeof(tstrFifoHandler));
	if(pstrFifoHandler)
	{
		NMI_memset(pstrFifoHandler,0,sizeof(tstrFifoHandler));
		pstrFifoHandler->pu8Buffer = NMI_MALLOC(u32BufferLength);
		if(pstrFifoHandler->pu8Buffer)
		{
			tstrNMI_SemaphoreAttrs strSemBufferAttrs;
			pstrFifoHandler->u32BufferLength = u32BufferLength;
			NMI_memset(pstrFifoHandler->pu8Buffer,0,u32BufferLength);
			//create semaphore
			NMI_SemaphoreFillDefault(&strSemBufferAttrs);	
			strSemBufferAttrs.u32InitCount = 1;
			NMI_SemaphoreCreate(&pstrFifoHandler->SemBuffer, &strSemBufferAttrs);						
			*hBuffer = pstrFifoHandler;
		}
		else
		{
			*hBuffer = NULL;
			u32Error = 1;
		}
	}
	else
	{
		u32Error = 1;
	}
	return u32Error;	
}
NMI_Uint32 FIFO_DeInit(tHANDLE hFifo)
{
	NMI_Uint32 u32Error = 0;
	tstrFifoHandler * pstrFifoHandler = (tstrFifoHandler*)hFifo;
	if(pstrFifoHandler)
	{
		if(pstrFifoHandler->pu8Buffer)
		{
			NMI_FREE(pstrFifoHandler->pu8Buffer);			
		}
		else
		{
			u32Error = 1;
		}

		NMI_SemaphoreDestroy(&pstrFifoHandler->SemBuffer, NMI_NULL);
		
		NMI_FREE(pstrFifoHandler);
	}
	else
	{
		u32Error = 1;
	}
	return u32Error;
}
NMI_Uint32 FIFO_ReadBytes(tHANDLE hFifo,NMI_Uint8 *pu8Buffer,NMI_Uint32 u32BytesToRead,NMI_Uint32 *pu32BytesRead)
{
	NMI_Uint32 u32Error = 0;
	tstrFifoHandler * pstrFifoHandler = (tstrFifoHandler*)hFifo;
	if(pstrFifoHandler && pu32BytesRead)
	{
		if(pstrFifoHandler->u32TotalBytes)
		{			
			if(NMI_SemaphoreAcquire(&pstrFifoHandler->SemBuffer, NMI_NULL) == NMI_SUCCESS)
			{
				if(u32BytesToRead > pstrFifoHandler->u32TotalBytes)
				{
					*pu32BytesRead = pstrFifoHandler->u32TotalBytes;
				}
				else
				{
					*pu32BytesRead = u32BytesToRead;
				}
				if((pstrFifoHandler->u32ReadOffset + u32BytesToRead) <= pstrFifoHandler->u32BufferLength)
				{
					NMI_memcpy(pu8Buffer,pstrFifoHandler->pu8Buffer + pstrFifoHandler->u32ReadOffset,
						*pu32BytesRead);
					//update read offset and total bytes
					pstrFifoHandler->u32ReadOffset += u32BytesToRead;
					pstrFifoHandler->u32TotalBytes -= u32BytesToRead;

				}
				else
				{
					NMI_Uint32 u32FirstPart =
						pstrFifoHandler->u32BufferLength - pstrFifoHandler->u32ReadOffset;
					NMI_memcpy(pu8Buffer,pstrFifoHandler->pu8Buffer + pstrFifoHandler->u32ReadOffset,
						u32FirstPart);
					NMI_memcpy(pu8Buffer + u32FirstPart,pstrFifoHandler->pu8Buffer,
						u32BytesToRead - u32FirstPart);
					//update read offset and total bytes
					pstrFifoHandler->u32ReadOffset = u32BytesToRead - u32FirstPart;
					pstrFifoHandler->u32TotalBytes -= u32BytesToRead;
				}				
				NMI_SemaphoreRelease(&pstrFifoHandler->SemBuffer, NMI_NULL);
			}
			else
			{
				u32Error = 1;
			}
		}
		else
		{
			u32Error = 1;
		}
	}
	else
	{
		u32Error = 1;
	}
	return u32Error;
}

NMI_Uint32 FIFO_WriteBytes(tHANDLE hFifo,NMI_Uint8 *pu8Buffer,NMI_Uint32 u32BytesToWrite,NMI_Bool bForceOverWrite)
{
	NMI_Uint32 u32Error = 0;
	tstrFifoHandler * pstrFifoHandler = (tstrFifoHandler*)hFifo;
	if(pstrFifoHandler)
	{
		if(u32BytesToWrite < pstrFifoHandler->u32BufferLength)
		{
			if((pstrFifoHandler->u32TotalBytes + u32BytesToWrite) <= pstrFifoHandler->u32BufferLength || 
				bForceOverWrite)
			{
				if(NMI_SemaphoreAcquire(&pstrFifoHandler->SemBuffer, NMI_NULL) == NMI_SUCCESS)
				{					
					if((pstrFifoHandler->u32WriteOffset + u32BytesToWrite) <= pstrFifoHandler->u32BufferLength)
					{
						NMI_memcpy(pstrFifoHandler->pu8Buffer + pstrFifoHandler->u32WriteOffset,pu8Buffer,
							u32BytesToWrite);
						//update read offset and total bytes
						pstrFifoHandler->u32WriteOffset += u32BytesToWrite;
						pstrFifoHandler->u32TotalBytes  += u32BytesToWrite;

					}
					else 					
					{
						NMI_Uint32 u32FirstPart =
							pstrFifoHandler->u32BufferLength - pstrFifoHandler->u32WriteOffset;
						NMI_memcpy(pstrFifoHandler->pu8Buffer + pstrFifoHandler->u32WriteOffset,pu8Buffer,
							u32FirstPart);
						NMI_memcpy(pstrFifoHandler->pu8Buffer,pu8Buffer + u32FirstPart,
							u32BytesToWrite - u32FirstPart);
						//update read offset and total bytes
						pstrFifoHandler->u32WriteOffset = u32BytesToWrite - u32FirstPart;
						pstrFifoHandler->u32TotalBytes += u32BytesToWrite;
					}
					//if data overwriten
					if(pstrFifoHandler->u32TotalBytes > pstrFifoHandler->u32BufferLength)
					{
						//adjust read offset to the oldest data available
						pstrFifoHandler->u32ReadOffset = pstrFifoHandler->u32WriteOffset;
						//data availabe is the buffer length
						pstrFifoHandler->u32TotalBytes = pstrFifoHandler->u32BufferLength;
					}
					NMI_SemaphoreRelease(&pstrFifoHandler->SemBuffer, NMI_NULL);
				}
			}
			else
			{
				u32Error = 1;
			}
		}
		else
		{
			u32Error = 1;
		}
	}
	else
	{
		u32Error = 1;
	}
	return u32Error;
}
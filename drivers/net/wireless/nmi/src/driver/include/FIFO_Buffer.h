
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"


#define tHANDLE	void *

typedef struct
{
	NMI_Uint8		*pu8Buffer;
	NMI_Uint32	u32BufferLength;
	NMI_Uint32	u32WriteOffset;
	NMI_Uint32	u32ReadOffset;
	NMI_Uint32	u32TotalBytes;	
	NMI_SemaphoreHandle	SemBuffer;
}tstrFifoHandler;


extern NMI_Uint32 FIFO_InitBuffer(tHANDLE * hBuffer,NMI_Uint32 u32BufferLength);
extern NMI_Uint32 FIFO_DeInit(tHANDLE hFifo);
extern NMI_Uint32 FIFO_ReadBytes(tHANDLE hFifo,NMI_Uint8 *pu8Buffer,NMI_Uint32 u32BytesToRead,
								 NMI_Uint32 *pu32BytesRead);
extern NMI_Uint32 FIFO_WriteBytes(tHANDLE hFifo,NMI_Uint8 *pu8Buffer,NMI_Uint32 u32BytesToWrite,
								  NMI_Bool bForceOverWrite);
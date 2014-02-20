#ifndef __NMI_MSG_QUEUE_H__
#define __NMI_MSG_QUEUE_H__

/*!  
*  @file	NMI_MsgQueue.h
*  @brief	Message Queue OS wrapper functionality
*  @author	syounan
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	30 Aug 2010
*  @version	1.0
*/

#ifndef CONFIG_NMI_MSG_QUEUE_FEATURE
#error the feature CONFIG_NMI_MSG_QUEUE_FEATURE must be supported to include this file
#endif

/*!
*  @struct 		tstrNMI_MsgQueueAttrs
*  @brief		Message Queue API options 
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
typedef struct
{
	#ifdef CONFIG_NMI_MSG_QUEUE_IPC_NAME
	NMI_Char* pcName;
	#endif

	#ifdef CONFIG_NMI_MSG_QUEUE_TIMEOUT
	NMI_Uint32 u32Timeout;
	#endif
	
	/* a dummy member to avoid compiler errors*/
	NMI_Uint8 dummy;
	
}tstrNMI_MsgQueueAttrs;

/*!
*  @brief		Fills the MsgQueueAttrs with default parameters
*  @param[out]	pstrAttrs structure to be filled
*  @sa			NMI_TimerAttrs
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
static void NMI_MsgQueueFillDefault(tstrNMI_MsgQueueAttrs* pstrAttrs)
{
	#ifdef CONFIG_NMI_MSG_QUEUE_IPC_NAME
	pstrAttrs->pcName = NMI_NULL;
	#endif

	#ifdef CONFIG_NMI_MSG_QUEUE_TIMEOUT
	pstrAttrs->u32Timeout = NMI_OS_INFINITY;
	#endif
}
/*!
*  @brief		Creates a new Message queue
*  @details		Creates a new Message queue, if the feature
				CONFIG_NMI_MSG_QUEUE_IPC_NAME is enabled and pstrAttrs->pcName
				is not Null, then this message queue can be used for IPC with
				any other message queue having the same name in the system
*  @param[in,out]	pHandle handle to the message queue object
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return		Error code indicating sucess/failure
*  @sa			tstrNMI_MsgQueueAttrs
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
NMI_ErrNo NMI_MsgQueueCreate(NMI_MsgQueueHandle* pHandle,
			tstrNMI_MsgQueueAttrs* pstrAttrs);


/*!
*  @brief		Sends a message
*  @details		Sends a message, this API will block unil the message is 
				actually sent or until it is timedout (as long as the feature
				CONFIG_NMI_MSG_QUEUE_TIMEOUT is enabled and pstrAttrs->u32Timeout
				is not set to NMI_OS_INFINITY), zero timeout is a valid value
*  @param[in]	pHandle handle to the message queue object
*  @param[in]	pvSendBuffer pointer to the data to send
*  @param[in]	u32SendBufferSize the size of the data to send
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return		Error code indicating sucess/failure
*  @sa			tstrNMI_MsgQueueAttrs
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
NMI_ErrNo NMI_MsgQueueSend(NMI_MsgQueueHandle* pHandle,
			const void * pvSendBuffer, NMI_Uint32 u32SendBufferSize,
			tstrNMI_MsgQueueAttrs* pstrAttrs);


/*!
*  @brief		Receives a message
*  @details		Receives a message, this API will block unil a message is 
				received or until it is timedout (as long as the feature
				CONFIG_NMI_MSG_QUEUE_TIMEOUT is enabled and pstrAttrs->u32Timeout
				is not set to NMI_OS_INFINITY), zero timeout is a valid value
*  @param[in]	pHandle handle to the message queue object
*  @param[out]	pvRecvBuffer pointer to a buffer to fill with the received message
*  @param[in]	u32RecvBufferSize the size of the receive buffer
*  @param[out]	pu32ReceivedLength the length of received data
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return		Error code indicating sucess/failure
*  @sa			tstrNMI_MsgQueueAttrs
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
NMI_ErrNo NMI_MsgQueueRecv(NMI_MsgQueueHandle* pHandle, 
			void * pvRecvBuffer, NMI_Uint32 u32RecvBufferSize, 
			NMI_Uint32* pu32ReceivedLength,
			tstrNMI_MsgQueueAttrs* pstrAttrs);


/*!
*  @brief		Destroys an existing  Message queue
*  @param[in]	pHandle handle to the message queue object
*  @param[in]	pstrAttrs Optional attributes, NULL for default
*  @return		Error code indicating sucess/failure
*  @sa			tstrNMI_MsgQueueAttrs
*  @author		syounan
*  @date		30 Aug 2010
*  @version		1.0
*/
NMI_ErrNo NMI_MsgQueueDestroy(NMI_MsgQueueHandle* pHandle,
			tstrNMI_MsgQueueAttrs* pstrAttrs);



#endif

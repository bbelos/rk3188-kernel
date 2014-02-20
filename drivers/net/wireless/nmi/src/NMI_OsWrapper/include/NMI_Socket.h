#ifndef __NMI_SOCKET_H__
#define __NMI_SOCKET_H__

/*!  
*  @file		NMI_Socket.h
*  @brief		Socket Connection functionality
*  @author		remil
*  @sa			NMI_OSWrapper.h top level OS wrapper file
*  @date		19 Oct 2010
*  @version		1.0
*/

#ifndef CONFIG_NMI_SOCKET_FEATURE
#error the feature CONFIG_NMI_SOCKET_FEATURE must be supported to include this file
#endif


/*!
*	Maximum length of IP address
*/
#define MAX_IP_LENGTH 16

/*============================================================================*/
/*!
*  @struct 		tpfNMI_TcpReceptionCB
*  @brief		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
typedef void (*tpfNMI_TcpReceptionCB)(NMI_Uint8* pu8Buffer,
									  NMI_Uint32 u32ReceivedDataLength,
									  void* vpUserData);
/*============================================================================*/
/*!
*  @struct 		tstrTcpConnection
*  @brief		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
typedef struct _tstrTcpConnection
{
	NMI_Uint32 u32SocketFd;
	NMI_Sint32 s32Port;
	NMI_Char cpIPAddress[MAX_IP_LENGTH];
	tpfNMI_TcpReceptionCB fpTcpReceptionCB;
	NMI_ThreadHandle strThreadHandle;
	tstrNMI_ThreadAttrs strThreadAttrs;
	void* vpUserData;

}tstrTcpConnection;


/*============================================================================*/
/*!
*  @brief		
*  @return		
*  @note		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
NMI_ErrNo NMI_TcpConnect(tstrTcpConnection* pstrTcpConnection);

/*============================================================================*/
/*!
*  @brief		
*  @return		
*  @note		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
void NMI_TcpDisconnect(tstrTcpConnection* pstrTcpConnection);

/*============================================================================*/
/*!
*  @brief		
*  @return		
*  @note		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
NMI_ErrNo NMI_TcpSend(tstrTcpConnection* pstrTcpConnection,
				  NMI_Uint8* u8Buf,
				  NMI_Uint32 u32DataSize);

#endif

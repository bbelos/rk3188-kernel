
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_SOCKET_FEATURE



/*=============================================================================
							Includes
==============================================================================*/

#ifdef _WIN32
//#include <winsock2.h>
//#include <ws2tcpip.h>
#elif defined unix
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#endif



#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#endif

#ifdef _WIN32
#define SOCKET_ERROR_CODE 		(WSAGetLastError())
#elif defined unix
#define SOCKET_ERROR_CODE 		(errno)
#endif


/*=============================================================================
							Macros
==============================================================================*/

#define BUFSIZE  ((NMI_Uint32)1024)




/*=============================================================================
							API Implementation
==============================================================================*/

/*============================================================================*/
/*!
*  @struct 		
*  @brief		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
void tcpReceiverThread(void* args)
{
	NMI_Sint32 s32Status;
	NMI_Uint32 u32SocketFd;
	NMI_Uint8 buf[BUFSIZE];

	tstrTcpConnection* pstrTcpConnection = (tstrTcpConnection*)args;
	
	NMI_NULLCHECK(s32Status,pstrTcpConnection);


	u32SocketFd = pstrTcpConnection->u32SocketFd;

	while(recv(u32SocketFd,buf,BUFSIZ,0)> 0)
	{
		if(pstrTcpConnection->fpTcpReceptionCB!= NMI_NULL)
		{
			pstrTcpConnection->fpTcpReceptionCB(buf,
				BUFSIZE,
				(void*)pstrTcpConnection);
		}	
	}
ERRORHANDLER:

	closesocket(pstrTcpConnection->u32SocketFd);
	return;

}



/*============================================================================*/
/*!
*  @struct 		
*  @brief		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
NMI_ErrNo NMI_TcpConnect(tstrTcpConnection* pstrTcpConnection)
{
	struct sockaddr_in server;
	struct hostent * hp;
	
	NMI_Sint32 s32Status;
	NMI_Uint32 addr;
	

#ifdef _MSC_VER
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
#endif


	NMI_ThreadFillDefault(&(pstrTcpConnection->strThreadAttrs));

	pstrTcpConnection->u32SocketFd =(NMI_Uint32) socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if(inet_addr(pstrTcpConnection->cpIPAddress)==INADDR_NONE)
	{
		hp=gethostbyname(pstrTcpConnection->cpIPAddress);
	}
	else
	{
		addr=inet_addr(pstrTcpConnection->cpIPAddress);
		hp=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
	}

	NMI_NULLCHECK(pstrTcpConnection->u32SocketFd,hp);

	server.sin_addr.s_addr=*((unsigned long*)hp->h_addr);
	server.sin_family=AF_INET;
	server.sin_port=htons(pstrTcpConnection->s32Port);

	s32Status = connect(pstrTcpConnection->u32SocketFd,(struct sockaddr*)&server, sizeof(server));
	NMI_ERRORCHECK(s32Status);

	return NMI_ThreadCreate(&(pstrTcpConnection->strThreadHandle),
		tcpReceiverThread,
		pstrTcpConnection,
		&(pstrTcpConnection->strThreadAttrs));

ERRORHANDLER:
	return NMI_FAIL;
}

/*============================================================================*/
/*!
*  @struct 		
*  @brief		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
NMI_ErrNo NMI_TcpSend(tstrTcpConnection* pstrTcpConnection,
				  NMI_Uint8* u8Buf,
				  NMI_Uint32 u32DataSize)
{	
	if( send(pstrTcpConnection->u32SocketFd,u8Buf, u32DataSize,0) > 0)
	{
		return NMI_SUCCESS;
	}
	return NMI_FAIL;
}
/*============================================================================*/
/*!
*  @struct 		
*  @brief		
*  @author		remil
*  @date		19 Oct 2010
*  @version		1.0
*/
/*============================================================================*/
void NMI_TcpDisconnect(tstrTcpConnection* pstrTcpConnection)
{	
	closesocket(pstrTcpConnection->u32SocketFd);
}




#endif

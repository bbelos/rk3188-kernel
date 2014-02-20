//#define SIMULATION_SHARED_SOCKET
#ifdef SIMULATION_SHARED_SOCKET


#define MAX_BUFFER_LEN 1596

/*!  
*  @file	UDP SOCKET.c.h
*  @brief	A Simulation for the communication betweeen simulator and 
*  @author	aismail
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	6 Mar 2012
*  @version	1.0
*/


/*******************************************************************************/
/*		SYSTEM INCLUDES														   */
/*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
WSADATA wsaData;

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
typedef int SOCKET;
#endif


/*******************************************************************************/
/*		PROJECTS INCLUDES			                      */
/*******************************************************************************/
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"


/*******************************************************************************/
/*		GLOBAL VARIABLES          		       		   */
/*******************************************************************************/
SOCKET SimulatorSocket,ConfiguratorSocket;
struct sockaddr_in SimulatorAddress,ConfiguratorAddress;
int sockaddr_len = sizeof(ConfiguratorAddress);
NMI_ThreadHandle hSimulator,hConfigurator;
NMI_Uint8 u8ThreadLoopExitCondition = 0x00;
NMI_Uint8* pu8Buffer_conf;
NMI_Sint8* ps8Buffer_sim;
NMI_Sint32 s32Err = 0;
NMI_Sint8 s8ExitFrame[] = "Exit Frame";
NMI_Sint8 s8TimeOutFrame[] = "TimeOut Frame";

#ifndef SIMULATION
NMI_Uint16 u16SimPort=50001;
NMI_Sint8 s8IPSim[] = "192.168.11.255";
#else
NMI_Uint16 u16SimPort=50001;
NMI_Sint8 s8IPSim[] = "192.168.10.43";
#endif
NMI_Uint16 u16ConPort=50000;
NMI_Sint8 s8IPCon[] = "192.168.10.43";

/*******************************************************************************/
/*		Stubbed Functions         				                    		   */
/*******************************************************************************/
extern NMI_Sint32 SimConfigPktReceived(NMI_Sint8* s8A, NMI_Sint32 s32B);
//extern NMI_Sint32 ConfigPktReceived(NMI_Sint8* pspacket, NMI_Sint32 s32PacketLen);
extern NMI_Sint32 ConfigPktReceived(NMI_Uint8* pu8RxPacket, NMI_Sint32 s32RxPacketLen);

void sendto_sim(void)
{
}



NMI_Sint32 SocketInit(struct sockaddr_in* pSourceAddress,struct sockaddr_in* pDestinationAddress,SOCKET *pSock,NMI_Uint16 u16PortSource,NMI_Uint16 u16PortDestination,NMI_Uint8* pu8IPSource,NMI_Uint8* pu8IPDestination)
{	
	NMI_Sint32 s32bind_status=1;
#ifdef WIN32
	WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

	/*Assign IP and port # of my address */
	pSourceAddress->sin_family = AF_INET; 
	pSourceAddress->sin_port = htons(u16PortSource);
#ifdef WIN32
	pSourceAddress->sin_addr.S_un.S_addr = inet_addr(pu8IPSource);
#else
	pSourceAddress->sin_addr.s_addr = inet_addr(pu8IPSource);
#endif

	/*Assign IP and port # of destination address */
	pDestinationAddress->sin_family = AF_INET; 
	pDestinationAddress->sin_port = htons(u16PortDestination);
#ifdef WIN32
	pDestinationAddress->sin_addr.S_un.S_addr = inet_addr(pu8IPDestination);
#else
	pDestinationAddress->sin_addr.s_addr= inet_addr(pu8IPDestination);
#endif

	/* Assign a socket number to my socket */
	*pSock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); // UDP configuration
	if(*pSock <0)
		perror("socket error");

	/* Enable broadcast */
	//setsockopt(*Pscok,SO_BROADCAST,
	/* Bind socket to source socket */
	s32bind_status = bind(*pSock,(struct sockaddr*) pSourceAddress ,sockaddr_len);

	if(s32bind_status == -1)
#ifdef WIN32
		printf("Bind Error: %d",WSAGetLastError());
#else
		perror("\nBind:");
#endif

	return s32bind_status;
}


/**
*  @brief 		Close socket.
*  @details 	close socket and release any allocated resources by the sockets.

*  @param[in,out] N/A
*  @param[in] 	mySocket the socket required to release its resources.
*  @return 		return 0 of successfully close.
*  @note 		could be implemented as inline function of macro
*  @author		aismail
*  @date		6 Mar 2012
*  @version		1.0
*/
NMI_Sint32 deInitSocket(SOCKET mySocket)
{
	u8ThreadLoopExitCondition = 0;
#ifdef WIN32
	return closesocket(mySocket);
#else
	return close(mySocket);
#endif
}


/**
*  @brief 		Represent the simulator side
*  @details 	Reseives data by socket and pass received data to SimConfigPktReceived to be parsed
				and configure the chip based on the received socket.
*  @param[in,out] N/A
*  @param[in] 	 N/A
*  @return 		N/A
*  @note 		N/A
*  @author		aismail
*  @date		6 Mar 2012
*  @version		1.0
*/

void SimulatorThread(void *p)
{
	NMI_Sint32 s32Error = 0;
	
	if((ps8Buffer_sim =  malloc(MAX_BUFFER_LEN))==NULL)
	{
		s32Err = NMI_FAIL;
	}
	else
	{
		while(!u8ThreadLoopExitCondition)
		{
			NMI_Sint32 s32PacketLength;
			NMI_Uint32 u32MaxBufferLength = MAX_BUFFER_LEN;
			NMI_Sint32 receive_status = -1;

			s32PacketLength = recvfrom(SimulatorSocket,ps8Buffer_sim,u32MaxBufferLength,0,
				(struct sockaddr*)&ConfiguratorAddress,&sockaddr_len);// recv from config
			printf("\nRecv from %d",s32PacketLength);
			NMI_Sleep(10);
			if(s32PacketLength < 0) /* Conection failure */
			{
#ifdef WIN32
				receive_status= WSAGetLastError();
#endif
				printf("\nConnection Failure: %d",receive_status);
			}
			else if(NMI_strcmp(pu8Buffer_conf,s8ExitFrame) == 0) /* check for exit frame packet to Exit while loop and terminate thread */
			{
				u8ThreadLoopExitCondition = 0xff; 
				break;
			}
			else
			{
				receive_status = 0;
				printf("\nMessage Received: Message Type: %c Seq Num:%d Messege Length: %x",ps8Buffer_sim[0],(NMI_Uint8)ps8Buffer_sim[1],(NMI_Uint16)*((NMI_Uint16*)&ps8Buffer_sim[4]));
				SimConfigPktReceived(ps8Buffer_sim,s32PacketLength);
			}
			
		} /* end while*/
	} /* end else if */

}


/**
*  @brief 		Represent the configurator side
*  @details 	Reseives data by socket and pass received data to SimConfigPktReceived to be parsed
				and configure the chip based on the received socket.
*  @param[in,out] N/A
*  @param[in] 	 N/A
*  @return 		N/A
*  @note 		N/A
*  @author		aismail
*  @date		6 Mar 2012
*  @version		1.0
*/

void ConfiguratorThread( void *p)
{
	//NMI_Uint8 u8Buffer[MAX_BUFFER_LEN];
	struct sockaddr_in strRecAddr; 
	if((pu8Buffer_conf =  malloc(MAX_BUFFER_LEN))== NULL)
	{
		s32Err = 6; /* To deInit previous function in TransInit */
	}
	else
	{
		while(!u8ThreadLoopExitCondition)
		{

		
			NMI_Sint32 s32PacketLength = 0;
			NMI_Uint32 u32MaxBufferLength = MAX_BUFFER_LEN;

			NMI_Sint32 receive_status = 0;

 			s32PacketLength = recvfrom(ConfiguratorSocket,pu8Buffer_conf,u32MaxBufferLength,0,
				(struct sockaddr*)&strRecAddr,&sockaddr_len);// recv from config
			
			if(s32PacketLength < 0)
			{
#ifdef WIN32
				receive_status = WSAGetLastError();
#endif
				printf("\nReceive Configurator Error: %d",receive_status);
			}
			else
			{
				if(NMI_strcmp(pu8Buffer_conf,s8ExitFrame) == 0) /* check for exit frame packet to Exit while loop and terminate thread */
				{
					u8ThreadLoopExitCondition = 0xff; 
					break;
				}
				else if(NMI_strcmp(pu8Buffer_conf,s8TimeOutFrame) == 0) /* check for exit frame packet to Exit while loop and terminate thread */
				{
					continue;
				}
				else 
				{
					receive_status = 0;
				}

				ConfigPktReceived(pu8Buffer_conf, s32PacketLength);
			}
		} /* End while */
	} /* End if*/
 
}



/**
*  @brief 		SendRawPacket to 
*  @details 	Opens a file, possibly creating a new file if write enabled and
 				pstrAttrs->bCreate is set to true
*  @param[in,out] s32PacketLen to get number of received bytes on the socket
*  @param[in] 	pspacket holds that data (packet) wanted to send on ConfiguratorSocket to Simulator.
*  @return 		Error code indicating success/failure: 0 success, error code for failure
*  @note 		N/A
				FILE *fopen( const char *filename, const char *mode );
*  @author		aismail
*  @date		6 Mar 2012
*  @version		1.0
*/
NMI_Sint32 SendRawPacket(NMI_Sint8* ps8Packet, NMI_Sint32 s32PacketLen)
{
	NMI_Sint32 s32SendStatus = NMI_FAIL;
	
	s32SendStatus = sendto(ConfiguratorSocket,ps8Packet,s32PacketLen,0,(struct sock_addr*)&SimulatorAddress,sockaddr_len);
	
	printf("\nSend Raw Data Status: %d",s32SendStatus);
	
	if(s32SendStatus < 0)
	{
#ifdef WIN32
		printf("Send Raw Packet Error:",WSAGetLastError());
#else
		perror("Send Raw Packet Error");
#endif
	}
	else 
	{
		s32SendStatus =  NMI_SUCCESS;
	}

	
	sendto_sim();
	printf("\nSend Done");

	return s32SendStatus;
}

 

 
/**
*  @brief 		Send response from chip(simulator) to configurator
*  @details 	Send a packet respond on a packet sent to chip.
*  @param[in,out] N/A
*  @param[in] 	pu8host_rsp holds that data (packet) wanted to send on ConfiguratorSocket to Simulator.
*  @param[in] 	u16host_rsp_len holds the size of datapacket
*  @return 		N/A
*  @note 		N/A
*  @author		aismail
*  @date		6 Mar 2012
*  @version		1.0
*/
void send_host_rsp(NMI_Uint8 *pu8host_rsp, NMI_Uint16 u16host_rsp_len)
{
	NMI_Sint32 send_status;
	send_status = sendto(SimulatorSocket,pu8host_rsp,u16host_rsp_len,0,(struct sock_addr*)&ConfiguratorAddress,sockaddr_len);
	if(send_status < 0)
		printf("SendHost Error: %d",GetLastError());
//		perror("Sending Host Error");
	/*send to config socket*/
	
	sendto_sim();
}

/**
*  @brief 		Initialize sockets and thread creation
*  @details 	Call functions used in initilizing sockets and thread creation.
*  @param[in,out] N/A
*  @param[in] 	N/A
*  @return 		s32Err 0: for success, otherwise for fail.
*  @note 		In case ofhaving a failure, the returned value is ORing or all failure done. i.e: the value expresses nothing
*  @author		aismail
*  @date		6 Mar 2012
*  @version		1.0
*/

NMI_Sint32 TransportInit( void)
{
	NMI_Sint32 s32Error = 0;
	

	/*Initialize Socket */
	s32Error = SocketInit(&ConfiguratorAddress,&SimulatorAddress,&ConfiguratorSocket,u16ConPort,u16SimPort,s8IPCon,s8IPSim);
	if(s32Error != 0)
		NMI_ERRORREPORT(s32Error,1);
#ifdef SIMULATION
	s32Error = SocketInit(&SimulatorAddress,&ConfiguratorAddress,&SimulatorSocket,u16SimPort,u16ConPort,s8IPSim,s8IPCon);	
	if(s32Error != 0)
		NMI_ERRORREPORT(s32Error,2);
	
	/* Create Thread */
	s32Error = NMI_ThreadCreate(&hSimulator,SimulatorThread,NULL, NULL);
	if(s32Error != 0)
		NMI_ERRORREPORT(s32Error,3);

#endif

	s32Error = NMI_ThreadCreate(&hConfigurator,ConfiguratorThread,NULL, NULL);
	if(s32Err != 0)
		NMI_ERRORREPORT(s32Error,4);




ERRORHANDLER:
	switch(s32Err)
	{
		case 4:
		case 6:
			NMI_ThreadDestroy(&hSimulator, NULL);
			free(ps8Buffer_sim);
		case 3:
		case 5:
			deInitSocket(SimulatorSocket);
		case 2:
			deInitSocket(ConfiguratorSocket);
			break;
	}

	return s32Err;
}


/**
*  @brief 		deInitialize sockets and thread creation
*  @details 	Wait till threads do their work and kill them and after that call deInitsocket
*  @param[in,out] N/A
*  @param[in] 	N/A
*  @return 		s32Err 0: for success, otherwise for fail.
*  @note 		In case ofhaving a failure, the returned value is ORing or all failure done. i.e: the value expresses nothing
*  @author		aismail
*  @date		6 Mar 2012
*  @version		1.0
*/

NMI_Sint32 TransportDeInit( void)
{
	NMI_Sint32 s32Err = 0;
	u8ThreadLoopExitCondition = 0xff;

	/* Send exit condition to running threads */
	sendto(SimulatorSocket,s8ExitFrame,NMI_strlen(s8ExitFrame)+1,0,(struct sock_addr*)&ConfiguratorAddress,sockaddr_len);
	sendto(ConfiguratorSocket,s8ExitFrame,NMI_strlen(s8ExitFrame)+1,0,(struct sock_addr*)&SimulatorAddress,sockaddr_len);

	/* Wait till finish to distory thread */
#ifdef SIMULATION
	s32Err |= NMI_ThreadDestroy(&hSimulator, NULL);
#endif
	s32Err |= NMI_ThreadDestroy(&hConfigurator, NULL);

	
	/* deInit socket and release used resources */
#ifdef SIMULATION
	s32Err |= deInitSocket(SimulatorSocket);
#endif
	s32Err |= deInitSocket(ConfiguratorSocket);

	free(pu8Buffer_conf);
	free(ps8Buffer_sim);


	return s32Err;

}


/**
*  @brief 		Send Time Out Frame to avoid receive blocking 
*  @details 	Send time out frame to avoid blocking on receiving state and back to normal state.
*  @param[in,out] N/A
*  @param[in] 	N/A
*  @return 		s32Err 0: for success, otherwise for fail.
*  @note 		In case ofhaving a failure, the returned value is ORing or all failure done. i.e: the value expresses nothing
*  @author		aismail
*  @date		3 Apr 2012
*  @version		1.0
*/
void SendTimeOutFrame(void *p)
{
	sendto(ConfiguratorSocket,s8TimeOutFrame,NMI_strlen(s8TimeOutFrame)+1,0,(struct sock_addr*)&SimulatorAddress,sockaddr_len);

}

#endif



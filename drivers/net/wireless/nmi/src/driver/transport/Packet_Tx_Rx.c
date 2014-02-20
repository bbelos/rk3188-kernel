//#define SIMULATION_SHARED
#ifndef SIMULATION_SHARED_SOCKET
/*!  
*  @file	Packet_Tx_Rx.c
*  @brief	
*  @author	
*  @sa		
*  @date	5 Mar 2012
*  @version	1.0
*/

#include "NMI_OsWrapper/include/NMI_OSWrapper.h"



extern NMI_Sint32 SimConfigPktReceived(NMI_Sint8* pspacket, NMI_Sint32 s32PacketLen);
extern NMI_Sint32 ConfigPktReceived(NMI_Uint8* pu8RxData, NMI_Sint32 s32RxDataLen);



NMI_Sint32 TransportInit(void)
{	
	NMI_Sint32 s32Error = NMI_SUCCESS;
	
	return s32Error;
}

NMI_Sint32 TransportDeInit(void)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	
	return s32Error;
}

NMI_Sint32 SendRawPacket(NMI_Sint8* pspacket, NMI_Sint32 s32PacketLen)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;

	s32Error = SimConfigPktReceived(pspacket, s32PacketLen);
	
	return s32Error;
}

/**
*  @brief 			sends the response to the host
*  @details 	
*  @param[in] 	host_rsp Pointer to the packet to be sent
*  @param[in] 	host_rsp_len length of the packet to be sent		
*  @return 		None
*  @note 		
*  @author		Ittiam
*  @date			18 Feb 2010
*  @version		
*/

void send_host_rsp(NMI_Uint8 *host_rsp, NMI_Uint16 host_rsp_len)
{
     ConfigPktReceived(host_rsp, host_rsp_len);
}



#endif
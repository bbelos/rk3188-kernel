/*++

THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. VIA TECHNOLOGIES, INC. 
DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES.  

File Name:
    Misc.h

Abstract:
   Please do not change any para of this files.  

Author:
      junfu zhang
Date : 
    02,23,2013

File Version:
    0.0.1

*/

#ifndef _MISC_H_
#define _MISC_H_

#include "constant.h"

//Please do not change any para of this files.  
#if(ICN838X_PACKAGE == ICN8380)
typedef enum{
    TX01=0,TX02,TX03,TX04,TX05,TX06,TX07,TX08,TX09,TX10,TX11,TX12,TX13,TX14,
    TX15,TX16,TX17,TX18,TX19,TX20,TX21,
}TX_No;

typedef enum{
    RX01=0,RX02,RX03,RX04,RX05,RX06,RX07,RX08,RX09,RX10,RX11,RX12,
}RX_No;

#elif(ICN838X_PACKAGE == ICN8383)
typedef enum{
    TX01=19,TX02=18,TX03=17,TX04=16,TX05=15,TX06=14,TX07=13,TX08=12,TX09=11,TX10=10,\
    TX11=9,TX12=8,TX13=7,TX14=6,TX15=5,TX16=4,TX17=3,TX18=2,TX19=1,TX20=0,
}TX_No;

typedef enum{
    RX01=0,RX02,RX03,RX04,RX05,RX06,RX07,RX08,RX09,RX10,RX11,RX12
}RX_No;

#elif(ICN838X_PACKAGE == ICN8382)
typedef enum{
    TX01=20,TX02=19,TX03=13,TX04=12,TX05=11,TX06=10,TX07=8,TX08=7,TX09=6,TX10=5,\
    TX11=4,TX12=3,TX13=2,TX14=1,TX15=0
}TX_No;

typedef enum{
    RX01=0,RX02,RX03,RX04,RX05,RX06,RX07,RX08,RX09,RX10,
}RX_No;

#else
#error"please check ICN838X_PACKAGE which is wrong!!! "
#endif


#endif

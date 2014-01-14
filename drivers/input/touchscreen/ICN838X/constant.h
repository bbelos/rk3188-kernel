/*++

THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. VIA TECHNOLOGIES, INC. 
DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES.  

File Name:
    Constant.h

Abstract:
    constant definition

Author:       JunFu Zhang
              Zhimin Tian
Date :        12 24,2012
Version:      1.0 for sensor only
History :
    03 28,2012,   0.1[first revision]
   

*/

#ifndef _CONSTANT_H_
#define _CONSTANT_H_

#define ICN8380                                   0x00    //for test 
#define ICN8382                                   0x01
#define ICN8383                                   0x02

#define CHIP0_SELECT                              0x01
#define CHIP1_SELECT                              0x02
#define CHIP_ALL_SELECT                           0x03 

#define CUSTOMER_ID_CHIPONE                       0x00
#define CUSTOMER_ID_NOKIA                         0x01
#define CUSTOMER_ID_O2TOUCH                       0x02
#define CUSTOMER_ID_LG                            0x03

 
#define PRODUCT_ID_CHIPONE                        0x00

//Nokia product id
#define PRODUCT_ID_NOKIA_X6                       0x01
#define PRODUCT_ID_NOKIA_N8                       0x02
//Lg product id
#define PRODUCT_ID_LG_990                         0x01


//
#define PHYSICAL_MAX_NUM_ROW                      21//28
#define PHYSICAL_MAX_NUM_COL                      12//16
#define PHYSICAL_MAX_TOUCH_NUM                    10
#define PHYSICAL_MAX_VK_NUM                       4

#define SCAN_MODE_AUTO_DETECT                     0    // system will auto detect scan mode ,will use one of the three existing modes.
#define SCAN_MODE_3F_MEAN                         1    // 3 pages rawdata will be received,call ALU mean
#define SCAN_MODE_3F_MEDIAN                       2    // 3 pages rawdata will be received,call ALU median
#define SCAN_MODE_1F_ONCE                         3    // 1 page rawdata will be received

#define HD_AVERAGE_MODE_OFF                       0x00    //33 wave
#define HD_AVERAGE_MODE_ONCE                      0x01    //11 wave
#define HD_AVERAGE_MODE_TWICE_MEAN                0x02    //11 *2  wave
#define HD_AVERAGE_MODE_THREE_MEAN                0x03    //11 *3  wave
#define HD_AVERAGE_MODE_THREE_MEDIAN              0x04    //11 * 3 wave


#define VK_MODE_1TX_NRX                           1
#define VK_MODE_NTX_1RX                           2
#define VK_MODE_1TX_NRX_REUSE                     3
#define VK_MODE_NTX_1RX_REUSE                     4
#define VK_MODE_NTX_NRX                           5


#define TX_VOL_330                                0
#define TX_VOL_633                                1
#define TX_VOL_665                                2
#define TX_VOL_701                                3
#define TX_VOL_741                                4
#define TX_VOL_786                                5
#define TX_VOL_837                                6
#define TX_VOL_895                                7

#define WINDOW_GAUSSIAN                           00 
#define WINDOW_RECTANGULAR                        BIT0 
    
#define BANDWIDTH_00                              0x00     //0:62k, 2:124k,3:248k,4:496           
#define BANDWIDTH_01                              0x01                         
#define BANDWIDTH_02                              0x02                            
#define BANDWIDTH_03                              0x03                         
#define BANDWIDTH_04                              0x04                         
#define BANDWIDTH_05                              0x05                         
#define BANDWIDTH_06                              0x06                         
#define BANDWIDTH_07                              0x07                         
#define BANDWIDTH_AUTO                            0x08  


#define SCAN_FREQUENCE_468K_OSR128                       0x02
#define SCAN_FREQUENCE_312K_OSR128                       0x03
#define SCAN_FREQUENCE_267K_OSR128                       0x7F
#define SCAN_FREQUENCE_234K_OSR128                       0x04
#define SCAN_FREQUENCE_187K_OSR128                       0x05
#define SCAN_FREQUENCE_156K_OSR128                       0x06
#define SCAN_FREQUENCE_133K_OSR128                       0x07
#define SCAN_FREQUENCE_117K_OSR128                       0x08
#define SCAN_FREQUENCE_104K_OSR128                       0x09
#define SCAN_FREQUENCE_93K_OSR128                        0x0a
#define SCAN_FREQUENCE_85K_OSR128                        0x0b
#define SCAN_FREQUENCE_78K_OSR128                        0x0c
#define SCAN_FREQUENCE_72K_OSR128                        0x0d
#define SCAN_FREQUENCE_66K_OSR128                        0x0e
#define SCAN_FREQUENCE_62K_OSR128                        0x0f
#define SCAN_FREQUENCE_58K_OSR128                        0x10
#define SCAN_FREQUENCE_55K_OSR128                        0x11
#define SCAN_FREQUENCE_52K_OSR128                        0x12
#define SCAN_FREQUENCE_49K_OSR128                        0x13
#define SCAN_FREQUENCE_46K_OSR128                        0x14
#define SCAN_FREQUENCE_44K_OSR128                        0x15
#define SCAN_FREQUENCE_42K_OSR128                        0x16


#define SCAN_FREQUENCE_937K_OSR64                       0x02|BIT7
#define SCAN_FREQUENCE_625K_OSR64                       0x03|BIT7
#define SCAN_FREQUENCE_535K_OSR64                       0x7F|BIT7
#define SCAN_FREQUENCE_468K_OSR64                       0x04|BIT7
#define SCAN_FREQUENCE_375K_OSR64                       0x05|BIT7
#define SCAN_FREQUENCE_312K_OSR64                       0x06|BIT7
#define SCAN_FREQUENCE_267K_OSR64                       0x07|BIT7
#define SCAN_FREQUENCE_234K_OSR64                       0x08|BIT7
#define SCAN_FREQUENCE_208K_OSR64                       0x09|BIT7
#define SCAN_FREQUENCE_187K_OSR64                       0x0a|BIT7
#define SCAN_FREQUENCE_170K_OSR64                       0x0b|BIT7
#define SCAN_FREQUENCE_156K_OSR64                       0x0c|BIT7
#define SCAN_FREQUENCE_144K_OSR64                       0x0d|BIT7
#define SCAN_FREQUENCE_133K_OSR64                       0x0e|BIT7
#define SCAN_FREQUENCE_125K_OSR64                       0x0f|BIT7
#define SCAN_FREQUENCE_117K_OSR64                       0x10|BIT7
#define SCAN_FREQUENCE_110K_OSR64                       0x11|BIT7
#define SCAN_FREQUENCE_104K_OSR64                       0x12|BIT7
#define SCAN_FREQUENCE_98K_OSR64                        0x13|BIT7
#define SCAN_FREQUENCE_93K_OSR64                        0x14|BIT7
#define SCAN_FREQUENCE_89K_OSR64                        0x15|BIT7
#define SCAN_FREQUENCE_85K_OSR64                        0x16|BIT7



#define BORDER_COMP_NONE                          0    //shut down border compensation
#define BORDER_COMP_ONE_SENSOR                    1    //use one sensor for border compensation
#define BORDER_COMP_ALL_SENSOR                    2    //use all sensor for border compensation
#define BORDER_COMP_PRESSURE                      3    //use pressure border compensation

#define OUTPUT_NOTHING                            0x00
#define OUTPUT_RAWDATA                            0x01
#define OUTPUT_DIFDATA                            0x02
#define OUTPUT_BASEDATA                           0x04
#define OUTPUT_BASE_COMPENSATE                    0x08
#define OUTPUT_FLOW_INFO                          0x10
#define OUTPUT_IIC_SPI_INFO                       0x20
#define OUTPUT_HOSTCOMM_INFO                      0x40

#define POWER_ON_LOCK                             0
#define POWER_ON_UNLOCK_WAIT                      1
#define POWER_ON_UNLOCK                           2


#define AUTO_CALIB_MODE_APPLE                     0
#define AUTO_CALIB_MODE_CHIPONE                   1


#define POWER_ON_FLOAT_PALM_MODE_OFF              0
#define POWER_ON_FLOAT_PALM_MODE_NAG              1
#define POWER_ON_FLOAT_PALM_MODE_NAG_MOVE         2

#define PANEL_PARA_FLAG_A_VALID_H                 0x6a
#define PANEL_PARA_FLAG_A_VALID_L                 0x66

#define PANEL_PARA_FLAG_B_VALID_H                 0x78
#define PANEL_PARA_FLAG_B_VALID_L                 0x79

#define FLASH_CODE_BLOCK                          0x00
#define FLASH_INFO_BLOCK                          0x01

#define GPIO_OUTPUT                               (1)
#define GPIO_INPUT                                (0)


#define GPIO0                                     (0x00)      // GPIO 0 
#define GPIO1                                     (0x01)      // GPIO 1
#define GPIO2                                     (0x02)      // GPIO 
#define GPIO3                                     (0x03)      // GPIO 
#define GPIO4                                     (0x04)      // GPIO 
#define GPIO5                                     (0x05)      // GPIO 
#define GPIO6                                     (0x06)      // GPIO 
#define GPIO7                                     (0x07)      // GPIO 

#define GPIO8                                     (0x08)      // GPIO 8
#define GPIO9                                     (0x09)      // GPIO 9
#define GPIO10                                    (0x0a)      // GPIO 10 
#define GPIO11                                    (0x0b)      // GPIO 11
#define GPIO12                                    (0x0C)      // GPIO 11

typedef enum{
    NORMAL_MODE = 0,FACTORY_MODE,CONFIG_MODE
}_WORK_MODE;


#define WAKE_UP_NONE                              0x00
#define WAKE_UP_GPIO                              0x01 
#define WAKE_UP_INTERFACE                         0x02

#define HOST_COMM_IIC                             0x01 
#define HOST_COMM_SPI                             0x02

#define OFFSET_LENGTH_1BYTE                       0x01
#define OFFSET_LENGTH_2BYTE                       0x02


#define TRUE                                      (1)
#define FALSE                                     (0)

#define USED                                      (1)
#define UNUSED                                    (0)


#define RESPOK                                    (0)
#define RESPFAIL                                  (1)

#define MUTE_STATE                                (1)
#define NOT_MUTE_STATE                            (0)

#define ENABLED                                   (1)
#define DISABLED                                  (0)
#define UNDEFINED                                 (2)

#define WAKE_UP_LOW_LEVEL                          0
#define WAKE_UP_HIGH_LEVEL                         1

#define INT_MODE_LOW                              0
#define INT_MODE_HIGH                             1

#define GPIO_VOLTAGE_33V                           0
#define GPIO_VOLTAGE_18V                           1

#define RAW_MODE                                   0x0
#define RCU_MODE                                   0x8


//Adddress of para in flash
#define FLASH_ADDR_A_PANEL_PARAMETER               0x7800
#define FLASH_ADDR_B_PANEL_PARAMETER               0x7c00

//Address of Para valid flag in flash
#define FLASH_INFO_ADDR_PARA_VALID_H               0x10
#define FLASH_INFO_ADDR_PARA_VALID_L               0x11


#define FLASH_INFO_ADDR_PATCH                      0x81
#define PATCH_VALID_LENGTH                         0x20

//BITs definition
#define BIT7                                      (0x80)
#define BIT6                                      (0x40)
#define BIT5                                      (0x20)
#define BIT4                                      (0x10)
#define BIT3                                      (0x08)
#define BIT2                                      (0x04)
#define BIT1                                      (0x02)
#define BIT0                                      (0x01)


// Bit operate
#define SET_BIT(var, bitMask)                     ((var) |= (bitMask))
#define CLR_BIT(var, bitMask)                     ((var) &= (~(bitMask)))
#define GET_BIT(var, bitMask)                     ((var) & (bitMask))

//
// typedef, struct, union and enum definitions
//
// define offset of some fixed code

#define LOBYTE(var)     (*((U8 *) &var)) 
#define HIBYTE(var)     (*((U8 *) &var + 1))       

#define LOUINT(var)     (*((U16 *) &var))  
#define HIUINT(var)     (*((U16 *) &var + 1))        

#define UL_BYTE0(var)   (*((U8 *) &var + 0)) 
#define UL_BYTE1(var)   (*((U8 *) &var + 1)) 
#define UL_BYTE2(var)   (*((U8 *) &var + 2)) 
#define UL_BYTE3(var)   (*((U8 *) &var + 3))       


#define UNREFERENED_PARA(var)                     var = var
#define M1(a,b)                                   a##b
#define STRUCT_OFFSET(StructName,MemberName)      (int)(&(((StructName*)0)->MemberName))


//-----------------------------------------------------------------

#endif  //end of _CONSTANT_H_


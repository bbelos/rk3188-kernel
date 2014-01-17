//--------------------------------------------------------
//
//
//	Melfas MMS100 Series Download base v1.0
//
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/melfas_ts.h>
#include "mms100_ISP_download_porting.h"


//=====================================================================
//
//   MELFAS Firmware download pharameters
//
//=====================================================================
#define MELFAS_TRANSFER_LENGTH					(32/8)		// Fixed value
#define MELFAS_FIRMWARE_MAX_SIZE				(32*1024)

//----------------------------------------------------
//   ISP Mode
//----------------------------------------------------
#define ISP_MODE_ERASE_FLASH					0x01
#define ISP_MODE_SERIAL_WRITE					0x02
#define ISP_MODE_SERIAL_READ					0x03
#define ISP_MODE_NEXT_CHIP_BYPASS				0x04

//----------------------------------------------------
//   Return values of download function
//----------------------------------------------------
#define MCSDL_RET_SUCCESS						0x00
#define MCSDL_RET_ERASE_FLASH_VERIFY_FAILED		0x01
#define MCSDL_RET_PROGRAM_VERIFY_FAILED			0x02

#define MCSDL_RET_PROGRAM_SIZE_IS_WRONG			0x10
#define MCSDL_RET_VERIFY_SIZE_IS_WRONG			0x11
#define MCSDL_RET_WRONG_BINARY					0x12

#define MCSDL_RET_READING_HEXFILE_FAILED		0x21
#define MCSDL_RET_FILE_ACCESS_FAILED			0x22
#define MCSDL_RET_MELLOC_FAILED					0x23

#define MCSDL_RET_WRONG_MODULE_REVISION			0x30

//============================================================
//
//	Delay parameter setting
//
//	These are used on 'mcsdl_delay()'
//
//============================================================
#define MCSDL_DELAY_1US 1
#define MCSDL_DELAY_2US 2
#define MCSDL_DELAY_3US 3
#define MCSDL_DELAY_5US 5
#define MCSDL_DELAY_7US 7

#define MCSDL_DELAY_10US 10
#define MCSDL_DELAY_15US 15
#define MCSDL_DELAY_20US 20
#define MCSDL_DELAY_40US 40

#define MCSDL_DELAY_100US 100
#define MCSDL_DELAY_150US 150
#define MCSDL_DELAY_300US 300
#define MCSDL_DELAY_500US 500
#define MCSDL_DELAY_800US 800

#define MCSDL_DELAY_1MS 1000
#define MCSDL_DELAY_5MS 5000

#define MCSDL_DELAY_10MS 10000
#define MCSDL_DELAY_25MS 25000
#define MCSDL_DELAY_30MS 30000
#define MCSDL_DELAY_40MS 40000
#define MCSDL_DELAY_45MS 45000
#define MCSDL_DELAY_60MS 60000


UINT8  ucVerifyBuffer[MELFAS_TRANSFER_LENGTH];		//	You may melloc *ucVerifyBuffer instead of this

//---------------------------------
//	Downloading functions
//---------------------------------
static void mcsdl_set_ready(void);
static void mcsdl_reboot_mcs(void);
static int  mcsdl_erase_flash(INT8 IdxNum);
static int  mcsdl_program_flash(UINT8 *pDataOriginal, UINT16 unLength, INT8 IdxNum);
static void mcsdl_program_flash_part(UINT8 *pData);
static int  mcsdl_verify_flash(UINT8 *pData, UINT16 nLength, INT8 IdxNum);
static void mcsdl_read_flash(UINT8 *pBuffer);
static void mcsdl_select_isp_mode(UINT8 ucMode);
static void mcsdl_unselect_isp_mode(void);
static void mcsdl_read_32bits(UINT8 *pData);
static void mcsdl_write_bits(UINT32 wordData, int nBits);
//---------------------------------
//	Delay functions
//---------------------------------
static void mcsdl_delay(UINT32 nCount);
//---------------------------------
//	For debugging display
//---------------------------------
#if MELFAS_ENABLE_DBG_PRINT
static void mcsdl_print_result(int nRet);
#endif



//------------------------------------------------------------------
//
//	Download function
//
//------------------------------------------------------------------
int isp_fw_download(const UINT8 *pBianry, const UINT16 unLength)
{
    int nRet;

    //---------------------------------
    // Check Binary Size
    //---------------------------------
    if (unLength >= MELFAS_FIRMWARE_MAX_SIZE)
    {
        nRet = MCSDL_RET_PROGRAM_SIZE_IS_WRONG;
        goto MCSDL_DOWNLOAD_FINISH;
    }


#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    printk(" - Starting download...\n");
#endif

    //---------------------------------
    // Make it ready
    //---------------------------------
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    printk(" > Ready\n");
#endif
    mcsdl_set_ready();
    mcsdl_delay(MCSDL_DELAY_1MS);

    //---------------------------------
    // Erase Flash
    //---------------------------------
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    printk(" > Erase\n");
#endif
    nRet = mcsdl_erase_flash(0);
    if (nRet != MCSDL_RET_SUCCESS)
        goto MCSDL_DOWNLOAD_FINISH;

    //---------------------------------
    // Program Flash
    //---------------------------------
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    printk(" > Program   ");
#endif
    nRet = mcsdl_program_flash((UINT8*)pBianry, (UINT16)unLength, 0);
    if (nRet != MCSDL_RET_SUCCESS)
        goto MCSDL_DOWNLOAD_FINISH;

    mcsdl_delay(MCSDL_DELAY_1MS);

    //---------------------------------
    // Verify flash
    //---------------------------------
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    printk(" > Verify    ");
#endif
    nRet = mcsdl_verify_flash((UINT8*)pBianry, (UINT16)unLength, 0);
    if (nRet != MCSDL_RET_SUCCESS)
        goto MCSDL_DOWNLOAD_FINISH;

    mcsdl_delay(MCSDL_DELAY_1MS);
    nRet = MCSDL_RET_SUCCESS;


MCSDL_DOWNLOAD_FINISH :

#if MELFAS_ENABLE_DBG_PRINT
    mcsdl_print_result(nRet);
#endif
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    printk(" > Rebooting\n");
    printk(" - Fin.\n\n");
#endif
    mcsdl_reboot_mcs();

    return nRet;
}



//------------------------------------------------------------------
//
//	Sub functions
//
//------------------------------------------------------------------
static int mcsdl_erase_flash(INT8 IdxNum)
{
    int	  i;
    UINT8 readBuffer[32];
    int eraseCompareValue = 0xFF;
    //----------------------------------------
    //	Do erase
    //----------------------------------------
    if (IdxNum > 0)
    {
        mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);
        mcsdl_delay(MCSDL_DELAY_3US);
    }
    mcsdl_select_isp_mode(ISP_MODE_ERASE_FLASH);
    mcsdl_unselect_isp_mode();

    return MCSDL_RET_SUCCESS;
}


static int mcsdl_program_flash(UINT8 *pDataOriginal, UINT16 unLength, INT8 IdxNum)
{
    int		i;

    UINT8	*pData;
    UINT8   ucLength;

    UINT16  addr;
    UINT32  header;

    addr   = 0;
    pData  = pDataOriginal;
    ucLength = MELFAS_TRANSFER_LENGTH;

    while ((addr*4) < (int)unLength)
    {
        if ((unLength - (addr*4)) < MELFAS_TRANSFER_LENGTH)
        {
            ucLength  = (UINT8)(unLength - (addr * 4));
        }

        //--------------------------------------
        //	Select ISP Mode
        //--------------------------------------
        mcsdl_delay(MCSDL_DELAY_40US);
        if (IdxNum > 0)
        {
            mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);
            mcsdl_delay(MCSDL_DELAY_3US);
        }
        mcsdl_select_isp_mode(ISP_MODE_SERIAL_WRITE);

        //---------------------------------------------
        //	Header
        //	Address[13ibts] <<1
        //---------------------------------------------
        header = ((addr & 0x1FFF) << 1) | 0x0 ;
        header = header << 14;
        mcsdl_write_bits(header, 18); //Write 18bits

        //---------------------------------
        //	Writing
        //---------------------------------
        //addr += (UINT16)ucLength;
        addr += 1;

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
        printk("#");
#endif
        mcsdl_program_flash_part(pData);
        pData  += ucLength;

        //---------------------------------------------
        //	Tail
        //---------------------------------------------
        MCSDL_GPIO_SDA_SET_HIGH();
        mcsdl_delay(MCSDL_DELAY_10US);

        for (i = 0; i < 6; i++)
        {
            if (i == 2)
            {
                mcsdl_delay(MCSDL_DELAY_20US);
            }
            else if (i == 3)
            {
                mcsdl_delay(MCSDL_DELAY_40US);
            }

            MCSDL_GPIO_SCL_SET_HIGH();  mcsdl_delay(MCSDL_DELAY_10US);
            MCSDL_GPIO_SCL_SET_LOW();   mcsdl_delay(MCSDL_DELAY_10US);
        }
        MCSDL_GPIO_SDA_SET_LOW();

        mcsdl_unselect_isp_mode();
        mcsdl_delay(MCSDL_DELAY_300US);
    }

    return MCSDL_RET_SUCCESS;
}

static void mcsdl_program_flash_part(UINT8 *pData)
{
    int     i;
    UINT32	data;


    //---------------------------------
    //	Body
    //---------------------------------
    data  = (UINT32)pData[0] <<  0;
    data |= (UINT32)pData[1] <<  8;
    data |= (UINT32)pData[2] << 16;
    data |= (UINT32)pData[3] << 24;
    mcsdl_write_bits(data, 32);
}

static int mcsdl_verify_flash(UINT8 *pDataOriginal, UINT16 unLength, INT8 IdxNum)
{
    int	  i, j;
    int	  nRet;

    UINT8 *pData;
    UINT8 ucLength;

    UINT16 addr;
    UINT32 wordData;

    addr  = 0;
    pData = (UINT8 *) pDataOriginal;
    ucLength  = MELFAS_TRANSFER_LENGTH;

    while ((addr*4) < (int)unLength)
    {
        if ((unLength - (addr*4)) < MELFAS_TRANSFER_LENGTH)
        {
            ucLength = (UINT8)(unLength - (addr * 4));
        }
        // start ADD DELAY
        mcsdl_delay(MCSDL_DELAY_40US);

        //--------------------------------------
        //	Select ISP Mode
        //--------------------------------------
        if (IdxNum > 0)
        {
            mcsdl_select_isp_mode(ISP_MODE_NEXT_CHIP_BYPASS);
            mcsdl_delay(MCSDL_DELAY_3US);
        }
        mcsdl_select_isp_mode(ISP_MODE_SERIAL_READ);


        //---------------------------------------------
        //	Header
        //	Address[13ibts] <<1
        //---------------------------------------------
        wordData   = ((addr & 0x1FFF) << 1) | 0x0;
        wordData <<= 14;
        mcsdl_write_bits(wordData, 18);
        addr += 1;

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
        printk("#");
#endif

        //--------------------
        // Read flash
        //--------------------
        mcsdl_read_flash(ucVerifyBuffer);

        MCSDL_GPIO_SDA_SET_OUTPUT();
        MCSDL_GPIO_SDA_SET_LOW();

        //--------------------
        // Comparing
        //--------------------
        if (IdxNum == 0)
        {
            for (j = 0; j < (int)ucLength; j++)
            {
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
                printk(" %02X", ucVerifyBuffer[j]);
#endif
                if (ucVerifyBuffer[j] != pData[j])
                {
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
                    printk("\n [Error] Address : 0x%04X : 0x%02X - 0x%02X\n", addr, pData[j], ucVerifyBuffer[j]);
#endif
                    nRet = MCSDL_RET_PROGRAM_VERIFY_FAILED;
                    goto MCSDL_VERIFY_FLASH_FINISH;
                }
            }
        }
        else // slave
        {
            for (j = 0; j < (int)ucLength; j++)
            {
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
                printk(" %02X", ucVerifyBuffer[j]);
#endif
                if ((0xff - ucVerifyBuffer[j]) != pData[j])
                {
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
                    printk("\n [Error] Address : 0x%04X : 0x%02X - 0x%02X\n", addr, pData[j], ucVerifyBuffer[j]);
#endif
                    nRet = MCSDL_RET_PROGRAM_VERIFY_FAILED;
                    goto MCSDL_VERIFY_FLASH_FINISH;
                }
            }
        }
        pData += ucLength;
        mcsdl_unselect_isp_mode();
    }

    nRet = MCSDL_RET_SUCCESS;

MCSDL_VERIFY_FLASH_FINISH:

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    printk("\n");
#endif

    mcsdl_unselect_isp_mode();
    return nRet;
}


static void mcsdl_read_flash(UINT8 *pBuffer)
{
    int i;

    MCSDL_GPIO_SCL_SET_LOW();
    MCSDL_GPIO_SDA_SET_LOW();
    mcsdl_delay(MCSDL_DELAY_40US);

    for (i = 0; i < 6; i++)
    {
        MCSDL_GPIO_SCL_SET_HIGH();  mcsdl_delay(MCSDL_DELAY_10US);
        MCSDL_GPIO_SCL_SET_LOW();  mcsdl_delay(MCSDL_DELAY_10US);
    }

    mcsdl_read_32bits(pBuffer);
}

static void mcsdl_set_ready(void)
{
    //--------------------------------------------
    // Tkey module reset
    //--------------------------------------------

	MCSDL_RESETB_IOMUX();
	MCSDL_CE_IOMUX();

	int ret;

	ret = gpio_request(GPIO_TSP_SDA_28V,"GPIO_TSP_SDA_28V");
	if(ret <0)
	{
		gpio_free(GPIO_TSP_SDA_28V);
		printk("gpio request err %s  %d  \n",__FUNCTION__,__LINE__);
		return ;
	}

	ret = gpio_request(GPIO_TSP_SCL_28V,"GPIO_TSP_SCL_28V");
	if(ret <0)
	{
		gpio_free(GPIO_TSP_SCL_28V);
		printk("gpio request err %s  %d  \n",__FUNCTION__,__LINE__);
		return ;
	}

#if 0
	ret = gpio_request(GPIO_TOUCH_INT,"GPIO_TOUCH_INT");
	if(ret <0)
	{
		gpio_free(GPIO_TOUCH_INT);
		printk("gpio request err %s  %d  \n",__FUNCTION__,__LINE__);
		return ;
	}
	ret = gpio_request(TOUCH_ENABLE_PIN,"TOUCH_ENABLE_PIN");
	if(ret <0)
	{
		gpio_free(TOUCH_ENABLE_PIN);
		printk("gpio request err %s  %d  \n",__FUNCTION__,__LINE__);
		return ;
	}
	ret = gpio_request(TOUCH_RESET_PIN,"TOUCH_RESET_PIN");
	if(ret <0)
	{
		gpio_free(TOUCH_RESET_PIN);
		printk("gpio request err %s  %d  \n",__FUNCTION__,__LINE__);
		return ;
	}
#endif

    MCSDL_VDD_SET_LOW(); // power
    MCSDL_CE_SET_OUTPUT();
    MCSDL_CE_SET_LOW();

    MCSDL_GPIO_SDA_SET_OUTPUT();
    MCSDL_GPIO_SDA_SET_LOW();

    MCSDL_GPIO_SCL_SET_OUTPUT();
    MCSDL_GPIO_SCL_SET_LOW();

    MCSDL_RESETB_SET_OUTPUT();
    MCSDL_RESETB_SET_LOW();

    mcsdl_delay(MCSDL_DELAY_25MS);						// Delay for Stable VDD

    MCSDL_VDD_SET_HIGH();
    MCSDL_CE_SET_HIGH();

    MCSDL_GPIO_SCL_SET_LOW();
    MCSDL_GPIO_SDA_SET_HIGH();
    mcsdl_delay(MCSDL_DELAY_40MS); 						// Delay '30 msec'
}


static void mcsdl_reboot_mcs(void)
{
    MCSDL_VDD_SET_LOW();
    MCSDL_CE_SET_OUTPUT();
    MCSDL_CE_SET_LOW();

    MCSDL_GPIO_SDA_SET_OUTPUT();
    MCSDL_GPIO_SDA_SET_HIGH();
    
    MCSDL_GPIO_SCL_SET_OUTPUT();
    MCSDL_GPIO_SCL_SET_HIGH();

    MCSDL_RESETB_SET_OUTPUT();
    MCSDL_RESETB_SET_LOW();

    mcsdl_delay(MCSDL_DELAY_25MS);						// Delay for Stable VDD

    MCSDL_VDD_SET_HIGH();
    MCSDL_CE_SET_HIGH();

    MCSDL_RESETB_SET_HIGH();
    MCSDL_RESETB_SET_ALT();
    MCSDL_GPIO_SCL_SET_ALT();
    MCSDL_GPIO_SDA_SET_ALT();

	gpio_free(GPIO_TSP_SDA_28V);
	gpio_free(GPIO_TSP_SCL_28V);
#if 0
	gpio_free(GPIO_TOUCH_INT);
	gpio_free(TOUCH_ENABLE_PIN);
	gpio_free(TOUCH_RESET_PIN);
#endif
    mcsdl_delay(MCSDL_DELAY_30MS); 						// Delay '25 msec'
}


//--------------------------------------------
//
//   Write ISP Mode entering signal
//
//--------------------------------------------
static void mcsdl_select_isp_mode(UINT8 ucMode)
{
    int    i;

    UINT8 enteringCodeMassErase[16]   = { 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1 };
    UINT8 enteringCodeSerialWrite[16] = { 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1 };
    UINT8 enteringCodeSerialRead[16]  = { 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1 };
    UINT8 enteringCodeNextChipBypass[16]  = { 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1 };

    UINT8 *pCode;


    //------------------------------------
    // Entering ISP mode : Part 1
    //------------------------------------
    if (ucMode == ISP_MODE_ERASE_FLASH) pCode = enteringCodeMassErase;
    else if (ucMode == ISP_MODE_SERIAL_WRITE) pCode = enteringCodeSerialWrite;
    else if (ucMode == ISP_MODE_SERIAL_READ) pCode = enteringCodeSerialRead;
    else if (ucMode == ISP_MODE_NEXT_CHIP_BYPASS) pCode = enteringCodeNextChipBypass;

    MCSDL_RESETB_SET_LOW();
    MCSDL_GPIO_SCL_SET_LOW();
    MCSDL_GPIO_SDA_SET_HIGH();

    for (i = 0; i < 16; i++)
    {
        if (pCode[i] == 1)
            MCSDL_RESETB_SET_HIGH();
        else
            MCSDL_RESETB_SET_LOW();

        mcsdl_delay(MCSDL_DELAY_3US);
        MCSDL_GPIO_SCL_SET_HIGH();	mcsdl_delay(MCSDL_DELAY_3US);
        MCSDL_GPIO_SCL_SET_LOW();	mcsdl_delay(MCSDL_DELAY_3US);
    }

    MCSDL_RESETB_SET_LOW();

    //---------------------------------------------------
    // Entering ISP mode : Part 2	- Only Mass Erase
    //---------------------------------------------------
    mcsdl_delay(MCSDL_DELAY_7US);

    MCSDL_GPIO_SCL_SET_LOW();
    MCSDL_GPIO_SDA_SET_HIGH();
    if (ucMode == ISP_MODE_ERASE_FLASH)
    {
        mcsdl_delay(MCSDL_DELAY_7US);
        for (i = 0; i < 4; i++)
        {
            if (i == 2) mcsdl_delay(MCSDL_DELAY_25MS);
            else if (i == 3) mcsdl_delay(MCSDL_DELAY_150US);

            MCSDL_GPIO_SCL_SET_HIGH();	mcsdl_delay(MCSDL_DELAY_3US);
            MCSDL_GPIO_SCL_SET_LOW();	mcsdl_delay(MCSDL_DELAY_7US);
        }
    }
    MCSDL_GPIO_SDA_SET_LOW();
}


static void mcsdl_unselect_isp_mode(void)
{
    int i;

    MCSDL_RESETB_SET_LOW();
    mcsdl_delay(MCSDL_DELAY_3US);

    for (i = 0; i < 10; i++)
    {
        MCSDL_GPIO_SCL_SET_HIGH();	mcsdl_delay(MCSDL_DELAY_3US);
        MCSDL_GPIO_SCL_SET_LOW();	mcsdl_delay(MCSDL_DELAY_3US);
    }
}



static void mcsdl_read_32bits(UINT8 *pData)
{
    int i, j;

    MCSDL_GPIO_SCL_SET_LOW();
    MCSDL_GPIO_SDA_SET_LOW();
    MCSDL_GPIO_SDA_SET_INPUT();


    for (i = 3; i >= 0; i--)
    {
        pData[i] = 0;
        for (j = 0; j < 8; j++)
        {
            pData[i] <<= 1;

            MCSDL_GPIO_SCL_SET_LOW();   mcsdl_delay(MCSDL_DELAY_1US);
            MCSDL_GPIO_SCL_SET_HIGH();  mcsdl_delay(MCSDL_DELAY_1US);

            if (MCSDL_GPIO_SDA_IS_HIGH())
                pData[i] |= 0x01;
        }
    }

    MCSDL_GPIO_SDA_SET_OUTPUT();
    MCSDL_GPIO_SDA_SET_LOW();
}



static void mcsdl_write_bits(UINT32 wordData, int nBits)
{
    int i;

    MCSDL_GPIO_SCL_SET_LOW();
    MCSDL_GPIO_SDA_SET_LOW();

    for (i = 0; i < nBits; i++)
    {
        if (wordData & 0x80000000) {MCSDL_GPIO_SDA_SET_HIGH();}
        else                       {MCSDL_GPIO_SDA_SET_LOW();}

        mcsdl_delay(MCSDL_DELAY_3US);
        MCSDL_GPIO_SCL_SET_HIGH();		mcsdl_delay(MCSDL_DELAY_3US);
        MCSDL_GPIO_SCL_SET_LOW();		mcsdl_delay(MCSDL_DELAY_3US);

        wordData <<= 1;
    }
}



//============================================================
//
//	Delay Function
//
//============================================================
static void mcsdl_delay(UINT32 nCount)
{

    switch (nCount)
    {
    case MCSDL_DELAY_1US   : {udelay(1);   break;}
    case MCSDL_DELAY_2US   : {udelay(2);   break;}
    case MCSDL_DELAY_3US   : {udelay(3);   break;}
    case MCSDL_DELAY_5US   : {udelay(5);   break;}
    case MCSDL_DELAY_7US   : {udelay(7);   break;}
    case MCSDL_DELAY_10US  : {udelay(10);  break;}
    case MCSDL_DELAY_15US  : {udelay(15);  break;}
    case MCSDL_DELAY_20US  : {udelay(20);  break;}
    case MCSDL_DELAY_40US  : {udelay(40);  break;}
    case MCSDL_DELAY_100US : {udelay(100); break;}
    case MCSDL_DELAY_150US : {udelay(150); break;}
    case MCSDL_DELAY_300US : {udelay(300); break;}
    case MCSDL_DELAY_500US : {udelay(500); break;}
    case MCSDL_DELAY_800US : {udelay(800); break;}
    case MCSDL_DELAY_1MS   : {msleep(1);   break;}
    case MCSDL_DELAY_5MS   : {msleep(5);   break;}
    case MCSDL_DELAY_10MS  : {msleep(10);  break;}
    case MCSDL_DELAY_25MS  : {msleep(25);  break;}
    case MCSDL_DELAY_30MS  : {msleep(30);  break;}
    case MCSDL_DELAY_40MS  : {msleep(40);  break;}
    case MCSDL_DELAY_45MS  : {msleep(45);  break;}
    case MCSDL_DELAY_60MS  : {msleep(60);  break;}
    default : {break;}
    }// Please, Use your delay function
}



//============================================================
//
//	Debugging print functions.
//
//============================================================

#ifdef MELFAS_ENABLE_DBG_PRINT
static void mcsdl_print_result(int nRet)
{
    if (nRet == MCSDL_RET_SUCCESS)
    {
        printk(" > MELFAS Firmware downloading SUCCESS.\n");
    }
    else
    {
        printk(" > MELFAS Firmware downloading FAILED  :  ");
        switch (nRet)
        {
        case MCSDL_RET_SUCCESS                  		:   printk("MCSDL_RET_SUCCESS\n"); break;
        case MCSDL_RET_ERASE_FLASH_VERIFY_FAILED		:   printk("MCSDL_RET_ERASE_FLASH_VERIFY_FAILED\n"); break;
        case MCSDL_RET_PROGRAM_VERIFY_FAILED			:   printk("MCSDL_RET_PROGRAM_VERIFY_FAILED\n"); break;

        case MCSDL_RET_PROGRAM_SIZE_IS_WRONG			:   printk("MCSDL_RET_PROGRAM_SIZE_IS_WRONG\n"); break;
        case MCSDL_RET_VERIFY_SIZE_IS_WRONG				:   printk("MCSDL_RET_VERIFY_SIZE_IS_WRONG\n"); break;
        case MCSDL_RET_WRONG_BINARY						:   printk("MCSDL_RET_WRONG_BINARY\n"); break;

        case MCSDL_RET_READING_HEXFILE_FAILED       	:   printk("MCSDL_RET_READING_HEXFILE_FAILED\n"); break;
        case MCSDL_RET_FILE_ACCESS_FAILED       		:   printk("MCSDL_RET_FILE_ACCESS_FAILED\n"); break;
        case MCSDL_RET_MELLOC_FAILED     		  		:   printk("MCSDL_RET_MELLOC_FAILED\n"); break;

        case MCSDL_RET_WRONG_MODULE_REVISION     		:   printk("MCSDL_RET_WRONG_MODULE_REVISION\n"); break;
        default                             			:	printk("UNKNOWN ERROR. [0x%02X].\n", nRet); break;
        }
        printk("\n");
    }
}

#endif






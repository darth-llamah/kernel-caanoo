//------------------------------------------------------------------------------
//
//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module     : Clock & Power Manager
//	File       : mes_clkpwr.c
//	Description:
//	Author     : Firmware Team
//	History    :
//------------------------------------------------------------------------------

#include <linux/module.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <linux/mutex.h>


//#include "pollux_clkpwr.h"
#include <asm/arch/pollux_clkpwr.h>

/// @brief  Clock & Power Manager Module's Register List
struct  MES_CLKPWR_RegisterSet
{
    volatile U32 CLKMODEREG;		    ///< 0x000 : Clock Mode Register
    volatile U32 PLLSETREG[2];		    ///< 0x004 ~ 0x008 : PLL Setting Register
    const    U32 __Reserved00[13];	    ///< 0x00C ~ 0x03F
    volatile U32 GPIOWAKEUPENB;		    ///< 0x040 : GPIO Wakeup Enable Register
    volatile U32 RTCWAKEUPENB;		    ///< 0x044 : RTC Wakeup Enable Register
    volatile U32 GPIOWAKEUPRISEENB;	    ///< 0x048 : GPIO Rising Edge Detect Enable Register
    volatile U32 GPIOWAKEUPFALLENB;     ///< 0x04C : GPIO Falling Edge Detect Enable Register
    volatile U32 GPIOPEND;              ///< 0x050 : GPIO Wakeup Detect Pending Register
    const    U32 __Reserved01;          ///< 0x054
    volatile U32 INTPENDSPAD;           ///< 0x058 : Interrupt Pending & Scratch PAD Register
    volatile U32 PWRRSTSTATUS;          ///< 0x05C : Power Reset Status Register
    volatile U32 INTENB;                ///< 0x060 : Interrupt Enable Register
    const    U32 __Reserved02[6];       ///< 0x064 ~ 0x07B
    volatile U32 PWRMODE;               ///< 0x07C : Power Mode Control Register
    const    U32 __Reserved03[20];      ///< 0x80 ~ 0xFC
    volatile U32 PADSTRENGTHGPIO[3][2]; ///< 0x100, 0x104 : GPIOA Pad Strength Register
                                        ///< 0x108, 0x10C : GPIOB Pad Strength Register
                                        ///< 0x110, 0x114 : GPIOC Pad Strength Register
    volatile U32 PADSTRENGTHBUS;        ///< 0x118        : Bus Pad Strength Register
};

static  struct
{
    struct MES_CLKPWR_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_CLKPWR_MODULE] = { CNULL, };

static  U32 __g_CurModuleIndex = 0;
//static  struct MES_CLKPWR_RegisterSet *__g_pRegister = CNULL;

static  struct MES_CLKPWR_RegisterSet *__g_pRegister = POLLUX_VA_CLKPWR;

//------------------------------------------------------------------------------
// Module Interface
//------------------------------------------------------------------------------
/**
 *  @brief  Initialize of prototype enviroment & local variables.
 *  @return \b CTRUE  indicate that Initialize is successed.\n
 *          \b CFALSE indicate that Initialize is failed.\n
 *  @see                                   MES_CLKPWR_SelectModule,
 *          MES_CLKPWR_GetCurrentModule,   MES_CLKPWR_GetNumberOfModule
 */
CBOOL   MES_CLKPWR_Initialize( void )
{
	static CBOOL bInit = CFALSE;
	U32 i;
	
	if( CFALSE == bInit )
	{
		__g_CurModuleIndex = 0;
		
		for( i=0; i < NUMBER_OF_CLKPWR_MODULE; i++ )
		{
			__g_ModuleVariables[i].pRegister = CNULL;
		}
		
		bInit = CTRUE;
	}
	
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Select module to control
 *  @param[in]  ModuleIndex     Module index to select.
 *	@return		the index of previously selected module.
 *  @see        MES_CLKPWR_Initialize,         
 *              MES_CLKPWR_GetCurrentModule,   MES_CLKPWR_GetNumberOfModule
 */
U32    MES_CLKPWR_SelectModule( U32 ModuleIndex )
{
	U32 oldindex = __g_CurModuleIndex;

    MES_ASSERT( NUMBER_OF_CLKPWR_MODULE > ModuleIndex );

    __g_CurModuleIndex = ModuleIndex;
    __g_pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	return oldindex;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get index of current selected module.
 *  @return     Current module's index.
 *  @see        MES_CLKPWR_Initialize,         MES_CLKPWR_SelectModule,
 *                                             MES_CLKPWR_GetNumberOfModule
 */
U32     MES_CLKPWR_GetCurrentModule( void )
{
    return __g_CurModuleIndex;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get number of modules in the chip.
 *  @return     Module's number.
 *  @see        MES_CLKPWR_Initialize,         MES_CLKPWR_SelectModule,
 *              MES_CLKPWR_GetCurrentModule   
 */
U32     MES_CLKPWR_GetNumberOfModule( void )
{
    return NUMBER_OF_CLKPWR_MODULE;
}

//------------------------------------------------------------------------------
// Basic Interface
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *  @brief      Get module's physical address.
 *  @return     Module's physical address
 *  @see                                           MES_CLKPWR_GetSizeOfRegisterSet,
 *              MES_CLKPWR_SetBaseAddress,         MES_CLKPWR_GetBaseAddress,
 *              MES_CLKPWR_OpenModule,             MES_CLKPWR_CloseModule,
 *              MES_CLKPWR_CheckBusy,              MES_CLKPWR_CanPowerDown
 */             
U32     MES_CLKPWR_GetPhysicalAddress( void )
{               
    MES_ASSERT( NUMBER_OF_CLKPWR_MODULE > __g_CurModuleIndex );

    return  (U32)(POLLUX_VA_CLKPWR + (OFFSET_OF_CLKPWR_MODULE * __g_CurModuleIndex) );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a size, in byte, of register set.
 *  @return     Size of module's register set.
 *  @see        MES_CLKPWR_GetPhysicalAddress,     
 *              MES_CLKPWR_SetBaseAddress,         MES_CLKPWR_GetBaseAddress,
 *              MES_CLKPWR_OpenModule,             MES_CLKPWR_CloseModule,
 *              MES_CLKPWR_CheckBusy,              MES_CLKPWR_CanPowerDown
 */
U32     MES_CLKPWR_GetSizeOfRegisterSet( void )
{
    return sizeof( struct MES_CLKPWR_RegisterSet );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a base address of register set.
 *  @param[in]  BaseAddress Module's base address
 *  @return     None.
 *  @see        MES_CLKPWR_GetPhysicalAddress,     MES_CLKPWR_GetSizeOfRegisterSet,
 *                                                 MES_CLKPWR_GetBaseAddress,
 *              MES_CLKPWR_OpenModule,             MES_CLKPWR_CloseModule,
 *              MES_CLKPWR_CheckBusy,              MES_CLKPWR_CanPowerDown
 */
void    MES_CLKPWR_SetBaseAddress( U32 BaseAddress )
{
    MES_ASSERT( CNULL != BaseAddress );
    MES_ASSERT( NUMBER_OF_CLKPWR_MODULE > __g_CurModuleIndex );

    __g_ModuleVariables[__g_CurModuleIndex].pRegister = (struct MES_CLKPWR_RegisterSet *)BaseAddress;
    __g_pRegister = (struct MES_CLKPWR_RegisterSet *)BaseAddress;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a base address of register set
 *  @return     Module's base address.
 *  @see        MES_CLKPWR_GetPhysicalAddress,     MES_CLKPWR_GetSizeOfRegisterSet,
 *              MES_CLKPWR_SetBaseAddress,         
 *              MES_CLKPWR_OpenModule,             MES_CLKPWR_CloseModule,
 *              MES_CLKPWR_CheckBusy,              MES_CLKPWR_CanPowerDown
 */
U32     MES_CLKPWR_GetBaseAddress( void )
{
    MES_ASSERT( NUMBER_OF_CLKPWR_MODULE > __g_CurModuleIndex );

    return (U32)__g_ModuleVariables[__g_CurModuleIndex].pRegister;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Initialize selected modules with default value.
 *  @return     \b CTRUE  indicate that Initialize is successed. \n
 *              \b CFALSE indicate that Initialize is failed.
 *  @see        MES_CLKPWR_GetPhysicalAddress,     MES_CLKPWR_GetSizeOfRegisterSet,
 *              MES_CLKPWR_SetBaseAddress,         MES_CLKPWR_GetBaseAddress,
 *                                                 MES_CLKPWR_CloseModule,
 *              MES_CLKPWR_CheckBusy,              MES_CLKPWR_CanPowerDown
 */
CBOOL   MES_CLKPWR_OpenModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Deinitialize selected module to the proper stage.
 *  @return     \b CTRUE  indicate that Deinitialize is successed. \n
 *              \b CFALSE indicate that Deinitialize is failed.
 *  @see        MES_CLKPWR_GetPhysicalAddress,     MES_CLKPWR_GetSizeOfRegisterSet,
 *              MES_CLKPWR_SetBaseAddress,         MES_CLKPWR_GetBaseAddress,
 *              MES_CLKPWR_OpenModule,             
 *              MES_CLKPWR_CheckBusy,              MES_CLKPWR_CanPowerDown
 */
CBOOL   MES_CLKPWR_CloseModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether the selected modules is busy or not.
 *  @return     \b CTRUE  indicate that Module is Busy. \n
 *              \b CFALSE indicate that Module is NOT Busy.
 *  @see        MES_CLKPWR_GetPhysicalAddress,     MES_CLKPWR_GetSizeOfRegisterSet,
 *              MES_CLKPWR_SetBaseAddress,         MES_CLKPWR_GetBaseAddress,
 *              MES_CLKPWR_OpenModule,             MES_CLKPWR_CloseModule,
 *                                                 MES_CLKPWR_CanPowerDown
 */
CBOOL   MES_CLKPWR_CheckBusy( void )
{
    if( MES_CLKPWR_IsPLLStable() )
    {
        return CFALSE;
    }

    return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicaes whether the selected modules is ready to enter power-down stage
 *  @return     \b CTRUE  indicate that Ready to enter power-down stage. \n
 *              \b CFALSE indicate that This module can't enter to power-down stage.
 *  @see        MES_CLKPWR_GetPhysicalAddress,     MES_CLKPWR_GetSizeOfRegisterSet,
 *              MES_CLKPWR_SetBaseAddress,         MES_CLKPWR_GetBaseAddress,
 *              MES_CLKPWR_OpenModule,             MES_CLKPWR_CloseModule,
 *              MES_CLKPWR_CheckBusy              
 */
CBOOL   MES_CLKPWR_CanPowerDown( void )
{
    if( MES_CLKPWR_IsPLLStable() )
    {
        return CTRUE;
    }

    return CFALSE;
}


//------------------------------------------------------------------------------
// Interrupt Interface
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number for interrupt controller.
 *  @return     Interrupt number
 *  @see                                               MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
S32     MES_CLKPWR_GetInterruptNumber( void )
{
    return  INTNUM_OF_CLKPWR_MODULE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a specified interrupt to be enable or disable.
 *  @param[in]  IntNum  Interrupt Number( 0 or 1 ). \n
 *                      0 indicate that GPIO event. \n
 *                      1 indicate that Battery Fault event. \n
 *  @param[in]  Enable  \b CTRUE  indicate that Interrupt Enable. \n
 *                      \b CFALSE indicate that Interrupt Disable.
 *  @return     None.
 *  @see        MES_CLKPWR_GetInterruptNumber,         
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
void    MES_CLKPWR_SetInterruptEnable( S32 IntNum, CBOOL Enable )
{
    MES_ASSERT( 0 == IntNum || 1 == IntNum );
    MES_ASSERT( (0==Enable) | (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    // 0:GPIOINT, 2:BATFINT
    __g_pRegister->INTENB = (U32)Enable << (IntNum * 2);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is enabled or disabled.
 *  @param[in]  IntNum Interrupt Number( 0 or 1 ). \n
 *              0 indicate that GPIO event. \n
 *              1 indicate that Battery Fault event. \n
 *  @return     \b CTRUE  indicate that Interrupt is enabled. \n
 *              \b CFALSE indicate that Interrupt is disabled.
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *                                                     MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
CBOOL   MES_CLKPWR_GetInterruptEnable( S32 IntNum )
{
    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );

    // 0:GPIOINT, 2:BATFINT
    return (CBOOL)( __g_pRegister->INTENB & (1UL<<(IntNum*2) ) );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is pended or not
 *  @param[in]  IntNum  Interrupt Number( 0 or 1 ). \n
 *              0 indicate that GPIO event. \n
 *              1 indicate that Battery Fault event. \n
 *  @return     \b CTRUE  indicate that Pending is seted. \n
 *              \b CFALSE indicate that Pending is Not Seted.
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
CBOOL   MES_CLKPWR_GetInterruptPending( S32 IntNum )
{
    const U32 PENDING_BIT_MASK	= 0x1;

    MES_ASSERT( 0 == IntNum || 1 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );

    // 2:GPIO, 3:BATF
    return ((__g_pRegister->PWRRSTSTATUS >>(IntNum + 2) ) & PENDING_BIT_MASK);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear a pending state of specified interrupt.
 *  @param[in]  IntNum  IntNum  Interrupt Number( 0 or 1 ). \n
 *              0 indicate that GPIO event. \n
 *              1 indicate that Battery Fault event. \n
 *  @return     None.
 *  @remarks    Clock & Power Manage Module have only one interrupt source. so \e IntNum must set to 0.
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *                                                     MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
void    MES_CLKPWR_ClearInterruptPending( S32 IntNum )
{
    const U32 RESERVE_MASK		= 0x7FFUL;
    register U32 temp;

    MES_ASSERT( 0 == IntNum || 1 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );

    temp = __g_pRegister->INTPENDSPAD & RESERVE_MASK;
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->INTPENDSPAD = temp | ( 1UL<<(13+IntNum));    // 13 : GPIORESETW, 14:BATFW
    __g_pRegister->INTPENDSPAD = temp;                                      
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set all interrupts to be enables or disables.
 *  @param[in]  Enable  \b CTRUE  indicate that Set to all interrupt enable. \n
 *                      \b CFALSE indicate that Set to all interrupt disable.
 *  @return     None.
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
void    MES_CLKPWR_SetInterruptEnableAll( CBOOL Enable )
{
    MES_ASSERT( (0==Enable) | (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->INTENB = (U32)( (Enable<<2) | Enable );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are enable or not.
 *  @return     \b CTRUE  indicate that At least one( or more ) interrupt is enabled. \n
 *              \b CFALSE indicate that All interrupt is disabled.
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *                                                     MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
CBOOL   MES_CLKPWR_GetInterruptEnableAll( void )
{
    const U32 BATFINTENB = 1UL << 2;
    const U32 GPIOINTENB = 1UL << 0;

    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->INTENB & (BATFINTENB | GPIOINTENB) )
    {
        return CTRUE;   
    }

    return CFALSE;       
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are pended or not.
 *  @return     \b CTRUE  indicate that At least one( or more ) pending is seted. \n
 *              \b CFALSE indicate that All pending is NOT seted.
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      
 *              MES_CLKPWR_ClearInterruptPendingAll,   MES_CLKPWR_GetInterruptPendingNumber
 */
CBOOL   MES_CLKPWR_GetInterruptPendingAll( void )
{
    const U32   BATF       = 1UL << 3;
    const U32   GPIORESET  = 1UL << 2;

    MES_ASSERT( CNULL != __g_pRegister );

    if(__g_pRegister->PWRRSTSTATUS & (BATF | GPIORESET) )
    {
        return CTRUE;
    }
    
    return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear pending state of all interrupts.
 *  @return     None.
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *                                                     MES_CLKPWR_GetInterruptPendingNumber
 */
void    MES_CLKPWR_ClearInterruptPendingAll( void )
{
    const U32    RESERVE_MASK	= 0x7FFUL;
    const U32    BATF           = 1UL << 14;
    const U32    GPIORESET      = 1UL << 13;
    register U32 temp;

    MES_ASSERT( CNULL != __g_pRegister );

    temp = __g_pRegister->INTPENDSPAD & RESERVE_MASK;
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->INTPENDSPAD = temp | (BATF | GPIORESET);
    __g_pRegister->INTPENDSPAD = temp;                                      
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number which has the most prority of pended interrupts
 *  @return     Pending Number( If all pending is not set then return -1 ).
 *  @see        MES_CLKPWR_GetInterruptNumber,         MES_CLKPWR_SetInterruptEnable,
 *              MES_CLKPWR_GetInterruptEnable,         MES_CLKPWR_GetInterruptPending,
 *              MES_CLKPWR_ClearInterruptPending,      MES_CLKPWR_SetInterruptEnableAll,
 *              MES_CLKPWR_GetInterruptEnableAll,      MES_CLKPWR_GetInterruptPendingAll,
 *              MES_CLKPWR_ClearInterruptPendingAll   
 */
S32     MES_CLKPWR_GetInterruptPendingNumber( void )  // -1 if None
{
    const U32   BATF       = 1UL << 3;
    const U32   GPIORESET  = 1UL << 2;
    register U32 Pend; 

    MES_ASSERT( CNULL != __g_pRegister );

    Pend = __g_pRegister->PWRRSTSTATUS;
     
    if( Pend & GPIORESET )
    {
        return 0;
    }
    else if( Pend & BATF )
    {
        return 1;
    }

    return -1;
}


//------------------------------------------------------------------------------
// Clock & Power Manger Operation.
//------------------------------------------------------------------------------

//--------------------------------------------------------------------------
//   Clock Management
//--------------------------------------------------------------------------
/**
 *  @brief	    Set PLL by P, M, S.
 *  @param[in]  pllnumber   PLL to be changed ( 0 : PLL0, 1 : PLL1 )
 *  @param[in]  PDIV	    Input frequency divider : 1 ~ 63
 *  @param[in]  MDIV	    VCO frequency divider   : PLL0(56~1023), PLL1(13~255)
 *  @param[in]  SDIV	    Output frequency scaler : PLL0(0~5), PLL1(0~4)
 *	@return	    None.
 *  @remark	    PLL can be calculated by following fomula. \n
 *              (m * Fin)/(p * 2^s ).
 *  		    - PLL0 : 16 ~ 500 Mhz \n
 *              - PLL1 : 16 ~ 300 Mhz \n
 *  		    - IMPORTANT : \n
 *  		      You should set a PMS value carefully. \n
 *  		      Please refer PMS table we are recommend or Contact Magiceyes or
 * 		  	      Samsung(manufacture of PLL core) if you wish to change it.
 *
 *  @remark	    PLL is not changed until MES_CLKPWR_DoPLLChange() function is called.
 *  @see	                                MES_CLKPWR_SetPLLPowerOn,
 *              MES_CLKPWR_DoPLLChange,     MES_CLKPWR_IsPLLStable,
 *              MES_CLKPWR_SetClockCPU,     MES_CLKPWR_SetClockBCLK 
 */
void 	MES_CLKPWR_SetPLLPMS ( U32 pllnumber, U32 PDIV, U32 MDIV, U32 SDIV )
{
    const U32 PLL_PDIV_BIT_POS = 18;
    const U32 PLL_MDIV_BIT_POS =  8;
    const U32 PLL_SDIV_BIT_POS =  0;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 2  > pllnumber );
    MES_ASSERT( 1  <= PDIV && PDIV <= 63 );
    MES_ASSERT( (0 != pllnumber) || (56 <= MDIV && MDIV <= 1023) );
    MES_ASSERT( (0 != pllnumber) || (5  >= SDIV) );    
    MES_ASSERT( (1 != pllnumber) || (13 <= MDIV && MDIV <= 255) );
    MES_ASSERT( (1 != pllnumber) || (4  >= SDIV) );

    __g_pRegister->PLLSETREG[pllnumber] = (PDIV<<PLL_PDIV_BIT_POS) | (MDIV<<PLL_MDIV_BIT_POS) | (SDIV<<PLL_SDIV_BIT_POS);
}

//------------------------------------------------------------------------------
/**
 *  @brief	    Set PLL1 power On/Off.
 *  @param[in]  On  \b CTRUE indicate that Normal mode . \n 
 *                  \b CFALSE indicate that Power Down.
 *	@return	    None.
 *  @remark     PLL1 is only available.
 *  @see	    MES_CLKPWR_SetPLLPMS,       
 *              MES_CLKPWR_DoPLLChange,     MES_CLKPWR_IsPLLStable,
 *              MES_CLKPWR_SetClockCPU,     MES_CLKPWR_SetClockBCLK 

 */
void 	MES_CLKPWR_SetPLLPowerOn ( CBOOL On )
{
    register U32 PLLPWDN = (U32)(1UL << 30);

    MES_ASSERT( CNULL != __g_pRegister );

    if( On )	__g_pRegister->CLKMODEREG &= ~PLLPWDN;
    else		__g_pRegister->CLKMODEREG |=  PLLPWDN;

}

//------------------------------------------------------------------------------
/**
 *  @brief	Change PLL with P, M, S value on PLLSETREG.
 *	@return	None.
 *  @remark If you call this function, you must check by MES_CLKPWR_IsPLLStable(),
 *          because PLL change need stable time.\n
 *  		Therefore the sequence for changing PLL is as follows.
 *  @code
 * 		MES_CLKPWR_SetPLLPMS( PLLn, P, M, S );  // Change P, M, S values
 * 		MES_CLKPWR_DoPLLChange();               // Change PLL
 * 		while( !MES_CLKPWR_IsPLLStable() );     // wait until PLL is stable.
 *  @endcode
 *  @see	    MES_CLKPWR_SetPLLPMS,       MES_CLKPWR_SetPLLPowerOn,
 *                                          MES_CLKPWR_IsPLLStable,
 *              MES_CLKPWR_SetClockCPU,     MES_CLKPWR_SetClockBCLK 

 */
void 	MES_CLKPWR_DoPLLChange ( void )
{
    const U32 CHGPLL = (1<<15);

    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->PWRMODE |= CHGPLL;
}

//------------------------------------------------------------------------------
/**
 *  @brief  Check whether PLL are stable or not.
 *  @return If PLL is stable, return \b CTRUE. Otherwise, return \b CFALSE.
 *  @see	    MES_CLKPWR_SetPLLPMS,       MES_CLKPWR_SetPLLPowerOn,
 *              MES_CLKPWR_DoPLLChange,     
 *              MES_CLKPWR_SetClockCPU,     MES_CLKPWR_SetClockBCLK 

 */
CBOOL 	MES_CLKPWR_IsPLLStable ( void )
{
    const U32 CHGPLL = (1<<15);
    MES_ASSERT( CNULL != __g_pRegister );

    return ((__g_pRegister->PWRMODE & CHGPLL ) ? CFALSE : CTRUE );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set clock for CPU.
 *  @param[in]  ClkSrc  	Source clock for CPU ( 0 : PLL0, 1 : PLL1 ).
 *  @param[in]  CoreDivider	Divider for CPU core clock, 1 ~ 16.
 *  @param[in]  BusDivider	Divider for CPU Bus clock, 2 ~ 16.
 *	@return	    None.
 *	@remark	    \n
 *			    - CPU core clock = Source clock / CoreDivider, CoreDivider = 1 ~ 16
 *			    - CPU bus clock = CPU core clock / BusDivider, BusDivider = 2 ~ 16
 *  @see	    MES_CLKPWR_SetPLLPMS,       MES_CLKPWR_SetPLLPowerOn,
 *              MES_CLKPWR_DoPLLChange,     MES_CLKPWR_IsPLLStable,
 *                                          MES_CLKPWR_SetClockBCLK 

 */
void 	MES_CLKPWR_SetClockCPU    ( U32 ClkSrc, U32 CoreDivider, U32 BusDivider )
{
    const U32	CLK_MASK	= 0x3FF;
    const U32	CLKDVO_POS	= 0;
    const U32	CLKSEL_POS	= 4;
    const U32	CLKDIV2_POS	= 6;
    register U32 temp;

    MES_ASSERT( 2 > ClkSrc );
    MES_ASSERT( 1 <= CoreDivider && CoreDivider <= 16 );
    MES_ASSERT( 2 <= BusDivider && BusDivider <= 16 );
    MES_ASSERT( CNULL != __g_pRegister );

    temp  = __g_pRegister->CLKMODEREG;

    temp &= ~CLK_MASK;
    temp |= (U32)( ((CoreDivider-1) << CLKDVO_POS)
    	 		 | ((U32)ClkSrc << CLKSEL_POS)
    	 		 | ((BusDivider-1)  << CLKDIV2_POS) );

	__g_pRegister->CLKMODEREG = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set clock for BCLK.
 *  @param[in]  ClkSrc  	Source clock for BCLK ( 0 : PLL0, 1 : PLL1 )
 *  @param[in]  BCLKDivider	Divider for BCLK, 1 ~ 16.
 *	@return	    None.
 *	@remark	    BCLK is system bus clock is IO bus clock.
 *			    - BCLK = Source clock / BCLKDivider, BCLKDivider = 1 ~ 16
 *
 *  @see	    MES_CLKPWR_SetPLLPMS,       MES_CLKPWR_SetPLLPowerOn,
 *              MES_CLKPWR_DoPLLChange,     MES_CLKPWR_IsPLLStable,
 *              MES_CLKPWR_SetClockCPU     

 */
void 	MES_CLKPWR_SetClockBCLK    ( U32 ClkSrc, U32 BCLKDivider )
{
    const U32	CLK_MASK	= (0x3F)<<20;
    const U32	CLKDV1_POS	= 20;
    const U32	CLKSEL_POS	= 24;
    register U32 temp;

    MES_ASSERT( 2 > ClkSrc );
    MES_ASSERT( 1 <= BCLKDivider && BCLKDivider <= 16 );
    MES_ASSERT( CNULL != __g_pRegister );

    temp = __g_pRegister->CLKMODEREG;
    temp &= ~CLK_MASK;
    temp |= (U32)( ((BCLKDivider-1) << CLKDV1_POS)
    	 		 | ((U32)ClkSrc << CLKSEL_POS) );

	__g_pRegister->CLKMODEREG = temp;
}


//------------------------------------------------------------------------------
// VDDPWRTOGGLE Wakeup Management
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable VDD Power Toggle Wakeup.
 *  @param[in]  Enable   	Set to \b CTRUE to enable VDD Power Toggle wakeup. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@see	    MES_CLKPWR_GetVDDPowerToggleWakeUpEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpPending, MES_CLKPWR_ClearVDDPowerToggleWakeUpPending
 */
void  	MES_CLKPWR_SetVDDPowerToggleWakeUpEnable ( CBOOL Enable )
{
    const U32   WKUPEN31_POS  = 31;
    const U32   WKUPEN31_MASK = 1UL << WKUPEN31_POS;

    register U32 temp;

    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    temp = __g_pRegister->GPIOWAKEUPENB;

    temp &= ~WKUPEN31_MASK;
    temp |=  (U32)Enable << WKUPEN31_POS;

    __g_pRegister->GPIOWAKEUPENB = temp;

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get a status of VDD Power Toggle Wakeup Enable.
 *	@return     If VDD Power Toggle event is enabled, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@see	    MES_CLKPWR_SetVDDPowerToggleWakeUpEnable,   
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpPending, MES_CLKPWR_ClearVDDPowerToggleWakeUpPending
 */
CBOOL	MES_CLKPWR_GetVDDPowerToggleWakeUpEnable ( void )
{
    const U32   WKUPEN31_POS  = 31;
    const U32   WKUPEN31_MASK = 1UL << WKUPEN31_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->GPIOWAKEUPENB & WKUPEN31_MASK) >> WKUPEN31_POS);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable to detect a falling edge for VDD Power Toggle wakeup.
 *  @param[in]  Enable   	Set to \b CTRUE to detect a falling edge for VDD Power Toggle wakeup. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@see	    MES_CLKPWR_SetVDDPowerToggleWakeUpEnable,   MES_CLKPWR_GetVDDPowerToggleWakeUpEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpPending, MES_CLKPWR_ClearVDDPowerToggleWakeUpPending
 */
void  	MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable ( CBOOL Enable )
{
    const U32   FALLWKSRC31_POS  = 31;
    const U32   FALLWKSRC31_MASK = 1UL << FALLWKSRC31_POS;

    register U32 temp;

    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    temp = __g_pRegister->GPIOWAKEUPRISEENB;

    temp &= ~FALLWKSRC31_MASK;
    temp |=  (U32)Enable << FALLWKSRC31_POS;

    __g_pRegister->GPIOWAKEUPRISEENB = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get a setting to detect a falling edge for VDD Power Toggle wakeup.
 *	@return	    \b CTRUE indicates a relevant VDD Power Toggle is enabled to detect a falling edge for VDD Power Toggle wakeup.\n
 *				\b CFALSE indicates a relevant VDD Power Toggle is disabled to detect a falling edge for VDD Power Toggle wakeup.
 *	@see	    MES_CLKPWR_SetVDDPowerToggleWakeUpEnable,   MES_CLKPWR_GetVDDPowerToggleWakeUpEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable, 
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpPending, MES_CLKPWR_ClearVDDPowerToggleWakeUpPending
 */
CBOOL 	MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable ( void )
{
    const U32   FALLWKSRC31_POS  = 31;
    const U32   FALLWKSRC31_MASK = 1UL << FALLWKSRC31_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->GPIOWAKEUPRISEENB & FALLWKSRC31_MASK) >> FALLWKSRC31_POS);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable to detect a rising edge for VDD Power Toggle wakeup.
 *  @param[in]  Enable   	Set to \b CTRUE to detect a rising edge for VDD Power Toggle wakeup. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@see	    MES_CLKPWR_SetVDDPowerToggleWakeUpEnable,   MES_CLKPWR_GetVDDPowerToggleWakeUpEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpPending, MES_CLKPWR_ClearVDDPowerToggleWakeUpPending
 */
void  	MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable ( CBOOL Enable )
{
    const U32   RISEWKSRC31_POS  = 31;
    const U32   RISEWKSRC31_MASK = 1UL << RISEWKSRC31_POS;

    register U32 temp;

    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    temp = __g_pRegister->GPIOWAKEUPFALLENB;

    temp &= ~RISEWKSRC31_MASK;
    temp |=  (U32)Enable << RISEWKSRC31_POS;

    __g_pRegister->GPIOWAKEUPFALLENB = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get a setting to detect a rising edge for VDD Power Toggle wakeup.
 *	@return	    \b CTRUE indicates a relevant VDD Power Toggle is enabled to detect a rising edge for VDD Power Toggle wakeup.\n
 *				\b CFALSE indicates a relevant VDD Power Toggle is disabled to detect a rising edge for VDD Power Toggle wakeup.
 *	@see	    MES_CLKPWR_SetVDDPowerToggleWakeUpEnable,   MES_CLKPWR_GetVDDPowerToggleWakeUpEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable, 
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpPending, MES_CLKPWR_ClearVDDPowerToggleWakeUpPending
 */
CBOOL 	MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable ( void )
{
    const U32   RISEWKSRC31_POS  = 31;
    const U32   RISEWKSRC31_MASK = 1UL << RISEWKSRC31_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->GPIOWAKEUPFALLENB & RISEWKSRC31_MASK) >> RISEWKSRC31_POS);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get pending status of VDD Power Toggle wakeup event.
 * 	@return     If relevant VDD Power Toggle event is pended, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@see	    MES_CLKPWR_SetVDDPowerToggleWakeUpEnable,   MES_CLKPWR_GetVDDPowerToggleWakeUpEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable,
 *              MES_CLKPWR_ClearVDDPowerToggleWakeUpPending
 */
CBOOL 	MES_CLKPWR_GetVDDPowerToggleWakeUpPending ( void )
{
    const U32   WKDET31_POS  = 31;
    const U32   WKDET31_MASK = 1UL << WKDET31_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->GPIOPEND & WKDET31_MASK) >> WKDET31_POS);

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Clear pending bit(s) of VDD Power Toggle wakeup event.
 *	@return	    None.
 *	@see	    MES_CLKPWR_SetVDDPowerToggleWakeUpEnable,   MES_CLKPWR_GetVDDPowerToggleWakeUpEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable,
 *              MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable, MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable,
 *              MES_CLKPWR_GetVDDPowerToggleWakeUpPending
 */
void  	MES_CLKPWR_ClearVDDPowerToggleWakeUpPending ( void )
{
    const U32   WKDET31_POS  = 31;

    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->GPIOPEND = ( 1UL << WKDET31_POS );
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->GPIOPEND = 0;
}


//--------------------------------------------------------------------------
// GPIO Wakeup Management
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable GPIO Wakeup.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 *  @param[in]  Enable   	Set to \b CTRUE to enable gpio wakeup. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_GetGPIOWakeUpEnable,
 *				MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable, MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable, MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpPending,        MES_CLKPWR_ClearGPIOWakeUpPending
 */
void  	MES_CLKPWR_SetGPIOWakeUpEnable ( U32 Group, U32 BitNumber, CBOOL Enable )
{
    register U32 temp;
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    temp = __g_pRegister->GPIOWAKEUPENB;

    temp &= ~(1UL << shift);
    temp |= (U32)Enable << shift;

    __g_pRegister->GPIOWAKEUPENB = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get a status of GPIO Wakeup Enable.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 *	@return     If relevant GPIO event is enabled, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_SetGPIOWakeUpEnable,
 *				MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable, MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable, MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpPending,        MES_CLKPWR_ClearGPIOWakeUpPending
 */
CBOOL	MES_CLKPWR_GetGPIOWakeUpEnable ( U32 Group, U32 BitNumber )
{
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    return (CBOOL) ((__g_pRegister->GPIOWAKEUPENB >> shift) & 0x01 );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable to detect a falling edge for GPIO wakeup.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 *  @param[in]  Enable   	Set to \b CTRUE to enable gpio wakeup. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_SetGPIOWakeUpEnable,            MES_CLKPWR_GetGPIOWakeUpEnable,
 *				MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable,    MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpPending,           MES_CLKPWR_ClearGPIOWakeUpPending
 */
void  	MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable ( U32 Group, U32 BitNumber, CBOOL Enable )
{
    register U32 temp;
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    temp = __g_pRegister->GPIOWAKEUPFALLENB;

    temp &= ~(1UL << shift);
    temp |= (U32)Enable << shift;

    __g_pRegister->GPIOWAKEUPFALLENB = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get a setting to detect a falling edge for GPIO wakeup.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 *	@return	    \b CTRUE indicates a relevant GPIO is enabled to detect a falling edge for GPIO wakeup.\n
 *				\b CFALSE indicates a relevant GPIO is disabled to detect a falling edge for GPIO wakeup.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_SetGPIOWakeUpEnable, MES_CLKPWR_GetGPIOWakeUpEnable,
 *				MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable, GetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpPending, MES_CLKPWR_ClearGPIOWakeUpPending
 */
CBOOL 	MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable ( U32 Group, U32 BitNumber )
{
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    return (CBOOL) ((__g_pRegister->GPIOWAKEUPFALLENB >> shift) & 0x01 );

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable to detect a rising edge for GPIO wakeup.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 *  @param[in]  Enable   	Set to \b CTRUE to enable gpio wakeup. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_SetGPIOWakeUpEnable, MES_CLKPWR_GetGPIOWakeUpEnable,
 *				MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable, MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpPending, MES_CLKPWR_ClearGPIOWakeUpPending
 */
void  	MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable ( U32 Group, U32 BitNumber, CBOOL Enable )
{
    register U32 temp;
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    temp = __g_pRegister->GPIOWAKEUPRISEENB;

    temp &= ~(1UL << shift);
    temp |= (U32)Enable << shift;

    __g_pRegister->GPIOWAKEUPRISEENB = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get a setting to detect a rising edge for GPIO wakeup.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 *	@return	    \b CTRUE indicates a relevant GPIO is enabled to detect a rising edge for GPIO wakeup.\n
 *				\b CFALSE indicates a relevant GPIO is disabled to detect a rising edge for GPIO wakeup.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_SetGPIOWakeUpEnable, MES_CLKPWR_GetGPIOWakeUpEnable,
 *				MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable, MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpPending, MES_CLKPWR_ClearGPIOWakeUpPending
 */
CBOOL 	MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable ( U32 Group, U32 BitNumber )
{
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    return (CBOOL) ((__g_pRegister->GPIOWAKEUPRISEENB >> shift) & 0x01 );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get pending status of GPIO wakeup event.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 * 	@return     If relevant GPIO event is pended, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_SetGPIOWakeUpEnable, MES_CLKPWR_GetGPIOWakeUpEnable,
 *				MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable, MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable, MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_ClearGPIOWakeUpPending
 */
CBOOL 	MES_CLKPWR_GetGPIOWakeUpPending ( U32 Group, U32 BitNumber )
{
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    return (CBOOL) ((__g_pRegister->GPIOPEND >> shift) & 0x01 );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Clear pending bit(s) of GPIO wakeup event.
 *  @param[in]  Group       Set GPIO group ( 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber 	bit number, GPIOB( 24 ~ 31 ), GPIOC ( 0 ~ 19 ).
 *	@return	    None.
 *	@remark	    Wakeup control for GPIOB 24 ~ 31 and GPIOC 0 ~ 19
 *	@see	    MES_CLKPWR_SetGPIOWakeUpEnable, MES_CLKPWR_GetGPIOWakeUpEnable,
 *				MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable, MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable,
 *				MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable, MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable,
 *				MES_CLKPWR_GetGPIOWakeUpPending
 */
void  	MES_CLKPWR_ClearGPIOWakeUpPending ( U32 Group, U32 BitNumber )
{
    register U32 shift;

    MES_ASSERT( (1==Group) || (2==Group) );
    MES_ASSERT( (1!=Group) || ( 24 <= BitNumber && BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( 1 == Group )
    {
        shift = BitNumber - 24;
    }
    else
    {
        shift = BitNumber + 8;
    }

    __g_pRegister->GPIOPEND = (1UL << shift );
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->GPIOPEND = 0;
}


    //--------------------------------------------------------------------------
    // RTC Wakeup Management
    //--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable RTC Wakeup.
 *  @param[in]  Enable    Set to \b CTRUE to enable RTC wakeup. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@remark	    Wakeup control for RTC.
 *	@see	    MES_CLKPWR_GetRTCWakeUpEnable
 */
void  	MES_CLKPWR_SetRTCWakeUpEnable ( CBOOL Enable )
{
    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->RTCWAKEUPENB = (U32)Enable;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Get a status of RTC Wakeup Enable.
 *	@return If RTC Wakeup is enabled, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@remark	Wakeup control for RTC.
 *	@see	MES_CLKPWR_SetRTCWakeUpEnable
 */
CBOOL	MES_CLKPWR_GetRTCWakeUpEnable ( void )
{
    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)(__g_pRegister->RTCWAKEUPENB & 0x01);
}


    //--------------------------------------------------------------------------
	// Reset Management
    //--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	Get reset status.
 *	@return	Pended reset status which consists of ORed RESETSTATUS combination.
 *	@remark	This function informs what operation causes a reset.
 *	@see	MES_CLKPWR_ClearResetStatus
 */
U32 	MES_CLKPWR_GetResetStatus ( void )
{
    const U32 RESET_MASK	= 7;
    MES_ASSERT( CNULL != __g_pRegister );
    return (U32)(__g_pRegister->PWRRSTSTATUS & RESET_MASK);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Clear reset status.
 *  @param[in]  rst    	status to be cleared. This value has to consist of
 *						ORed RESETSTATUS combination.\n
 *              		If an argument is void, then clear all status.
 *	@return	    None.
 *	@see	    MES_CLKPWR_ClearResetStatus
 */
void	MES_CLKPWR_ClearResetStatus ( U32 rst )
{
	MES_ASSERT( 8 > rst );
	MES_ASSERT( CNULL != __g_pRegister );

	__g_pRegister->INTPENDSPAD = (rst << 11 );      
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->INTPENDSPAD = 0;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Check whether a POR has occurred or not.
 *	@return	If a POR had occurred, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@see	MES_CLKPWR_ClearPowerOnReset
 */
CBOOL	MES_CLKPWR_CheckPowerOnReset( void )
{
    const U32 POWERONRST_MASK	= (1UL<<0);

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)(__g_pRegister->PWRRSTSTATUS & POWERONRST_MASK);
}

//------------------------------------------------------------------------------
/**
 *	@brief	Check whether a Watchdog reset has occurred or not.
 *	@return	If a Watchdog reset had occurred, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@see	MES_CLKPWR_ClearWatchdogReset
 */
CBOOL	MES_CLKPWR_CheckWatchdogReset( void )
{
    const U32 WATCHDOGRST_POS   = 1;
    const U32 WATCHDOGRST_MASK	= (1UL << WATCHDOGRST_POS);

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->PWRRSTSTATUS & WATCHDOGRST_MASK) >> WATCHDOGRST_POS );
}

//------------------------------------------------------------------------------
/**
 *	@brief	Check whether a GPIOSW(GPIO wakeup or a Software reset) has occurred or not.
 *	@return	If a GPIOSW had occurred, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@see	MES_CLKPWR_ClearGPIOSWReset
 */
CBOOL 	MES_CLKPWR_CheckGPIOSWReset( void )
{
    const U32 GPIORESET_POS   = 2;
    const U32 GPIORESET_MASK	= (1UL << GPIORESET_POS);

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->PWRRSTSTATUS & GPIORESET_MASK) >> GPIORESET_POS );
}


//------------------------------------------------------------------------------
/**
 *	@brief	Check whether a Battery fault has occurred or not.
 *	@return	If a Battery fault had occurred, return \b CTRUE. Otherwise, return \b CFALSE.
 *	@see	MES_CLKPWR_ClearBatteryFaultStatus
 */
CBOOL 	MES_CLKPWR_CheckBatteryFaultStatus( void )
{
    const U32 BATF_POS   = 3;
    const U32 BATF_MASK	= (1UL << BATF_POS);

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->PWRRSTSTATUS & BATF_MASK) >> BATF_POS );
}
//------------------------------------------------------------------------------
/**
 *	@brief	Clear a POR status.
 *	@return	None.
 *	@see	MES_CLKPWR_CheckPowerOnReset
 */
void	MES_CLKPWR_ClearPowerOnReset( void )
{
    const U32 POWERONRST_MASK = 1UL << 11;
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->INTPENDSPAD = POWERONRST_MASK;            
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->INTPENDSPAD = 0;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Clear a Watchdog reset status.
 *	@return	None.
 *	@see	MES_CLKPWR_CheckWatchdogReset
 */
void	MES_CLKPWR_ClearWatchdogReset( void )
{
    const U32 WATCHDOGRST_MASK = 1UL << 12;
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->INTPENDSPAD = WATCHDOGRST_MASK;            
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->INTPENDSPAD = 0;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Clear a GPIOSW(GPIO wakeup or a Software reset) status.
 *	@return	None.
 *	@see	MES_CLKPWR_CheckGPIOSWReset
 */
void	MES_CLKPWR_ClearGPIOSWReset( void )
{
    const U32 GPIORST_MASK = 1UL << 13;
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->INTPENDSPAD = GPIORST_MASK;            
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->INTPENDSPAD = 0;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Clear a Battery fault status.
 *	@return	None.
 *	@see	MES_CLKPWR_CheckBatteryFaultStatus
 */
void	MES_CLKPWR_ClearBatteryFaultStatus( void )
{
    const U32 BATF_MASK = 1UL << 14;
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->INTPENDSPAD = BATF_MASK;            
    // important : You must write 0 after writing 1 on relevant bit(s).     
    __g_pRegister->INTPENDSPAD = 0;
}


//------------------------------------------------------------------------------
/**
 *	@brief	Generate a software reset.
 *	@return	None.
 *	@remark	A reset signal is generated only when the GPIOSWReset is set to CTRUE. \n
 *			While you are on a boot sequence after this reset, you have to clear
 *			a GPIOSWRESET pending by ClearGPIOSWReset or ClearResetStatus.
 *	@see	MES_CLKPWR_SetGPIOSWResetEnable, MES_CLKPWR_GetGPIOSWResetEnable,
 *          MES_CLKPWR_ClearGPIOSWReset, MES_CLKPWR_ClearResetStatus
 */
void 	MES_CLKPWR_DoSoftwareReset( void )
{
    const U32 SWRST_MASK	= (1UL<<12);

    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->PWRMODE |= SWRST_MASK;
}


//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable a reset generated by GPIO Wakeup or Software Reset.
 *  @param[in]  Enable    Set to \b CTRUE to enable a reset. Otherwise, set to \b CFALSE.
 *	@return	    None.
 *	@see	    MES_CLKPWR_SetGPIOSWResetEnable, MES_CLKPWR_GetGPIOSWResetEnable
 */
void  	MES_CLKPWR_SetGPIOSWResetEnable ( CBOOL Enable )
{
    const U32 GPIOSWRSTENB_POS  = 13;
    const U32 GPIOSWRSTENB_MASK	= (1UL<<GPIOSWRSTENB_POS);
    register U32 temp;

    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    temp = __g_pRegister->PWRMODE;

    temp &= ~GPIOSWRSTENB_MASK;
    temp |= (U32)Enable <<  GPIOSWRSTENB_POS;

    __g_pRegister->PWRMODE = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Get a enable status of a reset generated by GPIO Wakeup or Software Reset.
 *	@return	\b CTURE indicates a reset will be generated by GPIO wakeup or Software reset.\n
 *			\b CFALSE indicates a reset will not be generated even though GPIO wakeup or Software reset is issued.
 *	@see	MES_CLKPWR_SetGPIOSWResetEnable
 */
CBOOL 	MES_CLKPWR_GetGPIOSWResetEnable ( void )
{
    const U32 GPIOSWRSTENB_POS  = 13;
    const U32 GPIOSWRSTENB_MASK	= (1UL<<GPIOSWRSTENB_POS);

    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->PWRMODE & GPIOSWRSTENB_MASK) >> GPIOSWRSTENB_POS );
}

//--------------------------------------------------------------------------
// Power Management
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	Get a last power mode before the chip entered into current mode.
 *	@return	a last power mode.
 *	@remark	This function is useful to check a power mode before reset.
 *	@see	MES_CLKPWR_SetCurrentPowerMode, GetCurrentPowerMode
 */
MES_CLKPWR_POWERMODE 	MES_CLKPWR_GetLastPowerMode ( void )
{
    const U32 LASTPWRMODE_POS	= 4UL;
    const U32 LASTPWRMODE_MASK 	= (((1UL<<3)-1)<<LASTPWRMODE_POS);;

    MES_ASSERT( CNULL != __g_pRegister );

    return (MES_CLKPWR_POWERMODE)((__g_pRegister->PWRMODE & LASTPWRMODE_MASK) >> LASTPWRMODE_POS);
}

//------------------------------------------------------------------------------
/**
 *	@brief	Get a current power mode.
 *	@return	a current power mode.
 *	@see	MES_CLKPWR_SetCurrentPowerMode, MES_CLKPWR_GetLastPowerMode
 */
MES_CLKPWR_POWERMODE 	MES_CLKPWR_GetCurrentPowerMode ( void )
{
    const U32 CURPWRMODE_MASK	= ((1UL<<2)-1);

    MES_ASSERT( CNULL != __g_pRegister );

    return (MES_CLKPWR_POWERMODE)(__g_pRegister->PWRMODE & CURPWRMODE_MASK);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set a power mode to be changed.
 *  @param[in]  powermode    a power mode to be changed.
 *	@return	    None.
 *	@remark	    You can select one of NORMAL, IDLE, STOP.
 *	@see	    MES_CLKPWR_GetCurrentPowerMode, MES_CLKPWR_GetLastPowerMode
 */
void    MES_CLKPWR_SetCurrentPowerMode ( MES_CLKPWR_POWERMODE powermode )
{
    const U32 CURPWRMODE_MASK	= ((1UL<<2)-1);
    register U32 temp;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > powermode );
    MES_ASSERT( 0 == MES_CLKPWR_GetResetStatus() );

    temp = __g_pRegister->PWRMODE;

    temp &= ~CURPWRMODE_MASK;
    temp |= (U32)powermode;

    __g_pRegister->PWRMODE = temp;

}


//--------------------------------------------------------------------------
//	PAD Strength Management
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	    Set GPIO Pad's output drive strength(current)
 *  @param[in]  Group       Set gpio group ( 0 : GPIOA, 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber   Set bit number ( GPIOA, GPIOB ( 0 ~ 31 ), GPIOC ( 0 ~ 19 ) )
 *  @param[in]  mA          Set gpio pad's output drive strength(current) ( 2mA, 4mA, 6mA, 8mA )
 *	@return	    None.
 *	@remark	    GPIOC group only can set bit 0 ~ 19.
 *	@see	    MES_CLKPWR_GetGpioPadStrength,
 *              MES_CLKPWR_SetBusPadStrength, MES_CLKPWR_GetBusPadStrength
 */
void    MES_CLKPWR_SetGpioPadStrength( U32 Group, U32 BitNumber, U32 mA )
{
    register U32 regvalue;
    U32 SetmA=0;
    U32 shift;
    U32 SelectReg;

    MES_ASSERT( 3 > Group );
    MES_ASSERT( (0!=Group) || ( BitNumber <= 31 ) );
    MES_ASSERT( (1!=Group) || ( BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( (2==mA) || (4==mA) || (6==mA) || (8==mA) );
    MES_ASSERT( CNULL != __g_pRegister );

    switch( mA )
    {
        case 2: SetmA = 0; break;
        case 4: SetmA = 1; break;
        case 6: SetmA = 2; break;
        case 8: SetmA = 3; break;
        default: MES_ASSERT( CFALSE );
    }

    if( BitNumber >= 16 ){  shift   =   (BitNumber-16) * 2; }
    else                 {  shift   =   BitNumber * 2;      }

    SelectReg = BitNumber/16;


    regvalue = __g_pRegister->PADSTRENGTHGPIO[Group][SelectReg];

    regvalue &= ~( 0x03 << shift );
    regvalue |= SetmA << shift;

    __g_pRegister->PADSTRENGTHGPIO[Group][SelectReg] = regvalue;

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get GPIO Pad's output drive strength(current)
 *  @param[in]  Group       Set gpio group ( 0 : GPIOA, 1 : GPIOB, 2 : GPIOC )
 *  @param[in]  BitNumber   Set bit number ( GPIOA, GPIOB ( 0 ~ 31 ), GPIOC ( 0 ~ 19 ) )
 *	@return     GPIO Pad's output drive strength(current) ( 2mA, 4mA, 6mA, 8mA )
 *	@remark	    GPIOC group only can set bit 0 ~ 19.
 *	@see	    MES_CLKPWR_SetGpioPadStrength,
 *              MES_CLKPWR_SetBusPadStrength, MES_CLKPWR_GetBusPadStrength
 */

U32     MES_CLKPWR_GetGpioPadStrength( U32 Group, U32 BitNumber )
{
    U32 shift;
    U32 SelectReg;
    U32 Value;
    U32 RetValue=0;

    MES_ASSERT( 3 > Group );
    MES_ASSERT( (0!=Group) || ( BitNumber <= 31 ) );
    MES_ASSERT( (1!=Group) || ( BitNumber <= 31 ) );
    MES_ASSERT( (2!=Group) || ( BitNumber <= 19 ) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( BitNumber >= 16 ){  shift   =   (BitNumber-16) * 2; }
    else                 {  shift   =   BitNumber * 2;      }

    SelectReg = BitNumber/16;

    Value = ( __g_pRegister->PADSTRENGTHGPIO[Group][SelectReg] >> shift ) & 0x03;

    switch( Value )
    {
        case 0:  RetValue =  2; break;
        case 1:  RetValue =  4; break;
        case 2:  RetValue =  6; break;
        case 3:  RetValue =  8; break;
        default:            MES_ASSERT( CFALSE );
    }

    return RetValue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set BUS Pad's output drive strength(current)
 *  @param[in]  Bus     Select bus to setting.
 *  @param[in]  mA      Set output drive strenght(current). \n
 *
 *              - MES_CLKPWR_BUSPAD_DDR_CNTL, MES_CLKPWR_BUSPAD_DDR_ADDR, MES_CLKPWR_BUSPAD_DDR_DATA
 *                  - Set to 4mA, 8mA, 10mA, 14mA
 *
 *              - MES_CLKPWR_BUSPAD_DDR_CLK
 *                  - Set to 6mA, 12mA, 18mA, 24mA
 *
 *              - MES_CLKPWR_BUSPAD_STATIC_CNTL, MES_CLKPWR_BUSPAD_STATIC_ADDR, MES_CLKPWR_BUSPAD_STATIC_DATA, MES_CLKPWR_BUSPAD_VSYNC,
 *                 MES_CLKPWR_BUSPAD_HSYNC, MES_CLKPWR_BUSPAD_DE, MES_CLKPWR_BUSPAD_PVCLK,
 *                  - Set to 2mA, 4mA, 6mA, 8mA
 *
 *	@return	    None.
 *	@see	    MES_CLKPWR_SetGpioPadStrength, MES_CLKPWR_GetGpioPadStrength,
 *              MES_CLKPWR_GetBusPadStrength
 */
void    MES_CLKPWR_SetBusPadStrength( MES_CLKPWR_BUSPAD Bus, U32 mA )
{

    register U32 regvalue;
    U32 SetmA=0;
    U32 shift;

    MES_ASSERT( (MES_CLKPWR_BUSPAD_DDR_DATA <= Bus && Bus <= MES_CLKPWR_BUSPAD_DDR_CNTL) ?
                ( ( (4==mA) || (8==mA) || (10==mA) || (14==mA) ) ? CTRUE : CFALSE )
                : CTRUE );

    MES_ASSERT( (MES_CLKPWR_BUSPAD_DDR_CLK == Bus) ?
                ( ( (6==mA) || (12==mA) || (18==mA) || (24==mA) ) ? CTRUE : CFALSE )
                : CTRUE );

    MES_ASSERT( ( Bus <= MES_CLKPWR_BUSPAD_STATIC_CNTL) ?
                ( ( (2==mA) || (4==mA) || (6==mA) || (8==mA) ) ? CTRUE : CFALSE )
                : CTRUE );

    MES_ASSERT( CNULL != __g_pRegister );

    switch( Bus )
    {
        case    MES_CLKPWR_BUSPAD_DDR_CNTL:
        case    MES_CLKPWR_BUSPAD_DDR_ADDR:
        case    MES_CLKPWR_BUSPAD_DDR_DATA:

                switch( mA ){

                    case 4:  SetmA = 0; break;
                    case 8:  SetmA = 1; break;
                    case 10: SetmA = 2; break;
                    case 14: SetmA = 3; break;
                    default:            MES_ASSERT( CFALSE );
                }

                break;

        case    MES_CLKPWR_BUSPAD_DDR_CLK:

                switch( mA ){

                    case 6:  SetmA = 0; break;
                    case 12: SetmA = 1; break;
                    case 18: SetmA = 2; break;
                    case 24: SetmA = 3; break;
                    default:            MES_ASSERT( CFALSE );
                }

                break;

        case MES_CLKPWR_BUSPAD_STATIC_CNTL:
        case MES_CLKPWR_BUSPAD_STATIC_ADDR:
        case MES_CLKPWR_BUSPAD_STATIC_DATA:
        case MES_CLKPWR_BUSPAD_VSYNC      :
        case MES_CLKPWR_BUSPAD_HSYNC      :
        case MES_CLKPWR_BUSPAD_DE         :
        case MES_CLKPWR_BUSPAD_PVCLK      :

                switch( mA ){

                    case 2:  SetmA = 0; break;
                    case 4:  SetmA = 1; break;
                    case 6:  SetmA = 2; break;
                    case 8:  SetmA = 3; break;
                    default:            MES_ASSERT( CFALSE );
                }

                break;

        default:
                MES_ASSERT( CFALSE );
    }

    shift = Bus;

    regvalue = __g_pRegister->PADSTRENGTHBUS;

    regvalue &= ~( 0x03 << shift );
    regvalue |= SetmA << shift;

    __g_pRegister->PADSTRENGTHBUS = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief      Get BUS Pad's output drive strength(current)
 *  @param[in]  Bus Select bus 
 *	@return     BUS Pad's output drive strength(current) ( 2mA ~ 24mA )
 *	@see	    MES_CLKPWR_SetGpioPadStrength, MES_CLKPWR_GetGpioPadStrength,
 *              MES_CLKPWR_SetBusPadStrength
 */
U32     MES_CLKPWR_GetBusPadStrength( MES_CLKPWR_BUSPAD Bus )
{
    U32 Value;
    U32 RetValue=0;
    
    MES_ASSERT( MES_CLKPWR_BUSPAD_DDR_CNTL >= Bus );
    MES_ASSERT( CNULL != __g_pRegister );
    
    Value = ( __g_pRegister->PADSTRENGTHBUS >> Bus ) & 0x03;
    
    switch( Bus )
    {
        case    MES_CLKPWR_BUSPAD_DDR_CNTL:
        case    MES_CLKPWR_BUSPAD_DDR_ADDR:
        case    MES_CLKPWR_BUSPAD_DDR_DATA:

                switch( Value ){

                    case 0:  RetValue =  4; break;
                    case 1:  RetValue =  8; break;
                    case 2:  RetValue =  10; break;
                    case 3:  RetValue =  14; break;
                    default:            MES_ASSERT( CFALSE );
                }

                break;

        case    MES_CLKPWR_BUSPAD_DDR_CLK:

                switch( Value ){

                    case 0: RetValue =   6; break;
                    case 1: RetValue =  12; break;
                    case 2: RetValue =  18; break;
                    case 3: RetValue =  24; break;
                    default:            MES_ASSERT( CFALSE );
                }

                break;

        case MES_CLKPWR_BUSPAD_STATIC_CNTL:
        case MES_CLKPWR_BUSPAD_STATIC_ADDR:
        case MES_CLKPWR_BUSPAD_STATIC_DATA:
        case MES_CLKPWR_BUSPAD_VSYNC      :
        case MES_CLKPWR_BUSPAD_HSYNC      :
        case MES_CLKPWR_BUSPAD_DE         :
        case MES_CLKPWR_BUSPAD_PVCLK      :

                switch( Value ){

                    case 0:  RetValue =  2; break;
                    case 1:  RetValue =  4; break;
                    case 2:  RetValue =  6; break;
                    case 3:  RetValue =  8; break;
                    default:            MES_ASSERT( CFALSE );
                }

                break;

        default:
                MES_ASSERT( CFALSE );
    }    
    
    return RetValue;
}





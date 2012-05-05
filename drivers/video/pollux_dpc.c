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
//	Module     : DPC
//	File       : mes_dpc.c
//	Description:
//	Author     :
//	History    :
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/delay.h>
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

#include <asm/io.h>
#include <asm/uaccess.h>
#include "pollux_dpc.h"

/// @brief  DPC Module's Register List
struct  MES_DPC_RegisterSet
{
	const	 U16 __Reserved00	;	        ///< 0x000
    volatile U16 VENCCTRLA		;	        ///< 0x002 : Video Encoder Control A Register
    volatile U16 VENCCTRLB		;	        ///< 0x004 : Video Encoder Control B Register
    const 	 U16 __Reserved01	;	        ///< 0x006
    volatile U16 VENCSCH		;	        ///< 0x008 : Video Encoder SCH Phase Control Register
    volatile U16 VENCHUE		;	        ///< 0x00A : Video Encoder HUE Phase Control Register
    volatile U16 VENCSAT		;	        ///< 0x00C : Video Encoder Chroma Saturation Control Register
    volatile U16 VENCCRT        ;	        ///< 0x00E : Video Encoder Contrast Control Register
    volatile U16 VENCBRT    	;	        ///< 0x010 : Video Encoder Bright Control Register
    volatile U16 VENCFSCADJH	;	        ///< 0x012 : Video Encoder Color Burst Frequency Adjustment High Register
    volatile U16 VENCFSCADJL	;	        ///< 0x014 : Video Encoder Color Burst Frequency Adjustment Low Register
    const	 U16 __Reserved02[5];	        ///< 0x016 ~ 0x01F
    volatile U16 VENCDACSEL		;           ///< 0x020 : Video Encoder DAC Output Select Register
    const 	 U16 __Reserved03[15];	        ///< 0x022 ~ 0x03F
    volatile U16 VENCICNTL		;	        ///< 0x040 : Video Encoder Timing Configuration Register
    const 	 U16 __Reserved04[3];	        ///< 0x042 ~ 0x047
    volatile U16 VENCHSVSO		;	        ///< 0x048 : Video Encoder Horizontal & Vertical Sync Register
    volatile U16 VENCHSOS		;	        ///< 0x04A : Video Encoder Horizontal Sync Start Register
    volatile U16 VENCHSOE		;	        ///< 0x04C : Video Encoder Horizontal Sync End Register
    volatile U16 VENCVSOS		;	        ///< 0x04E : Video Encoder Vertical Sync Start Register
    volatile U16 VENCVSOE		;	        ///< 0x050 : Video Encoder Vertical Sync End Register
	const	 U16 __Reserved05[21];	        ///< 0x052 ~ 0x07B

	volatile U16 DPCHTOTAL		;	        ///< 0x07C : DPC Horizontal Total Length Register
	volatile U16 DPCHSWIDTH		;	        ///< 0x07E : DPC Horizontal Sync Width Register
	volatile U16 DPCHASTART		;	        ///< 0x080 : DPC Horizontal Active Video Start Register
	volatile U16 DPCHAEND		;	        ///< 0x082 : DPC Horizontal Active Video End Register
	volatile U16 DPCVTOTAL		;	        ///< 0x084 : DPC Vertical Total Length Register
	volatile U16 DPCVSWIDTH		;	        ///< 0x086 : DPC Vertical Sync Width Register
	volatile U16 DPCVASTART		;	        ///< 0x088 : DPC Vertical Active Video Start Register
	volatile U16 DPCVAEND		;	        ///< 0x08A : DPC Vertical Active Video End Register
	volatile U16 DPCCTRL0		;	        ///< 0x08C : DPC Control 0 Register
	volatile U16 DPCCTRL1		;	        ///< 0x08E : DPC Control 1 Register
	volatile U16 DPCEVTOTAL		;	        ///< 0x090 : DPC Even Field Vertical Total Length Register
	volatile U16 DPCEVSWIDTH	;	        ///< 0x092 : DPC Even Field Vertical Sync Width Register
	volatile U16 DPCEVASTART	;	        ///< 0x094 : DPC Even Field Vertical Active Video Start Register
	volatile U16 DPCEVAEND		;	        ///< 0x096 : DPC Even Field Vertical Active Video End Register
	volatile U16 DPCCTRL2		;	        ///< 0x098 : DPC Control 2 Register
	volatile U16 DPCVSEOFFSET	;	        ///< 0x09A : DPC Vertical Sync End Offset Register
	volatile U16 DPCVSSOFFSET	;	        ///< 0x09C : DPC Vertical Sync Start Offset Register
	volatile U16 DPCEVSEOFFSET	;	        ///< 0x09E : DPC Even Field Vertical Sync End Offset Register
	volatile U16 DPCEVSSOFFSET	;	        ///< 0x0A0 : DPC Even Field Vertical Sync Start Offset Register
	volatile U16 DPCDELAY0		;	        ///< 0x0A2 : DPC Delay 0 Register
    volatile U16 DPCUPSCALECON0 ;           ///< 0x0A4 : DPC Sync Upscale Control Register 0
    volatile U16 DPCUPSCALECON1 ;           ///< 0x0A6 : DPC Sync Upscale Control Register 1
    volatile U16 DPCUPSCALECON2 ;           ///< 0x0A8 : DPC Sync Upscale Control Register 2
    volatile U16 DPCRNUMGENCON0 ;           ///< 0x0AA : DPC Sync Random Number Generator Control Register 0
    volatile U16 DPCRNUMGENCON1 ;           ///< 0x0AC : DPC Sync Random Number Generator Control Register 1
    volatile U16 DPCRNUMGENCON2 ;           ///< 0x0AE : DPC Sync Random Number Generator Control Register 2
    volatile U16 DPCRNUMGENFORMULASEL0 ;    ///< 0x0B0 : DPC Sync Random Number Generator Formula Select Low Register
    volatile U16 DPCRNUMGENFORMULASEL1 ;    ///< 0x0B2 : DPC Sync Random Number Generator Formula Select High Register
    volatile U16 DPCFDTADDR     ;           ///< 0x0B4 : DPc Sync Frame Dither Table Address Register
    volatile U16 DPCFRDITHERVALUE;          ///< 0x0B6 : DPC Sync Frame Red Dither Table Register
    volatile U16 DPCFGDITHERVALUE;          ///< 0x0B8 : DPC Sync Frame Green Dither Table Register
    volatile U16 DPCFBDITHERVALUE;          ///< 0x0BA : DPC Sync Frame Blue Dither Table Register
    const U16    __Reserved06[0x82];        ///< 0x0BC ~ 0x1BE
    volatile U32 DPCCLKENB      ;           ///< 0x1C0 : DPC Clock Generation Enable Register
    volatile U32 DPCCLKGEN[2]   ;           ///< 0x1C4, 0x1C8 : DPC Clock Generation Control 0/1 Register

};

static  struct
{
    struct MES_DPC_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_DPC_MODULE] = { CNULL, };

static  U32 __g_CurModuleIndex = 0;
static  struct MES_DPC_RegisterSet *__g_pRegister = CNULL;

//------------------------------------------------------------------------------
// Module Interface
//------------------------------------------------------------------------------
/**
 *  @brief  Initialize of prototype enviroment & local variables.
 *  @return \b CTRUE  indicate that  Initialize is successed.\n
 *          \b CFALSE  Initialize is failed.\n
 *  @see    MES_DPC_SelectModule,
 *          MES_DPC_GetCurrentModule,   MES_DPC_GetNumberOfModule
 */
CBOOL   MES_DPC_Initialize( void )
{
	static CBOOL bInit = CFALSE;
	U32 i;
	
	if( CFALSE == bInit )
	{
		__g_CurModuleIndex = 0;
		
		for( i=0; i < NUMBER_OF_DPC_MODULE; i++ )
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
 *  @see        MES_DPC_Initialize,
 *              MES_DPC_GetCurrentModule,   MES_DPC_GetNumberOfModule
 */
U32    MES_DPC_SelectModule( U32 ModuleIndex )
{
	U32 oldindex = __g_CurModuleIndex;

    MES_ASSERT( NUMBER_OF_DPC_MODULE > ModuleIndex );

    __g_CurModuleIndex = ModuleIndex;
    __g_pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	return oldindex;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get index of current selected module.
 *  @return     Current module's index.
 *  @see        MES_DPC_Initialize,         MES_DPC_SelectModule,
 *              MES_DPC_GetNumberOfModule
 */
U32     MES_DPC_GetCurrentModule( void )
{
    return __g_CurModuleIndex;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get number of modules in the chip.
 *  @return     Module's number.
 *  @see        MES_DPC_Initialize,         MES_DPC_SelectModule,
 *              MES_DPC_GetCurrentModule
 */
U32     MES_DPC_GetNumberOfModule( void )
{
    return NUMBER_OF_DPC_MODULE;
}

//------------------------------------------------------------------------------
// Basic Interface
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *  @brief      Get module's physical address.
 *  @return     Module's physical address
 *  @see        MES_DPC_GetSizeOfRegisterSet,
 *              MES_DPC_SetBaseAddress,       MES_DPC_GetBaseAddress,
 *              MES_DPC_OpenModule,           MES_DPC_CloseModule,
 *              MES_DPC_CheckBusy,            MES_DPC_CanPowerDown
 */
U32     MES_DPC_GetPhysicalAddress( void )
{
    MES_ASSERT( NUMBER_OF_DPC_MODULE > __g_CurModuleIndex );

    return  (U32)( POLLUX_PA_DPC_PRI + (OFFSET_OF_DPC_MODULE * __g_CurModuleIndex) );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a size, in byte, of register set.
 *  @return     Size of module's register set.
 *  @see        MES_DPC_GetPhysicalAddress,
 *              MES_DPC_SetBaseAddress,       MES_DPC_GetBaseAddress,
 *              MES_DPC_OpenModule,           MES_DPC_CloseModule,
 *              MES_DPC_CheckBusy,            MES_DPC_CanPowerDown
 */
U32     MES_DPC_GetSizeOfRegisterSet( void )
{
    return sizeof( struct MES_DPC_RegisterSet );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a base address of register set.
 *  @param[in]  BaseAddress Module's base address
 *  @return     None.
 *  @see        MES_DPC_GetPhysicalAddress,   MES_DPC_GetSizeOfRegisterSet,
 *              MES_DPC_GetBaseAddress,
 *              MES_DPC_OpenModule,           MES_DPC_CloseModule,
 *              MES_DPC_CheckBusy,            MES_DPC_CanPowerDown
 */
void    MES_DPC_SetBaseAddress( U32 BaseAddress )
{
    MES_ASSERT( CNULL != BaseAddress );
    MES_ASSERT( NUMBER_OF_DPC_MODULE > __g_CurModuleIndex );

    __g_ModuleVariables[__g_CurModuleIndex].pRegister = (struct MES_DPC_RegisterSet *)BaseAddress;
    __g_pRegister = (struct MES_DPC_RegisterSet *)BaseAddress;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a base address of register set
 *  @return     Module's base address.
 *  @see        MES_DPC_GetPhysicalAddress,   MES_DPC_GetSizeOfRegisterSet,
 *              MES_DPC_SetBaseAddress,
 *              MES_DPC_OpenModule,           MES_DPC_CloseModule,
 *              MES_DPC_CheckBusy,            MES_DPC_CanPowerDown
 */
U32     MES_DPC_GetBaseAddress( void )
{
    MES_ASSERT( NUMBER_OF_DPC_MODULE > __g_CurModuleIndex );

    return (U32)__g_ModuleVariables[__g_CurModuleIndex].pRegister;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Initialize selected modules with default value.
 *  @return     \b CTRUE  indicate that  Initialize is successed. \n
 *              \b CFALSE  Initialize is failed.
 *  @see        MES_DPC_GetPhysicalAddress,   MES_DPC_GetSizeOfRegisterSet,
 *              MES_DPC_SetBaseAddress,       MES_DPC_GetBaseAddress,
 *              MES_DPC_CloseModule,
 *              MES_DPC_CheckBusy,            MES_DPC_CanPowerDown
 */
CBOOL   MES_DPC_OpenModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Deinitialize selected module to the proper stage.
 *  @return     \b CTRUE  indicate that  Deinitialize is successed. \n
 *              \b CFALSE  Deinitialize is failed.
 *  @see        MES_DPC_GetPhysicalAddress,   MES_DPC_GetSizeOfRegisterSet,
 *              MES_DPC_SetBaseAddress,       MES_DPC_GetBaseAddress,
 *              MES_DPC_OpenModule,
 *              MES_DPC_CheckBusy,            MES_DPC_CanPowerDown
 */
CBOOL   MES_DPC_CloseModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether the selected modules is busy or not.
 *  @return     \b CTRUE  indicate that  Module is Busy. \n
 *              \b CFALSE  Module is NOT Busy.
 *  @see        MES_DPC_GetPhysicalAddress,   MES_DPC_GetSizeOfRegisterSet,
 *              MES_DPC_SetBaseAddress,       MES_DPC_GetBaseAddress,
 *              MES_DPC_OpenModule,           MES_DPC_CloseModule,
 *              MES_DPC_CanPowerDown
 */
CBOOL   MES_DPC_CheckBusy( void )
{
	return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicaes whether the selected modules is ready to enter power-down stage
 *  @return     \b CTRUE  indicate that  Ready to enter power-down stage. \n
 *              \b CFALSE  This module can't enter to power-down stage.
 *  @see        MES_DPC_GetPhysicalAddress,   MES_DPC_GetSizeOfRegisterSet,
 *              MES_DPC_SetBaseAddress,       MES_DPC_GetBaseAddress,
 *              MES_DPC_OpenModule,           MES_DPC_CloseModule,
 *              MES_DPC_CheckBusy
 */
CBOOL   MES_DPC_CanPowerDown( void )
{
    return CTRUE;
}


//------------------------------------------------------------------------------
// Interrupt Interface
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number for interrupt controller.
 *  @return     Interrupt number
 *  @see        MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
S32     MES_DPC_GetInterruptNumber( void )
{
    static const U32 IntNumDPC[NUMBER_OF_DPC_MODULE] = { INTNUM_OF_DPC0_MODULE, INTNUM_OF_DPC1_MODULE };

    MES_ASSERT( NUMBER_OF_DPC_MODULE > __g_CurModuleIndex );

    return  IntNumDPC[__g_CurModuleIndex];
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a specified interrupt to be enable or disable.
 *  @param[in]  IntNum  Interrupt Number ( 0 ).
 *  @param[in]  Enable  \b CTRUE  indicate that  Interrupt Enable. \n
 *                      \b CFALSE  Interrupt Disable.
 *  @return     None.
 *  @remarks    DPC Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_DPC_GetInterruptNumber,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
void    MES_DPC_SetInterruptEnable( S32 IntNum, CBOOL Enable )
{
    const U32   INTENB_POS = 11;
    const U32   INTENB_MASK = 1UL << INTENB_POS;
    const U32   INTPEND_POS = 10;
    const U32   INTPEND_MASK = 1UL << INTPEND_POS;

    register U32 regvalue;
    register struct MES_DPC_RegisterSet*    pRegister;
/*
    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );
*/
    pRegister = __g_pRegister;

    regvalue = pRegister->DPCCTRL0;

    regvalue &= ~(INTENB_MASK | INTPEND_MASK);
    regvalue |= (U32)Enable << INTENB_POS;
    
    pRegister->DPCCTRL0 = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is enabled or disabled.
 *  @param[in]  IntNum  Interrupt Number.
 *  @return     \b CTRUE  indicate that  Interrupt is enabled. \n
 *              \b CFALSE  Interrupt is disabled.
 *  @remarks    DPC Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
CBOOL   MES_DPC_GetInterruptEnable( S32 IntNum )
{
    const U32   INTENB_POS = 11;
    const U32   INTENB_MASK = 1UL << INTENB_POS;

    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->DPCCTRL0 & INTENB_MASK) >> INTENB_POS );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is pended or not
 *  @param[in]  IntNum  Interrupt Number.
 *  @return     \b CTRUE  indicate that  Pending is seted. \n
 *              \b CFALSE  Pending is Not Seted.
 *  @remarks    DPC Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
CBOOL   MES_DPC_GetInterruptPending( S32 IntNum )
{
    const U32   INTPEND_POS = 10;
    const U32   INTPEND_MASK = 1UL << INTPEND_POS;

    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->DPCCTRL0 & INTPEND_MASK) >> INTPEND_POS );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear a pending state of specified interrupt.
 *  @param[in]  IntNum  Interrupt number.
 *  @return     None.
 *  @remarks    DPC Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
void    MES_DPC_ClearInterruptPending( S32 IntNum )
{
    const U32   INTPEND_POS = 10;

    register struct MES_DPC_RegisterSet*    pRegister;

   // MES_ASSERT( 0 == IntNum );
   // MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    pRegister->DPCCTRL0 |= 1UL << INTPEND_POS;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set all interrupts to be enables or disables.
 *  @param[in]  Enable  \b CTRUE  indicate that  Set to all interrupt enable. \n
 *                      \b CFALSE  Set to all interrupt disable.
 *  @return     None.
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
void    MES_DPC_SetInterruptEnableAll( CBOOL Enable )
{
    const U32   INTENB_POS = 11;
    const U32   INTENB_MASK = 1UL << INTENB_POS;
    const U32   INTPEND_POS = 10;
    const U32   INTPEND_MASK = 1UL << INTPEND_POS;

    register U32 regvalue;
    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    regvalue = pRegister->DPCCTRL0;

    regvalue &= ~(INTENB_MASK | INTPEND_MASK);
    regvalue |= (U32)Enable << INTENB_POS;
    
     
    pRegister->DPCCTRL0 = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are enable or not.
 *  @return     \b CTRUE  indicate that  At least one( or more ) interrupt is enabled. \n
 *              \b CFALSE  All interrupt is disabled.
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
CBOOL   MES_DPC_GetInterruptEnableAll( void )
{
    const U32   INTENB_POS = 11;
    const U32   INTENB_MASK = 1UL << INTENB_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->DPCCTRL0 & INTENB_MASK) >> INTENB_POS );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are pended or not.
 *  @return     \b CTRUE  indicate that  At least one( or more ) pending is seted. \n
 *              \b CFALSE  All pending is NOT seted.
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,
 *              MES_DPC_ClearInterruptPendingAll, MES_DPC_GetInterruptPendingNumber
 */
CBOOL   MES_DPC_GetInterruptPendingAll( void )
{
    const U32   INTPEND_POS = 10;
    const U32   INTPEND_MASK = 1UL << INTPEND_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->DPCCTRL0 & INTPEND_MASK) >> INTPEND_POS );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear pending state of all interrupts.
 *  @return     None.
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_GetInterruptPendingNumber
 */
void    MES_DPC_ClearInterruptPendingAll( void )
{
    const U32   INTPEND_POS = 10;

    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    pRegister->DPCCTRL0 |= 1UL << INTPEND_POS;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number which has the most prority of pended interrupts
 *  @return     Pending Number( If all pending is not set then return -1 ).
 *  @see        MES_DPC_GetInterruptNumber,       MES_DPC_SetInterruptEnable,
 *              MES_DPC_GetInterruptEnable,       MES_DPC_GetInterruptPending,
 *              MES_DPC_ClearInterruptPending,    MES_DPC_SetInterruptEnableAll,
 *              MES_DPC_GetInterruptEnableAll,    MES_DPC_GetInterruptPendingAll,
 *              MES_DPC_ClearInterruptPendingAll
 */
S32     MES_DPC_GetInterruptPendingNumber( void )  // -1 if None
{
    const U32   INTPEND_POS = 10;
    const U32   INTPEND_MASK = 1UL << INTPEND_POS;

    MES_ASSERT( CNULL != __g_pRegister );

   if( __g_pRegister->DPCCTRL0 & INTPEND_MASK )
   {
       return 0;
   }

   return -1;
}

//------------------------------------------------------------------------------
// Clock Control Interface
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Set a PCLK mode
 *  @param[in]  mode    PCLK mode
 *  @return     None.
 *  @see        MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
void            MES_DPC_SetClockPClkMode( MES_PCLKMODE mode )
{
    const U32 PCLKMODE_POS  =   3;

    register U32 regvalue;
    register struct MES_DPC_RegisterSet* pRegister;

	U32 clkmode=0;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

	switch(mode)
	{
    	case MES_PCLKMODE_DYNAMIC:  clkmode = 0;		break;
    	case MES_PCLKMODE_ALWAYS:  	clkmode = 1;		break;
    	default: MES_ASSERT( CFALSE );
	}

    regvalue = pRegister->DPCCLKENB;

    regvalue &= ~(1UL<<PCLKMODE_POS);
    regvalue |= ( clkmode & 0x01 ) << PCLKMODE_POS;

    pRegister->DPCCLKENB = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get current PCLK mode
 *  @return     Current PCLK mode
 *  @see        MES_DPC_SetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
MES_PCLKMODE    MES_DPC_GetClockPClkMode( void )
{
    const U32 PCLKMODE_POS  = 3;

    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->DPCCLKENB & ( 1UL << PCLKMODE_POS ) )
    {
        return MES_PCLKMODE_ALWAYS;
    }

    return  MES_PCLKMODE_DYNAMIC;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock source of clock generator
 *  @param[in]  Index   Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @param[in]  ClkSrc  Select clock source of clock generator.\n
 *                      0:PLL0, 1:PLL1, 2:None, 3:P(S)VCLK, 4:~P(S)VCLK, 5:XTI \n
 *                      6:None, 7:ClKGEN0's Output( Only use Clock generator1 )
 *  @return     None.
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.\n
 *              Only Clock generator 1 can set to ClkGEN0's output.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
void    MES_DPC_SetClockSource( U32 Index, U32 ClkSrc )
{
    const U32 CLKSRCSEL_POS    = 1;
    const U32 CLKSRCSEL_MASK   = 0x07 << CLKSRCSEL_POS;

    register U32 ReadValue;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( (0!=Index) || ( (2!=ClkSrc) && (ClkSrc<=5) ) );
    MES_ASSERT( (1!=Index) || ( (2!=ClkSrc) && (6!=ClkSrc) && (ClkSrc<=7) ) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue = __g_pRegister->DPCCLKGEN[Index];

    ReadValue &= ~CLKSRCSEL_MASK;
    ReadValue |= ClkSrc << CLKSRCSEL_POS;

    __g_pRegister->DPCCLKGEN[Index] = ReadValue;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock source of specified clock generator.
 *  @param[in]  Index   Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @return     Clock source of clock generator \n
 *              0:PLL0, 1:PLL1, 2:None, 3:P(S)VCLK, 4:~P(S)VCLK, 5:XTI \n
 *              6:None, 7:ClKGEN0's Output( Only use Clock generator1 )
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
U32             MES_DPC_GetClockSource( U32 Index )
{
    const U32 CLKSRCSEL_POS    = 1;
    const U32 CLKSRCSEL_MASK   = 0x07 << CLKSRCSEL_POS;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return ( __g_pRegister->DPCCLKGEN[Index] & CLKSRCSEL_MASK ) >> CLKSRCSEL_POS;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock divisor of specified clock generator.
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @param[in]  Divisor     Clock divisor ( 1 ~ 64 ).
 *  @return     None.
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
void            MES_DPC_SetClockDivisor( U32 Index, U32 Divisor )
{
    const U32 CLKDIV_POS   =   4;
    const U32 CLKDIV_MASK  =   ((1<<6)-1) << CLKDIV_POS;

    register U32 ReadValue;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( 1 <= Divisor && Divisor <= 64 );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->DPCCLKGEN[Index];

    ReadValue   &= ~CLKDIV_MASK;
    ReadValue   |= (Divisor-1) << CLKDIV_POS;

    __g_pRegister->DPCCLKGEN[Index] = ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock divisor of specified clock generator.
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @return     Clock divisor ( 1 ~ 64 ).
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
U32             MES_DPC_GetClockDivisor( U32 Index )
{
    const U32 CLKDIV_POS   =   4;
    const U32 CLKDIV_MASK  =   ((1<<6)-1) << CLKDIV_POS;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return ((__g_pRegister->DPCCLKGEN[Index] & CLKDIV_MASK) >> CLKDIV_POS) + 1;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Set inverting of output clock
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @param[in]  OutClkInv   \b CTRUE indicate that Output clock Invert (Rising Edge). \n
 *                          \b CFALSE indicate that Output clock Normal (Fallng Edge).
 *  @return     None.
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
void            MES_DPC_SetClockOutInv( U32 Index, CBOOL OutClkInv )
{
    const U32 OUTCLKINV_POS    =   0;
    const U32 OUTCLKINV_MASK   =   1UL << OUTCLKINV_POS;

    register U32 ReadValue;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( (0==OutClkInv) ||(1==OutClkInv) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =    __g_pRegister->DPCCLKGEN[Index];

    ReadValue   &=  ~OUTCLKINV_MASK;
    ReadValue   |=  OutClkInv << OUTCLKINV_POS;

    __g_pRegister->DPCCLKGEN[Index]    =   ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get invert status of output clock.
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 )
 *  @return     \b CTRUE indicate that Output clock is Inverted. \n
 *              \b CFALSE indicate that Output clock is Normal.
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
CBOOL           MES_DPC_GetClockOutInv( U32 Index )
{
    const U32 OUTCLKINV_POS    =   0;
    const U32 OUTCLKINV_MASK   =   1UL << OUTCLKINV_POS;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->DPCCLKGEN[Index] & OUTCLKINV_MASK ) >> OUTCLKINV_POS);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock output pin's direction.
 *  @param[in]  Index       Select clock generator( 0: clock generator0 );
 *  @param[in]  OutClkEnb   \b CTRUE indicate that Pin's direction is Output.\n
 *                          \b CFALSE indicate that Pin's direction is Input.
 *  @return     None.
 *  @remarks    Decides the I/O direction when the output clock is connected to a bidirectional PAD.\n
 *              Only clock generator 0 can set the output pin's direction.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
void            MES_DPC_SetClockOutEnb( U32 Index, CBOOL OutClkEnb )
{
    const U32   OUTCLKENB_POS  = 15;
    const U32   OUTCLKENB_MASK = 1UL << OUTCLKENB_POS;

    register U32 ReadValue;

    MES_ASSERT( 0 == Index );
    MES_ASSERT( (0==OutClkEnb) ||(1==OutClkEnb) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->DPCCLKGEN[Index];

    ReadValue   &=  ~OUTCLKENB_MASK;

    if( ! OutClkEnb )
    {
           ReadValue    |=  1UL << OUTCLKENB_POS;
    }

    __g_pRegister->DPCCLKGEN[Index] = ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock output pin's direction.
 *  @param[in]  Index       Select clock generator( 0: clock generator0 );
 *  @return     \b CTRUE indicate that Pin's direction is Output. \n
 *              \b CFALSE indicate that Pin's direction is Input. 
 *  @remarks    Only clock generator 0 can set the output pin's direction. so \e Index must set to 0.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
CBOOL           MES_DPC_GetClockOutEnb( U32 Index )
{
    const U32   OUTCLKENB_POS  = 15;
    const U32   OUTCLKENB_MASK = 1UL << OUTCLKENB_POS;

    MES_ASSERT( 0 == Index );
    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->DPCCLKGEN[Index] & OUTCLKENB_MASK )
    {
        return CFALSE;
    }

    return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock output delay of specifed clock generator
 *  @param[in]  Index   Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @param[in]  delay   Select clock output delay of clock generator.\n
 *                      0:0ns, 1:0.5ns, 2:1.0ns, 3:1.5ns, 4:2.0ns, 5:2.5ns, \n
 *                      6:3.0ns 7:3.5ns
 *  @return     None.
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.\n
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *                                              MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
void            MES_DPC_SetClockOutDelay( U32 Index, U32 delay )
{
    const U32 OUTCLKDELAY_POS = 12;
    const U32 OUTCLKDELAY_MASK = 0x07 << OUTCLKDELAY_POS ;

    register U32 ReadValue;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( 8 > delay );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue = __g_pRegister->DPCCLKGEN[Index];

    ReadValue &= ~OUTCLKDELAY_MASK;
    ReadValue |= (U32)delay << OUTCLKDELAY_POS;

    __g_pRegister->DPCCLKGEN[Index] = ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock output delay of specifed clock generator
 *  @param[in]  Index   Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @return     Get clock output delay of specifed clock generator.\n
 *              0:0ns, 1:0.5ns, 2:1.0ns, 3:1.5ns, 4:2.0ns, 5:2.5ns, \n
 *              6:3.0ns 7:3.5ns
 *  @return     None.
 *  @remarks    DPC controller have two clock generator. so \e Index must set to 0 or 1.\n
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable,  MES_DPC_GetClockDivisorEnable
 */
U32             MES_DPC_GetClockOutDelay( U32 Index )
{
    const U32 OUTCLKDELAY_POS = 12;
    const U32 OUTCLKDELAY_MASK = 0x07 << OUTCLKDELAY_POS ;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return ((__g_pRegister->DPCCLKGEN[Index] & OUTCLKDELAY_MASK) >> OUTCLKDELAY_POS);
}


//------------------------------------------------------------------------------
/**
 *  @brief      Set clock generator's operation
 *  @param[in]  Enable  \b CTRUE indicate that Enable of clock generator. \n
 *                      \b CFALSE indicate that Disable of clock generator.
 *  @return     None.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_GetClockDivisorEnable
 */
void            MES_DPC_SetClockDivisorEnable( CBOOL Enable )
{
    const U32   CLKGENENB_POS   =   2;
    const U32   CLKGENENB_MASK  =   1UL << CLKGENENB_POS;

    register U32 ReadValue;

    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->DPCCLKENB;

    ReadValue   &=  ~CLKGENENB_MASK;
    ReadValue   |= (U32)Enable << CLKGENENB_POS;

    __g_pRegister->DPCCLKENB   =   ReadValue;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Get status of clock generator's operation
 *  @return     \b CTRUE indicate that Clock generator is enabled. \n
 *              \b CFALSE indicate that Clock generator is disabled.
 *  @see        MES_DPC_SetClockPClkMode,       MES_DPC_GetClockPClkMode,
 *              MES_DPC_SetClockBClkMode,       MES_DPC_GetClockBClkMode,
 *              MES_DPC_SetClockSource,         MES_DPC_GetClockSource,
 *              MES_DPC_SetClockDivisor,        MES_DPC_GetClockDivisor,
 *              MES_DPC_SetClockOutInv,         MES_DPC_GetClockOutInv,
 *              MES_DPC_SetClockOutEnb,         MES_DPC_GetClockOutEnb,
 *              MES_DPC_SetClockOutDelay,       MES_DPC_GetClockOutDelay,
 *              MES_DPC_SetClockDivisorEnable
 */
CBOOL           MES_DPC_GetClockDivisorEnable( void )
{
    const U32   CLKGENENB_POS   =   1;
    const U32   CLKGENENB_MASK  =   1UL << CLKGENENB_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->DPCCLKENB & CLKGENENB_MASK) >> CLKGENENB_POS );
}


//--------------------------------------------------------------------------
//	Display controller operations
//--------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable Display controller.
 *  @param[in]  bEnb    Set it to CTRUE to enable display controller.
 *	@return     None.
 *	@see	    MES_DPC_GetDPCEnable
 */
void	MES_DPC_SetDPCEnable( CBOOL bEnb )
{
    const U32 INTPEND_POS   = 10;
    const U32 INTPEND_MASK  = 1UL << INTPEND_POS;

    const U32 DPCENB_POS = 15;
    const U32 DPCENB_MASK = 1UL << DPCENB_POS;

    register struct MES_DPC_RegisterSet* pRegister;
    register U32 ReadValue;

    MES_ASSERT( (0==bEnb) ||(1==bEnb) );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister   =   __g_pRegister;

    ReadValue   =   pRegister->DPCCTRL0;

    ReadValue   &=  ~(INTPEND_MASK|DPCENB_MASK);
    ReadValue   |=  (U32)bEnb << DPCENB_POS;

    pRegister->DPCCTRL0  =   ReadValue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Informs whether DPC is enabled or disabled.
 *	@return     CTRUE indicates DPC is enabled.\n
 *			    CFALSE indicates DPC is disabled.
 *	@see	    MES_DPC_SetDPCEnable
 */
CBOOL	MES_DPC_GetDPCEnable( void )
{
    const U32 DPCENB_POS = 15;
    const U32 DPCENB_MASK = 1UL << DPCENB_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->DPCCTRL0 & DPCENB_MASK) >> DPCENB_POS );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set delay values for data and sync signals.
 *  @param[in]  DelayRGB_PVD  Specifies the delay value for RGB/PVD signal, 0 ~ 15.
 *  @param[in]  DelayHS_CP1	  Specifies the delay value for HSYNC/CP1 signal, 0 ~ 15.
 *  @param[in]  DelayVS_FRAM  Specifies the delay value for VSYNC/FRAM signal, 0 ~ 15.
 *  @param[in]  DelayDE_CP2	  Specifies the delay value for DE/CP2 signal, 0 ~ 15.
 *	@return     None.
 *	@remarks    Set delay value for TFT/STN LCD's data and sync signal.\n
 *              \b TFT \b LCD \n
 *              The delay valus for data is generally '0' for normal operation.
 * 			    but the delay values for sync signals depend on the output format.
 *			    The unit is VCLK2.\n
 *			    The setting values for normal operation is as follows,
 *	@code
 *	  +---------------------+----------+------------------------------+
 *	  |       FORMAT        | DelayRGB | DelayHS, VS, DE              |
 *	  +---------------------+----------+------------------------------+
 *	  | RGB                 |    0     |              4               |
 *	  +---------------------+----------+------------------------------+
 *	  | MRGB                |    0     |              8               |
 *	  +---------------------+----------+------------------------------+
 *	  | ITU-R BT.601A       |    0     |              6               |
 *	  +---------------------+----------+------------------------------+
 *	  | ITU-R BT.656 / 601B |    0     |             12               |
 *	  +---------------------+----------+------------------------------+
 *	@endcode
 *  @remarks    \b STN \b LCD \n
 *              The delay valus for data is generally '0' for normal operation.
 * 			    but the delay values for sync signals depend on the output format. \n
 *			    The unit is VCLK. \n
 *              Specifies the delay value for PVD, CP1, FRAM, CP2. \n
 *              Each signal more explained to MES_DPC_SetSTNHSync(), MES_DPC_SetSTNVSync() function.
 *	@code
 *	  +---------------------+---------------+------------------------------+
 *	  |       FORMAT        | DelayRGB(PVD) | Delay CP1, FRAM, CP2         |
 *	  +---------------------+---------------+------------------------------+
 *	  | 4096COLOR/16GRAY    |      0        |              8               |
 *	  +---------------------+---------------+------------------------------+
 *	@endcode
 *	@see	    MES_DPC_GetDelay, MES_DPC_SetSTNHSync, MES_DPC_SetSTVHSync
 */
void	MES_DPC_SetDelay( U32 DelayRGB_PVD, U32 DelayHS_CP1, U32 DelayVS_FRAM, U32 DelayDE_CP2 )
{
	const U16 INTPEND_MASK	= 1U<<10;
	const U16 DELAYRGB_POS	= 4;
	const U16 DELAYRGB_MASK	= 0xFU<<DELAYRGB_POS;
	register U16 temp;

	const U16 DELAYDE_POS	= 8;
	const U16 DELAYVS_POS	= 4;
	const U16 DELAYHS_POS	= 0;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( 16 > DelayRGB_PVD );
	MES_ASSERT( 16 > DelayHS_CP1 );
	MES_ASSERT( 16 > DelayVS_FRAM );
	MES_ASSERT( 16 > DelayDE_CP2 );

	temp = __g_pRegister->DPCCTRL0;
	temp &= (U16)~(INTPEND_MASK | DELAYRGB_MASK);	// unmask intpend & DELAYRGB bits.
	temp = (U16)(temp | (DelayRGB_PVD<<DELAYRGB_POS));
	__g_pRegister->DPCCTRL0 = temp;

	__g_pRegister->DPCDELAY0 	= (U16)((DelayDE_CP2<<DELAYDE_POS) | (DelayVS_FRAM<<DELAYVS_POS) | (DelayHS_CP1<<DELAYHS_POS));

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get current delay values for data and sync signals.
 *  @param[out] pDelayRGB_PVD	 Get current delay value for RGB/PVD signal.
 *  @param[out] pDelayHS_CP1	 Get current delay value for HSYNC/CP1 signal.
 *  @param[out] pDelayVS_FRAM	 Get current delay value for VSYNC/FRAM signal.
 *  @param[out] pDelayDE_CP2	 Get current delay value for DE/CP2 signal.
 *	@return	    None.
 *	@remarks    Arguments which does not required can be CNULL.\n
 *              \b TFT \b LCD
 *                  - Signal is RGB, HSYNC, VSYNC, DE.
 *			        - The unit is VCLK2.
 *
 *  @remarks   \b STN \b LCD
 *                  - Signal is PVD, CP1, FRAM, CP2.
 *			        - The unit is VCLK.
 *	@see	    MES_DPC_SetDelay
 */
void	MES_DPC_GetDelay( U32 *pDelayRGB_PVD, U32 *pDelayHS_CP1, U32 *pDelayVS_FRAM, U32 *pDelayDE_CP2 )
{
	const U16 DELAYRGB_POS	= 4;
	const U16 DELAYRGB_MASK	= 0xFU<<DELAYRGB_POS;

	const U16 DELAYDE_POS	= 8;
	const U16 DELAYDE_MASK	= 0xFU<<DELAYDE_POS;
	const U16 DELAYVS_POS	= 4;
	const U16 DELAYVS_MASK	= 0xFU<<DELAYVS_POS;
	const U16 DELAYHS_POS	= 0;
	const U16 DELAYHS_MASK	= 0xFU<<DELAYHS_POS;

	register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );

	temp = __g_pRegister->DPCCTRL0;
	if( CNULL != pDelayRGB_PVD )	*pDelayRGB_PVD	= (U32)((temp & DELAYRGB_MASK)>>DELAYRGB_POS);

	temp = __g_pRegister->DPCDELAY0;
	if( CNULL != pDelayHS_CP1  )	*pDelayHS_CP1	= (U32)((temp & DELAYHS_MASK )>>DELAYHS_POS );
	if( CNULL != pDelayVS_FRAM  )	*pDelayVS_FRAM	= (U32)((temp & DELAYVS_MASK )>>DELAYVS_POS );
	if( CNULL != pDelayDE_CP2  )	*pDelayDE_CP2	= (U32)((temp & DELAYDE_MASK )>>DELAYDE_POS );
}


//------------------------------------------------------------------------------
/**
 *	@brief	    Set RGB dithering mode.
 *  @param[in]  DitherR	  Specifies the dithering mode for Red component.
 *  @param[in]  DitherG	  Specifies the dithering mode for Green component.
 *  @param[in]  DitherB	  Specifies the dithering mode for Blue component.
 *	@return	    None.
 *	@remark	    The dithering is useful method for case which is that the color
 *			    depth of destination is less than one of source.
 *	@see	    MES_DPC_GetDither
 */
void	MES_DPC_SetDither( MES_DPC_DITHER DitherR, MES_DPC_DITHER DitherG, MES_DPC_DITHER DitherB )
{
	const U16 DITHER_MASK	= 0x3FU;
	const U16 RDITHER_POS	= 0;
	const U16 GDITHER_POS	= 2;
	const U16 BDITHER_POS	= 4;
	register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( 4 > DitherR );
	MES_ASSERT( 4 > DitherG );
	MES_ASSERT( 4 > DitherB );

	temp = __g_pRegister->DPCCTRL1;
	temp &= (U16)~DITHER_MASK;	// unmask dithering mode.
	temp = (U16)(temp | ((DitherB<<BDITHER_POS) | (DitherG<<GDITHER_POS) | (DitherR<<RDITHER_POS)));
	__g_pRegister->DPCCTRL1 = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get current RGB dithering mode.
 *  @param[out] pDitherR	 Get current dithering mode for Red component.
 *  @param[out] pDitherG	 Get current dithering mode for Green component.
 *  @param[out] pDitherB	 Get current dithering mode for Blue component.
 *	@return	    None.
 *	@remark	    Arguments which does not required can be CNULL.
 *	@see	    MES_DPC_SetDither
 */
void	MES_DPC_GetDither( MES_DPC_DITHER *pDitherR, MES_DPC_DITHER *pDitherG, MES_DPC_DITHER *pDitherB )
{
	const U16 RDITHER_POS	= 0;
	const U16 RDITHER_MASK	= 0x3U<<RDITHER_POS;
	const U16 GDITHER_POS	= 2;
	const U16 GDITHER_MASK	= 0x3U<<GDITHER_POS;
	const U16 BDITHER_POS	= 4;
	const U16 BDITHER_MASK	= 0x3U<<BDITHER_POS;
	register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );

	temp = __g_pRegister->DPCCTRL1;
	if( CNULL != pDitherR )		*pDitherR	= (MES_DPC_DITHER)((temp & RDITHER_MASK)>>RDITHER_POS);
	if( CNULL != pDitherG )		*pDitherG	= (MES_DPC_DITHER)((temp & GDITHER_MASK)>>GDITHER_POS);
	if( CNULL != pDitherB )		*pDitherB	= (MES_DPC_DITHER)((temp & BDITHER_MASK)>>BDITHER_POS);
}



//--------------------------------------------------------------------------
// TFT LCD specific control function
//--------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *	@brief	    Set display mode.
 *  @param[in] format		  Specifies data format.

 *  @param[in] bInterlace	  Specifies scan mode.\n
 *		       				  CTRUE = Interface, CFALSE = Progressive.
 *  @param[in] bInvertField	  Specifies internal field polarity.\n
 *		       				  CTRUE = Invert Field(Low is even field), CFALSE = Normal Field(Low is odd field).

 *  @param[in] bRGBMode		  Specifies pixel format.\n
 *		       				  CTRUE = RGB Mode, CFASE = YCbCr Mode.
 *  @param[in] bSwapRB		  Swap Red and Blue component for RGB output.\n
 *	           				  CTRUE = Swap Red and Blue, CFALSE = No swap.

 *  @param[in] ycorder		  Specifies output order for YCbCr Output.
 *  @param[in] bClipYC		  Specifies output range of RGB2YC.\n
 *	           				  CTRUE = Y(16 ~ 235), Cb/Cr(16 ~ 240), CFALSE = Y/Cb/Cr(0 ~ 255).\n
 *		       				  You have to set to CTRUE for ITU-R BT.656 and internal video encoder.
 *  @param[in] bEmbeddedSync  Specifies embedded sync mode(SAV/EAV).\n
 *             				  CTRUE = Enable, CFALSE = Disable.\n
 *		       				  You have to set to CTRUE for ITU-R BT.656.

 *  @param[in] clock		  Specifies the PAD output clock.
 *  @param[in] bInvertClock	  Sepcifies the pixel clock polarity.\n
 *		       				  CTRUE = rising edge, CFALSE = falling edge.
 *	@return     None.
 *	@see	    MES_DPC_GetMode
 */
void	MES_DPC_SetMode( MES_DPC_FORMAT format,
    					 CBOOL bInterlace, CBOOL bInvertField,
    					 CBOOL bRGBMode, CBOOL bSwapRB,
	        			 MES_DPC_YCORDER ycorder, CBOOL bClipYC, CBOOL bEmbeddedSync,
			        	 MES_DPC_PADCLK clock, CBOOL bInvertClock )
{
	// DPC Control 0 register
	const U16 POLFIELD_POS	= 2;
	const U16 SEAVENB_POS	= 8;
	const U16 SCANMODE_POS	= 9;
	const U16 INTPEND_POS	= 10;
	const U16 RGBMODE_POS	= 12;

	// DPC Control 1 register
	const U16 DITHER_MASK	= 0x3F;
	const U16 YCORDER_POS	= 6;
	const U16 FORMAT_POS	= 8;
	const U16 YCRANGE_POS	= 13;
	const U16 SWAPRB_POS	= 15;

	// DPC Control 2 register
	const U16 PADCLKSEL_POS		= 0;
	const U16 PADCLKSEL_MASK	= 1U<<PADCLKSEL_POS;
    const U16 LCDTYPE_POS       = 8;
    const U16 LCDTYPE_MASK      = 1U<<LCDTYPE_POS;

	register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( 14 > format );
	MES_ASSERT( 4 > ycorder );
	MES_ASSERT( 2 > clock );

	temp = __g_pRegister->DPCCTRL0;
	temp &= (U16)~(1U<<INTPEND_POS);	// unmask intpend bit.
	if( bInterlace )	temp |= (U16) (1U<<SCANMODE_POS);
	else				temp &= (U16)~(1U<<SCANMODE_POS);
	if( bInvertField )	temp |= (U16) (1U<<POLFIELD_POS);
	else				temp &= (U16)~(1U<<POLFIELD_POS);
	if( bRGBMode )		temp |= (U16) (1U<<RGBMODE_POS);
	else				temp &= (U16)~(1U<<RGBMODE_POS);
	if( bEmbeddedSync )	temp |= (U16) (1U<<SEAVENB_POS);
	else				temp &= (U16)~(1U<<SEAVENB_POS);
	__g_pRegister->DPCCTRL0 = temp;

	temp = __g_pRegister->DPCCTRL1;
	temp &= (U16)DITHER_MASK;		// mask other bits.
	temp = (U16)(temp | (ycorder << YCORDER_POS));
	temp = (U16)(temp | (format << FORMAT_POS));
	if( !bClipYC )	temp |= (U16)(1U<<YCRANGE_POS);
	if( bSwapRB )	temp |= (U16)(1U<<SWAPRB_POS);
	__g_pRegister->DPCCTRL1 = temp;

	temp = __g_pRegister->DPCCTRL2;
	temp &= (U16)~(PADCLKSEL_MASK | LCDTYPE_MASK );     // TFT or Video Encoder
	temp = (U16)(temp | (clock<<PADCLKSEL_POS));
	__g_pRegister->DPCCTRL2 = temp;

	// Determines whether invert or not the polarity of the pad clock.
	MES_DPC_SetClockOutInv( 0, bInvertClock );
	MES_DPC_SetClockOutInv( 1, bInvertClock );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get current display mode.
 *  @param[out] pFormat         Get current data format.

 *  @param[out] pbInterlace     Get current scan mode.\n
 *		        			    CTRUE = Interface, CFALSE = Progressive.
 *  @param[out] pbInvertField   Specifies internal field polarity.\n
 *		        			    CTRUE = Invert Field(Low is even field), CFALSE = Normal Field(Low is odd field).

 *  @param[out] pbRGBMode       Get current pixel format.\n
 *		        			    CTRUE = RGB Mode, CFASE = YCbCr Mode.
 *  @param[out] pbSwapRB        Get current setting about Swap Red and Blue component for RGB output.\n
 *		        			    CTRUE = Swap Red and Blue, CFALSE = No swap.

 *  @param[out] pYCorder		Get current output order for YCbCr Output.
 *  @param[out] pbClipYC		Get current output range of RGB2YC.\n
 *		        			    CTRUE = Y(16 ~ 235), Cb/Cr(16 ~ 240), CFALSE = Y/Cb/Cr(0 ~ 255).
 *  @param[out] pbEmbeddedSync  Get current embedded sync mode(SAV/EAV).\n
 *		        			    CTRUE = Enable, CFALSE = Disable.

 *  @param[out] pClock		    Get current PAD output clock.
 *  @param[out] pbInvertClock   Get current pixel clock polarity.\n
 *		                        CTRUE = rising edge, CFALSE = falling edge.
 *	@return     None.
 *	@remark	    Arguments which does not required can be CNULL.
 *	@see	    MES_DPC_SetMode
 */
void	MES_DPC_GetMode( MES_DPC_FORMAT *pFormat,
		        		 CBOOL *pbInterlace, CBOOL *pbInvertField,
				         CBOOL *pbRGBMode, CBOOL *pbSwapRB,
    					 MES_DPC_YCORDER *pYCorder, CBOOL *pbClipYC, CBOOL *pbEmbeddedSync,
	        			 MES_DPC_PADCLK *pClock, CBOOL *pbInvertClock )
{
	// DPC Control 0 register
	const U16 POLFIELD	= 1U<<2;
	const U16 SEAVENB	= 1U<<8;
	const U16 SCANMODE	= 1U<<9;
	const U16 RGBMODE	= 1U<<12;

	// DPC Control 1 register
	const U16 YCORDER_POS	= 6;
	const U16 YCORDER_MASK	= 0x3U<<YCORDER_POS;
	const U16 FORMAT_POS	= 8;
	const U16 FORMAT_MASK	= 0xFU<<FORMAT_POS;
	const U16 YCRANGE		= 1U<<13;
	const U16 SWAPRB		= 1U<<15;

	// DPC Control 2 register
	const U16 PADCLKSEL_POS		= 0;
	const U16 PADCLKSEL_MASK	= 3U<<PADCLKSEL_POS;

	register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );

	temp = __g_pRegister->DPCCTRL0;
	if( CNULL != pbInterlace )		*pbInterlace 	= (temp & SCANMODE) ? CTRUE : CFALSE;
	if( CNULL != pbInvertField )	*pbInvertField 	= (temp & POLFIELD) ? CTRUE : CFALSE;
	if( CNULL != pbRGBMode )		*pbRGBMode 		= (temp & RGBMODE ) ? CTRUE : CFALSE;
	if( CNULL != pbEmbeddedSync )	*pbEmbeddedSync	= (temp & SEAVENB ) ? CTRUE : CFALSE;

	temp = __g_pRegister->DPCCTRL1;
	if( CNULL != pYCorder )			*pYCorder	= (MES_DPC_YCORDER) ((temp & YCORDER_MASK)>>YCORDER_POS);
	if( CNULL != pFormat )			*pFormat	= (MES_DPC_FORMAT)((temp & FORMAT_MASK  )>>FORMAT_POS  );
	if( CNULL != pbClipYC )			*pbClipYC	= (temp & YCRANGE) ? CFALSE : CTRUE;
	if( CNULL != pbSwapRB )			*pbSwapRB	= (temp & SWAPRB) ? CTRUE : CFALSE;

	temp = __g_pRegister->DPCCTRL2;
	if( CNULL != pClock )			*pClock		= (MES_DPC_PADCLK)((temp & PADCLKSEL_MASK)>>PADCLKSEL_POS);

	// Determines whether invert or not the polarity of the pad clock.
	if( CNULL != pbInvertClock ) 	*pbInvertClock = MES_DPC_GetClockOutInv( 1 );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set parameters for horizontal sync.
 *  @param[in]  AVWidth	 Specifies the active video width in clocks.
 *  @param[in]  HSW		 Specifies the horizontal sync width in clocks
 *  @param[in]  HFP		 Specifies the horizontal sync front porch in clocks.
 *  @param[in]  HBP		 Specifies the horizontal sync back porch in clocks.
 *  @param[in]  bInvHSYNC Specifies HSYNC polarity. CTRUE = High active, CFALSE = Low active.
 *	@return	    None.
 *	@remark	    A sum of arguments except bInvHSYNC has to be less than or equal to 65536.
 *			    The unit is VCLK( one clock for a pixel).\n
 *			    See follwing figure for more details.
 *	@code
 *
 *	                   <---------------TOTAL----------------->
 * 	                   <-SW->
 *	   Sync  ---------+      +--------------/-/---------------+      +---
 *	                  |      |                                |      |
 *	                  +------+                                +------+
 *	            <-FP->        <-BP-> <--ACTIVE VIDEO-->
 *	  Active --+                    +-------/-/--------+
 *	  Video    |       (BLANK)      |  (ACTIVE DATA)   |      (BLANK)
 *	           +--------------------+                  +-----------------
 *	                   <---ASTART-->
 *	                   <-------------AEND------------->
 *	@endcode
 *	@see	    MES_DPC_GetHSync
 */
void	MES_DPC_SetHSync( U32 AVWidth, U32 HSW, U32 HFP, U32 HBP, CBOOL bInvHSYNC )
{
	const U16 INTPEND 	= 1U<<10;
	const U16 POLHSYNC	= 1U<<0;
    register U16 temp;

    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( 65536 >= (AVWidth + HFP + HSW + HBP) );
	MES_ASSERT( 0 < HSW );

    pRegister = __g_pRegister;

	pRegister->DPCHTOTAL 	= (U16)(HSW + HBP + AVWidth + HFP - 1);
	pRegister->DPCHSWIDTH	= (U16)(HSW - 1);
	pRegister->DPCHASTART	= (U16)(HSW + HBP - 1);
	pRegister->DPCHAEND	= (U16)(HSW + HBP + AVWidth - 1);

	temp = pRegister->DPCCTRL0;
	temp &= ~INTPEND;	// unmask intpend bit.
	if( bInvHSYNC )		temp |= (U16) POLHSYNC;
	else				temp &= (U16)~POLHSYNC;
	pRegister->DPCCTRL0 = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get current parameters for horizontal sync.
 *  @param[out] pAVWidth	Get current active video width in clocks.
 *  @param[out] pHSW		Get current horizontal sync width in clocks
 *  @param[out] pHFP		Get current horizontal sync front porch in clocks.
 *  @param[out] pHBP		Get current horizontal sync back porch in clocks.
 *  @param[out] pbInvHSYNC  Get current HSYNC polarity. CTRUE = High active, CFALSE = Low active.
 *	@return     None.
 *	@remark	    Arguments which does not required can be CNULL.\n
 *			    The unit is VCLK( one clock for a pixel).
 *	@see	    MES_DPC_SetHSync
 */
void	MES_DPC_GetHSync( U32 *pAVWidth, U32 *pHSW, U32 *pHFP, U32 *pHBP, CBOOL *pbInvHSYNC )
{
	const U16 POLHSYNC	= 1U<<0;

	U32	htotal, hsw, hab, hae;
	U32 avw, hfp, hbp;

    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

	htotal 	= (U32)pRegister->DPCHTOTAL + 1;
	hsw		= (U32)pRegister->DPCHSWIDTH + 1;
	hab		= (U32)pRegister->DPCHASTART + 1;
	hae		= (U32)pRegister->DPCHAEND + 1;

	hbp		= hab - hsw;
	avw		= hae - hab;
	hfp		= htotal - hae;

	if( CNULL != pAVWidth )		*pAVWidth 	= avw;
	if( CNULL != pHSW )			*pHSW		= hsw;
	if( CNULL != pHFP )			*pHFP		= hfp;
	if( CNULL != pHBP )			*pHBP		= hbp;

	if( CNULL != pbInvHSYNC )	*pbInvHSYNC	= (pRegister->DPCCTRL0 & POLHSYNC) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set parameters for vertical sync.
 *  @param[in]  AVHeight	  Specifies the active video height in lines.
 *  @param[in]  VSW 		  Specifies the vertical sync width in lines.
 *	            			  When interlace mode, this value is used for odd field.
 *  @param[in]  VFP 		  Specifies the vertical sync front porch in lines.
 *	            			  When interlace mode, this value is used for odd field.
 *  @param[in]  VBP 		  Specifies the vertical sync back porch in lines.
 *	            			  When interlace mode, this value is used for odd field.
 *  @param[in]  bInvVSYNC	  Specifies VSYNC polarity. CTRUE = High active, CFALSE = Low active.
 *  @param[in]  EAVHeight	  Specifies the active video height in lines for even field.
 *  @param[in]  EVSW    	  Specifies the vertical sync width in lines for even field.
 *  @param[in]  EVFP		  Specifies the vertical sync front porch in lines for even field.
 *  @param[in]  EVBP		  Specifies the vertical sync back porch in lines for even field.
 *	@return	    None.
 *	@remark	    A sum of arguments(AVHeight + VSW + VFP + VBP or AVHeight + EVSW + EVFP + EVBP)
 *			    has to be less than or equal to 65536.\n
 *			    See follwing figure for more details.
 *	@code
 *
 *	                   <---------------TOTAL----------------->
 * 	                   <-SW->
 *	   Sync  ---------+      +--------------/-/---------------+      +---
 *	                  |      |                                |      |
 *	                  +------+                                +------+
 *	            <-FP->        <-BP-> <--ACTIVE VIDEO-->
 *	  Active --+                    +-------/-/--------+
 *	  Video    |       (BLANK)      |  (ACTIVE DATA)   |      (BLANK)
 *	           +--------------------+                  +-----------------
 *	                   <---ASTART-->
 *	                   <-------------AEND------------->
 *	@endcode
 *	@see	    MES_DPC_GetVSync
 */
void	MES_DPC_SetVSync( U32 AVHeight, U32 VSW, U32 VFP, U32 VBP, CBOOL bInvVSYNC,
    					  U32 EAVHeight, U32 EVSW, U32 EVFP, U32 EVBP )
{
	const U16 INTPEND 	= 1U<<10;
	const U16 POLVSYNC	= 1U<<1;

    register U16 temp;

    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( 65536 >= (AVHeight + VFP + VSW + VBP) );
	MES_ASSERT( 65536 >= (AVHeight + EVFP + EVSW + EVBP) );
	MES_ASSERT( 0 < VSW );
	MES_ASSERT( 0 < EVSW );

    pRegister = __g_pRegister;

	pRegister->DPCVTOTAL 	= (U16)(VSW + VBP + AVHeight + VFP - 1);
	pRegister->DPCVSWIDTH	= (U16)(VSW - 1);
	pRegister->DPCVASTART	= (U16)(VSW + VBP - 1);
	pRegister->DPCVAEND		= (U16)(VSW + VBP + AVHeight - 1);

	pRegister->DPCEVTOTAL 	= (U16)(EVSW + EVBP + EAVHeight + EVFP - 1);
	pRegister->DPCEVSWIDTH	= (U16)(EVSW - 1);
	pRegister->DPCEVASTART	= (U16)(EVSW + EVBP - 1);
	pRegister->DPCEVAEND	= (U16)(EVSW + EVBP + EAVHeight - 1);

	temp = pRegister->DPCCTRL0;
	temp &= ~INTPEND;	// unmask intpend bit.
	if( bInvVSYNC )		temp |= (U16) POLVSYNC;
	else				temp &= (U16)~POLVSYNC;
	pRegister->DPCCTRL0 = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get current parameters for vertical sync.
 *  @param[out] pAVHeight	 Get current active video height in lines.
 *  @param[out] pVSW		 Get current vertical sync width in lines.
 *	            		     When interlace mode, this value is used for odd field.
 *  @param[out] pVFP		 Get current vertical sync front porch in lines.
 *	            		     When interlace mode, this value is used for odd field.
 *  @param[out] pVBP		 Get current vertical sync back porch in lines.
 *	            		     When interlace mode, this value is used for odd field.
 *  @param[out] pbInvVSYNC   Get current VSYNC polarity. CTRUE = High active, CFALSE = Low active.
 *  @param[out] pEAVHeight   Get current active video height in lines for even field.
 *  @param[out] pEVSW		 Get current vertical sync width in lines for even field.
 *  @param[out] pEVFP		 Get current vertical sync front porch in lines for even field.
 *  @param[out] pEVBP		 Get current vertical sync back porch in lines for even field.
 *	@return     None.
 *	@remark	    Arguments which does not required can be CNULL.
 *	@see	    MES_DPC_SetVSync
 */
void	MES_DPC_GetVSync( U32 *pAVHeight, U32 *pVSW, U32 *pVFP, U32 *pVBP, CBOOL *pbInvVSYNC,
	        			  U32 *pEAVHeight, U32 *pEVSW, U32 *pEVFP, U32 *pEVBP )
{
	const U16 POLVSYNC	= 1U<<1;

	U32 vtotal, vsw, vab, vae;
	U32 avh, vfp, vbp;

    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

	vtotal	= (U32)pRegister->DPCVTOTAL + 1;
	vsw		= (U32)pRegister->DPCVSWIDTH + 1;
	vab		= (U32)pRegister->DPCVASTART + 1;
	vae		= (U32)pRegister->DPCVAEND + 1;

	vbp		= vab - vsw;
	avh		= vae - vab;
	vfp		= vtotal - vae;

	if( CNULL != pAVHeight )	*pAVHeight 	= avh;
	if( CNULL != pVSW )			*pVSW		= vsw;
	if( CNULL != pVFP )			*pVFP		= vfp;
	if( CNULL != pVBP )			*pVBP		= vbp;

	vtotal	= (U32)pRegister->DPCEVTOTAL + 1;
	vsw		= (U32)pRegister->DPCEVSWIDTH + 1;
	vab		= (U32)pRegister->DPCEVASTART + 1;
	vae		= (U32)pRegister->DPCEVAEND + 1;

	vbp		= vab - vsw;
	avh		= vae - vab;
	vfp		= vtotal - vae;

	if( CNULL != pEAVHeight )	*pEAVHeight	= avh;
	if( CNULL != pEVSW )		*pEVSW		= vsw;
	if( CNULL != pEVFP )		*pEVFP		= vfp;
	if( CNULL != pEVBP )		*pEVBP		= vbp;

	if( CNULL != pbInvVSYNC )	*pbInvVSYNC	= (pRegister->DPCCTRL0 & POLVSYNC) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set offsets for vertical sync.
 *  @param[in]  VSSOffset Specifies the number of clocks from the start of horizontal sync to the start of vertical sync,
 *		       			  where horizontal sync is the last one in vertical front porch.
 *		       			  If this value is 0 then the start of vertical sync synchronizes with the start of horizontal sync
 *		       			  which is the new one in vertical sync.
 *		       			  This value has to be less than HTOTAL. When interlace mode, this vaule is used for odd field.
 *  @param[in]  VSEOffset Specifies the number of clocks from the start of horizontal sync to the end of vertical sync,
 * 	           			  where horizontal sync is the last one in vertical sync.
 * 	           			  If this value is 0 then the end of vertical sync synchronizes with the start of horizontal sync
 *		       			  which is the new one in vertical back porch.
 *		       			  This value has to be less than HTOTAL. When interlace mode, this vaule is used for odd field.
 *  @param[in]  EVSSOffset Specifies the number of clocks from the start of horizontal sync to the start of vertical sync,
 *		       			  where horizontal sync is the last one in vertical front porch.
 *	           			  If this value is 0 then the start of vertical sync synchronizes with the start of horizontal sync
 *		       			  which is the new one in vertical sync.
 *		       			  This value has to be less than HTOTAL and is used for even field.
 *  @param[in]  EVSEOffset Specifies the number of clocks from the start of horizontal sync to the end of vertical sync,
 * 	           			  where horizontal sync is the last one in vertical sync.
 * 	           			  If this value is 0 then the end of vertical sync synchronizes with the start of horizontal sync
 *		       			  which is the new one in vertical back porch.
 *		       			  This value has to be less than HTOTAL and is used for even field.
 *	@return	    None.
 *	@remark	    All arguments has to be less than HTOTAL or 65536.
 *			    The unit is VCLK( one clock for a pixel).\n
 *			    See follwing figure for more details.
 *	@code
 *	            <---HTOTAL--->
 *	  HSYNC  --+ +------------+ +-----/-/----+ +------------+ +-------------
 *	           | |            | |            | |            | |
 *	           +-+            +-+            +-+            +-+
 *
 *	  If VSSOffset == 0 and VSEOffset == 0 then
 *
 *	  VSYNC  -----------------+                             +---------------
 *	              (VFP)       |           (VSW)             |     (VBP)
 *	                          +------------/-/--------------+
 *
 *	  ,else
 *
 *	  VSYNC  -----------+                             +---------------------
 *	                    |<---> tVSSO                  |<---> tVSEO
 *	                    +-------------/-/-------------+
 *	            <------>                      <------>
 *	            VSSOffset                     VSEOffset
 *	         = HTOTAL - tVSSO              = HTOTAL - tVSEO
 *	@endcode
 *	@see	    MES_DPC_GetVSyncOffset
 */
void	MES_DPC_SetVSyncOffset( U32 VSSOffset, U32 VSEOffset, U32 EVSSOffset, U32 EVSEOffset )
{
    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( 65536 > VSSOffset );
	MES_ASSERT( 65536 > VSEOffset );
	MES_ASSERT( 65536 > EVSSOffset );
	MES_ASSERT( 65536 > EVSEOffset );

    pRegister = __g_pRegister;

	pRegister->DPCVSEOFFSET 	= (U16)VSEOffset;
	pRegister->DPCVSSOFFSET 	= (U16)VSSOffset;
	pRegister->DPCEVSEOFFSET 	= (U16)EVSEOffset;
	pRegister->DPCEVSSOFFSET 	= (U16)EVSSOffset;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get current offsets for vertical sync.
 *  @param[out] pVSSOffset	     Get current number of clocks from the start of horizontal sync to the start of vertical sync,
 *		        			     where horizontal sync is the last one in vertical front porch.
 *		        			     If this value is 0 then the start of vertical sync synchronizes with the start of horizontal sync
 *		        			     which is the new one in vertical sync. When interlace mode, this vaule is used for odd field.
 *  @param[out] pVSEOffset	     Get current number of clocks from the start of horizontal sync to the end of vertical sync,
 * 	            			     where horizontal sync is the last one in vertical sync.
 * 	            			     If this value is 0 then the end of vertical sync synchronizes with the start of horizontal sync
 *		        			     which is the new one in vertical back porch. When interlace mode, this vaule is used for odd field.
 *  @param[out] pEVSSOffset      Get current number of clocks from the start of horizontal sync to the start of vertical sync,
 *		        			     where horizontal sync is the last one in vertical front porch.
 *		        			     If this value is 0 then the start of vertical sync synchronizes with the start of horizontal sync
 *		        			     which is the new one in vertical sync. This value is used for even field.
 *  @param[out] pEVSEOffset	     Get current number of clocks from the start of horizontal sync to the end of vertical sync,
 * 	            			     where horizontal sync is the last one in vertical sync.
 * 	            			     If this value is 0 then the end of vertical sync synchronizes with the start of horizontal sync
 *		        			     which is the new one in vertical back porch. This value is used for even field.
 *	@return	    None.
 *	@remark	    Arguments which does not required can be CNULL.
 *			    The unit is VCLK( one clock for a pixel).
 *	@see	    MES_DPC_SetVSyncOffset
 */
void	MES_DPC_GetVSyncOffset( U32 *pVSSOffset, U32 *pVSEOffset, U32 *pEVSSOffset, U32 *pEVSEOffset )
{
    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

	if( CNULL != *pVSEOffset )    *pVSEOffset  = (U32)pRegister->DPCVSEOFFSET;
	if( CNULL != *pVSSOffset )    *pVSSOffset  = (U32)pRegister->DPCVSSOFFSET;
	if( CNULL != *pEVSEOffset)    *pEVSEOffset = (U32)pRegister->DPCEVSEOFFSET;
	if( CNULL != *pEVSSOffset)    *pEVSSOffset = (U32)pRegister->DPCEVSSOFFSET;
}

//--------------------------------------------------------------------------
// STN LCD specific control function
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 * @brief       Set STN LCD display mode
 * @param[in]   Format          Specifies data format(MES_DPC_FORMAT_4096COLOR or MES_DPC_FORMAT_16GRAY).
 * @param[in]   bRGBMode		Specifies pixel format.\n
 *		       	                CTRUE = RGB Mode, CFASE = YCbCr Mode.
 * @param[in]   BusWidth        Set STN LCD data bus bit width ( 4bit or 8bit )
 * @param[in]   bInvertVM       Set VM(Frame signal) polarity ( CTRUE : Invert, CFALSE : Normal )
 * @param[in]   bInvertClock    Sepcifies the pixel clock polarity.\n
 *             				    CTRUE = rising edge, CFALSE = falling edge.
 * @return      None.
 * @see                                             MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void	MES_DPC_SetSTNMode( MES_DPC_FORMAT Format, CBOOL bRGBMode, U32 BusWidth, CBOOL bInvertVM, CBOOL bInvertClock )
{
	// DPC Control 0 register
	const U16 INTPEND_POS	= 10;
	const U16 RGBMODE_POS	= 12;

    // DPC control 1 register
    const U16 FORMAT_POS = 8;
    const U16 FORMAT_MASK = 0x0F << FORMAT_POS;

    // DPC control 2 register
    const U16 STNLCDBITWIDTH_POS = 9;
    const U16 STNLCDBITWIDTH_MASK = 1U << STNLCDBITWIDTH_POS;

    const U16 LCDTYPE_POS = 8;
    const U16 LCDTYPE_MASK = 1U << LCDTYPE_POS;

    const U16 PADCLKSEL_POS = 0;
    const U16 PADCLKSEL_MASK = 1U << PADCLKSEL_POS;

    register U16 regvalue;
    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( (MES_DPC_FORMAT_4096COLOR==Format) || (MES_DPC_FORMAT_16GRAY==Format) );
    MES_ASSERT( (4==BusWidth) || (8==BusWidth) );
    MES_ASSERT( (0==bInvertVM) || (1==bInvertVM) );
    MES_ASSERT( (0==bInvertClock) || (1==bInvertClock) );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    // DPC control 0 register
	regvalue = __g_pRegister->DPCCTRL0;
	regvalue &= (U16)~(1U<<INTPEND_POS);	// unmask intpend bit.
	if( bRGBMode )		regvalue |= (U16) (1U<<RGBMODE_POS);
	else				regvalue &= (U16)~(1U<<RGBMODE_POS);
	__g_pRegister->DPCCTRL0 = regvalue;

    // DPC control 1 register
    regvalue = pRegister->DPCCTRL1;

    regvalue &= ~FORMAT_MASK;
    regvalue |= Format << FORMAT_POS;

    pRegister->DPCCTRL1 = regvalue;

    // DPC control 2 register
    regvalue = pRegister->DPCCTRL2;

    regvalue &= ~(STNLCDBITWIDTH_MASK | LCDTYPE_MASK | PADCLKSEL_MASK);
    regvalue |= ((BusWidth>>3)<<STNLCDBITWIDTH_POS) | (1U<<LCDTYPE_POS) | ((U16)bInvertVM<<PADCLKSEL_POS);

    pRegister->DPCCTRL2 = regvalue;

	// Determines whether invert or not the polarity of the pad clock.
	MES_DPC_SetClockOutInv( 0, bInvertClock );
	MES_DPC_SetClockOutInv( 1, bInvertClock );
}

//------------------------------------------------------------------------------
/**
 * @brief        Get STN LCD display mode
 * @param[out]   pFormat        Get current format
 * @param[out]   pbRGBMode      Get current pixel format.\n
 *		        			    CTRUE = RGB Mode, CFASE = YCbCr Mode.
 * @param[out]   pBusWidth      Get STN LCD data bus bit width
 * @param[out]   pbInvertVM     Get VM(Frame signal) polarity ( CTRUE : Invert, CFALSE : Normal )
 * @param[out]   pbInvertClock  Get current pixel clock polarity.\n
 *		                        CTRUE = rising edge, CFALSE = falling edge.
 * @return       None.
 * @remarks	     Arguments which does not required can be CNULL.
 * @see          MES_DPC_SetSTNMode,
 *               MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *               MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *               MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *               MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *               MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *               MES_DPC_GetHorizontalUpScaler
*/
void	MES_DPC_GetSTNMode( MES_DPC_FORMAT *pFormat, CBOOL *pbRGBMode, U32 *pBusWidth, CBOOL* pbInvertVM, CBOOL *pbInvertClock )
{
	// DPC Control 0 register
	const U16 RGBMODE	= 1U<<12;
    
    // DPC control 1 register
    const U16 FORMAT_POS = 8;
    const U16 FORMAT_MASK = 0x0F << FORMAT_POS;

    // DPC control 2 register
    const U16 STNLCDBITWIDTH_POS = 9;
    const U16 STNLCDBITWIDTH_MASK = 1U << STNLCDBITWIDTH_POS;

    const U16 PADCLKSEL_POS = 0;
    const U16 PADCLKSEL_MASK = 1U << PADCLKSEL_POS;

    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

	if( CNULL != pbRGBMode )		*pbRGBMode = (__g_pRegister->DPCCTRL0 & RGBMODE ) ? CTRUE : CFALSE;

    // DPC control 1 register
    if( CNULL != pFormat )  *pFormat = (MES_DPC_FORMAT)((pRegister->DPCCTRL1 & FORMAT_MASK) >> FORMAT_POS);

    // DPC control 2 register
    if( CNULL != pBusWidth )    *pBusWidth  = ((pRegister->DPCCTRL2 & STNLCDBITWIDTH_MASK) ? 8 : 4 );
    if( CNULL != pbInvertVM )   *pbInvertVM = (pRegister->DPCCTRL2 & PADCLKSEL_MASK) >> PADCLKSEL_POS;

	// Determines whether invert or not the polarity of the pad clock.
	if( CNULL != pbInvertClock ) 	*pbInvertClock = MES_DPC_GetClockOutInv( 1 );

}

//------------------------------------------------------------------------------
/**
 * @brief	    Set parameters for horizontal sync.
 * @param[in]   Format      Set STN LCD format ( MES_DPC_FORMAT_4096COLOR, MES_DPC_FORMAT_16GRAY )
 * @param[in]   BusWidth    Set STN LCD data bus bit width ( 4bit or 8bit )
 * @param[in]   AVWidth     Set active width ( 1 ~ 65536 )
 * @param[in]   CP1HW       Set horizontal sync width ( 1 ~ 65536 )
 * @param[in]   CP1ToCP2DLY Set delay from the end point of the horizontal sync pulse to the start
 *                          potion of the horizontal active.
 * @param[in]   CP2CYC      Set CP2 cycle (1~16)
 * @return      None.
 * @remarks     The unit is VCLK. \n
 *              Argument (\e Format, \e BusWidth) just use for calculation of horizontal sync.\n
 *              STN LCD's format and bus width is seted by MES_DPC_SetSTNMode().
 * @code
 *             <- CP1HW ->
 *             +---------+
 *     CP1  ---+         +-----------------------------------------------------------------------------
 *                       <- CP1ToCP2DLY ->
 *                                        +---------+         +---------+         +---------+
 *     CP2  ------------------------------+         +---------+         +---------+         +----------
 *                                        <-CP2CYC->
 *          ______________________________ ___________________ ___________________ ____________________
 *   PVD[0] ____ Horizontal Blank ________X_______ R0_________X________B2_________X_________G5_________
 *   PVD[1] ____ Horizontal Blank ________X_______ G0_________X________R3_________X_________B5_________
 *   PVD[2] ____ Horizontal Blank ________X_______ B0_________X________G3_________X_________...________
 *   PVD[3] ____ Horizontal Blank ________X_______ R1_________X________B3_________X____________________
 *   PVD[4] ____ Horizontal Blank ________X_______ G1_________X________R4_________X____________________
 *   PVD[5] ____ Horizontal Blank ________X_______ B1_________X________G4_________X____________________
 *   PVD[6] ____ Horizontal Blank ________X_______ R2_________X________B4_________X____________________
 *   PVD[7] ____ Horizontal Blank ________X_______ G2_________X________R5_________X____________________
 *
 * @endcode
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *                                                  MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void	MES_DPC_SetSTNHSync( MES_DPC_FORMAT Format, U32 BusWidth, U32 AVWidth, U32 CP1HW, U32 CP1ToCP2DLY, U32 CP2CYC )
{
    // DPC control 2 register
    const U16 CPCYC_POS = 12;
    const U16 CPCYC_MASK = 0x0FU << CPCYC_POS;

    register struct MES_DPC_RegisterSet*    pRegister;
    register U16 regvalue;
    register U16 colors;

    MES_ASSERT( (MES_DPC_FORMAT_4096COLOR==Format) || (MES_DPC_FORMAT_16GRAY==Format) );
    MES_ASSERT( (4==BusWidth) || (8==BusWidth) );

    MES_ASSERT( 1 <= AVWidth && AVWidth <= 65536 );
    MES_ASSERT( 1 <= CP1HW && CP1HW <= 65536 );
    MES_ASSERT( 1 <= (CP1HW+CP1ToCP2DLY) && (CP1HW+CP1ToCP2DLY) <= 65536 );
    MES_ASSERT( 1 <= CP2CYC && CP2CYC <= 16 );
    MES_ASSERT( CNULL != __g_pRegister );
    pRegister = __g_pRegister;

    if( MES_DPC_FORMAT_4096COLOR == Format ){ colors = 3; }
    else                                    { colors = 1; }

    // DPC control 2 register
    regvalue = pRegister->DPCCTRL2;

    regvalue &= ~CPCYC_MASK;
    regvalue |= (CP2CYC-1) << CPCYC_POS;

    pRegister->DPCCTRL2 = regvalue ;

    // HTOTAL = CP1HW + CP1ToCP2DLY + (((tAVW * 3 or 1)/BitWidth)*CPcYC*2) - 1;
    pRegister->DPCHTOTAL = CP1HW + CP1ToCP2DLY + ( ((AVWidth * colors)/BusWidth) * (CP2CYC*2) ) - 1;

    // HSWIDTH = CP1HW - 1
    pRegister->DPCHSWIDTH = CP1HW - 1;

    // HASTART = CP1HW + CP1ToCP2DLY -1
    pRegister->DPCHASTART = CP1HW + CP1ToCP2DLY - 1;

    // HAEND = AVWidth - 1
    pRegister->DPCHAEND = AVWidth - 1;
}

//------------------------------------------------------------------------------
/**
 * @brief	    Get parameters for horizontal sync.
 * @param[in]   pAVWidth     Get active width
 * @param[in]   pCP1HW       Get horizontal sync width
 * @param[in]   pCP1ToCP2DLY Get delay from the end point of the horizontal sync pulse to the start
 *                           potion of the horizontal active.
 * @param[in]   pCP2CYC      Get CP2 cycle
 * @return      None.
 * @remark	    Arguments which does not required can be CNULL.\n
 *              The unit is VCLK.
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void	MES_DPC_GetSTNHSync( U32* pAVWidth, U32* pCP1HW, U32* pCP1ToCP2DLY, U32* pCP2CYC )
{
    // DPC control 2 register
    const U16 CPCYC_POS = 12;
    const U16 CPCYC_MASK = 0x0FU << CPCYC_POS;

    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    // DPCHAEND = AVWidth - 1
    if( CNULL != pAVWidth )     *pAVWidth   = pRegister->DPCHAEND + 1;

    // DPCHSWIDTH = CP1HW - 1
    if( CNULL != pCP1HW )       *pCP1HW     = pRegister->DPCHSWIDTH + 1;

    // DPCHASTART = CP1HW + CP1ToCP2DLY -1
    if( CNULL != pCP1ToCP2DLY ) *pCP1ToCP2DLY = (pRegister->DPCHASTART+1) - (pRegister->DPCHSWIDTH+1);

    // CP2CYC
    if( CNULL != pCP2CYC )      *pCP2CYC = ((pRegister->DPCCTRL2 & CPCYC_MASK) >> CPCYC_POS )+1;
}

//------------------------------------------------------------------------------
/**
 * @brief	    Set parameters for vertical sync.
 * @param[in]   ActiveLine  Indicate that active line( 1 ~ 65536 ).
 * @param[in]   BlankLine   Indicate that blank line ( 1 ~ 65536 ).
 * @return      None.
 * @remarks     Total include  active line and blank line.
 *              The unit is VCLK. 
 * @code
 *                   +-------+                                      +-------+
 *    FRM/VM --------+       +--------------------------------------+       +-----
 *
 *                |<----------------- VTOTAL ------------------->|
 *                |<------ Active Line -------->|<- BLANK LINE ->|
 *
 *                +--+           +--+           +--+             +--+
 *     CP1   -----+  +-----------+  +-/-/-------+  +-------------+  +-------------
 *
 *     PVD   -------------\\\\\\\\----/-/-\\\\\\\------------------------\\\\\\----
 *                        line 1   ...   line N                          line 1
 *
 * @endcode
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *                                                  MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void    MES_DPC_SetSTNVSync( U32 ActiveLine, U32 BlankLine )
{
    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( 1 <= ActiveLine && ActiveLine <= 65536 );
    MES_ASSERT( 1 <= BlankLine && BlankLine <= 65536 );
    MES_ASSERT( 65536 > (ActiveLine+BlankLine) );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    pRegister->DPCVTOTAL  = ActiveLine + BlankLine - 1;
    pRegister->DPCVSWIDTH = BlankLine - 1;
    pRegister->DPCVAEND   = ActiveLine - 1;
}

//------------------------------------------------------------------------------
/**
 * @brief	    Get parameters for vertical sync.
 * @param[out]  pActiveLine  Get active line number.
 * @param[out]  pBlankLine   Get blank line number.
 * @return      None.
 * @remark	    Arguments which does not required can be CNULL.\n
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void    MES_DPC_GetSTNVSync( U32* pActiveLine, U32* pBlankLine )
{
    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    if( CNULL != pActiveLine )  *pActiveLine = pRegister->DPCVAEND + 1;
    if( CNULL != pBlankLine )   *pBlankLine  = pRegister->DPCVSWIDTH + 1;
}

//------------------------------------------------------------------------------
/**
 * @brief       Set value for randon number generator's operation
 * @param[in]   initShiftValue  Random number generator init shift value.
 * @param[in]   formula         Random number generator formula value.
 * @param[in]   subData         Random number generator sub data.
 * @return      None.
 * @remarks     Randon number generator(RNG) is the block to eliminate flicker( refer to chapter 21 DPC of databook ).\n
 *              RNG need initial value( shift, formula, sub data) for operatione. \n
 *              Recommand Value is \n
 *              - initShiftValue : 0xEEEEEEEE
 *              - formula        : 0xEEEEEEEE
 *              - subData        : 0xEE
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *                                                  MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void    MES_DPC_SetSTNRandomGenerator( U32 initShiftValue, U32 formula, U8 subData )
{
    const U32   HI_POS  = 16;
    const U32   HI_MASK = 0xFFFF << HI_POS;
    const U32   LI_MASK = 0x0000FFFF;

    const U16   RNCONVALUE1_MASK = 0xFF;

    register struct MES_DPC_RegisterSet*    pRegister;
    register U16    regvalue;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    pRegister->DPCRNUMGENCON0   =   (U16) (initShiftValue & LI_MASK);
    pRegister->DPCRNUMGENCON1   =   (U16)((initShiftValue & HI_MASK) >> HI_POS);

    pRegister->DPCRNUMGENFORMULASEL0   =   (U16) (formula & LI_MASK);
    pRegister->DPCRNUMGENFORMULASEL1   =   (U16)((formula & HI_MASK) >> HI_POS);

    regvalue = pRegister->DPCRNUMGENCON2;

    regvalue &= ~RNCONVALUE1_MASK;
    regvalue |= (U16)subData;

    pRegister->DPCRNUMGENCON2 = regvalue;
}

//------------------------------------------------------------------------------
/**
 * @brief       Get setting value of randon number generator
 * @param[out]  pinitShiftValue  Randon number generator init shift value.
 * @param[out]  pformula         Randon number generator formula value.
 * @param[out]  psubData         Randon number generator sub data.
 * @return      None.
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void    MES_DPC_GetSTNRandomGenerator( U32* pinitShiftValue, U32* pformula, U8* psubData )
{
    const U16   RNCONVALUE1_MASK = 0xFF;

    register struct MES_DPC_RegisterSet*    pRegister;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    if( CNULL != pinitShiftValue ) *pinitShiftValue = ((U32)(pRegister->DPCRNUMGENCON1<<16) | (U32)(pRegister->DPCRNUMGENCON0));
    if( CNULL != pformula )        *pformula = ((U32)(pRegister->DPCRNUMGENFORMULASEL1<<16) | (U32)(pRegister->DPCRNUMGENFORMULASEL0));
    if( CNULL != psubData )        *psubData = (U8)  (pRegister->DPCRNUMGENCON2 & RNCONVALUE1_MASK);
}

//------------------------------------------------------------------------------
/**
 * @brief       Set random number generator's operation
 * @param[in]   bEnb    \b CTRUE indicate that random number generator Enable. \n
                        \b CFALSE indicate that random number generator Disable.
 * @return      None.
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *                                                  MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
void    MES_DPC_SetSTNRandomGeneratorEnable( CBOOL bEnb )
{
    const U16 RNUMBGENENB_POS   = 12;
    const U16 RNUMBGENENB_MASK  = 1U << RNUMBGENENB_POS;

    register struct MES_DPC_RegisterSet* pRegister;
    register U32 regvalue;

    MES_ASSERT( (0==bEnb) ||(1==bEnb) );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister   =   __g_pRegister;

    regvalue   =   pRegister->DPCRNUMGENCON2;

    regvalue   &=  ~RNUMBGENENB_MASK;
    regvalue   |=  (U32)bEnb << RNUMBGENENB_POS;

    pRegister->DPCRNUMGENCON2  =   regvalue;
}

//------------------------------------------------------------------------------
/**
 * @brief       Get state of random number generator
 * @return      \b CTRUE indicate that random number generator is Enabled. \n
                \b CFALSE indicate that random number generator is Disabled.
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/
CBOOL   MES_DPC_GetSTNRandomGeneratorEnable( void )
{
    const U16 RNUMBGENENB_POS   = 12;
    const U16 RNUMBGENENB_MASK  = 1U << RNUMBGENENB_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->DPCRNUMGENCON2 & RNUMBGENENB_MASK) >> RNUMBGENENB_POS );
}

//------------------------------------------------------------------------------
/**
 * @brief       Set frame dither table
 * @param[in]   addr    Frame dither table address ( 0 ~ 15 )
 * @param[in]   red     Frame Red/Luminace dither table data.
 * @param[in]   green   Frame green dither table data.
 * @param[in]   blue    Frame blue dither table data.
 * @return      None.
 * @remarks     STN LCD express color by dot's on/off count. \n
 *              Suppose Red dot is composed 4bit. \n
 *              So, each Red dot express color 1 ~ 16 value using on/off count. \n
 *                  - Max Red color expressed by on/off count 16. \n
 *                  - Half Red color expressed by on/off count 8. \n
 *                  - Minimum Red color expressed by on/off count value 1. \n
 * @remarks     If Red dot on/off period is \b NOT \b regular, then STN lcd can occur flickering. \n
 *              Dither table use for elimate this flickering. \n
 *              So, dither table value have to set bits as \b regular as possible. \n  
 *              below example dither table shows that set bit positioned regularly  
 * @code
 *              // example dither table
 *              DitherTable = 
 *              {           
 *                  0x0080,             // 0000_0000_1000_0000 ( 1 )
 *                  0x8080,             // 1000_0000_1000_0000 ( 2 )
 *                  0x8210,             // 1000_0010_0001_0000 ( 3 )
 *                  0x8888,             // 1000_1000_1000_1000 ( 4 )
 *                  0x9244,             // 1001_0010_0100_0100 ( 5 )
 *                  0x9492,             // 1001_0100_1001_0010 ( 6 )
 *                  0x9552,             // 1001_0101_0101_0010 ( 7 )
 *                  0xAAAA,             // 1010_1010_1010_1010 ( 8 )
 *                  0xABAA,             // 1010_1011_1010_1010 ( 9 )
 *                  0xABAB,             // 1010_1011_1010_1011 ( 10 )
 *                  0xBBAB,             // 1011_1011_1010_1011 ( 11 )
 *                  0xBBBB,             // 1011_1011_1011_1011 ( 12 )
 *                  0xFBBB,             // 1111_1011_1011_1011 ( 13 )
 *                  0xFBFB,             // 1111_1011_1111_1011 ( 14 )
 *                  0xFFFB,             // 1111_1111_1111_1011 ( 15 )
 *                  0xFFFF              // 1111_1111_1111_1111 ( 16 )
 *              };
 * @endcode              
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *                                                  MES_DPC_SetHorizontalUpScaler,
 *              MES_DPC_GetHorizontalUpScaler
*/ 
void    MES_DPC_SetSTNDitherTable( U8 addr, U16 red, U16 green, U16 blue )
{
    register struct MES_DPC_RegisterSet* pRegister;

    MES_ASSERT( 16 > addr );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    pRegister->DPCFDTADDR = addr;
    pRegister->DPCFRDITHERVALUE = red;
    pRegister->DPCFGDITHERVALUE = green;
    pRegister->DPCFBDITHERVALUE = blue;
}

//------------------------------------------------------------------------------
/**
 * @brief       Set horizontal scale up ratio.
 * @param[in]   bEnb    \b CTRUE indicate that scaler enable.\n
 *                      \b CFALSE indicate that scaler disable.\n
 * @param[in]   sourceWidth Source image width that valuse have to same of MLC source width.
 * @param[in]   destWidth   Destination image width.
 * @return      None.
 * @remarks     Only seconary DPC can scale up of horizontal width.\n
 *              Scale ration calculation is below
 *                  - UpScale = (source width - 1 ) * (1<<11) / (destination width - 1)
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,
 *              MES_DPC_GetHorizontalUpScaler
 */
void    MES_DPC_SetHorizontalUpScaler( CBOOL bEnb, U32 sourceWidth, U32 destWidth )
{
    const U16 UPSCALEL_POS = 8;

    const U16 UPSCALEH_POS = 0;
    const U16 UPSCALEH_MASK = ((1<<15)-1) << UPSCALEH_POS;

    const U16 UPSCALERENB_POS = 0;

    register struct MES_DPC_RegisterSet* pRegister;
    register U16 regvalue;
    register U32 UpScale;

    MES_ASSERT( (0==bEnb) ||(1==bEnb) );
    MES_ASSERT( 1 <= sourceWidth && sourceWidth <= 65536 );
    MES_ASSERT( 1 <= destWidth && destWidth <= 65536 );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister   =   __g_pRegister;

    UpScale = ((sourceWidth-1) * (1<<11)) / (destWidth-1) ;

    regvalue = 0;
    regvalue |= (((U16)bEnb<<UPSCALERENB_POS) | (UpScale & 0xFF)<<UPSCALEL_POS );

    pRegister->DPCUPSCALECON0 = regvalue;
    pRegister->DPCUPSCALECON1 = ( UpScale >> 0x08 ) & UPSCALEH_MASK;
    pRegister->DPCUPSCALECON2 = sourceWidth - 1;
}

//------------------------------------------------------------------------------
/**
 * @brief       Get horizontal scale up ratio.
 * @param[out]  pbEnb   \b CTRUE indicate that scaler is enabled.\n
 *                      \b CFALSE indicate that scaler is disabled.\n
 * @param[out]  psourceWidth Source image width. 
 * @param[out]  pdestWidth   Destination image width.
 * @return      None.
 * @remarks     Only seconary DPC can scale up of horizontal width.\n
 *              Scale ration calculation is below
 *                  - UpScale = (source width - 1 ) * (1<<11) / (destination width - 1)
 * @see         MES_DPC_SetSTNMode,                 MES_DPC_GetSTNMode,
 *              MES_DPC_SetSTNHSync,                MES_DPC_GetSTNHSync,
 *              MES_DPC_SetSTNVSync,                MES_DPC_GetSTNVSync,
 *              MES_DPC_SetSTNRandomGenerator,      MES_DPC_GetSTNRandomGenerator,
 *              MES_DPC_SetSTNRandomGeneratorEnable,MES_DPC_GetSTNRandomGeneratorEnable,
 *              MES_DPC_SetSTNDitherTable,          MES_DPC_SetHorizontalUpScaler
 */
void    MES_DPC_GetHorizontalUpScaler( CBOOL* pbEnb, U32* psourceWidth, U32* pdestWidth )
{
    const U16 UPSCALERENB_POS = 0;
    const U16 UPSCALERENB_MASK = 1U << UPSCALERENB_POS;

    register struct MES_DPC_RegisterSet* pRegister;
    U32 UpScale;
    U16 destwidth, srcwidth;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister   =   __g_pRegister;

    UpScale = ((U32)(pRegister->DPCUPSCALECON1 & 0x7FFF)<<8) | ((U32)(pRegister->DPCUPSCALECON0>>8) & 0xFF );
    srcwidth    =   pRegister->DPCUPSCALECON2;
    destwidth   =   (srcwidth * (1<<11)) / UpScale;

    if( CNULL != pbEnb )        *pbEnb         = (pRegister->DPCUPSCALECON0 & UPSCALERENB_MASK );
    if( CNULL != psourceWidth ) *psourceWidth  = srcwidth + 1;
    if( CNULL != pdestWidth )   *pdestWidth    = destwidth + 1;
}

//------------------------------------------------------------------------------
// Internal video encoder operations
//------------------------------------------------------------------------------

/**
 *	@brief	    Set primary DPC operation with secondary DPC
 *  @param[in]  bEnb    \b CTRUE indicates primary DPC  \b sync operation with secondary DPC.\n
 *			            \b CFALSE indicates primary DPC \b Async operation with secondary DPC .
 *	@return	    None.
 *	@remark     This function is only valid for primary display controller.
 *              STN LCD is always operate Async Mode.
 *	@see	    MES_DPC_GetSecondaryDPCSync,
 *              MES_DPC_GetENCEnable, MES_DPC_SetVideoEncoderPowerDown, MES_DPC_GetVideoEncoderPowerDown,
 *			    MES_DPC_SetVideoEncoderMode, MES_DPC_SetVideoEncoderSCHLockControl, MES_DPC_GetVideoEncoderSCHLockControl,
 *			    MES_DPC_SetVideoEncoderBandwidth, MES_DPC_GetVideoEncoderBandwidth,
 *			    MES_DPC_SetVideoEncoderColorControl, MES_DPC_GetVideoEncoderColorControl,
 *			    MES_DPC_SetVideoEncoderTiming, MES_DPC_GetVideoEncoderTiming
 */
void    MES_DPC_SetSecondaryDPCSync( CBOOL bEnb )
{
    const U16 ENCMODE	= 1U<<14;
	const U16 INTPEND	= 1U<<10;

    register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );

	temp = __g_pRegister->DPCCTRL0;
	temp &= (U16)~INTPEND;
	if( bEnb )	temp |= (U16) ENCMODE;
	else		temp &= (U16)~ENCMODE;
	__g_pRegister->DPCCTRL0 = temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Get current setting of primary DPC sync operation.
 *	@return \b CTRUE indicates primary DPC  \b sync operation with secondary DPC.\n
 *			\b CFALSE indicates primary DPC \b Async operation with secondary DPC .
 *	@remark This function is only valid for primary display controller.
 *	@see	MES_DPC_SetSecondaryDPCSync
 */
CBOOL   MES_DPC_GetSecondaryDPCSync( void )
{
	const U16 ENCMODE	= 1U<<14;

	MES_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->DPCCTRL0 & ENCMODE) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable the internal video encoder.
 *  @param[in]  bEnb    Set it to CTRUE to enable the internal video encoder.
 *	@return	    None.
 *	@remark	    This function also controls the RESET of the internal video encoder and 
 *              internal DAC's Enable signal.
 *			    Therefore you have to enable the internal video encoder before using
 *			    funtions relative to the internal video encoder. And also if you
 *			    disable the internal video encoder, all registers of the internal
 *			    video encoder will be reset. Therefore you have to re-initialize the
 *			    internal video encoder again. This function is only valid for secondary
 *			    display controller.
 *	@see	    MES_DPC_GetENCEnable, MES_DPC_SetVideoEncoderPowerDown, MES_DPC_GetVideoEncoderPowerDown,
 *			    MES_DPC_SetVideoEncoderMode, MES_DPC_SetVideoEncoderSCHLockControl, MES_DPC_GetVideoEncoderSCHLockControl,
 *			    MES_DPC_SetVideoEncoderBandwidth, MES_DPC_GetVideoEncoderBandwidth,
 *			    MES_DPC_SetVideoEncoderColorControl, MES_DPC_GetVideoEncoderColorControl,
 *			    MES_DPC_SetVideoEncoderTiming, MES_DPC_GetVideoEncoderTiming
 */
void    MES_DPC_SetENCEnable( CBOOL	bEnb )
{
    const U16 ENCMODE	= 1U<<14;
	const U16 ENCRST	= 1U<<13;
	const U16 INTPEND	= 1U<<10;

    register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );
	temp = __g_pRegister->DPCCTRL0;
	temp &= (U16)~INTPEND;
	if( bEnb )	temp |= (U16) ENCRST;
	else		temp &= (U16)~ENCRST;
	__g_pRegister->DPCCTRL0 = (temp | ENCMODE);

	__g_pRegister->VENCICNTL = 7;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Informs whether the internal video encoder is enabled or disabled.
 *	@return \b CTRUE indicates the internal video encoder is enabled.\n
 *			\b CFALSE indicates the internal video encoder is disabled.
 *	@remark This function is only valid for secondary display controller.
 *	@see	MES_DPC_SetENCEnable
 */
CBOOL   MES_DPC_GetENCEnable( void )
{
	const U16 ENCRST	= 1U<<13;

	MES_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->DPCCTRL0 & ENCRST) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Specifies power down mode of the internal video encoder.
 *  @param[in]  bEnb    \b CTRUE specifies power down mode for the entire digital logic circuits
 *		        of encoder digital core.\n
 *		        \b CFALSE specifies normal operatation.
 *	@return	    None.
 *	@remark	    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_GetVideoEncoderPowerDown
 */
void    MES_DPC_SetVideoEncoderPowerDown( CBOOL	bEnb )
{
	const U16 PWDENC = 1U<<7;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );

	if( bEnb )
	{
		__g_pRegister->VENCCTRLA |= (U16) PWDENC;
		__g_pRegister->VENCDACSEL = 0;
	}
	else
	{
		__g_pRegister->VENCDACSEL = 1;
		__g_pRegister->VENCCTRLA &= (U16)~PWDENC;
	}
}

//------------------------------------------------------------------------------
/**
 *	@brief	Determines whether the internal video encoder is power down or not.
 *	@return	\b CTRUE indicates power down mode for the entire digital logic circuits
 *			of encoder digital core.\n
 *			\b CFALSE indicates normal operation.\n
 *	@remark	You have to call this function only when the internal video encoder is enabled.
 *			And this function is only valid for secondary display controller.
 *	@see	MES_DPC_SetENCEnable, MES_DPC_SetVideoEncoderPowerDown
 */
CBOOL   MES_DPC_GetVideoEncoderPowerDown( void )
{
	const U16 PWDENC = 1U<<7;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );

	return (__g_pRegister->VENCCTRLA & PWDENC) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set Video Broadcast Standard mode for the internal video encoder.
 *  @param[in]  vbs     	Specifies video broadcast standard.
 *  @param[in]  bPedestal   Specifies whether the video output has a pedestal or not.
 *	@return	    None.
 *	@remark	    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable
 */
void    MES_DPC_SetVideoEncoderMode( MES_DPC_VBS vbs, CBOOL	bPedestal )
{
	// to avoid below compile error of gcc :	
	// 		error: initializer element is not constant
	// 			   (near initialization for ¡®VENCCTRLA_TABLE[0]¡¯)
	#define PHALT 			(1U<<0)
	#define IFMT 			(1U<<1)
	#define PED 			(1U<<3)
	#define FSCSEL_NTSC 	(0U<<4)
	#define FSCSEL_PAL 		(1U<<4)
	#define FSCSEL_PALM 	(2U<<4)
	#define FSCSEL_PALN 	(3U<<4)
	#define FDRST 			(1U<<6)
	#define PWDENC 			(1U<<7)
/*
	const U8 PHALT	= 1U<<0;
	const U8 IFMT	= 1U<<1;
	const U8 PED	= 1U<<3;
	const U8 FSCSEL_NTSC	= 0U<<4;
	const U8 FSCSEL_PAL 	= 1U<<4;
	const U8 FSCSEL_PALM	= 2U<<4;
	const U8 FSCSEL_PALN	= 3U<<4;
	const U8 FDRST	= 1U<<6;
	const U16 PWDENC	= 1U<<7;
*/
	register U16 temp;

	static const U8 VENCCTRLA_TABLE[] =
	{
		(U8)(       FSCSEL_NTSC         | FDRST),	// NTSC-M             59.94 Hz(525)
		(U8)(IFMT | FSCSEL_NTSC                ),	// NTSC-N             50 Hz(625)
		(U8)(       FSCSEL_PAL                 ),	// NTSC-4.43          60 Hz(525)
		(U8)(       FSCSEL_PALM | PHALT        ),	// PAL-M              59.952 Hz(525)
		(U8)(IFMT | FSCSEL_PALN | PHALT        ),	// PAL-combination N  50 Hz(625)
		(U8)(IFMT | FSCSEL_PAL  | PHALT | FDRST),	// PAL-B,D,G,H,I,N    50 Hz(625)
		(U8)(       FSCSEL_PAL  | PHALT        ),	// Pseudo PAL         60 Hz(525)
		(U8)(IFMT | FSCSEL_NTSC                )	// Pseudo NTSC        50 Hz (625)
	};

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );
	MES_ASSERT( 8 > vbs );

	temp = __g_pRegister->VENCCTRLA;
	temp &= (U16)PWDENC;		// mask PWDENB bit.
	temp = (U16)( temp | (U16)VENCCTRLA_TABLE[vbs]);
	if( bPedestal )	temp |= (U16)PED;
	__g_pRegister->VENCCTRLA = temp;

	#undef PHALT
	#undef IFMT
	#undef PED
	#undef FSCSEL_NTSC
	#undef FSCSEL_PAL
	#undef FSCSEL_PALM
	#undef FSCSEL_PALN
	#undef FDRST
	#undef PWDENC
}   

//------------------------------------------------------------------------------
/** 
 *	@brief	    Set SCH Chroma-Luma locking control for the internal video encoder.
 *  @param[in]  bFreeRun    \b CTRUE specifies Chroma is free running as compared to horizontal sync.\n
 *		        \b CFALSE specifies Constant relationship between color burst and horizontal
 *		        sync maintained for appropriate video standards.
 *	@return	    None.
 *	@remark	    The function SetVideoEncoderMode also affects the result of this function.\n
 *			    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_SetVideoEncoderMode, MES_DPC_GetVideoEncoderSCHLockControl
 */
void    MES_DPC_SetVideoEncoderSCHLockControl( CBOOL bFreeRun )
{
	const U16 FDRST = 1U<<6;
	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );

	if( bFreeRun )	__g_pRegister->VENCCTRLA |= (U16) FDRST;
	else			__g_pRegister->VENCCTRLA &= (U16)~FDRST;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Get SCH Chroma-Luma locking control for the internal video encoder.
 *	@return	\b CTRUE indicates Chroma is free running as compared to horizontal sync.\n
 *			\b CFALSE indicates Constant relationship between color burst and horizontal
 *			sync maintained for appropriate video standards.
 *	@remark	The function SetVideoEncoderMode also affects the result of this function.\n
 *			You have to call this function only when the internal video encoder is enabled.
 *			And this function is only valid for secondary display controller.
 *	@see	MES_DPC_SetENCEnable, MES_DPC_SetVideoEncoderMode, MES_DPC_SetVideoEncoderSCHLockControl
 */
CBOOL   MES_DPC_GetVideoEncoderSCHLockControl( void )
{
	const U16 FDRST = 1U<<6;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );

	return (__g_pRegister->VENCCTRLA & FDRST) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set Chroma and Luma bandwidth control for the internal video encoder.
 *  @param[in]  Luma	Specifies luma bandwidth.
 *  @param[in]  Chroma	Specifies chroma bandwidth.
 *	@return	    None.
 *	@remark	    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_GetVideoEncoderBandwidth
 */
void    MES_DPC_SetVideoEncoderBandwidth( MES_DPC_BANDWIDTH	Luma, MES_DPC_BANDWIDTH	Chroma )
{
	const U16 YBW_POS	= 0;
	const U16 CBW_POS	= 2;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );
	MES_ASSERT( 3 > Luma );
	MES_ASSERT( 3 > Chroma );

	__g_pRegister->VENCCTRLB = (U16)((Chroma<<CBW_POS) | (Luma<<YBW_POS));
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get Chroma and Luma bandwidth control for the internal video encoder.
 *  @param[out] pLuma		Indicates current luma bandwidth.
 *  @param[out] pChroma	    Indicates current chroma bandwidth.
 *	@return	    None.
 *	@remark	    Arguments which does not required can be CNULL.\n
 *			    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_SetVideoEncoderBandwidth
 */
void    MES_DPC_GetVideoEncoderBandwidth( MES_DPC_BANDWIDTH	*pLuma, MES_DPC_BANDWIDTH	*pChroma )
{
	const U16 YBW_POS	= 0;
	const U16 YBW_MASK	= 3U<<YBW_POS;
	const U16 CBW_POS	= 2;
	const U16 CBW_MASK	= 3U<<CBW_POS;

    register U16 temp;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );
	temp = __g_pRegister->VENCCTRLB;
	if( CNULL != pLuma  )	*pLuma		= (MES_DPC_BANDWIDTH)((temp & YBW_MASK)>>YBW_POS);
	if( CNULL != pChroma )	*pChroma	= (MES_DPC_BANDWIDTH)((temp & CBW_MASK)>>CBW_POS);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the color burst phase and HUE, Saturation for the internal video encoder.
 *  @param[in]  SCH		Specifies the color burst phase relative to the horizontal sync.
 *		        	    The 8 bit control covers the entire 360 range as a 2's compliment number.
 *		        	    '0' is nominal value.
 *  @param[in]  HUE		Specifies the active video color burst phase relative to color burst.
 *		        	    The 8 bit control covers the entire 360 range as a 2's compliment number.
 *		        	    '0' is nominal value.
 *  @param[in]  SAT		Specifies the active video chroma gain relative to the color burst gain.
 *		        	    This value is 2's compliment with '0' the nominal value.
 *  @param[in]  CRT     Specifies the Luma gain. This value is 2's compliment with '0' the nominal value.
 *  @param[in]  BRT     Specifies the Luma offset. This value is 2's compliment with '0' the nominal value.
 *	@return	    None.
 *	@remark	    This funcion can operate only when SCH Chroma-Luma locking control is constant mode.\n
 *			    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_GetVideoEncoderColorControl
 */
void    MES_DPC_SetVideoEncoderColorControl( S8 SCH, S8 HUE, S8 SAT, S8 CRT, S8 BRT )
{
    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );

    pRegister = __g_pRegister;

	pRegister->VENCSCH = (U16)SCH;
	pRegister->VENCHUE = (U16)HUE;
	pRegister->VENCSAT = (U16)SAT;
	pRegister->VENCCRT = (U16)CRT;
	pRegister->VENCBRT = (U16)BRT;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get the color burst phase and HUE, Saturation for the internal video encoder.
 *  @param[out] pSCH	 Get current color burst phase relative to the horizontal sync.
 *		        		 The 8 bit control covers the entire 360 range as a 2's compliment number.
 *		        		 '0' is nominal value.
 *  @param[out] pHUE	 Get current active video color burst phase relative to color burst.
 *		        		 The 8 bit control covers the entire 360 range as a 2's compliment number.
 *		        		 '0' is nominal value.
 *  @param[out] pSAT	 Get current active video chroma gain relative to the color burst gain.
 *		        		 This value is 2's compliment with '0' the nominal value.
 *  @param[out] pCRT     Get current Luma gain. This value is 2's compliment with '0' the nominal value.
 *  @param[out] pBRT     Get current Luma offset. This value is 2's compliment with '0' the nominal value.
 *	@return	    None.
 *	@remark	    Arguments which does not required can be CNULL.\n
 *			    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_SetVideoEncoderColorControl
 */
void    MES_DPC_GetVideoEncoderColorControl( S8 *pSCH, S8 *pHUE, S8 *pSAT, S8 *pCRT, S8 *pBRT )
{
    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );

    pRegister = __g_pRegister;

	if( CNULL != pSCH )    *pSCH = (S8)pRegister->VENCSCH;
	if( CNULL != pHUE )    *pHUE = (S8)pRegister->VENCHUE;
	if( CNULL != pSAT )    *pSAT = (S8)pRegister->VENCSAT;
	if( CNULL != pCRT )    *pCRT = (S8)pRegister->VENCCRT;
	if( CNULL != pBRT )    *pBRT = (S8)pRegister->VENCBRT;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the adjust value for the color burst frequency of the internal video encoder.
 *  @param[in]  Adjust	Specifies the color burst frequency adjustment value.\n
 *						Allows the pixel clock to be varied up to +/-200 ppm of
 * 						its nominal value. This allows dot crawl adjustment.
 *						This 16 bit value is multiplied by 4 and added to the
 *						internal chroma frequency constant.
 *	@return	    None.
 *	@remark	    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_GetVideoEncoderFSCAdjust
 */
void    MES_DPC_SetVideoEncoderFSCAdjust( S16 Adjust )
{
	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );
	__g_pRegister->VENCFSCADJH = (U16)(Adjust >> 8);
	__g_pRegister->VENCFSCADJL = (U16)(Adjust & 0xFF);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get the adjust value for the color burst frequency of the internal video encoder.
 *	@return	    The color burst frequency adjustment value.
 *	@remark	    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_SetVideoEncoderFSCAdjust
 */
U16 MES_DPC_GetVideoEncoderFSCAdjust( void )
{
	register U32 temp;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );

	temp = (U32)__g_pRegister->VENCFSCADJH;
	temp <<= 8;
	temp |= (((U32)__g_pRegister->VENCFSCADJL) & 0xFF);
	return (U16)temp;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set parameters for sync signals of the internal video encoder.
 *  @param[in]  HSOS	 Specifies the number of clocks for the horizontal sync width, 1 ~ 2048.
 *  @param[in]  HSOE	 Specifies the number of total clocks for the horizontal period, 1 ~ 2048.
 *  @param[in]  VSOS	 Specifies the number of lines for the start of vertical sync, 0 ~ 511.
 *  @param[in]  VSOE	 Specifies the number of lines for the end of vertical sync, 0 ~ 63.
 *	@return	    None.
 *	@remark	    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.\n
 *			    The unit is VCLK2(two clock for a pixel) for HSOS and HSOE.\n
 *			    See follwing figure for more details.
 *	@code
 *
 *	                   <----------------HSOE----------------->
 * 	                   <-HSOS->
 *	  HSync  ---------+        +--------------/-/-------------+        +---
 *	                  |        |                              |        |
 *	                  +--------+                              +--------+
 *
 *    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 *	                   <------VSOE------>
 * 	                   <-VSOS->
 *	  VSync  ------------------+         +--------------/-/-----------------
 *	                  ^        |         |
 *	                  |        +---------+
 *	        Vertical -+
 *          Start
 *
 *	@endcode
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_GetVideoEncoderTiming
 */
void    MES_DPC_SetVideoEncoderTiming( U32	HSOS, U32 HSOE,	U32 VSOS, U32 VSOE )
{
    register struct MES_DPC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );
	MES_ASSERT( 2048 > (HSOS-1) );
	MES_ASSERT( 2048 > (HSOE-1) );
	MES_ASSERT( 512 > VSOS );
	MES_ASSERT( 64 > VSOE );

    pRegister = __g_pRegister;

	HSOS -= 1;
	HSOE -= 1;
	pRegister->VENCHSVSO	= (U16)((((VSOS >> 8)&1U)<<6) | (((HSOS >> 8)&7U)<<3) | (((HSOE >> 8)&7U)<<0));
	pRegister->VENCHSOS	= (U16)(HSOS & 0xFFU);
	pRegister->VENCHSOE	= (U16)(HSOE & 0xFFU);
	pRegister->VENCVSOS	= (U16)(VSOS & 0xFFU);
	pRegister->VENCVSOE	= (U16)(VSOE & 0x1FU);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get current parameters for sync signals of the internal video encoder.
 *  @param[out] pHSOS	Get current number of clocks for the horizontal sync width, 1 ~ 2048.
 *  @param[out] pHSOE	Get current number of total clocks for the horizontal period, 1 ~ 2048.
 *  @param[out] pVSOS	Get current number of lines for the start of vertical sync, 0 ~ 511.
 *  @param[out] pVSOE	Get current number of lines for the end of vertical sync, 0 ~ 63.
 *	@return	    None.
 *	@remark	    The unit is VCLK2(two clock for a pixel) for HSOS and HSOE.\n
 *			    Arguments which does not required can be CNULL.\n
 *			    You have to call this function only when the internal video encoder is enabled.
 *			    And this function is only valid for secondary display controller.
 *	@see	    MES_DPC_SetENCEnable, MES_DPC_SetVideoEncoderTiming
 */
void    MES_DPC_GetVideoEncoderTiming
(
	U32	*pHSOS,
	U32 *pHSOE,
	U32 *pVSOS,
	U32 *pVSOE
)
{
    register U16 HSVSO;

	MES_ASSERT( CNULL != __g_pRegister );
	MES_ASSERT( CTRUE == MES_DPC_GetENCEnable() );
	HSVSO = __g_pRegister->VENCHSVSO;
	if( CNULL != pHSOS )	*pHSOS = (U32)((((HSVSO>>3)&7U)<<8) | (__g_pRegister->VENCHSOS & 0xFFU)) + 1;
	if( CNULL != pHSOE )	*pHSOE = (U32)((((HSVSO>>0)&7U)<<8) | (__g_pRegister->VENCHSOE & 0xFFU)) + 1;
	if( CNULL != pVSOS )	*pVSOS = (U32)((((HSVSO>>8)&1U)<<8) | (__g_pRegister->VENCVSOS & 0xFFU));
	if( CNULL != pVSOE )	*pVSOE = (U32)(__g_pRegister->VENCVSOE & 0x1FU);
}


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
//	Module     : AUDIO
//	File       : mes_audio.c
//	Description:
//	Author     : Firmware Team
//	History    :
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/pm.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/string.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>

#include <asm/arch/regs-dma.h>
#include "pollux-iis.h"

/// @brief  AUDIO Module's Register List
struct  MES_AUDIO_RegisterSet
{
    const    U16    __Reserved0[2];          ///< 0x00, 0x02 : Reserved 
    volatile U16    I2S_CTRL;                ///< 0x04 : I2S  Control Register
    volatile U16    I2S_CONFIG;              ///< 0x06 : I2S  Configuration Register
    volatile U16    AUDIO_BUFF_CTRL;         ///< 0x08 : Audio Buffer Control Register
    volatile U16    AUDIO_BUFF_CONFIG;       ///< 0x0A : Audio Buffer Configuration Register
    volatile U16    AUDIO_IRQ_ENA;           ///< 0x0C : Audio Interrupt Enable Register
    volatile U16    AUDIO_IRQ_PEND;          ///< 0x0E : Audio Interrupt Pending Register
    const    U16    __Reserved1[3];          ///< 0x10, 0x12, 0x14 : Reserved
    volatile U16    AUDIO_STATUS0;           ///< 0x16 : Audio Status0 Register
    volatile U16    AUDIO_STATUS1;           ///< 0x18 : Audio Status1 Register
    const    U16    __Reserved2[0x1D3];      ///< 0x1A ~ 0x3BE : Reserved 
    volatile U32    CLKENB;                  ///< 0x3C0 : Clock Enable Register
    volatile U32    CLKGEN[2];               ///< 0x3C4, 0x3C8 : Clock Generate Register 0,1
};

static  struct
{
    struct MES_AUDIO_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_AUDIO_MODULE];// = { CNULL, };

static  U32 __g_CurModuleIndex = 0;
static  struct MES_AUDIO_RegisterSet *__g_pRegister = CNULL;

//------------------------------------------------------------------------------
// Module Interface
//------------------------------------------------------------------------------
/**
 *  @brief  Initialize of prototype enviroment & local variables.
 *  @return \b CTRUE  indicate that Initialize is successed.\n
 *          \b CFALSE indicate that Initialize is failed.\n
 *  @see    MES_AUDIO_SelectModule,
 *          MES_AUDIO_GetCurrentModule,     MES_AUDIO_GetNumberOfModule
 */
CBOOL   MES_AUDIO_Initialize( void )
{
	static CBOOL bInit = CFALSE;
	U32 i;
	
	if( CFALSE == bInit )
	{
		__g_CurModuleIndex = 0;
		
		for( i=0; i < NUMBER_OF_AUDIO_MODULE; i++ )
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
 *  @see        MES_AUDIO_Initialize,           
 *              MES_AUDIO_GetCurrentModule,     MES_AUDIO_GetNumberOfModule
 */
U32    MES_AUDIO_SelectModule( U32 ModuleIndex )
{
	U32 oldindex = __g_CurModuleIndex;

    MES_ASSERT( NUMBER_OF_AUDIO_MODULE > ModuleIndex );

    __g_CurModuleIndex = ModuleIndex;
    __g_pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	return oldindex;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get index of current selected module.
 *  @return     Current module's index.
 *  @see        MES_AUDIO_Initialize,           MES_AUDIO_SelectModule,
 *              MES_AUDIO_GetNumberOfModule
 */
U32     MES_AUDIO_GetCurrentModule( void )
{
    return __g_CurModuleIndex;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get number of modules in the chip.
 *  @return     Module's number.
 *  @see        MES_AUDIO_Initialize,           MES_AUDIO_SelectModule,
 *              MES_AUDIO_GetCurrentModule     
 */
U32     MES_AUDIO_GetNumberOfModule( void )
{
    return NUMBER_OF_AUDIO_MODULE;
}

//------------------------------------------------------------------------------
// Basic Interface
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *  @brief      Get module's physical address.
 *  @return     Module's physical address
 *  @see        MES_AUDIO_GetSizeOfRegisterSet, 
 *              MES_AUDIO_SetBaseAddress,       MES_AUDIO_GetBaseAddress,
 *              MES_AUDIO_OpenModule,           MES_AUDIO_CloseModule,
 *              MES_AUDIO_CheckBusy,            MES_AUDIO_CanPowerDown
 */
U32     MES_AUDIO_GetPhysicalAddress( void )
{
    MES_ASSERT( NUMBER_OF_AUDIO_MODULE > __g_CurModuleIndex );

    return  (U32)( POLLUX_PA_SOUND + (OFFSET_OF_AUDIO_MODULE * __g_CurModuleIndex) );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a size, in byte, of register set.
 *  @return     Size of module's register set.
 *  @see        MES_AUDIO_GetPhysicalAddress,    
 *              MES_AUDIO_SetBaseAddress,       MES_AUDIO_GetBaseAddress,
 *              MES_AUDIO_OpenModule,           MES_AUDIO_CloseModule,
 *              MES_AUDIO_CheckBusy,            MES_AUDIO_CanPowerDown
 */
U32     MES_AUDIO_GetSizeOfRegisterSet( void )
{
    return sizeof( struct MES_AUDIO_RegisterSet );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a base address of register set.
 *  @param[in]  BaseAddress Module's base address
 *  @return     None.
 *  @see        MES_AUDIO_GetPhysicalAddress,   MES_AUDIO_GetSizeOfRegisterSet, 
 *              MES_AUDIO_GetBaseAddress,
 *              MES_AUDIO_OpenModule,           MES_AUDIO_CloseModule,
 *              MES_AUDIO_CheckBusy,            MES_AUDIO_CanPowerDown
 */
void    MES_AUDIO_SetBaseAddress( U32 BaseAddress )
{
    MES_ASSERT( CNULL != BaseAddress );
    MES_ASSERT( NUMBER_OF_AUDIO_MODULE > __g_CurModuleIndex );

    __g_ModuleVariables[__g_CurModuleIndex].pRegister = (struct MES_AUDIO_RegisterSet *)BaseAddress;
    __g_pRegister = (struct MES_AUDIO_RegisterSet *)BaseAddress;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a base address of register set
 *  @return     Module's base address.
 *  @see        MES_AUDIO_GetPhysicalAddress,   MES_AUDIO_GetSizeOfRegisterSet, 
 *              MES_AUDIO_SetBaseAddress,       
 *              MES_AUDIO_OpenModule,           MES_AUDIO_CloseModule,
 *              MES_AUDIO_CheckBusy,            MES_AUDIO_CanPowerDown
 */
U32     MES_AUDIO_GetBaseAddress( void )
{
    MES_ASSERT( NUMBER_OF_AUDIO_MODULE > __g_CurModuleIndex );

    return (U32)__g_ModuleVariables[__g_CurModuleIndex].pRegister;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Initialize selected modules with default value.
 *  @return     \b CTRUE  indicate that Initialize is successed. \n
 *              \b CFALSE indicate that Initialize is failed.
 *  @see        MES_AUDIO_GetPhysicalAddress,   MES_AUDIO_GetSizeOfRegisterSet, 
 *              MES_AUDIO_SetBaseAddress,       MES_AUDIO_GetBaseAddress,
 *              MES_AUDIO_CloseModule,
 *              MES_AUDIO_CheckBusy,            MES_AUDIO_CanPowerDown
 */
CBOOL   MES_AUDIO_OpenModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Deinitialize selected module to the proper stage.
 *  @return     \b CTRUE  indicate that Deinitialize is successed. \n
 *              \b CFALSE indicate that Deinitialize is failed.
 *  @see        MES_AUDIO_GetPhysicalAddress,   MES_AUDIO_GetSizeOfRegisterSet, 
 *              MES_AUDIO_SetBaseAddress,       MES_AUDIO_GetBaseAddress,
 *              MES_AUDIO_OpenModule,           
 *              MES_AUDIO_CheckBusy,            MES_AUDIO_CanPowerDown
 */
CBOOL   MES_AUDIO_CloseModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether the selected modules is busy or not.
 *  @return     \b CTRUE  indicate that Module is Busy. \n
 *              \b CFALSE indicate that Module is NOT Busy.
 *  @see        MES_AUDIO_GetPhysicalAddress,   MES_AUDIO_GetSizeOfRegisterSet, 
 *              MES_AUDIO_SetBaseAddress,       MES_AUDIO_GetBaseAddress,
 *              MES_AUDIO_OpenModule,           MES_AUDIO_CloseModule,
 *              MES_AUDIO_CanPowerDown
 */
CBOOL   MES_AUDIO_CheckBusy( void )
{
	return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicaes whether the selected modules is ready to enter power-down stage
 *  @return     \b CTRUE  indicate that Ready to enter power-down stage. \n
 *              \b CFALSE indicate that This module can't enter to power-down stage.
 *  @see        MES_AUDIO_GetPhysicalAddress,   MES_AUDIO_GetSizeOfRegisterSet, 
 *              MES_AUDIO_SetBaseAddress,       MES_AUDIO_GetBaseAddress,
 *              MES_AUDIO_OpenModule,           MES_AUDIO_CloseModule,
 *              MES_AUDIO_CheckBusy            
 */
CBOOL   MES_AUDIO_CanPowerDown( void )
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
 *  @see        MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll,
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
S32     MES_AUDIO_GetInterruptNumber( void )
{
    return  IRQ_AUDIOIF; // ghcstop fix
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a specified interrupt to be enable or disable.
 *  @param[in]  IntNum  Interrupt Number ( 0 : PCM Out, 1 : PCM In ).
 *  @param[in]  Enable  \b CTRUE  indicate that Interrupt Enable. \n
 *                      \b CFALSE indicate that Interrupt Disable.
 *  @return     None.
 *  @see        MES_AUDIO_GetInterruptNumber,       
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll,
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
void    MES_AUDIO_SetInterruptEnable( S32 IntNum, CBOOL Enable )
{
    register U32 ReadValue;

    MES_ASSERT( (0==IntNum) || (1==IntNum) );
    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue = __g_pRegister->AUDIO_IRQ_ENA;

    ReadValue &= ~( 1UL << IntNum );
    ReadValue |= ( (U32)Enable << (IntNum) );

    __g_pRegister->AUDIO_IRQ_ENA = ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is enabled or disabled.
 *  @param[in]  IntNum  Interrupt Number( 0 : PCM Out, 1 : PCM In ).
 *  @return     \b CTRUE  indicate that Interrupt is enabled. \n
 *              \b CFALSE indicate that Interrupt is disabled.
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll,
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
CBOOL   MES_AUDIO_GetInterruptEnable( S32 IntNum )
{
    MES_ASSERT( (0==IntNum) || (1==IntNum) );
    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)( (__g_pRegister->AUDIO_IRQ_ENA) >> (IntNum)) & 0x01;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is pended or not
 *  @param[in]  IntNum  Interrupt Number( 0 : PCM Out, 1 : PCM In ).
 *  @return     \b CTRUE  indicate that Pending is seted. \n
 *              \b CFALSE indicate that Pending is Not Seted.
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,        
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll,
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
CBOOL   MES_AUDIO_GetInterruptPending( S32 IntNum )
{
    MES_ASSERT( (0==IntNum) || (1==IntNum) );
    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)( (__g_pRegister->AUDIO_IRQ_PEND) >> (IntNum)) & 0x01;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear a pending state of specified interrupt.
 *  @param[in]  IntNum  Interrupt number( 0 : PCM Out, 1 : PCM In ).
 *  @return     None.
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll,
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
void    MES_AUDIO_ClearInterruptPending( S32 IntNum )
{
    MES_ASSERT( (0==IntNum) || (1==IntNum) );
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->AUDIO_IRQ_PEND = 1UL << (IntNum) ;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set all interrupts to be enables or disables.
 *  @param[in]  Enable  \b CTRUE  indicate that Set to all interrupt enable. \n
 *                      \b CFALSE indicate that Set to all interrupt disable.
 *  @return     None.
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll,
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
void    MES_AUDIO_SetInterruptEnableAll( CBOOL Enable )
{
    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    if( Enable ){ __g_pRegister->AUDIO_IRQ_ENA = 0x03;  }
    else        { __g_pRegister->AUDIO_IRQ_ENA = 0;  }
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are enable or not.
 *  @return     \b CTRUE  indicate that At least one( or more ) interrupt is enabled. \n
 *              \b CFALSE indicate that All interrupt is disabled.
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptPendingAll,
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
CBOOL   MES_AUDIO_GetInterruptEnableAll( void )
{
    const U32 IRQENA_MASK = 0x03;
    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->AUDIO_IRQ_ENA & IRQENA_MASK )
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
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    
 *              MES_AUDIO_ClearInterruptPendingAll, MES_AUDIO_GetInterruptPendingNumber
 */
CBOOL   MES_AUDIO_GetInterruptPendingAll( void )
{
    const U32 IRQPEND_MASK = 0x03;
    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->AUDIO_IRQ_PEND & IRQPEND_MASK )
    {
        return CTRUE;
    }

    return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear pending state of all interrupts.
 *  @return     None.
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll
 *              MES_AUDIO_GetInterruptPendingNumber
 */
void    MES_AUDIO_ClearInterruptPendingAll( void )
{
    const U32 IRQPEND_MASK = 0x03;
    MES_ASSERT( CNULL != __g_pRegister );

    __g_pRegister->AUDIO_IRQ_PEND = IRQPEND_MASK;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number which has the most prority of pended interrupts
 *  @return     Pending Number( If all pending is not set then return -1 ).\n
 *              0 (PCM Out), 1 (PCM In)
 *  @see        MES_AUDIO_GetInterruptNumber,       MES_AUDIO_SetInterruptEnable,
 *              MES_AUDIO_GetInterruptEnable,       MES_AUDIO_GetInterruptPending,  
 *              MES_AUDIO_ClearInterruptPending,    MES_AUDIO_SetInterruptEnableAll,
 *              MES_AUDIO_GetInterruptEnableAll,    MES_AUDIO_GetInterruptPendingAll
 *              MES_AUDIO_ClearInterruptPendingAll 
 */
S32     MES_AUDIO_GetInterruptPendingNumber( void )  // -1 if None
{
    const U32   POUND_PEND  =   1<<0;
    const U32   PIOVR_PEND  =   1<<1;

    register U32 Pend;

    MES_ASSERT( CNULL != __g_pRegister );

    Pend = __g_pRegister->AUDIO_IRQ_PEND;

    if( Pend & POUND_PEND )
    {
        return 0;
    }
    else if( Pend & PIOVR_PEND )
    {
        return 1;
    }

    return -1;
}

//------------------------------------------------------------------------------
// DMA Interface
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief  Get DMA peripheral index of audio controller's pcm input 
 *  @return DMA peripheral index of audio controller's pcm input 
 *  @see    MES_AUDIO_GetDMAIndex_PCMOut, MES_AUDIO_GetDMABusWidth
 */         
U32     MES_AUDIO_GetDMAIndex_PCMIn( void )
{
    return DMAINDEX_OF_AUDIO_MODULE_PCMIN;
}

/**
 *  @brief  Get DMA peripheral index of pcm output 
 *  @return DMA peripheral index of audio controller's pcm output 
 *  @see    MES_AUDIO_GetDMAIndex_PCMIn, MES_AUDIO_GetDMABusWidth
 */
U32     MES_AUDIO_GetDMAIndex_PCMOut( void )
{
    return DMAINDEX_OF_AUDIO_MODULE_PCMOUT;
}

/**
 *  @brief      Get bus width of audio controller
 *  @return     DMA bus width of audio controller.
 *  @see        MES_AUDIO_GetDMAIndex_PCMIn, MES_AUDIO_GetDMAIndex_PCMOut
 */
U32     MES_AUDIO_GetDMABusWidth( void )
{
    return 16;
}


//------------------------------------------------------------------------------
// Clock Control Interface
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Set a PCLK mode
 *  @param[in]  mode    PCLK mode
 *  @return     None.
 *  @remarks    Audio controller only support MES_PCLKMODE_ALWAYS.\n
 *              If user set to MES_PCLKMODE_DYNAMIC, then audio controller \b NOT operate.
 *  @see        MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
void            MES_AUDIO_SetClockPClkMode( MES_PCLKMODE mode )
{
    const U32 PCLKMODE_POS  =   3;

    register U32 regvalue;
    register struct MES_AUDIO_RegisterSet* pRegister;

	U32 clkmode=0;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

	switch(mode)
	{
    	case MES_PCLKMODE_DYNAMIC:  clkmode = 0;		break;
    	case MES_PCLKMODE_ALWAYS:  	clkmode = 1;		break;
    	default: MES_ASSERT( CFALSE );
	}

    regvalue = pRegister->CLKENB;

    regvalue &= ~(1UL<<PCLKMODE_POS);
    regvalue |= ( clkmode & 0x01 ) << PCLKMODE_POS;

    pRegister->CLKENB = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get current PCLK mode
 *  @return     Current PCLK mode
 *  @see        MES_AUDIO_SetClockPClkMode,         
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
MES_PCLKMODE    MES_AUDIO_GetClockPClkMode( void )
{
    const U32 PCLKMODE_POS  = 3;

    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->CLKENB & ( 1UL << PCLKMODE_POS ) )
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
 *                      0:PLL0, 1:PLL1, 2:None, 3:Bit Clock, 4: Invert Bit Clock\n
 *                      5:AV Clock, 6:Invert AV Clock, 7: CLKGEN0's output clock ( Only clock generator1 use ).
 *  @remarks    Audio controller have two clock generator. so \e Index must set to 0 or 1.
 *  @return     None.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
void    MES_AUDIO_SetClockSource( U32 Index, U32 ClkSrc )
{
    const U32 CLKSRCSEL_POS    = 1;
    const U32 CLKSRCSEL_MASK   = 0x07 << CLKSRCSEL_POS;

    register U32 ReadValue;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( (0!=ClkSrc) || ( (2!=ClkSrc) && (ClkSrc<=6) ) );
    MES_ASSERT( (1!=ClkSrc) || ( (2!=ClkSrc) && (ClkSrc<=7) ) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue = __g_pRegister->CLKGEN[Index];

    ReadValue &= ~CLKSRCSEL_MASK;
    ReadValue |= ClkSrc << CLKSRCSEL_POS;

    __g_pRegister->CLKGEN[Index] = ReadValue;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock source of specified clock generator.
 *  @param[in]  Index   Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @return     Clock source of clock generator \n
 *              0:PLL0, 1:PLL1, 2:None, 3:Bit Clock, 4: Invert Bit Clock\n
 *              5:AV Clock, 6:Invert AV Clock, 7: CLKGEN0's output clock
 *  @remarks    Audio controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
U32             MES_AUDIO_GetClockSource( U32 Index )
{
    const U32 CLKSRCSEL_POS    = 1;
    const U32 CLKSRCSEL_MASK   = 0x07 << CLKSRCSEL_POS;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return ( __g_pRegister->CLKGEN[Index] & CLKSRCSEL_MASK ) >> CLKSRCSEL_POS;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock divisor of specified clock generator.
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @param[in]  Divisor     Clock divisor ( 1 ~ 64 ).
 *  @return     None.
 *  @remarks    Audio controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
void            MES_AUDIO_SetClockDivisor( U32 Index, U32 Divisor )
{
    const U32 CLKDIV_POS   =   4;
    const U32 CLKDIV_MASK  =   0x3F << CLKDIV_POS;

    register U32 ReadValue;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( 1 <= Divisor && Divisor <= 64 );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->CLKGEN[Index];

    ReadValue   &= ~CLKDIV_MASK;
    ReadValue   |= (Divisor-1) << CLKDIV_POS;

    __g_pRegister->CLKGEN[Index] = ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock divisor of specified clock generator.
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @return     Clock divisor ( 1 ~ 64 ).
 *  @remarks    Audio controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
U32             MES_AUDIO_GetClockDivisor( U32 Index )
{
    const U32 CLKDIV_POS   =   4;
    const U32 CLKDIV_MASK  =   0x3F << CLKDIV_POS;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return ((__g_pRegister->CLKGEN[Index] & CLKDIV_MASK) >> CLKDIV_POS) + 1;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Set inverting of output clock
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 );
 *  @param[in]  OutClkInv   \b CTRUE indicate that Output clock Invert.\n
 *                          \b CFALSE indicate that Output clock Normal.
 *  @return     None.
 *  @remarks    Audio controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
void            MES_AUDIO_SetClockOutInv( U32 Index, CBOOL OutClkInv )
{
    const U32 OUTCLKINV_POS    =   0;
    const U32 OUTCLKINV_MASK   =   1UL << OUTCLKINV_POS;

    register U32 ReadValue;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( (0==OutClkInv) ||(1==OutClkInv) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =    __g_pRegister->CLKGEN[Index];

    ReadValue   &=  ~OUTCLKINV_MASK;
    ReadValue   |=  OutClkInv << OUTCLKINV_POS;

    __g_pRegister->CLKGEN[Index]    =   ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get invert status of output clock.
 *  @param[in]  Index       Select clock generator( 0 : clock generator 0, 1: clock generator1 )
 *  @return     \b CTRUE indicate that Output clock is Inverted. \n
 *              \b CFALSE indicate that Output clock is Normal.
 *  @remarks    Audio controller have two clock generator. so \e Index must set to 0 or 1.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
CBOOL           MES_AUDIO_GetClockOutInv( U32 Index )
{
    const U32 OUTCLKINV_POS    =   0;
    const U32 OUTCLKINV_MASK   =   1UL << OUTCLKINV_POS;

    MES_ASSERT( 2 > Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)((__g_pRegister->CLKGEN[Index] & OUTCLKINV_MASK ) >> OUTCLKINV_POS);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock output pin's direction.
 *  @param[in]  Index       Select clock generator( 1: clock generator1 );
 *  @param[in]  OutClkEnb   \b CTRUE indicate that Pin's direction is Output.\n
 *                          \b CFALSE indicate that Pin's direction is Iutput.
 *  @return     None.
 *  @remarks    Decides the I/O direction when the output clock is connected to a bidirectional PAD.\n
 *              Only clock generator 1 can set the output pin's direction.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
void            MES_AUDIO_SetClockOutEnb( U32 Index, CBOOL OutClkEnb )
{
    const U32   OUTCLKENB1_POS  =   15;
    const U32   OUTCLKENB1_MASK = 1UL << OUTCLKENB1_POS;

    register U32 ReadValue;

    MES_ASSERT( 1 == Index );
    MES_ASSERT( (0==OutClkEnb) ||(1==OutClkEnb) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->CLKGEN[Index];

    ReadValue   &=  ~OUTCLKENB1_MASK;

    if( ! OutClkEnb )
    {
           ReadValue    |=  1UL << OUTCLKENB1_POS;
    }

    __g_pRegister->CLKGEN[Index] = ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock output pin's direction.
 *  @param[in]  Index       Select clock generator( 1: clock generator1 );
 *  @return     \b CTRUE indicate that Pin's direction is Output.\n
 *              \b CFALSE indicate that Pin's direction is Input.
 *  @remarks    Only clock generator 1 can set the output pin's direction. so \e Index must set to 1.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           
 *              MES_AUDIO_SetClockDivisorEnable,    MES_AUDIO_GetClockDivisorEnable
 */
CBOOL           MES_AUDIO_GetClockOutEnb( U32 Index )
{
    const U32   OUTCLKENB1_POS  =   15;
    const U32   OUTCLKENB1_MASK = 1UL << OUTCLKENB1_POS;

    MES_ASSERT( 1 == Index );
    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->CLKGEN[Index] & OUTCLKENB1_MASK )
    {
        return CFALSE;
    }

    return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock generator's operation
 *  @param[in]  Enable  \b CTRUE indicate that Enable of clock generator. \n
 *                      \b CFALSE indicate that Disable of clock generator.
 *  @return     None.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_GetClockDivisorEnable
 */
void            MES_AUDIO_SetClockDivisorEnable( CBOOL Enable )
{
    const U32   CLKGENENB_POS   =   2;
    const U32   CLKGENENB_MASK  =   1UL << CLKGENENB_POS;

    register U32 ReadValue;

    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->CLKENB;

    ReadValue   &=  ~CLKGENENB_MASK;
    ReadValue   |= (U32)Enable << CLKGENENB_POS;

    __g_pRegister->CLKENB   =   ReadValue;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Get status of clock generator's operation
 *  @return     \b CTRUE indicate that Clock generator is enabled.\n
 *              \b CFALSE indicate that Clock generator is disabled.
 *  @see        MES_AUDIO_SetClockPClkMode,         MES_AUDIO_GetClockPClkMode,
 *              MES_AUDIO_SetClockSource,           MES_AUDIO_GetClockSource,
 *              MES_AUDIO_SetClockDivisor,          MES_AUDIO_GetClockDivisor,
 *              MES_AUDIO_SetClockOutInv,           MES_AUDIO_GetClockOutInv,
 *              MES_AUDIO_SetClockOutEnb,           MES_AUDIO_GetClockOutEnb,
 *              MES_AUDIO_SetClockDivisorEnable    
 */
CBOOL           MES_AUDIO_GetClockDivisorEnable( void )
{
    const U32   CLKGENENB_POS   =   2;
    const U32   CLKGENENB_MASK  =   1UL << CLKGENENB_POS;

    MES_ASSERT( CNULL != __g_pRegister );

    return  (CBOOL)( (__g_pRegister->CLKENB & CLKGENENB_MASK) >> CLKGENENB_POS );
}


//--------------------------------------------------------------------------
// Audio Configuration Function
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Set I2S's Controller Mode ( Master or Slave )
 *  @param[in]  Enable  \b CTRUE indicate that Master mode. \n
 *                      \b CFALSE indicate that Slave mode.
 *  @return     None.
 *  @see        MES_AUDIO_GetI2SMasterMode,
 *              MES_AUDIO_SetSyncPeriod,    MES_AUDIO_GetSyncPeriod,   
 *              MES_AUDIO_SetInterfaceMode, MES_AUDIO_GetInterfaceMode 
 */
void      MES_AUDIO_SetI2SMasterMode( CBOOL Enable )        
{
    const U32   MST_SLV_POS     =   0;
    const U32   MST_SLV_MASK    =   1UL << MST_SLV_POS;
    register U32 ReadValue;
    
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->I2S_CONFIG;
    
    ReadValue   &=  ~MST_SLV_MASK;

    if( !Enable )
    {
        ReadValue   |=  1UL << MST_SLV_POS;        
    }
    
    __g_pRegister->I2S_CONFIG   =   (U16)ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get I2S's Controller Mode ( Master or Slave )
 *  @return     \b CTRUE indicate that Master mode.\n
 *              \b CFALSE indicate that Slave mode.
 *  @see        MES_AUDIO_SetI2SMasterMode, 
 *              MES_AUDIO_SetSyncPeriod,    MES_AUDIO_GetSyncPeriod,   
 *              MES_AUDIO_SetInterfaceMode, MES_AUDIO_GetInterfaceMode 
 */
CBOOL     MES_AUDIO_GetI2SMasterMode( void )        
{
    const U32   MST_SLV_POS     =   0;
    const U32   MST_SLV_MASK    =   1UL << MST_SLV_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );
    
    if(__g_pRegister->I2S_CONFIG & MST_SLV_MASK)
    {
        return CFALSE;   
    }

    return CTRUE;   
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set audio interface mode.
 *  @param[in]  mode    Interface mode ( 0 : I2S, 1 : None, 2 : Left-Justified, 3 : Right-Justified )
 *  @return     None.
 *  @see        MES_AUDIO_SetI2SMasterMode, MES_AUDIO_GetI2SMasterMode,
 *              MES_AUDIO_GetInterfaceMode 
 *              MES_AUDIO_SetSyncPeriod,    MES_AUDIO_GetSyncPeriod,   
 */
void            MES_AUDIO_SetInterfaceMode( MES_AUDIO_IF mode )
{
    const U32   IF_MODE_POS     =   6;
    const U32   IF_MODE_MASK    =   0x03UL << IF_MODE_POS;
    register U32 ReadValue;
    
    MES_ASSERT( (0==mode) || (2==mode) || (3==mode) );
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue   =   __g_pRegister->I2S_CONFIG;
    
    ReadValue   &=  ~IF_MODE_MASK;
    ReadValue   |=  (U32)mode << IF_MODE_POS;
    
    __g_pRegister->I2S_CONFIG   =   (U16)ReadValue;     

}

//------------------------------------------------------------------------------
/**
 *  @brief      Get status of audio interface mode.
 *  @return     Interface mode ( 0 : I2S, 1 : None, 2 : Left-Justified, 3 : Right-Justified  )
 *  @see        MES_AUDIO_SetI2SMasterMode, MES_AUDIO_GetI2SMasterMode,
 *              MES_AUDIO_SetInterfaceMode, 
 *              MES_AUDIO_SetSyncPeriod,    MES_AUDIO_GetSyncPeriod   
 */
MES_AUDIO_IF    MES_AUDIO_GetInterfaceMode( void )
{
    const U32   IF_MODE_POS     =   6;
    const U32   IF_MODE_MASK    =   0x03UL << IF_MODE_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );
    
    return  (MES_AUDIO_IF)( (__g_pRegister->I2S_CONFIG & IF_MODE_MASK) >> IF_MODE_POS );        
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set Sync Period of I2S ( 32fs, 48fs, 64fs )
 *  @param[in]  period     Sync Period ( 32fs, 48fs, 64fs )
 *  @return     None.
 *  @remarks    The setting is required in mater mode also.
 *  @see        MES_AUDIO_SetI2SMasterMode, MES_AUDIO_GetI2SMasterMode,
 *              MES_AUDIO_SetInterfaceMode, MES_AUDIO_GetInterfaceMode 
 *              MES_AUDIO_GetSyncPeriod,   
 */
void   MES_AUDIO_SetSyncPeriod( U32 period )
{
    const U32   SYNC_PERIOD_POS     =   4;
    const U32   SYNC_PERIOD_MASK    =   0x03UL << SYNC_PERIOD_POS;

    register U32 ReadValue;
    register U32 SetValue=0;
    
    MES_ASSERT( (32==period) ||(48==period) || (64==period) );
    MES_ASSERT( CNULL != __g_pRegister );

    switch( period )
    {
        case    32: SetValue = 0;   break;
        case    48: SetValue = 1;   break;
        case    64: SetValue = 2;   break;                   
        default:    MES_ASSERT( CFALSE );
    }

    ReadValue   =   __g_pRegister->I2S_CONFIG;
    
    ReadValue   &=  ~SYNC_PERIOD_MASK;
    ReadValue   |=  (U32)SetValue << SYNC_PERIOD_POS;
    
    __g_pRegister->I2S_CONFIG   =   (U16)ReadValue;        
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get Sync Period of I2S ( 32fs, 48fs, 64fs )
 *  @return     Sync Period ( 32fs, 48fs, 64fs )
 *  @see        MES_AUDIO_SetI2SMasterMode, MES_AUDIO_GetI2SMasterMode,
 *              MES_AUDIO_SetInterfaceMode, MES_AUDIO_GetInterfaceMode, 
 *              MES_AUDIO_SetSyncPeriod    
 */
U32    MES_AUDIO_GetSyncPeriod( void )
{
    const U32   SYNC_PERIOD_POS     =   4;
    const U32   SYNC_PERIOD_MASK    =   0x03UL << SYNC_PERIOD_POS;
    
    register U32 ReadValue;
    register U32 RetValue=0;    
    
    MES_ASSERT( CNULL != __g_pRegister );

    ReadValue = ((__g_pRegister->I2S_CONFIG & SYNC_PERIOD_MASK) >> SYNC_PERIOD_POS );        
    
    switch( ReadValue )
    {
        case    0: RetValue = 32;   break;
        case    1: RetValue = 48;   break;
        case    2: RetValue = 64;   break;                   
        default:    MES_ASSERT( CFALSE );
    }
    
    return RetValue;
}


//--------------------------------------------------------------------------
// Audio Control Function
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Set I2S controller Link On
 *  @return     None.
 *  @remarks    If user want I2S link off, reset the I2S controller 
 *  @see                                            MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
void  MES_AUDIO_SetI2SLinkOn( void )
{
    const U32 I2SLINK_RUN_MASK  = 1UL << 1;
    
    register struct MES_AUDIO_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( CNULL != __g_pRegister );
    
    pRegister   =   __g_pRegister;
    
    ReadValue   =   pRegister->I2S_CTRL;
    
    ReadValue   |=  I2SLINK_RUN_MASK;
    
    pRegister->I2S_CTRL =   (U16)ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get link status of I2S controller
 *  @return     \b CTRUE indicate that Link On. \n
 *              \b CFALSE indicate that None. 
 *  @see        MES_AUDIO_SetI2SLinkOn,             
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
CBOOL MES_AUDIO_GetI2SLinkOn( void )
{
    const U32 I2SLINK_RUN_POS   = 1;
    const U32 I2SLINK_RUN_MASK  = 1UL << I2SLINK_RUN_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );    
    
    return (CBOOL)((__g_pRegister->I2S_CTRL & I2SLINK_RUN_MASK) >> I2SLINK_RUN_POS);
}

 //------------------------------------------------------------------------------
/**
 *  @brief      I2S Controller Reset
 *  @param[in]  Enable     \b CTRUE indicate that Contoller Reset.\n
 *                         \b CFALSE indicate that Nomal Operation.
 *  @return     None.
 *  @remarks    After Reset You should set normal operation
 *  @code
 *              MES_AUDIO_SetI2SControllerReset( CTRUE );     // I2S Controller Reset
 *              MES_AUDIO_SetI2SControllerReset( CFALSE );    // Normal Operation
 *  @endcode    
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *                                                  MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
void  MES_AUDIO_SetI2SControllerReset( CBOOL Enable )
{
    const U32 I2S_EN_MASK  = 1UL << 0;
    
    register struct MES_AUDIO_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister   =   __g_pRegister;
    
    ReadValue   =   pRegister->I2S_CTRL;
    
    ReadValue   &=  ~I2S_EN_MASK;

    if( Enable )
    {
        ReadValue = 0x00;      
    }
    else
    {
        ReadValue   |=  (U32)(Enable^1);    // exclusive OR ( 0=>1, 1=>0 )
    }
    
    pRegister->I2S_CTRL  =   (U16)ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get status of I2S Controller
 *  @return     \b CTRUE indicate that Contoller Reset.\n
 *              \b CFALSE indicate that Nomal Operation.
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
CBOOL MES_AUDIO_GetI2SControllerReset( void )   
{
    const U32 I2S_EN_MASK   = 1UL << 0;    
    
    MES_ASSERT( CNULL != __g_pRegister );    
    
    if(__g_pRegister->I2S_CTRL & I2S_EN_MASK)
    {
        return CFALSE;   
    }    

    return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set I2S controller's output operation
 *  @param[in]  Enable  \b CTRUE indicate that Output Enable. \n
 *                      \b CFALSE indicate that Output Disable.
 *  @return     None.
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *                                                  MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
void  MES_AUDIO_SetI2SOutputEnable( CBOOL Enable )
{
    const U32 I2SO_EN_POS   = 1;
    const U32 I2SO_EN_MASK  = 1UL << I2SO_EN_POS;
    
    register struct MES_AUDIO_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister   =   __g_pRegister;
    
    ReadValue   =   pRegister->I2S_CONFIG;
    
    ReadValue   &=  ~I2SO_EN_MASK;
    ReadValue   |=  (U32)Enable << I2SO_EN_POS;    
    
    pRegister->I2S_CONFIG  =   (U16)ReadValue;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get output operation status of I2S controller
 *  @return     \b CTRUE indicate that Output is Enabled. \n
 *              \b CFALSE indicate that Output is Disabled.
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
CBOOL MES_AUDIO_GetI2SOutputEnable( void )
{
    const U32 I2SO_EN_POS   = 1;
    const U32 I2SO_EN_MASK  = 1UL << I2SO_EN_POS;

    MES_ASSERT( CNULL != __g_pRegister );    
    
    return  (CBOOL)( (__g_pRegister->I2S_CONFIG & I2SO_EN_MASK) >> I2SO_EN_POS );        
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set I2S controller's input operation
 *  @param[in]  Enable  \b CTRUE indicate that Input Enable. \n
 *                      \b CFALSE indicate that Input Disable.
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *                                                  MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
void  MES_AUDIO_SetI2SInputEnable( CBOOL Enable )
{
    const U32 I2SI_EN_POS   = 2;
    const U32 I2SI_EN_MASK  = 1UL << I2SI_EN_POS;
    
    register struct MES_AUDIO_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister   =   __g_pRegister;
    
    ReadValue   =   pRegister->I2S_CONFIG;
    
    ReadValue   &=  ~I2SI_EN_MASK;
    ReadValue   |=  (U32)Enable << I2SI_EN_POS;    
    
    pRegister->I2S_CONFIG  =   (U16)ReadValue;        
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get Input operation status of I2S controller
 *  @return     \b CTRUE indicate that Input is Enabled. \n
 *              \b CFALSE indicate that Input is Disabled.
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        
 *              MES_AUDIO_SetI2SLoopBackEnable,     MES_AUDIO_GetI2SLoopBackEnable
 */
CBOOL MES_AUDIO_GetI2SInputEnable( void )
{
    const U32 I2SI_EN_POS   = 2;
    const U32 I2SI_EN_MASK  = 1UL << I2SI_EN_POS;

    MES_ASSERT( CNULL != __g_pRegister );    
    
    return  (CBOOL)( (__g_pRegister->I2S_CONFIG & I2SI_EN_MASK) >> I2SI_EN_POS );            
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set Loop Back operation
 *  @param[in]  Enable  \b CTRUE indicate that Loop Back mode Enable. \n
 *                      \b CFALSE indicate that Loop Back mode Disable.
 *  @return     None.
 *  @remarks    User need to set MES_AUDIO_SetI2SInputEnable( CTRUE ) for look back operation.
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *                                                  MES_AUDIO_GetI2SLoopBackEnable 
 */
void  MES_AUDIO_SetI2SLoopBackEnable( CBOOL Enable )
{
    const U32 LOOP_BACK_POS   = 3;
    const U32 LOOP_BACK_MASK  = 1UL << LOOP_BACK_POS;
    
    register struct MES_AUDIO_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister   =   __g_pRegister;
    
    ReadValue   =   pRegister->I2S_CONFIG;
    
    ReadValue   &=  ~LOOP_BACK_MASK;
    ReadValue   |=  (U32)Enable << LOOP_BACK_POS;    
    
    pRegister->I2S_CONFIG  =   (U16)ReadValue;            
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get status of Loop Back operation
 *  @return     \b CTRUE indicate that Loop Back mode is Enabled.\n
 *              \b CFALSE indicate that Loop Back mode is Disabled.
 *  @see        MES_AUDIO_SetI2SLinkOn,             MES_AUDIO_GetI2SLinkOn,
 *              MES_AUDIO_SetI2SControllerReset,    MES_AUDIO_GetI2SControllerReset,
 *              MES_AUDIO_SetI2SOutputEnable,       MES_AUDIO_GetI2SOutputEnable,
 *              MES_AUDIO_SetI2SInputEnable,        MES_AUDIO_GetI2SInputEnable,
 *              MES_AUDIO_SetI2SLoopBackEnable     
 */
CBOOL MES_AUDIO_GetI2SLoopBackEnable( void )
{
    const U32 LOOP_BACK_POS   = 3;
    const U32 LOOP_BACK_MASK  = 1UL << LOOP_BACK_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );
    
    return  (CBOOL)( (__g_pRegister->I2S_CONFIG & LOOP_BACK_MASK) >> LOOP_BACK_POS );        
}


//--------------------------------------------------------------------------
// Audio Buffer Function
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Set audio's output buffer operation.
 *  @param[in]  Enable  \b CTRUE indicate that PCM output buffer Enable. \n
 *                      \b CFALSE indicate that PCM output buffer Disable.
 *  @return     None.
 *  @remarks    Audio output buffer's enable and disable means that setting data transfer to the 
 *              Audio buffer throught DMA.\n
 *              Enable( DMA \b can transfer data to Audio's output buffer )\n
 *              Disable( DMA \b can't transfer data to Audio's output buffer )
 *  @see                                                MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
void  MES_AUDIO_SetAudioBufferPCMOUTEnable( CBOOL Enable )
{
    const U32 PCMOBUF_EN_POS   = 0;
    const U32 PCMOBUF_EN_MASK  = 1UL << PCMOBUF_EN_POS;
    
    register struct MES_AUDIO_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister   =   __g_pRegister;
    
    ReadValue   =   pRegister->AUDIO_BUFF_CTRL;
    
    ReadValue   &=  ~PCMOBUF_EN_MASK;
    ReadValue   |=  (U32)Enable << PCMOBUF_EN_POS;    
    
    pRegister->AUDIO_BUFF_CTRL  =   (U16)ReadValue;            
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get operation status of audio output buffer
 *  @return     \b CTRUE indicate that PCM output buffer is Enabled. \n
 *              \b CFALSE indicate that PCM output buffer is Disabled.
 *  @remarks    Audio output buffer's enable and disable means that setting data transfer to the 
 *              Audio buffer throught DMA.\n
 *              Enable( DMA \b can transfer data to Audio's output buffer )\n
 *              Disable( DMA \b can't transfer data to Audio's output buffer )
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
CBOOL MES_AUDIO_GetAudioBufferPCMOUTEnable( void )
{
    const U32 PCMOBUF_EN_POS   = 0;
    const U32 PCMOBUF_EN_MASK  = 1UL << PCMOBUF_EN_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );
    
    return  (CBOOL)( (__g_pRegister->AUDIO_BUFF_CTRL & PCMOBUF_EN_MASK) >> PCMOBUF_EN_POS );        
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set audio's input buffer operation.
 *  @param[in]  Enable  \b CTRUE indicate that PCM input buffer Enable. \n
 *                      \b CFALSE indicate that PCM input buffer Disable.
 *  @return     None.
 *  @remarks    Audio input buffer's enable and disable means that setting data transfer to the 
 *              Audio buffer throught DMA.\n
 *              Enable( DMA \b can receive data from  Audio's input buffer )\n
 *              Disable( DMA \b can't receive data from Audio's input buffer )
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *                                                      MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
void  MES_AUDIO_SetAudioBufferPCMINEnable( CBOOL Enable )
{
    const U32 PCMIBUF_EN_POS   = 1;
    const U32 PCMIBUF_EN_MASK  = 1UL << PCMIBUF_EN_POS;
    
    register struct MES_AUDIO_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister   =   __g_pRegister;
    
    ReadValue   =   pRegister->AUDIO_BUFF_CTRL;
    
    ReadValue   &=  ~PCMIBUF_EN_MASK;
    ReadValue   |=  (U32)Enable << PCMIBUF_EN_POS;    
    
    pRegister->AUDIO_BUFF_CTRL  =   (U16)ReadValue;            
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get operation status of audio input buffer
 *  @return     \b CTRUE indicate that PCM input buffer is Enabled. \n
 *              \b CFALSE indicate that PCM input buffer is Disabled.
 *  @remarks    Audio input buffer's enable and disable means that setting data transfer to the 
 *              Audio buffer throught DMA.\n
 *              Enable( DMA \b can receive data from  Audio's input buffer )\n
 *              Disable( DMA \b can't receive data from Audio's input buffer )
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
CBOOL MES_AUDIO_GetAudioBufferPCMINEnable( void )
{
    const U32 PCMIBUF_EN_POS   = 1;
    const U32 PCMIBUF_EN_MASK  = 1UL << PCMIBUF_EN_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );
    
    return  (CBOOL)( (__g_pRegister->AUDIO_BUFF_CTRL & PCMIBUF_EN_MASK) >> PCMIBUF_EN_POS );        
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set data width of pcm output buffer
 *  @param[in]  DataWidth   Data width ( 16 bit, 18 bit )
 *  @return     None.
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *                                                      MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
void  MES_AUDIO_SetPCMOUTDataWidth( U32 DataWidth )
{
    const U32   PO_WIDTH_POS    =   0;
    const U32   PO_WIDTH_MASK   =  0x03UL << PO_WIDTH_POS;
    
    U32 ReadValue;
    U32 SetValue=0;

    MES_ASSERT( (16==DataWidth) || (18==DataWidth) );
    MES_ASSERT( CNULL != __g_pRegister );
    
    switch( DataWidth )
    {
        case 16:    SetValue = 0; break;
        case 18:    SetValue = 3; break;   
        default:    MES_ASSERT( CFALSE );
    }
    
    ReadValue   =   __g_pRegister->AUDIO_BUFF_CONFIG;
    
    ReadValue   &=  ~PO_WIDTH_MASK;
    ReadValue   |=  (U32)SetValue << PO_WIDTH_POS;    
    
    __g_pRegister->AUDIO_BUFF_CONFIG  =   (U16)ReadValue;        
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get data width of pcm output buffer
 *  @return     Data width ( 16 bit, 18 bit )
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
U32   MES_AUDIO_GetPCMOUTDataWidth( void )
{
    const U32   PO_WIDTH_POS    =   0;
    const U32   PO_WIDTH_MASK   =  0x03UL << PO_WIDTH_POS;
    
    U32 ReadValue;
    U32 RetValue=0;

    MES_ASSERT( CNULL != __g_pRegister );    
    
    ReadValue = ((__g_pRegister->AUDIO_BUFF_CONFIG & PO_WIDTH_MASK) >> PO_WIDTH_POS );        
    
    switch( ReadValue )
    {
        case    0:  RetValue = 16;  break;
        case    3:  RetValue = 18;  break;
        default:    MES_ASSERT( CFALSE );        
    }
    
    return RetValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set data width of pcm input buffer
 *  @param[in]  DataWidth   Data width ( 16 bit )
 *  @return     None.
 *  @remarks    PCM input buffer surpport only 16bit data width.
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *                                                      MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
void  MES_AUDIO_SetPCMINDataWidth( U32 DataWidth )
{
    const U32   PI_WIDTH_POS    =   2;
    const U32   PI_WIDTH_MASK   =  0x03UL << PI_WIDTH_POS;
    
    U32 ReadValue;
    U32 SetValue=0;

    MES_ASSERT( 16==DataWidth );
    MES_ASSERT( CNULL != __g_pRegister );
    
    switch( DataWidth )
    {
        case 16:    SetValue = 0; break;
        default:    MES_ASSERT( CFALSE );
    }
    
    ReadValue   =   __g_pRegister->AUDIO_BUFF_CONFIG;
    
    ReadValue   &=  ~PI_WIDTH_MASK;
    ReadValue   |=  (U32)SetValue << PI_WIDTH_POS;    
    
    __g_pRegister->AUDIO_BUFF_CONFIG  =   (U16)ReadValue;            
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get data width of pcm input buffer
 *  @return     Data width ( 16 bit )
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            
 *              MES_AUDIO_IsPCMInBufferReady,           MES_AUDIO_IsPCMOutBufferReady
 */
U32   MES_AUDIO_GetPCMINDataWidth( void )
{
    const U32   PI_WIDTH_POS    =   2;
    const U32   PI_WIDTH_MASK   =  0x03UL << PI_WIDTH_POS;

    
    U32 ReadValue;
    U32 RetValue=0;

    MES_ASSERT( CNULL != __g_pRegister );    
    
    ReadValue = ((__g_pRegister->AUDIO_BUFF_CONFIG & PI_WIDTH_MASK) >> PI_WIDTH_POS );        
    
    switch( ReadValue )
    {
        case    0:  RetValue = 16;  break;
        default:    MES_ASSERT( CFALSE );        
    }
    
    return RetValue;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Check PCM input buffer's status
 *  @return     \b CTRUE indicate that Input buffer is ready. \n
 *              \b CFALSE indicate that None. 
 *  @remarks    Input buffer's ready means that Input buffer have some space to receive data.
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *                                                      MES_AUDIO_IsPCMOutBufferReady
 */
CBOOL MES_AUDIO_IsPCMInBufferReady( void )
{
    const U32   PIBUF_RDY_POS   = 1;
    const U32   PIBUF_RDY_MASK  = 1UL << PIBUF_RDY_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );        
    
    return (CBOOL)((__g_pRegister->AUDIO_STATUS1 & PIBUF_RDY_MASK) >> PIBUF_RDY_POS );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Check PCM output buffer's status
 *  @return     \b CTRUE indicate that output buffer is ready. \n
 *              \b CFALSE indicate that None.
 *  @remarks    Output buffer's ready means that Output buffer have some data to send.
 *  @see        MES_AUDIO_SetAudioBufferPCMOUTEnable,   MES_AUDIO_GetAudioBufferPCMOUTEnable,
 *              MES_AUDIO_SetAudioBufferPCMINEnable,    MES_AUDIO_GetAudioBufferPCMINEnable,                                    
 *              MES_AUDIO_SetPCMOUTDataWidth,           MES_AUDIO_GetPCMOUTDataWidth,
 *              MES_AUDIO_SetPCMINDataWidth,            MES_AUDIO_GetPCMINDataWidth,
 *              MES_AUDIO_IsPCMInBufferReady
 */
CBOOL MES_AUDIO_IsPCMOutBufferReady( void )
{
    const U32   POBUF_RDY_POS   = 0;
    const U32   POBUF_RDY_MASK  = 1UL << POBUF_RDY_POS;
    
    MES_ASSERT( CNULL != __g_pRegister );        
    
    return (CBOOL)(__g_pRegister->AUDIO_STATUS1 & POBUF_RDY_MASK); 
}


//--------------------------------------------------------------------------
// Audio State Function
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Get I2S controller's state.
 *  @return     I2S controller's state ( MES_AUDIO_I2SSTATE_IDLE, MES_AUDIO_I2SSTATE_READY, MES_AUDIO_I2SSTATE_RUN )
 *  @remarks    Idle  : controller OFF, link OFF. \n
 *              ready : controller ON, link OFF. \n
 *              run   : controller ON, link ON.  \n
 *  @code
 *          State = MES_AUDIO_GetI2SState();
 *
 *          if( State & MES_AUDIO_I2SSTATE_IDLE )
 *          {
 *              // I2S's current state is IDLE...
 *          }
 *          ...
 *  @endcode
 */
MES_AUDIO_I2SSTATE    MES_AUDIO_GetI2SState( void )
{
    const U32 I2S_FSM_MASK   =   0x07 << 0;

    MES_ASSERT( CNULL != __g_pRegister );            
    
    return (MES_AUDIO_I2SSTATE)(__g_pRegister->AUDIO_STATUS0 & I2S_FSM_MASK );
}






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
//	Module     : I2C
//	File       : mes_i2c.c
//	Description:
//	Author     :
//	History    :
//------------------------------------------------------------------------------

#include <linux/module.h>	// For AESOP
#include "mes_i2c.h"

/// @brief  I2C Module's Register List
struct  MES_I2C_RegisterSet
{
    volatile U32 ICCR;              ///< 0x00 : I2C Control Register
    volatile U32 ICSR;              ///< 0x04 : I2C Status Register
    volatile U32 IAR;               ///< 0x08 : I2C Address Register
    volatile U32 IDSR;              ///< 0x0C : I2C Data Register
	volatile U32 QCNT_MAX;	        ///< 0x10 : I2C Quarter Period Register
	const    U32 __Reserved0[4];	///< 0x14, 0x18, 0x1C, 0x20 : Reserved region        
    volatile U32 PEND;		        ///< 0x24 : I2C IRQ PEND Register
    const    U32 __Reserved1[0x36]; ///< 0x28 ~ 0xFC : Reserved region
    volatile U32 CLKENB;            ///< 0x100 : Clock Enable Register.
};

static  struct
{
    struct MES_I2C_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_I2C_MODULE];
//} __g_ModuleVariables[NUMBER_OF_I2C_MODULE] = { CNULL, }; // ghcstop delete

static CBOOL bInit = CFALSE; // ghcstop fix


//------------------------------------------------------------------------------
// Module Interface
//------------------------------------------------------------------------------
/**
 *  @brief  Initialize of prototype enviroment & local variables.
 *  @return \b CTRUE  indicate that Initialize is successed.\n
 *          \b CFALSE indicate that Initialize is failed.\n
 *  @see    MES_I2C_SelectModule,
 *          MES_I2C_GetCurrentModule,   MES_I2C_GetNumberOfModule
 */
CBOOL   MES_I2C_Initialize( void )
{
	//static CBOOL bInit = CFALSE; // ghcstop fix	
	U32 i;
	
	if( CFALSE == bInit )
	{
		
		for( i=0; i < NUMBER_OF_I2C_MODULE; i++ )
		{
			__g_ModuleVariables[i].pRegister = CNULL;
		}
		
		bInit = CTRUE;
	}
	
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get number of modules in the chip.
 *  @return     Module's number.
 *  @see        MES_I2C_Initialize,         MES_I2C_SelectModule,
 *              MES_I2C_GetCurrentModule
 */
U32     MES_I2C_GetNumberOfModule( void )
{
    return NUMBER_OF_I2C_MODULE;
}

//------------------------------------------------------------------------------
// Basic Interface
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *  @brief      Get module's physical address.
 *  @return     Module's physical address
 *  @see        MES_I2C_GetSizeOfRegisterSet,
 *              MES_I2C_SetBaseAddress,       MES_I2C_GetBaseAddress,
 *              MES_I2C_OpenModule,           MES_I2C_CloseModule,
 *              MES_I2C_CheckBusy,            MES_I2C_CanPowerDown
 */
#if 0 // ghcstop delete
U32     MES_I2C_GetPhysicalAddress( U32 ModuleIndex )
{
    MES_ASSERT( NUMBER_OF_I2C_MODULE > ModuleIndex );

    return  (U32)( PHY_BASEADDR_I2C_MODULE + (OFFSET_OF_I2C_MODULE * ModuleIndex) );
}
#endif

//------------------------------------------------------------------------------
/**
 *  @brief      Get a size, in byte, of register set.
 *  @return     Size of module's register set.
 *  @see        MES_I2C_GetPhysicalAddress,
 *              MES_I2C_SetBaseAddress,       MES_I2C_GetBaseAddress,
 *              MES_I2C_OpenModule,           MES_I2C_CloseModule,
 *              MES_I2C_CheckBusy,            MES_I2C_CanPowerDown
 */
U32     MES_I2C_GetSizeOfRegisterSet( U32 ModuleIndex )
{
    return sizeof( struct MES_I2C_RegisterSet );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a base address of register set.
 *  @param[in]  BaseAddress Module's base address
 *  @return     None.
 *  @see        MES_I2C_GetPhysicalAddress,   MES_I2C_GetSizeOfRegisterSet,
 *              MES_I2C_GetBaseAddress,
 *              MES_I2C_OpenModule,           MES_I2C_CloseModule,
 *              MES_I2C_CheckBusy,            MES_I2C_CanPowerDown
 */
void    MES_I2C_SetBaseAddress( U32 ModuleIndex, U32 BaseAddress )
{
    MES_ASSERT( 0 != BaseAddress );
    MES_ASSERT( NUMBER_OF_I2C_MODULE > ModuleIndex );

    __g_ModuleVariables[ModuleIndex].pRegister = (struct MES_I2C_RegisterSet *)BaseAddress;
    //__g_ModuleVariables[ModuleIndex].pRegister = (struct MES_I2C_RegisterSet *)BaseAddress; // ghcstop delete
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a base address of register set
 *  @return     Module's base address.
 *  @see        MES_I2C_GetPhysicalAddress,   MES_I2C_GetSizeOfRegisterSet,
 *              MES_I2C_SetBaseAddress,
 *              MES_I2C_OpenModule,           MES_I2C_CloseModule,
 *              MES_I2C_CheckBusy,            MES_I2C_CanPowerDown
 */
U32     MES_I2C_GetBaseAddress( U32 ModuleIndex )
{
    MES_ASSERT( NUMBER_OF_I2C_MODULE > ModuleIndex );

    return (U32)__g_ModuleVariables[ModuleIndex].pRegister;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Initialize selected modules with default value.
 *  @return     \b CTRUE  indicate that Initialize is successed. \n
 *              \b CFALSE indicate that Initialize is failed.
 *  @see        MES_I2C_GetPhysicalAddress,   MES_I2C_GetSizeOfRegisterSet,
 *              MES_I2C_SetBaseAddress,       MES_I2C_GetBaseAddress,
 *              MES_I2C_CloseModule,
 *              MES_I2C_CheckBusy,            MES_I2C_CanPowerDown
 */
CBOOL   MES_I2C_OpenModule( U32 ModuleIndex )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Deinitialize selected module to the proper stage.
 *  @return     \b CTRUE  indicate that Deinitialize is successed. \n
 *              \b CFALSE indicate that Deinitialize is failed.
 *  @see        MES_I2C_GetPhysicalAddress,   MES_I2C_GetSizeOfRegisterSet,
 *              MES_I2C_SetBaseAddress,       MES_I2C_GetBaseAddress,
 *              MES_I2C_OpenModule,
 *              MES_I2C_CheckBusy,            MES_I2C_CanPowerDown
 */
CBOOL   MES_I2C_CloseModule( U32 ModuleIndex )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether the selected modules is busy or not.
 *  @return     \b CTRUE  indicate that Module is Busy. \n
 *              \b CFALSE indicate that Module is NOT Busy.
 *  @see        MES_I2C_GetPhysicalAddress,   MES_I2C_GetSizeOfRegisterSet,
 *              MES_I2C_SetBaseAddress,       MES_I2C_GetBaseAddress,
 *              MES_I2C_OpenModule,           MES_I2C_CloseModule,
 *              MES_I2C_CanPowerDown
 */
CBOOL   MES_I2C_CheckBusy( U32 ModuleIndex )
{
    return MES_I2C_IsBusy(ModuleIndex);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicaes whether the selected modules is ready to enter power-down stage
 *  @return     \b CTRUE  indicate that Ready to enter power-down stage. \n
 *              \b CFALSE indicate that This module can't enter to power-down stage.
 *  @see        MES_I2C_GetPhysicalAddress,   MES_I2C_GetSizeOfRegisterSet,
 *              MES_I2C_SetBaseAddress,       MES_I2C_GetBaseAddress,
 *              MES_I2C_OpenModule,           MES_I2C_CloseModule,
 *              MES_I2C_CheckBusy
 */
CBOOL   MES_I2C_CanPowerDown( U32 ModuleIndex )
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
 *  @see        MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
#if 0  // ghcstop delete
S32     MES_I2C_GetInterruptNumber( U32 ModuleIndex )
{
    const U32   I2CInterruptNumber[NUMBER_OF_I2C_MODULE] = { INTNUM_OF_I2C0_MODULE, INTNUM_OF_I2C1_MODULE };

    MES_ASSERT( NUMBER_OF_I2C_MODULE > ModuleIndex );

    return  I2CInterruptNumber[ModuleIndex];
}
#endif

//------------------------------------------------------------------------------
/**
 *  @brief      Set a specified interrupt to be enable or disable.
 *  @param[in]  IntNum  Interrupt Number ( 0 ).
 *  @param[in]  Enable  \b CTRUE  indicate that Interrupt Enable. \n
 *                      \b CFALSE indicate that Interrupt Disable.
 *  @return     None.
 *  @remarks    I2C Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_I2C_GetInterruptNumber,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
void    MES_I2C_SetInterruptEnable( U32 ModuleIndex, S32 IntNum, CBOOL Enable )
{
    const U32 IRQ_ENB_POS   = 5;
    const U32 IRQ_ENB_MASK  = 1UL << IRQ_ENB_POS;

    register struct MES_I2C_RegisterSet* pRegister;
    register U32 ReadValue;

    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    pRegister   =   __g_ModuleVariables[ModuleIndex].pRegister;

    ReadValue   =   pRegister->ICCR;

    ReadValue   &=  ~IRQ_ENB_MASK;
    ReadValue   |=  (U32)Enable << IRQ_ENB_POS;

    pRegister->ICCR  =   ReadValue;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is enabled or disabled.
 *  @param[in]  IntNum  Interrupt Number ( 0 ).
 *  @return     \b CTRUE  indicate that Interrupt is enabled. \n
 *              \b CFALSE indicate that Interrupt is disabled.
 *  @remarks    I2C Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
CBOOL   MES_I2C_GetInterruptEnable( U32 ModuleIndex, S32 IntNum )
{
    const U32 IRQ_ENB_POS   = 5;
    const U32 IRQ_ENB_MASK  = 1UL << IRQ_ENB_POS;

    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICCR & IRQ_ENB_MASK) >> IRQ_ENB_POS );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is pended or not
 *  @param[in]  IntNum  Interrupt Number ( 0 ).
 *  @return     \b CTRUE  indicate that Pending is seted. \n
 *              \b CFALSE indicate that Pending is Not Seted.
 *  @remarks    I2C Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
CBOOL   MES_I2C_GetInterruptPending( U32 ModuleIndex, S32 IntNum )
{
    const U32 PEND_POS   = 0;
    const U32 PEND_MASK  = 1UL << PEND_POS;

    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    return  (CBOOL)(__g_ModuleVariables[ModuleIndex].pRegister->PEND & PEND_MASK);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear a pending state of specified interrupt.
 *  @param[in]  IntNum  Interrupt number ( 0 ).
 *  @return     None.
 *  @remarks    I2C Module have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
void    MES_I2C_ClearInterruptPending( U32 ModuleIndex, S32 IntNum )
{
    const U32 PEND_POS   = 0;
    const U32 PEND_MASK  = 1UL << PEND_POS;

    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    __g_ModuleVariables[ModuleIndex].pRegister->PEND =  PEND_MASK;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set all interrupts to be enables or disables.
 *  @param[in]  Enable  \b CTRUE  indicate that Set to all interrupt enable. \n
 *                      \b CFALSE indicate that Set to all interrupt disable.
 *  @return     None.
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
void    MES_I2C_SetInterruptEnableAll( U32 ModuleIndex, CBOOL Enable )
{
    const U32 IRQ_ENB_POS   = 5;
    const U32 IRQ_ENB_MASK  = 1UL << IRQ_ENB_POS;

    register struct MES_I2C_RegisterSet* pRegister;
    register U32 ReadValue;

    MES_ASSERT( (0==Enable) ||(1==Enable) );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    pRegister   =   __g_ModuleVariables[ModuleIndex].pRegister;

    ReadValue   =   pRegister->ICCR;

    ReadValue   &=  ~IRQ_ENB_MASK;
    ReadValue   |=  (U32)Enable << IRQ_ENB_POS;

    pRegister->ICCR  =   ReadValue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are enable or not.
 *  @return     \b CTRUE  indicate that At least one( or more ) interrupt is enabled. \n
 *              \b CFALSE indicate that All interrupt is disabled.
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
CBOOL   MES_I2C_GetInterruptEnableAll( U32 ModuleIndex )
{
    const U32 IRQ_ENB_POS   = 5;
    const U32 IRQ_ENB_MASK  = 1UL << IRQ_ENB_POS;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICCR & IRQ_ENB_MASK) >> IRQ_ENB_POS );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are pended or not.
 *  @return     \b CTRUE  indicate that At least one( or more ) pending is seted. \n
 *              \b CFALSE indicate that All pending is NOT seted.
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,
 *              MES_I2C_ClearInterruptPendingAll, MES_I2C_GetInterruptPendingNumber
 */
CBOOL   MES_I2C_GetInterruptPendingAll( U32 ModuleIndex )
{
    const U32 PEND_POS   = 0;
    const U32 PEND_MASK  = 1UL << PEND_POS;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    return  (CBOOL)(__g_ModuleVariables[ModuleIndex].pRegister->PEND & PEND_MASK);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear pending state of all interrupts.
 *  @return     None.
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_GetInterruptPendingNumber
 */
void    MES_I2C_ClearInterruptPendingAll( U32 ModuleIndex )
{
    const U32 PEND_POS   = 0;
    const U32 PEND_MASK  = 1UL << PEND_POS;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    __g_ModuleVariables[ModuleIndex].pRegister->PEND =  PEND_MASK;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number which has the most prority of pended interrupts
 *  @return     Pending Number( If all pending is not set then return -1 ).
 *  @see        MES_I2C_GetInterruptNumber,       MES_I2C_SetInterruptEnable,
 *              MES_I2C_GetInterruptEnable,       MES_I2C_GetInterruptPending,
 *              MES_I2C_ClearInterruptPending,    MES_I2C_SetInterruptEnableAll,
 *              MES_I2C_GetInterruptEnableAll,    MES_I2C_GetInterruptPendingAll,
 *              MES_I2C_ClearInterruptPendingAll
 */
S32     MES_I2C_GetInterruptPendingNumber( U32 ModuleIndex )  // -1 if None
{
    const U32 PEND_POS   = 0;
    const U32 PEND_MASK  = 1UL << PEND_POS;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    if( __g_ModuleVariables[ModuleIndex].pRegister->PEND &  PEND_MASK )
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
 *  @remarks    I2C controller only support MES_PCLKMODE_ALWAYS.\n
 *              If user set to MES_PCLKMODE_DYNAMIC, then uart controller \b NOT operate.
 *  @see        MES_I2C_GetClockPClkMode
 */
void            MES_I2C_SetClockPClkMode( U32 ModuleIndex, MES_PCLKMODE mode )
{
    const U32 PCLKMODE_POS  =   3;

    register U32 regvalue;
    register struct MES_I2C_RegisterSet* pRegister;

	U32 clkmode=0;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

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
 *  @remarks    I2C controller only support MES_PCLKMODE_ALWAYS.\n
 *              If user set to MES_PCLKMODE_DYNAMIC, then uart controller \b NOT operate.
 *  @see        MES_I2C_SetClockPClkMode,
 */
MES_PCLKMODE    MES_I2C_GetClockPClkMode( U32 ModuleIndex )
{
    const U32 PCLKMODE_POS  = 3;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

    if( __g_ModuleVariables[ModuleIndex].pRegister->CLKENB & ( 1UL << PCLKMODE_POS ) )
    {
        return MES_PCLKMODE_ALWAYS;
    }

    return  MES_PCLKMODE_DYNAMIC;
}

//--------------------------------------------------------------------------
// Configuration Function
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Set PCLK divider and clock prescaler.
 *  @param[in]  PclkDivider     Set PCLK divider ( 16, 256 )
 *  @param[in]  Prescaler       Set prescaler. \n
 *                              2 ~ 16 ( when PCLK divider is seted 16 ). \n
 *                              1 ~ 16 ( when PCLK divider is seted 256 ). 
 *  @return     None.
 *  @see        MES_I2C_GetClockPrescaler,
*               MES_I2C_SetExtendedIRQEnable,   MES_I2C_GetExtendedIRQEnable,
 *              MES_I2C_SetSlaveAddress,        MES_I2C_GetSlaveAddress,
 *              MES_I2C_SetDataDelay,           MES_I2C_SetDataDelay
 */
void    MES_I2C_SetClockPrescaler( U32 ModuleIndex, U32 PclkDivider, U32 Prescaler )
{
    const U32   CLKSRC_POS  = 6;
    const U32   CLKSRC_MASK = 1UL << CLKSRC_POS;
    const U32   CLK_SCALER_MASK = 0x0F;

    register U32 SetPclkDivider=0;
    register U32 ReadValue;

    MES_ASSERT( (16==PclkDivider) || (256==PclkDivider) );
    MES_ASSERT( 1 <= Prescaler && Prescaler <= 16 );
    MES_ASSERT( (16!=PclkDivider) || ( 2 <= Prescaler && Prescaler <= 16) );
    MES_ASSERT( (CNULL != __g_ModuleVariables[ModuleIndex].pRegister) );

    if( 16 == PclkDivider )
    {
        SetPclkDivider = 0;
    }
    else if( 256 == PclkDivider )
    {
        SetPclkDivider = 1;        
    }

    ReadValue   =   __g_ModuleVariables[ModuleIndex].pRegister->ICCR;

    ReadValue   &= ~( CLKSRC_MASK | CLK_SCALER_MASK );
    ReadValue   |= ((SetPclkDivider << CLKSRC_POS) | (Prescaler-1)); 

    __g_ModuleVariables[ModuleIndex].pRegister->ICCR = ReadValue;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Set PCLK divider and clock prescaler.
 *  @param[out] pPclkDivider     Get PCLK divider ( 16, 256 )
 *  @param[out] pPrescaler       Get prescaler. \n
 *  @return     None.
 *  @see        MES_I2C_SetClockPrescaler,
 *              MES_I2C_SetExtendedIRQEnable,   MES_I2C_GetExtendedIRQEnable,
 *              MES_I2C_SetSlaveAddress,        MES_I2C_GetSlaveAddress,
 *              MES_I2C_SetDataDelay,           MES_I2C_SetDataDelay
 */
void    MES_I2C_GetClockPrescaler( U32 ModuleIndex, U32* pPclkDivider, U32* pPrescaler )
{
    const U32   CLKSRC_POS  = 6;
    const U32   CLKSRC_MASK = 1UL << CLKSRC_POS;
    const U32   CLK_SCALER_MASK = 0x0F;

    register U32 ReadValue;

    MES_ASSERT( CNULL != pPclkDivider );
    MES_ASSERT( CNULL != pPrescaler );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );    
    
    ReadValue   =   __g_ModuleVariables[ModuleIndex].pRegister->ICCR;
    
    if( ReadValue & CLKSRC_MASK )  
    {
        *pPclkDivider = 256;
    }
    else
    {
        *pPclkDivider = 16;        
    }
    
    *pPrescaler = (ReadValue & CLK_SCALER_MASK)+1;
}

//------------------------------------------------------------------------------    
/**
 * @brief      Set Extend IRQ operation
 * @param[in]  bEnable     \b CTRUE indicate that Extend IRQ Enable. \n
 *                         \b CFALSE indicate that Extend IRQ Disable.
 * @return     None.
 * @remarks    If Extend IRQ is Enabled, IRQ is occurred when slave address match occur,\n
 *             general call occur, slave tx last byte condition occur, slave rx stop condition occur.
 * @see        MES_I2C_SetClockPrescaler,      MES_I2C_GetClockPrescaler
 *                                             MES_I2C_GetExtendedIRQEnable,
 *             MES_I2C_SetSlaveAddress,        MES_I2C_GetSlaveAddress,
 *             MES_I2C_SetDataDelay,           MES_I2C_GetDataDelay
 */
void    MES_I2C_SetExtendedIRQEnable( U32 ModuleIndex, CBOOL bEnable )
{
    const U32    EXT_IRQ_ENB   = ( 0x01 << 4 );
    register U32    temp;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   

    
    temp = __g_ModuleVariables[ModuleIndex].pRegister->ICCR ;
    
    if( CTRUE == bEnable )
    {
        temp |= EXT_IRQ_ENB;
    }
    else
    {
        temp &= ~EXT_IRQ_ENB;
    }
    
    __g_ModuleVariables[ModuleIndex].pRegister->ICCR = temp;
}

//------------------------------------------------------------------------------    
/**
 * @brief      Get Extend IRQ operation
 * @return     \b CTRUE Indicate that Extend IRQ Enabled. \n
 *             \b CFALSE Indicate that Extend IRQ Disabled.
 * @see        MES_I2C_SetClockPrescaler,      MES_I2C_GetClockPrescaler
 *             MES_I2C_SetExtendedIRQEnable,   
 *             MES_I2C_SetSlaveAddress,        MES_I2C_GetSlaveAddress,
 *             MES_I2C_SetDataDelay,           MES_I2C_GetDataDelay
 */
CBOOL   MES_I2C_GetExtendedIRQEnable( U32 ModuleIndex )
{
    const U32    EXT_IRQ_ENB   = ( 0x01 << 4 );

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   

    if( __g_ModuleVariables[ModuleIndex].pRegister->ICCR & EXT_IRQ_ENB )
    {
        return CTRUE;
    }
 
    return CFALSE;   
 
}

//------------------------------------------------------------------------------    
/**
 * @brief       Set Salve Address
 * @param[in]   Address    Value of Salve Address ( 0x02 ~ 0xFE ) 
 * @return      None.
 * @remarks     LSB[0] bit must set to 0.
 * @see         MES_I2C_SetClockPrescaler,      MES_I2C_GetClockPrescaler
 *              MES_I2C_SetExtendedIRQEnable,   MES_I2C_GetExtendedIRQEnable,
 *                                              MES_I2C_GetSlaveAddress,
 *              MES_I2C_SetDataDelay,           MES_I2C_GetDataDelay
 */
void    MES_I2C_SetSlaveAddress( U32 ModuleIndex, U8 Address)
{
    MES_ASSERT( 0 == (Address & 0x01) );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   
    
    __g_ModuleVariables[ModuleIndex].pRegister->IAR  =   (U32)Address;    
}

//------------------------------------------------------------------------------    
/**
 * @brief      Get Slave Address
 * @return     Value of Salve Address ( 0x02 ~ 0xFE )
 * @see        MES_I2C_SetClockPrescaler,      MES_I2C_GetClockPrescaler
 *             MES_I2C_SetExtendedIRQEnable,   MES_I2C_GetExtendedIRQEnable,
 *             MES_I2C_SetSlaveAddress,        
 *             MES_I2C_SetDataDelay,           MES_I2C_GetDataDelay
 */
U8      MES_I2C_GetSlaveAddress(U32 ModuleIndex)
{
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   
    
    return (U8)__g_ModuleVariables[ModuleIndex].pRegister->IAR;    
}

//------------------------------------------------------------------------------    
/**
 * @brief       Set delay, in PCLKs, between SCL and SDA 
 * @param[in]   delay    number of PCLK ( 1 ~ 32 ) for delay
 * @return      None.
 * @remarks     SDA must be changed at center of low state of SCL from I2C spec.
 *              For this, you have to set delay for SDA change position from falling edge of SCL when TX.
 *              This delay is also used for SDA fetch postiion from rising edge of SCL when RX.
 *              Usually this dealy is 1/4 of SCL period in PCLKs.
 * @see         MES_I2C_SetClockPrescaler,      MES_I2C_GetClockPrescaler
 *              MES_I2C_SetExtendedIRQEnable,   MES_I2C_GetExtendedIRQEnable,
 *              MES_I2C_SetSlaveAddress,        MES_I2C_GetSlaveAddress,
 *                                              MES_I2C_GetDataDelay
 */
void    MES_I2C_SetDataDelay( U32 ModuleIndex, U32 delay )
{
    MES_ASSERT( 1 <= delay && delay <= 32 );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );       
     
    __g_ModuleVariables[ModuleIndex].pRegister->QCNT_MAX = delay-1;
    
}

//------------------------------------------------------------------------------    
/**
 * @brief       Get PCLK number from rising or falling edge to data.
 * @return      Number of PCLK ( 1 ~ 32 )
 * @remarks     SDA must be changed at center of low state of SCL from I2C spec.
 *              For this, you have to set delay for SDA change position from falling edge of SCL when TX.
 *              This delay is also used for SDA fetch postiion from rising edge of SCL when RX.
 *              Usually this dealy is 1/4 of SCL period in PCLKs.
 * @see         
 *              MES_I2C_SetExtendedIRQEnable,   MES_I2C_GetExtendedIRQEnable,
 *              MES_I2C_SetSlaveAddress,        MES_I2C_GetSlaveAddress,
 *              MES_I2C_SetDataDelay           
 */
U32     MES_I2C_GetDataDelay (U32 ModuleIndex)
{
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );       

    return (__g_ModuleVariables[ModuleIndex].pRegister->QCNT_MAX + 1);   
}


//------------------------------------------------------------------------------
// Operation Function
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------    
/**
 * @brief      Set Ack Generation Enable or Diable
 * @param[in]  bAckGen   \b CTRUE indicate that Ack Generate. \n
 *                       \b CFALSE indicate that Ack Not Generation.
 * @return     None.
 * @remarks    Use only for receiver mode.
 * @see                                            MES_I2C_GetAckGenerationEnable,
 *             MES_I2C_ClearOperationHold,         MES_I2C_ControlMode,
 *             MES_I2C_WriteByte,                  MES_I2C_ReadByte,
 *             MES_I2C_IsBusy,
 *             MES_I2C_BusDisable
 */
void        MES_I2C_SetAckGenerationEnable( U32 ModuleIndex, CBOOL bAckGen )
{
    const U32 ACK_GEN_POS   = 7;
    const U32 ACK_GEN_MASK  = 1UL << ACK_GEN_POS;
    
    register struct MES_I2C_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( (0==bAckGen) ||(1==bAckGen) );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );    
    
    pRegister   =   __g_ModuleVariables[ModuleIndex].pRegister;
    
    ReadValue   =   pRegister->ICCR;
    
    ReadValue   &=  ~ACK_GEN_MASK;
    ReadValue   |=  (U32)bAckGen << ACK_GEN_POS;    
    
    pRegister->ICCR  =   ReadValue;        
}

//------------------------------------------------------------------------------    
/**
 * @brief      Get Setting Value of Ack Generation 
 * @return     \b CTRUE Indicate that Ack Generation Enabled. \n
 *             \b CFALSE Indicate that Ack Generation Disabled.
 * @see        MES_I2C_SetAckGenerationEnable,     
 *             MES_I2C_ClearOperationHold,         MES_I2C_ControlMode,
 *             MES_I2C_WriteByte,                  MES_I2C_ReadByte,
 *             MES_I2C_IsBusy,
 *             MES_I2C_BusDisable
 */
CBOOL       MES_I2C_GetAckGenerationEnable( U32 ModuleIndex )
{
    const U32 ACK_GEN_POS   = 7;
    const U32 ACK_GEN_MASK  = 1UL << ACK_GEN_POS;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );
    
    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICCR & ACK_GEN_MASK) >> ACK_GEN_POS );        
}

//------------------------------------------------------------------------------    
/**
 * @brief      Clear Operation Hold. 
 * @return     None.
 * @remarks    I2C module's operation will be hold after transmiting or 
 *             receiving a byte. Therefore you have to clear hold status to continue
 *             next progress.\n
 *             Also, user must clear hold status after module's start/stop setting.
 * @see        MES_I2C_SetAckGenerationEnable,     MES_I2C_GetAckGenerationEnable,
 *                                                 MES_I2C_ControlMode,
 *             MES_I2C_WriteByte,                  MES_I2C_ReadByte,
 *             MES_I2C_IsBusy,
 *             MES_I2C_BusDisable
 */
void        MES_I2C_ClearOperationHold  ( U32 ModuleIndex )
{
    const U32    OP_HOLD_MASK = ( 0x01 << 1 ) ;

    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   
    
    __g_ModuleVariables[ModuleIndex].pRegister->PEND = (U16)OP_HOLD_MASK;
        
}

//------------------------------------------------------------------------------    
/**
 * @brief      Set I2C Control Mode
 * @param[in]  TxRxMode     Value of I2C's Mode ( Master tx/rx or Slave tx/rx )     
 * @param[in]  Signal       Select Start/Stop signal ( MES_I2C_SIGNAL_START, MES_I2C_SIGNAL_STOP )
 * @return     None.
 * @remarks    This function make start/stop signal of I2C mode ( master tx/rx, slave tx/rx ).
 * @see        MES_I2C_SetAckGenerationEnable,     MES_I2C_GetAckGenerationEnable,
 *             MES_I2C_ClearOperationHold,         
 *             MES_I2C_WriteByte,                  MES_I2C_ReadByte,
 *             MES_I2C_IsBusy,
 *             MES_I2C_BusDisable
 */
void        MES_I2C_ControlMode ( U32 ModuleIndex, MES_I2C_TXRXMODE TxRxMode, MES_I2C_SIGNAL Signal )
{
    const U32   TX_RX_POS       =   6;
    const U32   ST_BUSY_POS     =   5;
    const U32   TXRX_ENB_MASK   =   1UL << 4;
    const U32   ST_ENB_MASK     =   1UL << 12;
    
    register struct MES_I2C_RegisterSet*    pRegister;
    register U32    temp;

    MES_ASSERT( 4 > TxRxMode );
    MES_ASSERT( 2 > Signal );
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   

    pRegister = __g_ModuleVariables[ModuleIndex].pRegister;
    
    temp = pRegister->ICSR & 0x1F0F;

    pRegister->ICSR =   ( temp | (TxRxMode<<TX_RX_POS) | (Signal<<ST_BUSY_POS) | TXRX_ENB_MASK | ST_ENB_MASK );
}

//------------------------------------------------------------------------------    
/**
 * @brief      Check I2C's status 
 * @return     \b CTRUE Indicate that I2C is Busy.\n
 *             \b CFALSE Indicate that I2C is Not Busy.
 * @see        MES_I2C_SetAckGenerationEnable,     MES_I2C_GetAckGenerationEnable,
 *             MES_I2C_ClearOperationHold,         MES_I2C_ControlMode,
 *             MES_I2C_WriteByte,                  MES_I2C_ReadByte,
 *             MES_I2C_BusDisable
 */
CBOOL       MES_I2C_IsBusy( U32 ModuleIndex )
{
    const U32   ST_BUSY_POS     =   5;
    const U32   ST_BUSY_MASK    = 1UL << ST_BUSY_POS;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   
    
    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICSR & ST_BUSY_MASK) >> ST_BUSY_POS );    
}

//------------------------------------------------------------------------------    
/**
 * @brief      Set Send Data
 * @param[in]  WriteData     Value of Write Data ( 0x0 ~ 0xFF )
 * @return     None.
 * @see        MES_I2C_SetAckGenerationEnable,     MES_I2C_GetAckGenerationEnable,
 *             MES_I2C_ClearOperationHold,         MES_I2C_ControlMode,
 *                                                 MES_I2C_ReadByte,
 *             MES_I2C_IsBusy,                     MES_I2C_BusDisable
 */
void        MES_I2C_WriteByte ( U32 ModuleIndex, U8 WriteData)
{
    MES_ASSERT( 0xff >= WriteData ); // ghcstop fix
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   
    
    __g_ModuleVariables[ModuleIndex].pRegister->IDSR = (U32)( WriteData );    
}

//------------------------------------------------------------------------------    
/**
 * @brief      Get Received Data
 * @return     Received Data ( 0x0 ~ 0xFF )
 * @see        MES_I2C_SetAckGenerationEnable,     MES_I2C_GetAckGenerationEnable,
 *             MES_I2C_ClearOperationHold,         MES_I2C_ControlMode,
 *             MES_I2C_WriteByte,                  
 *             MES_I2C_IsBusy,                     MES_I2C_BusDisable
 */
U8          MES_I2C_ReadByte (U32 ModuleIndex)
{
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   
    
    return (U8)__g_ModuleVariables[ModuleIndex].pRegister->IDSR;    
}

//------------------------------------------------------------------------------    
/**
 * @brief       I2C's Bus Disable
 * @return      None.
 * @remarks     Only use for Clear Arbitration fail status. \n
 *              Arbitration status means that conflicting two I2C master device when
 *              data send. \n
 *              This case, master device ( high prority ) send data, but master 
 *              device(low prority) become arbitraion fail status.
 *              
 * @see         MES_I2C_SetAckGenerationEnable,     MES_I2C_GetAckGenerationEnable,
 *              MES_I2C_ClearOperationHold,         MES_I2C_ControlMode,
 *              MES_I2C_WriteByte,                  MES_I2C_ReadByte,
 *              MES_I2C_IsBusy
 */
void        MES_I2C_BusDisable( U32 ModuleIndex )
{
    const U32 TXRX_ENB_MASK = ( 0x01 << 4 );
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );   
        
    __g_ModuleVariables[ModuleIndex].pRegister->ICSR &= ~TXRX_ENB_MASK;
}

//------------------------------------------------------------------------------
// Checking Function of external Interrupt's source when interrupt is occurred.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------    
/**
 * @brief       Check state of slave address is matched or NOT.
 * @return      \b CTRUE Indicate that Slave address is matched. \n
 *              \b CFALSE Indicate that Slave address is NOT matched.
 * @remarks     Interrupt is occurred when slave address is matched. \n 
 * @see                                         MES_I2C_ClearSlaveAddressMatch,
 *              MES_I2C_IsGeneralCall,          MES_I2C_ClearGeneralCall,
 *              MES_I2C_IsSlaveRxStop,          MES_I2C_ClearSlaveRxStop,
 *              MES_I2C_IsBusArbitFail,         MES_I2C_IsACKReceived
 */                                       
CBOOL       MES_I2C_IsSlaveAddressMatch( U32 ModuleIndex )
{
    const U32   SLAVE_MATCH_OCCUR_POS     =   10;
    const U32   SLAVE_MATCH_OCCUR_MASK    =   1UL << SLAVE_MATCH_OCCUR_POS;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );
    
    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICSR & SLAVE_MATCH_OCCUR_MASK) >> SLAVE_MATCH_OCCUR_POS );    
}

//------------------------------------------------------------------------------    
/**
 * @brief       Clear state of slave address is matched.
 * @return      None.
 * @see         MES_I2C_IsSlaveAddressMatch,    
 *              MES_I2C_IsGeneralCall,          MES_I2C_ClearGeneralCall,
 *              MES_I2C_IsSlaveRxStop,          MES_I2C_ClearSlaveRxStop,
 *              MES_I2C_IsBusArbitFail,         MES_I2C_IsACKReceived
 */                                       
void        MES_I2C_ClearSlaveAddressMatch( U32 ModuleIndex )
{
    const U32   ST_ENB_MASK               = 1UL << 12; 
    const U32   SLAVE_MATCH_OCCUR_POS     = 10;
    const U32   SLAVE_MATCH_OCCUR_MASK    = 1UL << SLAVE_MATCH_OCCUR_POS;
 
    register struct MES_I2C_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );    
    
    pRegister   =   __g_ModuleVariables[ModuleIndex].pRegister;

    ReadValue   =    pRegister->ICSR;
    ReadValue   &= ~( ST_ENB_MASK | SLAVE_MATCH_OCCUR_MASK );
    pRegister->ICSR = ReadValue; 
}

//------------------------------------------------------------------------------    
/**
 * @brief       Check state of General call is occurred or NOT.
 * @return      \b CTRUE Indicate that General call is occurred. \n
 *              \b CFALSE Indicate that General call is NOT occurred.
 * @remarks     Interrupt is occurred when general call is occurred.\n 
 *              General call means that master device send a command to all slave device( broadcasting ).
 * @see         MES_I2C_IsSlaveAddressMatch,    MES_I2C_ClearSlaveAddressMatch,
 *                                              MES_I2C_ClearGeneralCall,
 *              MES_I2C_IsSlaveRxStop,          MES_I2C_ClearSlaveRxStop,
 *              MES_I2C_IsBusArbitFail,         MES_I2C_IsACKReceived
 */
CBOOL       MES_I2C_IsGeneralCall( U32 ModuleIndex )
{
    const U32   GENERAL_CALL_OCCUR_POS     =   9;
    const U32   GENERAL_CALL_OCCUR_MASK    =   1UL << GENERAL_CALL_OCCUR_POS;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );
    
    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICSR & GENERAL_CALL_OCCUR_MASK) >> GENERAL_CALL_OCCUR_POS );        
}

//------------------------------------------------------------------------------    
/**
 * @brief       Clear state of general call is occurred.
 * @return      None.
 * @see         MES_I2C_IsSlaveAddressMatch,    MES_I2C_ClearSlaveAddressMatch,
 *              MES_I2C_IsGeneralCall,          
 *              MES_I2C_IsSlaveRxStop,          MES_I2C_ClearSlaveRxStop,
 *              MES_I2C_IsBusArbitFail,         MES_I2C_IsACKReceived
 */                                       
void        MES_I2C_ClearGeneralCall( U32 ModuleIndex )
{
    const U32   ST_ENB_MASK                = 1UL << 12; 
    const U32   GENERAL_CALL_OCCUR_POS     = 9;
    const U32   GENERAL_CALL_OCCUR_MASK    = 1UL << GENERAL_CALL_OCCUR_POS;
 
    register struct MES_I2C_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );    
    
    pRegister   =   __g_ModuleVariables[ModuleIndex].pRegister;

    ReadValue   =    pRegister->ICSR;
    ReadValue   &= ~( ST_ENB_MASK | GENERAL_CALL_OCCUR_MASK );
    pRegister->ICSR = ReadValue; 
}

//------------------------------------------------------------------------------    
/**
 * @brief       Check state of slave RX is stopped or NOT.
 * @return      \b CTRUE Indicate that Slave RX is stopped. \n
 *              \b CFALSE Indicate that Slave RX is NOT stopped.
 * @remarks     Interrupt is occurred when slave RX is stopped.\n 
 * @see         MES_I2C_IsSlaveAddressMatch,    MES_I2C_ClearSlaveAddressMatch,
 *              MES_I2C_IsGeneralCall,          MES_I2C_ClearGeneralCall,
 *                                              MES_I2C_ClearSlaveRxStop,
 *              MES_I2C_IsBusArbitFail,         MES_I2C_IsACKReceived
 */                                       
CBOOL       MES_I2C_IsSlaveRxStop( U32 ModuleIndex )
{
    const U32   SLV_RX_STOP_POS     = 8;
    const U32   SLV_RX_STOP_MASK    = 1UL << SLV_RX_STOP_POS;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );
    
    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICSR & SLV_RX_STOP_MASK) >> SLV_RX_STOP_POS );        
}

/**
 * @brief       Clear state of Slave RX is stopped.
 * @return      None.
 * @see         MES_I2C_IsSlaveAddressMatch,    MES_I2C_ClearSlaveAddressMatch,
 *              MES_I2C_IsGeneralCall,          MES_I2C_ClearGeneralCall,
 *              MES_I2C_IsSlaveRxStop,          
 *              MES_I2C_IsBusArbitFail,         MES_I2C_IsACKReceived
 */                          
void        MES_I2C_ClearSlaveRxStop( U32 ModuleIndex )
{
    const U32   ST_ENB_MASK         = 1UL << 12; 
    const U32   SLV_RX_STOP_POS     = 8;
    const U32   SLV_RX_STOP_MASK    = 1UL << SLV_RX_STOP_POS;
 
    register struct MES_I2C_RegisterSet* pRegister;
    register U32 ReadValue;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );    
    
    pRegister   =   __g_ModuleVariables[ModuleIndex].pRegister;

    ReadValue   =    pRegister->ICSR;
    ReadValue   &= ~( ST_ENB_MASK | SLV_RX_STOP_MASK );
    pRegister->ICSR = ReadValue;     
}

//------------------------------------------------------------------------------    
/**
 * @brief      Check Bus Arbitration status
 * @return     \b CTRUE Indicate that Bus Arbitration Failed. \n
 *             \b CFALSE Indicate that Bus Arbitration is Not Failed.
 * @remarks    Interrupt is Occured when Extend IRQ Enable and Bus arbitration is failed.
 * @see        MES_I2C_IsSlaveAddressMatch,    MES_I2C_ClearSlaveAddressMatch,
 *             MES_I2C_IsGeneralCall,          MES_I2C_ClearGeneralCall,
 *             MES_I2C_IsSlaveRxStop,          MES_I2C_ClearSlaveRxStop,
 *                                             MES_I2C_IsACKReceived
 */
CBOOL       MES_I2C_IsBusArbitFail( U32 ModuleIndex )
{
    const U32   ARBIT_FAIL_POS     = 3;
    const U32   ARBIT_FAIL_MASK   = 1UL << ARBIT_FAIL_POS;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );
    
    return  (CBOOL)( (__g_ModuleVariables[ModuleIndex].pRegister->ICSR & ARBIT_FAIL_MASK) >> ARBIT_FAIL_POS );            
}

//------------------------------------------------------------------------------    
/**
 * @brief      Check ACK Status
 * @return     \b CTRUE Indicate that ACK Received.\n 
 *             \b CFALSE Indicate that ACK \b NOT received.
 * @remarks    Interrupt is Occured when Extend IRQ Enable and NAck Received.\n
 * @see        MES_I2C_IsSlaveAddressMatch,    MES_I2C_ClearSlaveAddressMatch,
 *             MES_I2C_IsGeneralCall,          MES_I2C_ClearGeneralCall,
 *             MES_I2C_IsSlaveRxStop,          MES_I2C_ClearSlaveRxStop,
 *             MES_I2C_IsBusArbitFail         
 */
CBOOL       MES_I2C_IsACKReceived( U32 ModuleIndex )
{
    const U32   ACK_STATUS_POS    = 0;
    const U32   ACK_STATUS_MASK   = 1UL << ACK_STATUS_POS;
    
    MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );
    
    return  (CBOOL)(((__g_ModuleVariables[ModuleIndex].pRegister->ICSR & ACK_STATUS_MASK) >> ACK_STATUS_POS ) ^ 1);  // 0 : CTRUE, 1 : CFALSE
}


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
//	Module     : PWM
//	File       : mes_pwm.h
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

#include "pollux_pwm.h"



/// @brief  PWM Module's Register List
struct  MES_PWM_RegisterSet
{
    struct {
        volatile U16    PWM_PREPOL;         ///< 0x00       : Prescaler Register  
        volatile U16    PWM_DUTY[2];        ///< 0x02, 0x04 : Duty Cycle Register
        volatile U16    PWM_PERIOD[2];      ///< 0x06, 0x08 : Period Cycle Register
        volatile U16    RESERVED0[3];       ///< 0xA, 0xC, 0xE : Reserved Region
    } PWM2[2];

    volatile U32        RESERVED1[0x08];    ///< 0x20~0x3C  : Reserved Region
    volatile U32        PWM_CLKENB;         ///< 0x40      : Clock Enable Register
    volatile U32        PWM_CLKGEN;         ///< 0x44      : Clock Generater Register
};

static  struct
{
    struct MES_PWM_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_PWM_MODULE] = { CNULL, };

static  U32 __g_CurModuleIndex = 0;
static  struct MES_PWM_RegisterSet *__g_pRegister = CNULL;

//------------------------------------------------------------------------------
// Module Interface
//------------------------------------------------------------------------------
/**
 *  @brief  Initialize of prototype enviroment & local variables.
 *  @return \b CTRUE  indicate that Initialize is successed.\n
 *          \b CFALSE indicate that Initialize is failed.\n
 *  @see                                MES_PWM_SelectModule,
 *          MES_PWM_GetCurrentModule,   MES_PWM_GetNumberOfModule
 */
CBOOL   MES_PWM_Initialize( void )
{
	static CBOOL bInit = CFALSE;
	U32 i;
	
	if( CFALSE == bInit )
	{
		__g_CurModuleIndex = 0;
		
		for( i=0; i < NUMBER_OF_PWM_MODULE; i++ )
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
 *  @see        MES_PWM_Initialize,         
 *              MES_PWM_GetCurrentModule,   MES_PWM_GetNumberOfModule
 */
U32    MES_PWM_SelectModule( U32 ModuleIndex )
{
	U32 oldindex = __g_CurModuleIndex;

    MES_ASSERT( NUMBER_OF_PWM_MODULE > ModuleIndex );

    __g_CurModuleIndex = ModuleIndex;
    __g_pRegister = __g_ModuleVariables[ModuleIndex].pRegister;    

	return oldindex;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get index of current selected module.
 *  @return     Current module's index.
 *  @see        MES_PWM_Initialize,         MES_PWM_SelectModule,
 *                                          MES_PWM_GetNumberOfModule
 */
U32     MES_PWM_GetCurrentModule( void )
{
    return __g_CurModuleIndex;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get number of modules in the chip.
 *  @return     Module's number.
 *  @see        MES_PWM_Initialize,         MES_PWM_SelectModule,
 *              MES_PWM_GetCurrentModule   
 */
U32     MES_PWM_GetNumberOfModule( void )
{
    return NUMBER_OF_PWM_MODULE;
}

//------------------------------------------------------------------------------
// Basic Interface
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *  @brief      Get module's physical address.
 *  @return     Module's physical address
 *  @see                                        MES_PWM_GetSizeOfRegisterSet,
 *              MES_PWM_SetBaseAddress,         MES_PWM_GetBaseAddress,
 *              MES_PWM_OpenModule,             MES_PWM_CloseModule,
 *              MES_PWM_CheckBusy,              MES_PWM_CanPowerDown
 */             
U32     MES_PWM_GetPhysicalAddress( void )
{               
    MES_ASSERT( NUMBER_OF_PWM_MODULE > __g_CurModuleIndex );

    return  (U32)POLLUX_PA_PWM;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a size, in byte, of register set.
 *  @return     Size of module's register set.
 *  @see        MES_PWM_GetPhysicalAddress,     
 *              MES_PWM_SetBaseAddress,         MES_PWM_GetBaseAddress,
 *              MES_PWM_OpenModule,             MES_PWM_CloseModule,
 *              MES_PWM_CheckBusy,              MES_PWM_CanPowerDown
 */
U32     MES_PWM_GetSizeOfRegisterSet( void )
{
    return sizeof( struct MES_PWM_RegisterSet );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a base address of register set.
 *  @param[in]  BaseAddress Module's base address
 *  @return     None.
 *  @see        MES_PWM_GetPhysicalAddress,     MES_PWM_GetSizeOfRegisterSet,
 *                                              MES_PWM_GetBaseAddress,
 *              MES_PWM_OpenModule,             MES_PWM_CloseModule,
 *              MES_PWM_CheckBusy,              MES_PWM_CanPowerDown
 */
void    MES_PWM_SetBaseAddress( U32 BaseAddress )
{
    MES_ASSERT( CNULL != BaseAddress );
    MES_ASSERT( NUMBER_OF_PWM_MODULE > __g_CurModuleIndex );

    __g_ModuleVariables[__g_CurModuleIndex].pRegister = (struct MES_PWM_RegisterSet *)BaseAddress;
    __g_pRegister = (struct MES_PWM_RegisterSet *)BaseAddress;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a base address of register set
 *  @return     Module's base address.
 *  @see        MES_PWM_GetPhysicalAddress,     MES_PWM_GetSizeOfRegisterSet,
 *              MES_PWM_SetBaseAddress,         
 *              MES_PWM_OpenModule,             MES_PWM_CloseModule,
 *              MES_PWM_CheckBusy,              MES_PWM_CanPowerDown
 */
U32     MES_PWM_GetBaseAddress( void )
{
    MES_ASSERT( NUMBER_OF_PWM_MODULE > __g_CurModuleIndex );

    return (U32)__g_ModuleVariables[__g_CurModuleIndex].pRegister;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Initialize selected modules with default value.
 *  @return     \b CTRUE  indicate that Initialize is successed. \n
 *              \b CFALSE indicate that Initialize is failed.
 *  @see        MES_PWM_GetPhysicalAddress,     MES_PWM_GetSizeOfRegisterSet,
 *              MES_PWM_SetBaseAddress,         MES_PWM_GetBaseAddress,
 *                                              MES_PWM_CloseModule,
 *              MES_PWM_CheckBusy,              MES_PWM_CanPowerDown
 */
CBOOL   MES_PWM_OpenModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Deinitialize selected module to the proper stage.
 *  @return     \b CTRUE  indicate that Deinitialize is successed. \n
 *              \b CFALSE indicate that Deinitialize is failed.
 *  @see        MES_PWM_GetPhysicalAddress,     MES_PWM_GetSizeOfRegisterSet,
 *              MES_PWM_SetBaseAddress,         MES_PWM_GetBaseAddress,
 *              MES_PWM_OpenModule,             
 *              MES_PWM_CheckBusy,              MES_PWM_CanPowerDown
 */
CBOOL   MES_PWM_CloseModule( void )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether the selected modules is busy or not.
 *  @return     \b CTRUE  indicate that Module is Busy. \n
 *              \b CFALSE indicate that Module is NOT Busy.
 *  @see        MES_PWM_GetPhysicalAddress,     MES_PWM_GetSizeOfRegisterSet,
 *              MES_PWM_SetBaseAddress,         MES_PWM_GetBaseAddress,
 *              MES_PWM_OpenModule,             MES_PWM_CloseModule,
 *                                                MES_PWM_CanPowerDown
 */
CBOOL   MES_PWM_CheckBusy( void )
{
	return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicaes whether the selected modules is ready to enter power-down stage
 *  @return     \b CTRUE  indicate that Ready to enter power-down stage. \n
 *              \b CFALSE indicate that This module can't enter to power-down stage.
 *  @see        MES_PWM_GetPhysicalAddress,     MES_PWM_GetSizeOfRegisterSet,
 *              MES_PWM_SetBaseAddress,         MES_PWM_GetBaseAddress,
 *              MES_PWM_OpenModule,             MES_PWM_CloseModule,
 *              MES_PWM_CheckBusy              
 */
CBOOL   MES_PWM_CanPowerDown( void )
{
    return CTRUE;
}

//------------------------------------------------------------------------------
// Clock Control Interface
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Set a PCLK mode
 *  @param[in]  mode    PCLK mode
 *  @return     None.
 *  @see                                        MES_PWM_GetClockPClkMode,
 *              MES_PWM_SetClockDivisorEnable,  MES_PWM_GetClockDivisorEnable,
 *              MES_PWM_SetClockSource,         MES_PWM_GetClockSource,
 *              MES_PWM_SetClockDivisor,        MES_PWM_GetClockDivisor
 */
void            MES_PWM_SetClockPClkMode( MES_PCLKMODE mode )
{
    const U32 PCLKMODE_POS  =   3;

    register struct MES_PWM_RegisterSet* pRegister;
    register U32 regvalue;
	U32 clkmode=0;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;


	switch(mode)
	{
    	case MES_PCLKMODE_DYNAMIC:  clkmode = 0;		break;
    	case MES_PCLKMODE_ALWAYS:  	clkmode = 1;		break;
    	default: MES_ASSERT( CFALSE );
	}
	
    regvalue = pRegister->PWM_CLKENB;

    regvalue &= ~(1UL<<PCLKMODE_POS);
    regvalue |= ( clkmode & 0x01 ) << PCLKMODE_POS;

    pRegister->PWM_CLKENB = regvalue;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get current PCLK mode
 *  @return     Current PCLK mode
 *  @see        MES_PWM_SetClockPClkMode,       
 *              MES_PWM_SetClockDivisorEnable,  MES_PWM_GetClockDivisorEnable,
 *              MES_PWM_SetClockSource,         MES_PWM_GetClockSource,
 *              MES_PWM_SetClockDivisor,        MES_PWM_GetClockDivisor
 */
MES_PCLKMODE    MES_PWM_GetClockPClkMode( void )
{
    const U32 PCLKMODE_POS  = 3;

    MES_ASSERT( CNULL != __g_pRegister );

    if( __g_pRegister->PWM_CLKENB & ( 1UL << PCLKMODE_POS ) )
    {
        return MES_PCLKMODE_ALWAYS;
    }
    
    return  MES_PCLKMODE_DYNAMIC;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a clock generator to be enabled or disabled.
 *  @param[in]  Enable  \b CTRUE indicate that Clock generator enable. \n
 *                      \b CFALSE indicate that Clock generator disable. 
 *  @return     None.
 *  @see        MES_PWM_SetClockPClkMode,       MES_PWM_GetClockPClkMode,
 *                                              MES_PWM_GetClockDivisorEnable,
 *              MES_PWM_SetClockSource,         MES_PWM_GetClockSource,
 *              MES_PWM_SetClockDivisor,        MES_PWM_GetClockDivisor
 */
void            MES_PWM_SetClockDivisorEnable( CBOOL Enable )
{
    const U32 CLKGENENB_POS  = 2;
    register struct MES_PWM_RegisterSet* pRegister;
    register U32 regvalue;

    MES_ASSERT( (0==Enable) || (1==Enable) );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    regvalue = pRegister->PWM_CLKENB;

    regvalue &= ~(1UL        << CLKGENENB_POS);
    regvalue |=  (U32)Enable << CLKGENENB_POS;

    pRegister->PWM_CLKENB = regvalue;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a clock generator is enabled or not.
 *  @return     \b CTRUE indicate that Clock generator is enabled. \n
 *              \b CFALSE indicate that Clock generator is disabled.
 *  @see        MES_PWM_SetClockPClkMode,       MES_PWM_GetClockPClkMode,
 *              MES_PWM_SetClockDivisorEnable,  
 *              MES_PWM_SetClockSource,         MES_PWM_GetClockSource,
 *              MES_PWM_SetClockDivisor,        MES_PWM_GetClockDivisor
 */
CBOOL           MES_PWM_GetClockDivisorEnable( void )
{
    const U32 CLKGENENB_POS  = 2;
    MES_ASSERT( CNULL != __g_pRegister );

    return (CBOOL)( ( __g_pRegister->PWM_CLKENB >> CLKGENENB_POS ) & 0x01 );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a clock source for clock generator.
 *  @param[in]  Index       Select clock generator
 *  @param[in]  ClkSrc      Select clock source ( 0 (PLL0), 1 (PLL1) )
 *  @return     None.
 *  @remarks    PWM module have only ONE clock generator. so Index is always setting to 0.
 *  @see        MES_PWM_SetClockPClkMode,       MES_PWM_GetClockPClkMode,
 *              MES_PWM_SetClockDivisorEnable,  MES_PWM_GetClockDivisorEnable,
 *                                              MES_PWM_GetClockSource,
 *              MES_PWM_SetClockDivisor,        MES_PWM_GetClockDivisor
 */
void            MES_PWM_SetClockSource( U32 Index, U32 ClkSrc )
{
    const U32   CLKSRCSEL_POS = 1;
    const U32   CLKSRCSEL_MASK = 0x7;
    register struct MES_PWM_RegisterSet* pRegister;
    register U32 regvalue;

    MES_ASSERT( 0 == Index );
    MES_ASSERT( 1 >= ClkSrc );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;

    regvalue = pRegister->PWM_CLKGEN;

    regvalue &= ~(CLKSRCSEL_MASK << CLKSRCSEL_POS);
    regvalue |= ( ClkSrc << CLKSRCSEL_POS );

    pRegister->PWM_CLKGEN = regvalue;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get current clock source for clock generator.
 *  @param[in]  Index       Select clock generator .
 *  @return     Clock source of clock generator( 0 (PLL0), 1 (PLL1) ).
 *  @remarks    PWM module have only ONE clock generator. so Index is always setting to 0.
 *  @see        MES_PWM_SetClockPClkMode,       MES_PWM_GetClockPClkMode,
 *              MES_PWM_SetClockDivisorEnable,  MES_PWM_GetClockDivisorEnable,
 *              MES_PWM_SetClockSource,         
 *              MES_PWM_SetClockDivisor,        MES_PWM_GetClockDivisor
 */
U32             MES_PWM_GetClockSource( U32 Index )
{
    const U32   CLKSRCSEL_POS  = 1;
    const U32   CLKSRCSEL_MASK = 0x7;

    MES_ASSERT( 0 == Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return (U32)(( __g_pRegister->PWM_CLKGEN >> CLKSRCSEL_POS ) & CLKSRCSEL_MASK );    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a clock divider for clock generator.
 *  @param[in]  Index       Select clock generator.
 *  @param[in]  Divisor     Divide value of clock generator( 1 ~ 64 ).
 *  @return     None.
 *  @remarks    PWM module have only ONE clock generator. so Index is always setting to 0.
 *  @see        MES_PWM_SetClockPClkMode,       MES_PWM_GetClockPClkMode,
 *              MES_PWM_SetClockDivisorEnable,  MES_PWM_GetClockDivisorEnable,
 *              MES_PWM_SetClockSource,         MES_PWM_GetClockSource,
 *                                              MES_PWM_GetClockDivisor
 */
void            MES_PWM_SetClockDivisor( U32 Index, U32 Divisor )
{
    const U32 CLKDIV_POS   = 4;
    const U32 CLKDIV_MASK  = 0x3F;

    register struct MES_PWM_RegisterSet* pRegister;
    register U32 regvalue;

    MES_ASSERT( 0 == Index );
    MES_ASSERT( (1 <= Divisor) && ( 64 >= Divisor ) );
    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;
    regvalue = pRegister->PWM_CLKGEN;

    regvalue &= ~(CLKDIV_MASK << CLKDIV_POS);
    regvalue |= (Divisor-1) << CLKDIV_POS;

    pRegister->PWM_CLKGEN = regvalue;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get current clock divider of clock generator
 *  @param[in]  Index       Select clock generator.
 *  @return     Clock divider value of current clock generator.
 *  @remarks    PWM module have only ONE clock generator. so Index is always setting to 0.
 *  @see        MES_PWM_SetClockPClkMode,       MES_PWM_GetClockPClkMode,
 *              MES_PWM_SetClockDivisorEnable,  MES_PWM_GetClockDivisorEnable,
 *              MES_PWM_SetClockSource,         MES_PWM_GetClockSource,
 *              MES_PWM_SetClockDivisor        
 */
U32             MES_PWM_GetClockDivisor( U32 Index )
{
    const U32 CLKDIV_POS   = 4;
    const U32 CLKDIV_MASK  = 0x3F;

    MES_ASSERT( 0 == Index );
    MES_ASSERT( CNULL != __g_pRegister );

    return (U32)(((__g_pRegister->PWM_CLKGEN >> CLKDIV_POS ) & CLKDIV_MASK ) +1 ) ;    
}

//------------------------------------------------------------------------------
// PWM Operation.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 * @brief      Set PreScaler Value
 * @param[in]  channel      Select PWM Channel ( 0 ~ 2 )
 * @param[in]  prescale     PreSacler Value ( 1 ~ 128 )
 * @return     None.
 * @see                                MES_PWM_GetPreScale,
 *             MES_PWM_SetPolarity,    MES_PWM_GetPolarity,
 *             MES_PWM_SetPeriod,      MES_PWM_GetPeriod,
 *             MES_PWM_SetDutyCycle,   MES_PWM_GetDutyCycle
 */
void    MES_PWM_SetPreScale( U32 channel, U32 prescale )
{
    volatile U16 *pRegister;
    register U32 shift, temp;
    const    U32 mask = 0x7F;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );
    MES_ASSERT( 0 < prescale && 128 >= prescale );

    pRegister = &__g_pRegister->PWM2[channel/2].PWM_PREPOL;

    shift = ( channel & 1 ) * 8;
    temp  = *pRegister;
    temp &= ~(mask << shift );
    temp |= ( (prescale-1) << shift );
    *pRegister = (U16)temp;    
}
EXPORT_SYMBOL(MES_PWM_SetPreScale);

//------------------------------------------------------------------------------
/**
 * @brief      Get PreScaler Value
 * @param[in]  channel     Select PWM Channel ( 0 ~ 2 )
 * @return     PreScaler Value ( 1 ~ 128 )
 * @see        MES_PWM_SetPreScale,    
 *             MES_PWM_SetPolarity,    MES_PWM_GetPolarity,
 *             MES_PWM_SetPeriod,      MES_PWM_GetPeriod,
 *             MES_PWM_SetDutyCycle,   MES_PWM_GetDutyCycle
 */
U32     MES_PWM_GetPreScale( U32 channel )
{
    volatile U16 *pRegister;
    register U32 shift, temp;
    const    U32 mask = 0x7F;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );

    pRegister = &__g_pRegister->PWM2[channel/2].PWM_PREPOL;

    shift = ( channel & 1 ) * 8;
    temp  = *pRegister;

    return ((temp >> shift) & mask )+1;
}

//------------------------------------------------------------------------------
/**
 * @brief      Set Polarity of PWM signal.
 * @param[in]  channel      Select PWM Channel ( 0 ~ 2 )
 * @param[in]  bypass       \b CTRUE indicate that Polarity set to Bypass. \n
 *                          \b CFALSE indicate that Polarity set to Invert.
 * @return     None.
 * @see        MES_PWM_SetPreScale,    MES_PWM_GetPreScale,
 *                                     MES_PWM_GetPolarity,
 *             MES_PWM_SetPeriod,      MES_PWM_GetPeriod,
 *             MES_PWM_SetDutyCycle,   MES_PWM_GetDutyCycle
 */
void    MES_PWM_SetPolarity( U32 channel, CBOOL bypass )
{
    volatile U16 *pRegister;
    register U32 shift, temp;
    const    U32 mask = 0x01;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );
    MES_ASSERT( (0==bypass) || (1==bypass) );

    pRegister = &__g_pRegister->PWM2[channel/2].PWM_PREPOL;

    if( channel & 1 ){ shift = 15; }
    else             { shift = 7; }

    temp  = *pRegister;
    temp &= ~(mask << shift );
    temp |= ( (U32)bypass << shift );

    *pRegister = (U16)temp;
}
EXPORT_SYMBOL(MES_PWM_SetPolarity);


//------------------------------------------------------------------------------
/**
 * @brief      Get Polarity of PWM signal.
 * @param[in]  channel     Select PWM Channel ( 0 ~ 2 )
 * @return     \b CTRUE indicate that Polarity setting is Bypass. \n
 *             \b CFALSE indicate that Polarity setting is Invert.
 * @see        MES_PWM_SetPreScale,    MES_PWM_GetPreScale,
 *             MES_PWM_SetPolarity,    
 *             MES_PWM_SetPeriod,      MES_PWM_GetPeriod,
 *             MES_PWM_SetDutyCycle,   MES_PWM_GetDutyCycle
 */
CBOOL   MES_PWM_GetPolarity( U32 channel )
{
    volatile U16 *pRegister;
    register U32 shift, temp;
    const    U32 mask = 0x01;
    
    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );

    pRegister = &__g_pRegister->PWM2[channel/2].PWM_PREPOL;

    if( channel & 1 ){ shift = 15; }
    else             { shift = 7; }

    temp  = *pRegister;
    
    return (CBOOL)( (temp >> shift) & mask );
}

//------------------------------------------------------------------------------
/**
 * @brief      Set Period
 * @param[in]  channel      Select PWM Channel ( 0 ~ 2 )
 * @param[in]  period       Period Value ( 0 ~ 1023 )
 * @return     None.
 * @see        MES_PWM_SetPreScale,    MES_PWM_GetPreScale,
 *             MES_PWM_SetPolarity,    MES_PWM_GetPolarity,
 *                                     MES_PWM_GetPeriod,
 *             MES_PWM_SetDutyCycle,   MES_PWM_GetDutyCycle
 */
void    MES_PWM_SetPeriod( U32 channel, U32 period )
{
    volatile U16 *pRegister;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );
    MES_ASSERT( 1024 > period );

    pRegister = &__g_pRegister->PWM2[channel/2].PWM_PERIOD[channel%2];
    *pRegister = (U16)period;
}
EXPORT_SYMBOL(MES_PWM_SetPeriod);



//------------------------------------------------------------------------------
/**
 * @brief      Get Period
 * @param[in]  channel     Select PWM Channel ( 0 ~ 2 )
 * @return     Period Value ( 0 ~ 1023 )
 * @see        MES_PWM_SetPreScale,    MES_PWM_GetPreScale,
 *             MES_PWM_SetPolarity,    MES_PWM_GetPolarity,
 *             MES_PWM_SetPeriod,      
 *             MES_PWM_SetDutyCycle,   MES_PWM_GetDutyCycle
 */
U32     MES_PWM_GetPeriod( U32 channel )
{
    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );

    return(U32)__g_pRegister->PWM2[channel/2].PWM_PERIOD[channel%2];
}

//------------------------------------------------------------------------------
/**
 * @brief      Set Duty Cycle ( Set the length of PWM Signal's Low area )
 * @param[in]  channel     Select PWM Channel ( 0 ~ 2 )
 * @param[in]  duty        duty Value( 0 ~ 1023 )
 * @return     None.
 * @remarks    If set to 0, the PWM Output is 1.\n
 *             If set bigger than PWM Period, the PWM Output is 0.
 * @see        MES_PWM_SetPreScale,    MES_PWM_GetPreScale,
 *             MES_PWM_SetPolarity,    MES_PWM_GetPolarity,
 *             MES_PWM_SetPeriod,      MES_PWM_GetPeriod,
 *                                     MES_PWM_GetDutyCycle
 */
void    MES_PWM_SetDutyCycle( U32 channel, U32 duty )
{

    volatile U16 *pRegister;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );
    MES_ASSERT( 1024 > duty );

    pRegister = &__g_pRegister->PWM2[channel/2].PWM_DUTY[channel%2];
    *pRegister = (U16)duty;
}
EXPORT_SYMBOL(MES_PWM_SetDutyCycle);


//------------------------------------------------------------------------------
/**
 * @brief      Get Duty Cycle
 * @param[in]  channel     Select PWM Channel ( 0 ~ 2 )
 * @return     duty Value( 0 ~ 1023 )
 * @see        MES_PWM_SetPreScale,    MES_PWM_GetPreScale,
 *             MES_PWM_SetPolarity,    MES_PWM_GetPolarity,
 *             MES_PWM_SetPeriod,      MES_PWM_GetPeriod,
 *             MES_PWM_SetDutyCycle   
 */
U32     MES_PWM_GetDutyCycle( U32 channel )
{
    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 3 > channel );

    return(U32)__g_pRegister->PWM2[channel/2].PWM_DUTY[channel%2];
}



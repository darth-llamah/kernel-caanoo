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
//	Module     : ADC
//	File       : mes_adc.h
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

#include <asm/io.h>
#include <asm/uaccess.h>

#include "pollux-adc.h"


/// @brief  ADC Module's Register List
struct  MES_ADC_RegisterSet
{
    volatile U16    ADCCON;                 ///< 0x00 : ADC Control Register
    volatile U16    RESERVED_0;             ///< 0x02 : RESERVED_0
    volatile U16    ADCDAT;                 ///< 0x04 : ADC Output Data Register
    volatile U16    RESERVED_1;             ///< 0x06 : RESERVED_1
    volatile U16    ADCINTENB;              ///< 0x08 : ADC Interrupt Enable Register
    volatile U16    RESERVED_2;             ///< 0x0A : RESERVED_2
    volatile U16    ADCINTCLR;              ///< 0x0C : ADC Interrutp Pending and Clear Register
    volatile U16    RESERVED[0x19];         ///< 0x0E ~ 0x3E : Reserved Region      
    volatile U32    CLKENB;                 ///< 0x40 : ADC Clock Enable Register
};

static  struct
{
    struct MES_ADC_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_ADC_MODULE] = { CNULL, };

static  U32 __g_CurModuleIndex = 0;
static  struct MES_ADC_RegisterSet *__g_pRegister = CNULL;

//------------------------------------------------------------------------------
// Module Interface
//------------------------------------------------------------------------------
/**
 *  @brief  Initialize of prototype enviroment & local variables.
 *  @return \b CTRUE indicate that   Initialize is successed.\n
 *          \b CFALSE indicate that  Initialize is failed.\n
 *  @see                                MES_ADC_SelectModule,
 *          MES_ADC_GetCurrentModule,   MES_ADC_GetNumberOfModule
 */
CBOOL   MES_ADC_Initialize( void )
{
	static CBOOL bInit = CFALSE;
	U32 i;
	
	if( CFALSE == bInit )
	{
		__g_CurModuleIndex = 0;
		
		for( i=0; i < NUMBER_OF_ADC_MODULE; i++ )
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
 *  @return     the index of previously selected module.
 *  @see        MES_ADC_Initialize,         
 *              MES_ADC_GetCurrentModule,   MES_ADC_GetNumberOfModule
 */
U32    MES_ADC_SelectModule( U32 ModuleIndex )
{
	U32 oldindex = __g_CurModuleIndex;

    MES_ASSERT( NUMBER_OF_ADC_MODULE > ModuleIndex );

    __g_CurModuleIndex = ModuleIndex;
    __g_pRegister = __g_ModuleVariables[ModuleIndex].pRegister;    
    
    return oldindex;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get index of current selected module.
 *  @return     Current module's index.
 *  @see    MES_ADC_Initialize,         MES_ADC_SelectModule,
 *                                      MES_ADC_GetNumberOfModule
 */
U32     MES_ADC_GetCurrentModule( void )
{
    return __g_CurModuleIndex;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get number of modules in the chip.
 *  @see        MES_ADC_Initialize,         MES_ADC_SelectModule,
 *              MES_ADC_GetCurrentModule   
 */
U32     MES_ADC_GetNumberOfModule( void )
{
    return NUMBER_OF_ADC_MODULE;
}

//------------------------------------------------------------------------------
// Basic Interface
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *  @brief      Get module's physical address.
 *  @return     Module's physical address
 *  @see                                        MES_ADC_GetSizeOfRegisterSet,
 *              MES_ADC_SetBaseAddress,         MES_ADC_GetBaseAddress,
 *              MES_ADC_OpenModule,             MES_ADC_CloseModule,
 *              MES_ADC_CheckBusy,              MES_ADC_CanPowerDown
 */
U32     MES_ADC_GetPhysicalAddress( void )
{
    MES_ASSERT( NUMBER_OF_ADC_MODULE > __g_CurModuleIndex );

    return  (U32)( POLLUX_VA_ADC + (OFFSET_OF_ADC_MODULE * __g_CurModuleIndex) );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a size, in byte, of register set.
 *  @return     Size of module's register set.
 *  @see        MES_ADC_GetPhysicalAddress,     
 *              MES_ADC_SetBaseAddress,         MES_ADC_GetBaseAddress,
 *              MES_ADC_OpenModule,             MES_ADC_CloseModule,
 *              MES_ADC_CheckBusy,              MES_ADC_CanPowerDown
 */
U32     MES_ADC_GetSizeOfRegisterSet( void )
{
    return sizeof( struct MES_ADC_RegisterSet );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a base address of register set.
 *  @param[in]  BaseAddress Module's base address
 *  @return     None.
 *  @see        MES_ADC_GetPhysicalAddress,     MES_ADC_GetSizeOfRegisterSet,
 *                                              MES_ADC_GetBaseAddress,
 *              MES_ADC_OpenModule,             MES_ADC_CloseModule,
 *              MES_ADC_CheckBusy,              MES_ADC_CanPowerDown
 */
void    MES_ADC_SetBaseAddress( U32 BaseAddress )
{
    MES_ASSERT( CNULL != BaseAddress );
    MES_ASSERT( NUMBER_OF_ADC_MODULE > __g_CurModuleIndex );

    __g_ModuleVariables[__g_CurModuleIndex].pRegister = (struct MES_ADC_RegisterSet *)BaseAddress;
    __g_pRegister = (struct MES_ADC_RegisterSet *)BaseAddress;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a base address of register set
 *  @return     Module's base address.
 *  @see        MES_ADC_GetPhysicalAddress,     MES_ADC_GetSizeOfRegisterSet,
 *              MES_ADC_SetBaseAddress,         
 *              MES_ADC_OpenModule,             MES_ADC_CloseModule,
 *              MES_ADC_CheckBusy,              MES_ADC_CanPowerDown
 */
U32     MES_ADC_GetBaseAddress( void )
{
    MES_ASSERT( NUMBER_OF_ADC_MODULE > __g_CurModuleIndex );

    return (U32)__g_ModuleVariables[__g_CurModuleIndex].pRegister;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Initialize selected modules with default value.
 *  @return     \b CTRUE indicate that   Initialize is successed. \n
 *              \b CFALSE indicate that  Initialize is failed.
 *  @see        MES_ADC_GetPhysicalAddress,     MES_ADC_GetSizeOfRegisterSet,
 *              MES_ADC_SetBaseAddress,         MES_ADC_GetBaseAddress,
 *                                              MES_ADC_CloseModule,
 *              MES_ADC_CheckBusy,              MES_ADC_CanPowerDown
 */
CBOOL   MES_ADC_OpenModule( void )
{
	  return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Deinitialize selected module to the proper stage.
 *  @return     \b CTRUE indicate that   Deinitialize is successed. \n
 *              \b CFALSE indicate that  Deinitialize is failed.
 *  @see        MES_ADC_GetPhysicalAddress,     MES_ADC_GetSizeOfRegisterSet,
 *              MES_ADC_SetBaseAddress,         MES_ADC_GetBaseAddress,
 *              MES_ADC_OpenModule,             
 *              MES_ADC_CheckBusy,              MES_ADC_CanPowerDown
 */
CBOOL   MES_ADC_CloseModule( void )
{
    return CTRUE;   
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether the selected modules is busy or not.
 *  @return     \b CTRUE indicate that   Module is Busy. \n
 *              \b CFALSE indicate that  Module is NOT Busy.
 *  @see        MES_ADC_GetPhysicalAddress,     MES_ADC_GetSizeOfRegisterSet,
 *              MES_ADC_SetBaseAddress,         MES_ADC_GetBaseAddress,
 *              MES_ADC_OpenModule,             MES_ADC_CloseModule,
 *                                              MES_ADC_CanPowerDown
 */
CBOOL   MES_ADC_CheckBusy( void )
{
    const U32    ADEN_BITPOS   =   0;

    MES_ASSERT( CNULL != __g_pRegister );
    
    return (CBOOL)((__g_pRegister->ADCCON >> ADEN_BITPOS) & 0x01 );

}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicaes whether the selected modules is ready to enter power-down stage
 *  @return     \b CTRUE indicate that   Ready to enter power-down stage. \n
 *              \b CFALSE indicate that  This module can't enter to power-down stage.
 *  @see        MES_ADC_GetPhysicalAddress,     MES_ADC_GetSizeOfRegisterSet,
 *              MES_ADC_SetBaseAddress,         MES_ADC_GetBaseAddress,
 *              MES_ADC_OpenModule,             MES_ADC_CloseModule,
 *              MES_ADC_CheckBusy              
 */
CBOOL   MES_ADC_CanPowerDown( void )
{
    if( MES_ADC_IsBusy() )
    {
        return CFALSE;   
    }
    
    return CTRUE;
}


//------------------------------------------------------------------------------
// Interrupt Interface
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number for interrupt controller.
 *  @return     Interrupt number
 *  @see                                            MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
S32     MES_ADC_GetInterruptNumber( void )
{
    return  INTNUM_OF_ADC_MODULE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a specified interrupt to be enable or disable.
 *  @param[in]  IntNum  Interrupt Number ( 0 ).
 *  @param[in]  Enable  \b CTRUE indicate that   Interrupt Enable. \n
 *                      \b CFALSE indicate that  Interrupt Disable.
 *  @return     None.
 *  @remarks    ADC Module Only have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_ADC_GetInterruptNumber,         
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
void    MES_ADC_SetInterruptEnable( S32 IntNum, CBOOL Enable )
{
    const U16 ADCINTENB_MASK = 0x01;
    const U16 ADCINTENB_POS  = 0;

    register struct MES_ADC_RegisterSet*   pRegister;
    register U16 regvalue;
    
    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( (0==Enable) || (1==Enable) );    
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister = __g_pRegister;
    
    regvalue = pRegister->ADCINTENB;
    
    regvalue &= ~ADCINTENB_MASK;
    regvalue |= ((U16)Enable << ADCINTENB_POS);
    
    pRegister->ADCINTENB = regvalue;    

}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is enabled or disabled.
 *  @param[in]  IntNum  Interrupt Number ( 0 ).
 *  @return     \b CTRUE indicate that   Interrupt is enabled. \n
 *              \b CFALSE indicate that  Interrupt is disabled.
 *  @remarks    ADC Module Only have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *                                                  MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
CBOOL   MES_ADC_GetInterruptEnable( S32 IntNum )
{
    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );    

    return (CBOOL)( __g_pRegister->ADCINTENB & 0x01);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether a specified interrupt is pended or not
 *  @param[in]  IntNum  Interrupt Number ( 0 ).
 *  @return     \b CTRUE indicate that   Pending is seted. \n
 *              \b CFALSE indicate that  Pending is Not Seted.
 *  @remarks    ADC Module Only have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
CBOOL   MES_ADC_GetInterruptPending( S32 IntNum )
{
    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );    

    return (CBOOL)( __g_pRegister->ADCINTCLR & 0x01);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear a pending state of specified interrupt.
 *  @param[in]  IntNum  Interrupt number ( 0 ).
 *  @return     None.
 *  @remarks    ADC Module Only have one interrupt. So always \e IntNum set to 0.
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *                                                  MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
void    MES_ADC_ClearInterruptPending( S32 IntNum )
{
    MES_ASSERT( 0 == IntNum );
    MES_ASSERT( CNULL != __g_pRegister );    

     __g_pRegister->ADCINTCLR = 0x01;

}

//------------------------------------------------------------------------------
/**
 *  @brief      Set all interrupts to be enables or disables.
 *  @param[in]  Enable  \b CTRUE indicate that   Set to all interrupt enable. \n
 *                      \b CFALSE indicate that  Set to all interrupt disable.
 *  @return     None.
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
void    MES_ADC_SetInterruptEnableAll( CBOOL Enable )
{
    const U16 ADCINTENB_MASK = 0x01;
    const U16 ADCINTENB_POS  = 0;

    register struct MES_ADC_RegisterSet*   pRegister;
    register U16 regvalue;
    
    MES_ASSERT( (0==Enable) || (1==Enable) );    
    MES_ASSERT( CNULL != __g_pRegister );    
    
    pRegister = __g_pRegister;
    
    regvalue = pRegister->ADCINTENB;
    
    regvalue &= ~ADCINTENB_MASK;
    regvalue |= ((U16)Enable << ADCINTENB_POS);
    
    pRegister->ADCINTENB = regvalue;    
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are enable or not.
 *  @return     \b CTRUE indicate that   At least one( or more ) interrupt is enabled. \n
 *              \b CFALSE indicate that  All interrupt is disabled.
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *                                                  MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
CBOOL   MES_ADC_GetInterruptEnableAll( void )
{
    MES_ASSERT( CNULL != __g_pRegister );    

    return (CBOOL)( __g_pRegister->ADCINTENB & 0x01);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether some of interrupts are pended or not.
 *  @return     \b CTRUE indicate that   At least one( or more ) pending is seted. \n
 *              \b CFALSE indicate that  All pending is NOT seted.
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      
 *              MES_ADC_ClearInterruptPendingAll,   MES_ADC_GetInterruptPendingNumber
 */
CBOOL   MES_ADC_GetInterruptPendingAll( void )
{
    MES_ASSERT( CNULL != __g_pRegister );    

    return (CBOOL)( __g_pRegister->ADCINTCLR & 0x01);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Clear pending state of all interrupts.
 *  @return     None.
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *                                                  MES_ADC_GetInterruptPendingNumber
 */
void    MES_ADC_ClearInterruptPendingAll( void )
{
    MES_ASSERT( CNULL != __g_pRegister );    

     __g_pRegister->ADCINTCLR = 0x01;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a interrupt number which has the most prority of pended interrupts
 *  @return     Pending Number( If all pending is not set then return -1 ).
 *  @see        MES_ADC_GetInterruptNumber,         MES_ADC_SetInterruptEnable,
 *              MES_ADC_GetInterruptEnable,         MES_ADC_GetInterruptPending,
 *              MES_ADC_ClearInterruptPending,      MES_ADC_SetInterruptEnableAll,
 *              MES_ADC_GetInterruptEnableAll,      MES_ADC_GetInterruptPendingAll,
 *              MES_ADC_ClearInterruptPendingAll   
 */
S32     MES_ADC_GetInterruptPendingNumber( void )  // -1 if None
{
    MES_ASSERT( CNULL != __g_pRegister );    

    if( __g_pRegister->ADCINTCLR & 0x01 )
    {
        return 0;   
    }
    
    return -1;
}


void  MES_ADC_SetClockPClkMode( MES_PCLKMODE mode )
{
    const U32 PCLKMODE_POS  =   3;

    register        struct MES_ADC_RegisterSet*    pRegister;
    register U32    regvalue;

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
// ADC Operation.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 * @brief      Set Clock Prescaler Value of A/D Converter
*  @param[in]  value     Value of Prescaler ( Range 20 ~ 256 ) 
 * @return     None.
 * @remarks    This Function must be set before SetPrescalerEnable( ) Function.\n 
 *             Max ADC Clock is 2.5Mhz(400ns) when Prescaler Value is 256. 
 * @code
 *             MES_ADC_SetPrescalerValue(0x256);
 *             MES_ADC_SetPrescalerEnable(CTRUE);
 * @endcode
 * @see                                    MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
void    MES_ADC_SetPrescalerValue( U32 value )
{
    const U32    APSV_MASK   = ( 0xFFUL << 6 );
    const U32    APSV_BITPOS = 6;

    register        struct MES_ADC_RegisterSet*    pRegister;
    register U32    regvalue;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( (256 >= value) && (20 <= value) );

    pRegister = __g_pRegister;
    
    regvalue    =   pRegister->ADCCON;
    
    regvalue = ( regvalue & ~APSV_MASK ) | ( (value-1) << APSV_BITPOS ) ;
    
    pRegister->ADCCON =   (U16)regvalue;  
}

//------------------------------------------------------------------------------
/**
 * @brief  Get Prescaler Value of A/D Converter
 * @return Value of Prescaler
 * @see        MES_ADC_SetPrescalerValue,  
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
U32     MES_ADC_GetPrescalerValue( void )
{
    const    U32    APSV_MASK   = ( 0xFF << 6 );
    const    U32    APSV_BITPOS = 6;

    MES_ASSERT( CNULL != __g_pRegister );

    return (U32)((( __g_pRegister->ADCCON & APSV_MASK ) >> APSV_BITPOS ) + 1 ) ;
}

//------------------------------------------------------------------------------
/**
 * @brief      Prescaler Enable
*  @param[in]  enable     \b CTRUE indicate that Prescaler Enable. \n
*                         \b CFALSE indicate that Prescaler Disable.
 * @return     None.
 * @remarks    This function is set after SetPrescalerValue() function
 * @code
 *             MES_ADC_SetPrescalerValue(256);
 *             MES_ADC_SetPrescalerEnable(CTRUE);
 * @endcode 
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *                                         MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
void    MES_ADC_SetPrescalerEnable( CBOOL enable )
{
    const    U32    APEN_MASK = ( 0x01UL << 14 );
    const    U32    APEN_POS  =  14;
    register U32    regvalue;
    register struct MES_ADC_RegisterSet*   pRegister;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( (0==enable) || (1==enable) );

    pRegister = __g_pRegister;
    
    regvalue = pRegister->ADCCON;

    regvalue &= ~APEN_MASK;
    regvalue |= (U32)enable << APEN_POS;
    
    pRegister->ADCCON = (U16)regvalue;
}

//------------------------------------------------------------------------------
/**
 * @brief  Check Prescaler is enabled or disabled 
 * @return \b CTRUE indicate that Prescaler is Enabled.\n
 *         \b CFALSE indicate that Prescaler is Disabled.
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, 
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
CBOOL   MES_ADC_GetPrescalerEnable( void )
{
    const    U32    APEN_POS  =  14;

    MES_ASSERT( CNULL != __g_pRegister );
    
    return (CBOOL)( ( __g_pRegister->ADCCON >> APEN_POS ) & 0x01 );
}

//------------------------------------------------------------------------------
/**
 * @brief      Set Input Channel 
*  @param[in]  channel     Value of Input Channel ( 0 ~ 7 )
 * @return     None.
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *                                         MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
void    MES_ADC_SetInputChannel( U32 channel )
{
    const    U32    ASEL_MASK     =   ( 0x07 << 3 );
    const    U32    ASEL_BITPOS   =   3;
    register struct MES_ADC_RegisterSet*   pRegister;
    register U32    regvalue;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( 8 > channel );
    
    pRegister = __g_pRegister;
    
    regvalue    =   pRegister->ADCCON;
    
    regvalue    =   ( regvalue & ~ASEL_MASK ) | ( channel << ASEL_BITPOS );
    
    pRegister->ADCCON =   (U16)regvalue;  
}

//------------------------------------------------------------------------------
/**
 * @brief      Get Input Channel
 * @return     Value of Input Channel ( 0 ~ 7 )
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
U32     MES_ADC_GetInputChannel( void )
{
    const    U32    ASEL_MASK     =   ( 0x07 << 3 );
    const    U32    ASEL_BITPOS   =   3;

    MES_ASSERT( CNULL != __g_pRegister );

    return (U32)( ( __g_pRegister->ADCCON & ASEL_MASK ) >> ASEL_BITPOS );  
}

//------------------------------------------------------------------------------
/**
 * @brief      Set Standby Mode 
 * @param[in]  enable     \b CTRUE indicate that Standby Mode ON. \n
 *                        \b CFALSE indicate that Standby Mode OFF.
 * @return     None.
 * @remark     If Standby Mode is enabled then ADC power is cut offed.\n
 *             You have to disable the standby mode to use ADC.
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *                                         MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */  
void    MES_ADC_SetStandbyMode( CBOOL enable )
{
    const    U32 STBY_MASK = ( 0x01UL << 2 );
    const    U32 STBY_POS  = 2;
    register U32 regvalue;
    register struct MES_ADC_RegisterSet*   pRegister;

    MES_ASSERT( CNULL != __g_pRegister );
    MES_ASSERT( (0==enable) || (1==enable) );

    pRegister = __g_pRegister;
    
    regvalue = pRegister->ADCCON;
    
    regvalue &= ~STBY_MASK;
    regvalue |= (U32)enable << STBY_POS;
    
    pRegister->ADCCON = regvalue;    
}

//------------------------------------------------------------------------------
/**
 * @brief      Get ADC's Standby Mode  
 * @return     \b CTRUE indicate that Standby Mode is Enabled.\n
 *             \b CFALSE indicate that Standby Mode is Disabled. 
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     
 *             MES_ADC_Start,              MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
CBOOL   MES_ADC_GetStandbyMode( void )
{
    const    U32 STBY_MASK = ( 0x01UL << 2 );
    const    U32 STBY_POS  = 2;

    return (CBOOL)( ( __g_pRegister->ADCCON & STBY_MASK ) >> STBY_POS );
}

//------------------------------------------------------------------------------
/**
 * @brief      ADC Start
 * @return     None.
 * @remarks    Sequence of ADC operation
 * @code
 *     MES_ADC_SetStandbyMode( CFALSE );        // Standby mode disable
 *     ...
 *     MES_ADC_SetPrescalerValue( 256 );        // Set prescaler value ( 20 ~ 256 )
 *     MES_ADC_SetPrescalerEnable( CTRUE );     // Prescaler enable
 *     MES_ADC_SetInputChannel( 0 );            // Set input channel
 *     ... 
 *     MES_ADC_Start();                         // Start ADC converting
 *     ...
 *     while( MES_ADC_IsBusy() ){ }             // Wait during ADC converting
 *     ... 
 *     Value = MES_ADC_GetConvertedData();      // Read Converted ADC data
 *
 * @endcode
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *                                         MES_ADC_IsBusy,
 *             MES_ADC_GetConvertedData    
 */
void    MES_ADC_Start( void )
{
    const    U32    ADEN_MASK = ( 0x01 << 0 );
    register U32    regvalue;
    register struct MES_ADC_RegisterSet*   pRegister;

    MES_ASSERT( CNULL != __g_pRegister );

    pRegister = __g_pRegister;
   
    regvalue = pRegister->ADCCON;

    regvalue |= ADEN_MASK;
    
    pRegister->ADCCON =   (U16)regvalue;    
}

//------------------------------------------------------------------------------
/**
 * @brief      Check ADC's operation
 * @return     \b CTRUE indicate that ADC is Busy. \n
               \b CFALSE indicate that ADC Conversion is ended.
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              
 *             MES_ADC_GetConvertedData    
 */
CBOOL   MES_ADC_IsBusy( void )
{
    const    U32    ADEN_MASK = (0x01 << 0);

    MES_ASSERT( CNULL != __g_pRegister );
  
    return (CBOOL)(__g_pRegister->ADCCON & ADEN_MASK );
}

//------------------------------------------------------------------------------
/**
 * @brief      Get Conversioned Data of ADC
 * @return     10bit Data of ADC 
 * @see        MES_ADC_SetPrescalerValue,  MES_ADC_GetPrescalerValue,
 *             MES_ADC_SetPrescalerEnable, MES_ADC_GetPrescalerEnable,
 *             MES_ADC_SetInputChannel,    MES_ADC_GetInputChannel,
 *             MES_ADC_SetStandbyMode,     MES_ADC_GetStandbyMode,
 *             MES_ADC_Start,              MES_ADC_IsBusy
 */
U32     MES_ADC_GetConvertedData( void )
{
    MES_ASSERT( CNULL != __g_pRegister );
    
    return (U32)(__g_pRegister->ADCDAT);  
}


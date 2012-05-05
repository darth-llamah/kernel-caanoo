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
//	Module     : MLC
//	File       : mes_mlc.c
//	Description:
//	Author     : Firmware Team
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

#include "pollux_mlc.h"


/// @brief  MLC Module's Register List
struct  MES_MLC_RegisterSet
{
	volatile U32 MLCCONTROLT;		      ///< 0x00 : MLC Top Control Register
	volatile U32 MLCSCREENSIZE;		      ///< 0x04 : MLC Screen Size Register
	volatile U32 MLCBGCOLOR;		      ///< 0x08 : MLC Background Color Register

	struct {
	volatile U32 MLCLEFTRIGHT;	          ///< 0x0C, 0x40 : MLC RGB Layer Left Right Register 0/1
	volatile U32 MLCTOPBOTTOM; 	          ///< 0x10, 0x44 : MLC RGB Layer Top Bottom Register 0/1
    volatile U32 MLCINVALIDLEFTRIGHT0;    ///< 0x14, 0x48 : MLC RGB Invalid Area0 Left, Right Register 0/1
    volatile U32 MLCINVALIDTOPBOTTOM0;    ///< 0x18, 0x4C : MLC RGB Invalid Area0 Top, Bottom Register 0/1
    volatile U32 MLCINVALIDLEFTRIGHT1;    ///< 0x1C, 0x50 : MLC RGB Invalid Area1 Left, Right Register 0/1
    volatile U32 MLCINVALIDTOPBOTTOM1;    ///< 0x20, 0x54 : MLC RGB Invalid Area1 Top, Bottom Register 0/1
	volatile U32 MLCCONTROL;	          ///< 0x24, 0x58 : MLC RGB Layer Control Register 0/1
	volatile S32 MLCHSTRIDE;	          ///< 0x28, 0x5C : MLC RGB Layer Horizontal Stride Register 0/1
	volatile S32 MLCVSTRIDE;	          ///< 0x2C, 0x60 : MLC RGB Layer Vertical Stride Register 0/1
	volatile U32 MLCTPCOLOR;	          ///< 0x30, 0x64 : MLC RGB Layer Transparency Color Register 0/1
	volatile U32 MLCINVCOLOR;	          ///< 0x34, 0x68 : MLC RGB Layer Inversion Color Register 0/1
	volatile U32 MLCADDRESS;	          ///< 0x38, 0x6C : MLC RGB Layer Base Address Register 0/1
    volatile U32 MLCPALETTE;              ///< 0x3C, 0x70 : MLC RGB Layer Palette Register 0/1
	} MLCRGBLAYER[2];

    volatile U32 MLCLEFTRIGHT2;           ///< 0x74 : MLC Video Layer Left Right Register
    volatile U32 MLCTOPBOTTOM2;           ///< 0x78 : MLC Video Layer Top Bottom Register
    volatile U32 MLCCONTROL2;             ///< 0x7C : MLC Video Layer Control Register
    volatile U32 MLCVSTRIDE2;             ///< 0x80 : MLC Video Layer Y Vertical Stride Register
    volatile U32 MLCTPCOLOR2;             ///< 0x84 : MLC Video Layer Transparency Color Register
    volatile U32 __Reserved0[1];          ///< 0x88 : Reserved Region
    volatile U32 MLCADDRESS2;             ///< 0x8C : MLC Video Layer Y Base Address Register
	volatile U32 MLCADDRESSCB;            ///< 0x90 : MLC Video Layer Cb Base Address Register
	volatile U32 MLCADDRESSCR;            ///< 0x94 : MLC Video Layer Cr Base Address Register
	volatile S32 MLCVSTRIDECB;            ///< 0x98 : MLC Video Layer Cb Vertical Stride Register
	volatile S32 MLCVSTRIDECR;            ///< 0x9C : MLC Video Layer Cr Vertical Stride Register
	volatile U32 MLCHSCALE;	              ///< 0xA0 : MLC Video Layer Horizontal Scale Register
	volatile U32 MLCVSCALE;	              ///< 0xA4 : MLC Video Layer Vertical Scale Register
	volatile U32 MLCLUENH;	              ///< 0xA8 : MLC Video Layer Luminance Enhancement Control Register
    volatile U32 MLCCHENH[4];		      ///< 0xAC, 0xB0, 0xB4, 0xB8 : MLC Video Layer Chrominance Enhancement Control Register 0/1/2/3
    volatile U32 __Reserved[0xC1];        ///< 0xBC ~ 0x3BC : Reserved Region
    volatile U32 MLCCLKENB;               ///< 0x3C0 : MLC Clock Enable Register
};

static  struct
{
    struct MES_MLC_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_MLC_MODULE] = { CNULL, };

//------------------------------------------------------------------------------
// Module Interface
//------------------------------------------------------------------------------
/**
 *  @brief  Initialize of prototype enviroment & local variables.
 *  @return \b CTRUE  indicate that Initialize is successed.\n
 *          \b CFALSE indicate that Initialize is failed.\n
 *  @see    MES_MLC_GetNumberOfModule
 */
CBOOL   MES_MLC_Initialize( void )
{
	static CBOOL bInit = CFALSE;
	U32 i;
	
	if( CFALSE == bInit )
	{
		for( i=0; i < NUMBER_OF_MLC_MODULE; i++ )
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
 *  @see        MES_MLC_Initialize
 */
U32     MES_MLC_GetNumberOfModule( void )
{
    return NUMBER_OF_MLC_MODULE;
}

//------------------------------------------------------------------------------
// Basic Interface
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 *  @brief      Get module's physical address.
 *  @return     Module's physical address
 *  @see        MES_MLC_GetSizeOfRegisterSet,
 *              MES_MLC_SetBaseAddress,       MES_MLC_GetBaseAddress,
 *              MES_MLC_OpenModule,           MES_MLC_CloseModule,
 *              MES_MLC_CheckBusy,            MES_MLC_CanPowerDown
 */
U32     MES_MLC_GetPhysicalAddress( U32 ModuleIndex )
{
    MES_ASSERT( NUMBER_OF_MLC_MODULE > ModuleIndex );

    return  (U32)( POLLUX_PA_MLC_PRI + (OFFSET_OF_MLC_MODULE * ModuleIndex) );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a size, in byte, of register set.
 *  @return     Size of module's register set.
 *  @see        MES_MLC_GetPhysicalAddress,
 *              MES_MLC_SetBaseAddress,       MES_MLC_GetBaseAddress,
 *              MES_MLC_OpenModule,           MES_MLC_CloseModule,
 *              MES_MLC_CheckBusy,            MES_MLC_CanPowerDown
 */
U32     MES_MLC_GetSizeOfRegisterSet( U32 ModuleIndex )
{
    return sizeof( struct MES_MLC_RegisterSet );
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set a base address of register set.
 *  @param[in]  BaseAddress Module's base address
 *  @return     None.
 *  @see        MES_MLC_GetPhysicalAddress,   MES_MLC_GetSizeOfRegisterSet,
 *              MES_MLC_GetBaseAddress,
 *              MES_MLC_OpenModule,           MES_MLC_CloseModule,
 *              MES_MLC_CheckBusy,            MES_MLC_CanPowerDown
 */
void    MES_MLC_SetBaseAddress( U32 ModuleIndex, U32 BaseAddress )
{
    MES_ASSERT( CNULL != BaseAddress );
    //MES_ASSERT( NUMBER_OF_MLC_MODULE > ModuleIndex );

    __g_ModuleVariables[ModuleIndex].pRegister = (struct MES_MLC_RegisterSet *)BaseAddress;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get a base address of register set
 *  @return     Module's base address.
 *  @see        MES_MLC_GetPhysicalAddress,   MES_MLC_GetSizeOfRegisterSet,
 *              MES_MLC_SetBaseAddress,
 *              MES_MLC_OpenModule,           MES_MLC_CloseModule,
 *              MES_MLC_CheckBusy,            MES_MLC_CanPowerDown
 */
U32     MES_MLC_GetBaseAddress( U32 ModuleIndex )
{
    MES_ASSERT( NUMBER_OF_MLC_MODULE > ModuleIndex );

    return (U32)__g_ModuleVariables[ModuleIndex].pRegister;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Initialize selected modules with default value.
 *  @return     \b CTRUE  indicate that Initialize is successed. \n
 *              \b CFALSE indicate that Initialize is failed.
 *  @see        MES_MLC_GetPhysicalAddress,   MES_MLC_GetSizeOfRegisterSet,
 *              MES_MLC_SetBaseAddress,       MES_MLC_GetBaseAddress,
 *              MES_MLC_CloseModule,
 *              MES_MLC_CheckBusy,            MES_MLC_CanPowerDown
 */
CBOOL   MES_MLC_OpenModule( U32 ModuleIndex )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Deinitialize selected module to the proper stage.
 *  @return     \b CTRUE  indicate that Deinitialize is successed. \n
 *              \b CFALSE indicate that Deinitialize is failed.
 *  @see        MES_MLC_GetPhysicalAddress,   MES_MLC_GetSizeOfRegisterSet,
 *              MES_MLC_SetBaseAddress,       MES_MLC_GetBaseAddress,
 *              MES_MLC_OpenModule,
 *              MES_MLC_CheckBusy,            MES_MLC_CanPowerDown
 */
CBOOL   MES_MLC_CloseModule( U32 ModuleIndex )
{
	return CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicates whether the selected modules is busy or not.
 *  @return     \b CTRUE  indicate that Module is Busy. \n
 *              \b CFALSE indicate that Module is NOT Busy.
 *  @see        MES_MLC_GetPhysicalAddress,   MES_MLC_GetSizeOfRegisterSet,
 *              MES_MLC_SetBaseAddress,       MES_MLC_GetBaseAddress,
 *              MES_MLC_OpenModule,           MES_MLC_CloseModule,
 *              MES_MLC_CanPowerDown
 */
CBOOL   MES_MLC_CheckBusy( U32 ModuleIndex )
{
	return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Indicaes whether the selected modules is ready to enter power-down stage
 *  @return     \b CTRUE  indicate that Ready to enter power-down stage. \n
 *              \b CFALSE indicate that This module can't enter to power-down stage.
 *  @see        MES_MLC_GetPhysicalAddress,   MES_MLC_GetSizeOfRegisterSet,
 *              MES_MLC_SetBaseAddress,       MES_MLC_GetBaseAddress,
 *              MES_MLC_OpenModule,           MES_MLC_CloseModule,
 *              MES_MLC_CheckBusy
 */
CBOOL   MES_MLC_CanPowerDown( U32 ModuleIndex )
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
 *  @see        MES_MLC_GetClockPClkMode,
 *              MES_MLC_SetClockBClkMode,       MES_MLC_GetClockBClkMode,
 */
void            MES_MLC_SetClockPClkMode( U32 ModuleIndex, MES_PCLKMODE mode )
{
    const U32 PCLKMODE_POS  =   3;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

	U32 clkmode=0;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	switch(mode)
	{
    	case MES_PCLKMODE_DYNAMIC:  clkmode = 0;		break;
    	case MES_PCLKMODE_ALWAYS:  	clkmode = 1;		break;
    	default: MES_ASSERT( CFALSE );
	}

    regvalue = pRegister->MLCCLKENB;

    regvalue &= ~(1UL<<PCLKMODE_POS);
    regvalue |= ( clkmode & 0x01 ) << PCLKMODE_POS;

    pRegister->MLCCLKENB = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get current PCLK mode
 *  @return     Current PCLK mode
 *  @see        MES_MLC_SetClockPClkMode,
 *              MES_MLC_SetClockBClkMode,       MES_MLC_GetClockBClkMode,
 */
MES_PCLKMODE    MES_MLC_GetClockPClkMode( U32 ModuleIndex )
{
    const U32 PCLKMODE_POS  = 3;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( pRegister->MLCCLKENB & ( 1UL << PCLKMODE_POS ) )
    {
        return MES_PCLKMODE_ALWAYS;
    }

    return  MES_PCLKMODE_DYNAMIC;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set System Bus Clock's operation Mode
 *  @param[in]  mode     BCLK Mode
 *  @return     None.
 *  @see        MES_MLC_SetClockPClkMode,       MES_MLC_GetClockPClkMode,
 *              MES_MLC_GetClockBClkMode,
 */
void            MES_MLC_SetClockBClkMode( U32 ModuleIndex, MES_BCLKMODE mode )
{
    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;
	U32 clkmode=0;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	switch(mode)
	{
    	case MES_BCLKMODE_DISABLE: 	clkmode = 0;		break;
    	case MES_BCLKMODE_DYNAMIC:	clkmode = 2;		break;
    	case MES_BCLKMODE_ALWAYS:	clkmode = 3;		break;
    	default: MES_ASSERT( CFALSE );
	}

	regvalue = pRegister->MLCCLKENB;
	regvalue &= ~(0x3);
	regvalue |= clkmode & 0x3;
	pRegister->MLCCLKENB = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get System Bus Clock's operation Mode
 *  @return     BCLK Mode
 *  @see        MES_MLC_SetClockPClkMode,       MES_MLC_GetClockPClkMode,
 *              MES_MLC_SetClockBClkMode,
 */
MES_BCLKMODE    MES_MLC_GetClockBClkMode( U32 ModuleIndex )
{
	const U32 BCLKMODE	= 3UL<<0;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	switch( pRegister->MLCCLKENB & BCLKMODE )
	{
    	case 0 :	return MES_BCLKMODE_DISABLE;
    	case 2 :	return MES_BCLKMODE_DYNAMIC;
    	case 3 :	return MES_BCLKMODE_ALWAYS;
	}

	MES_ASSERT( CFALSE );
	return MES_BCLKMODE_DISABLE;
}

//--------------------------------------------------------------------------
// MLC Main Settings
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	    Select block for synchronize of 3D layer's address change.
 *	@param[in]	sync    select block for sync
 *  @return     None.
 *  @remarks    3D layer's adddress is changed with select block's sync.
 *              Only primary MLC can set .
 *      	    The result of this function will be applied to MLC after calling function MES_MLC_SetTopDirtyFlag(). 9
 *  @see                                    MES_MLC_GetTop3DAddrChangeSync,
 *	            MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize, 
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
void    MES_MLC_SetTop3DAddrChangeSync( U32 ModuleIndex, MES_MLC_3DSYNC sync )
{
    const U32 GRP3DSEL_POS = 12;
    const U32 GRP3DSEL_MASK = 3 << GRP3DSEL_POS;
    const U32 DITTYFLAG_MASK = 1UL << 3;

    register struct MES_MLC_RegisterSet*    pRegister;
    register U32 regvalue;

    MES_ASSERT( 4 > sync );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    regvalue = pRegister->MLCCONTROLT;

    regvalue &= ~( GRP3DSEL_MASK | DITTYFLAG_MASK );
    regvalue |= ((U32)sync << GRP3DSEL_POS);

    pRegister->MLCCONTROLT = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get block that is use for 3D layer's change sync.
 *  @return     Block for synchronize of 3D layer's address change.
 *  @see        MES_MLC_SetTop3DAddrChangeSync,
 *	            MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize, 
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
MES_MLC_3DSYNC  MES_MLC_GetTop3DAddrChangeSync( U32 ModuleIndex )
{
    const U32 GRP3DSEL_POS = 12;
    const U32 GRP3DSEL_MASK = 3 << GRP3DSEL_POS;
    
    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    return (MES_MLC_3DSYNC)((pRegister->MLCCONTROLT & GRP3DSEL_MASK) >> GRP3DSEL_POS);
}


//------------------------------------------------------------------------------
/**
 *	@brief	    Set Power On/Off of MLC's Pixel Buffer Unit
 *  @param      bPower  \b CTRUE indicate that Power ON of pixel buffer unit. \n
 *                      \b CFALSE indicate that Power OFF of pixel buffer unit. \n
 *	@return     None.
 *	@remark	    When MLC ON, first pixel buffer power ON, set to Normal Mode(pixel buffer) and MLC enable.\n
 *              When MLC Off, first MLC disable, Set pixel buffer to Sleep Mode(pixel buffer) and power OFF.\n
 *  @code       
 *              // MLC ON sequence
 *              MES_MLC_SetTopPowerMode( CTRUE );           // pixel buffer power on
 *              MES_MLC_SetTopSleepMode( CFALSE );          // pixel buffer normal mode
 *              MES_MLC_SetMLCEnable( CTRUE );              // mlc enable
 *              MES_MLC_SetTopDirtyFlag();                  // apply setting value
 *              ...    
 *              // MLC OFF sequence    
 *              MES_MLC_SetMLCEnable( CFALSE );             // mlc disable
 *              MES_MLC_SetTopDirtyFlag();                  // apply setting value
 *              while( CTRUE == MES_MLC_GetTopDirtyFlag())  // wait until mlc is disabled
 *              { CNULL; }
 *              MES_MLC_SetTopSleepMode( CTRUE );           // pixel buffer sleep mode
 *              MES_MLC_SetTopPowerMode( CFALSE );          // pixel buffer power off
 *  @endcode    
 *	@see                                    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize, 
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */             
void    MES_MLC_SetTopPowerMode( U32 ModuleIndex, CBOOL bPower )
{               
    const U32 PIXELBUFFER_PWD_POS  = 11;
    const U32 PIXELBUFFER_PWD_MASK = 1UL << PIXELBUFFER_PWD_POS;
    const U32 DITTYFLAG_MASK = 1UL << 3;

    register struct MES_MLC_RegisterSet*    pRegister;
    register U32 regvalue;

    MES_ASSERT( (0==bPower) || (1==bPower) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    regvalue = pRegister->MLCCONTROLT;

    regvalue &= ~( PIXELBUFFER_PWD_MASK | DITTYFLAG_MASK );
    regvalue |= (bPower << PIXELBUFFER_PWD_POS);

    pRegister->MLCCONTROLT = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get power state of MLC's pixel buffer unit.
 *	@return     \b CTRUE indicate that pixel buffer is power ON.\n
 *              \b CFALSE indicate that pixel buffer is power OFF.\n
 *	@see        MES_MLC_SetTopPowerMode,    
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize, 
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
CBOOL   MES_MLC_GetTopPowerMode( U32 ModuleIndex )
{
    const U32 PIXELBUFFER_PWD_POS  = 11;
    const U32 PIXELBUFFER_PWD_MASK = 1UL << PIXELBUFFER_PWD_POS;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    return (CBOOL)((pRegister->MLCCONTROLT & PIXELBUFFER_PWD_MASK) >> PIXELBUFFER_PWD_POS );

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set Sleep Mode Enable/Disalbe of MLC's Pixel Buffer Unit
 *  @param      bSleep  \b CTRUE indicate that Sleep Mode Enable of pixel buffer unit. \n
 *                      \b CFALSE indicate that Sleep Mode Disable of pixel buffer unit. \n
 *	@return     None.
 *	@remark	    When MLC ON, first pixel buffer power ON, set to Normal Mode(pixel buffer) and MLC enable.\n
 *              When MLC Off, first MLC disable, Set pixel buffer to Sleep Mode(pixel buffer) and power OFF.\n
 *  @code       
 *              // MLC ON sequence
 *              MES_MLC_SetTopPowerMode( CTRUE );           // pixel buffer power on
 *              MES_MLC_SetTopSleepMode( CFALSE );          // pixel buffer normal mode
 *              MES_MLC_SetMLCEnable( CTRUE );              // mlc enable
 *              MES_MLC_SetTopDirtyFlag();                  // apply setting value
 *              ...    
 *              // MLC OFF sequence    
 *              MES_MLC_SetMLCEnable( CFALSE );             // mlc disable
 *              MES_MLC_SetTopDirtyFlag();                  // apply setting value
 *              while( CTRUE == MES_MLC_GetTopDirtyFlag())  // wait until mlc is disabled
 *              { CNULL; }
 *              MES_MLC_SetTopSleepMode( CTRUE );           // pixel buffer sleep mode
 *              MES_MLC_SetTopPowerMode( CFALSE );          // pixel buffer power off
 *  @endcode    
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *                                          MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize, 
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
void    MES_MLC_SetTopSleepMode( U32 ModuleIndex, CBOOL bSleep )
{
    const U32 PIXELBUFFER_SLD_POS  = 10;
    const U32 PIXELBUFFER_SLD_MASK = 1UL << PIXELBUFFER_SLD_POS;
    const U32 DITTYFLAG_MASK = 1UL << 3;

    register struct MES_MLC_RegisterSet*    pRegister;
    register U32 regvalue;

    MES_ASSERT( (0==bSleep) || (1==bSleep) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	bSleep = (CBOOL)((U32)bSleep ^ 1);

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    regvalue = pRegister->MLCCONTROLT;

    regvalue &= ~( PIXELBUFFER_SLD_MASK | DITTYFLAG_MASK );
    regvalue |= (bSleep << PIXELBUFFER_SLD_POS);

    pRegister->MLCCONTROLT = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get Sleep Mode state of MLC's pixel buffer unit.
 *	@return     \b CTRUE indicate that pixel buffer is Sleep Mode.\n
 *              \b CFALSE indicate that pixel buffer is Normal Mode.\n
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize, 
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
CBOOL   MES_MLC_GetTopSleepMode( U32 ModuleIndex )
{
    const U32 PIXELBUFFER_SLD_POS  = 11;
    const U32 PIXELBUFFER_SLD_MASK = 1UL << PIXELBUFFER_SLD_POS;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    return (CBOOL)(((pRegister->MLCCONTROLT & PIXELBUFFER_SLD_MASK) >> PIXELBUFFER_SLD_POS) ^ 0x01 );
}

//------------------------------------------------------------------------------
/**
 *	@brief	Apply modified MLC Top registers to MLC.
 *	@return None.
 *	@remark	MLC has dual register set architecture. Therefore you have to set a
 *			dirty flag to apply modified settings to MLC's current settings.
 *			If a dirty flag is set, MLC will update current settings to
 *			register values on a vertical blank. You can also check whether MLC
 *			 has been updated by using function MES_MLC_GetTopDirtyFlag().
 *	@see    MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *          MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *                                      MES_MLC_GetTopDirtyFlag,
 *          MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *          MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *          MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize,
 *          MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
void	MES_MLC_SetTopDirtyFlag( U32 ModuleIndex )
{
	const U32 DIRTYFLAG = 1UL<<3;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	pRegister->MLCCONTROLT |= DIRTYFLAG;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Informs whether modified settings is applied to MLC or not.
 *	@return \b CTRUE indicates MLC does not update to modified settings yet.\n
 *			\b CFALSE indicates MLC has already been updated to modified settings.
 *	@see    MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *          MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *                                      MES_MLC_GetTopDirtyFlag,
 *          MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *          MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *          MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize,
 *          MES_MLC_SetBackground,      MES_MLC_GetScreenSize

 */
CBOOL	MES_MLC_GetTopDirtyFlag( U32 ModuleIndex )
{
    const U32 DIRTYFLAG_POS = 3;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	return (CBOOL)((pRegister->MLCCONTROLT & DIRTYFLAG_MASK) >> DIRTYFLAG_POS );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable MLC.
 *  @param[in]  bEnb		   Set it to CTRUE to enable MLC.
 *	@return     None.
 *  @remark	    The result of this function will be applied	to MLC after calling
 * 			    function MES_MLC_SetTopDirtyFlag().
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *                                          MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize,
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
void	MES_MLC_SetMLCEnable( U32 ModuleIndex, CBOOL bEnb )
{
    const U32 MLCENB_POS  = 1;
	const U32 MLCENB_MASK = 1UL<<MLCENB_POS;

	const U32 DIRTYFLAG_POS  = 3;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( (0==bEnb) || (1==bEnb) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	regvalue = pRegister->MLCCONTROLT;

	regvalue &= ~( MLCENB_MASK | DIRTYFLAG_MASK );
	regvalue |= (bEnb<<MLCENB_POS);

	pRegister->MLCCONTROLT = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Informs whether MLC is enabled or disabled.
 *	@return \b CTRUE indicates MLC is enabled.\n
 *			\b CFALSE indicates MLC is disabled.
 *	@see    MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *          MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *          MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *          MES_MLC_SetMLCEnable,
 *          MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *          MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize,
 *          MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
CBOOL	MES_MLC_GetMLCEnable( U32 ModuleIndex )
{
    const U32 MLCENB_POS  = 1;
	const U32 MLCENB_MASK = 1UL<<MLCENB_POS;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	return (CBOOL)((pRegister->MLCCONTROLT & MLCENB_MASK) >> MLCENB_POS );
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable interlace mode.
 *  @param[in]  bEnb			Set it to CTRUE to enable interlace mode.
 *	@return     None.
 *  @remark	    The result of this function will be applied	to MLC after calling
 * 			    function MES_MLC_SetTopDirtyFlag().
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *                                          MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize,
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
void	MES_MLC_SetFieldEnable( U32 ModuleIndex, CBOOL bEnb )
{
    const U32 FIELDENB_POS  = 0;
	const U32 FIELDENB_MASK = 1UL<<FIELDENB_POS;

	const U32 DIRTYFLAG_POS  = 3;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( (0==bEnb) || (1==bEnb) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	regvalue = pRegister->MLCCONTROLT;

	regvalue &= ~( FIELDENB_MASK | DIRTYFLAG_MASK );
	regvalue |= (bEnb<<FIELDENB_POS);

	pRegister->MLCCONTROLT = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	Informs whether MLC is interlace mode or progressive mode.
 *	@return \b CTRUE indicates MLC is interlace mode.\n
 *			\b CFALSE indicates MLC is progressive mode.
 *	@see    MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *          MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *          MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *          MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *          MES_MLC_SetFieldEnable,
 *          MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize,
 *          MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
CBOOL	MES_MLC_GetFieldEnable( U32 ModuleIndex )
{
    const U32 FIELDENB_POS  = 0;
	const U32 FIELDENB_MASK = 1UL<<FIELDENB_POS;

    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	return (CBOOL)(pRegister->MLCCONTROLT & FIELDENB_MASK);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the priority of layers.
 *  @param[in]  priority	    the priority of layers
 *	@return     None.
 *  @remark	    The result of this function will be applied	to MLC after calling
 * 			    function MES_MLC_SetTopDirtyFlag().
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *                                          MES_MLC_SetScreenSize,
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
void	MES_MLC_SetLayerPriority( U32 ModuleIndex, MES_MLC_PRIORITY priority )
{
    const U32 PRIORITY_POS = 8;
    const U32 PRIORITY_MASK = 0x03 << PRIORITY_POS;

	const U32 DIRTYFLAG_POS  = 3;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( (MES_MLC_PRIORITY_VIDEOFIRST == priority) || (MES_MLC_PRIORITY_VIDEOSECOND == priority) || (MES_MLC_PRIORITY_VIDEOTHIRD == priority) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	regvalue = pRegister->MLCCONTROLT;

	regvalue &= ~( PRIORITY_MASK | DIRTYFLAG_MASK );
	regvalue |= (priority<<PRIORITY_POS);

	pRegister->MLCCONTROLT = regvalue;

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the screen size.
 *  @param[in]  width 		the screen width, 1 ~ 4096.
 *  @param[in]  height		the screen height, 1 ~ 4096.
 *	@return     None.
 *  @remark	    The result of this function will be applied	to MLC after calling
 * 			    function MES_MLC_SetTopDirtyFlag().
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,
 *              MES_MLC_SetBackground,      MES_MLC_GetScreenSize
 */
void	MES_MLC_SetScreenSize( U32 ModuleIndex, U32 width, U32 height )
{
    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( 4096 > (width - 1) );
	MES_ASSERT( 4096 > (height - 1) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	pRegister->MLCSCREENSIZE = ((height-1)<<16) | (width-1);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get the screen size.
 *  @param[out] pWidth 		the screen width, 1 ~ 4096.
 *  @param[out] pHeight		the screen height, 1 ~ 4096.
 *	@return     None.
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize,
 *              MES_MLC_SetBackground
 */
void    MES_MLC_GetScreenSize( U32 ModuleIndex, U32 *pWidth, U32 *pHeight )
{
    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( CNULL != pWidth )
    {
        *pWidth = (pRegister->MLCSCREENSIZE & 0x0FFF) + 1;
    }

    if( CNULL != pHeight )
    {
        *pHeight = ((pRegister->MLCSCREENSIZE>>16) & 0x0FFF) + 1;
    }

}


//------------------------------------------------------------------------------
/**
 *	@brief	    Set the background color.
 *  @param[in]  color    24 bit RGB format, 0xXXRRGGBB = { R[7:0], G[7:0], B[7:0] }
 *	@return     None.
 *  @remark	    The background color is default color that is shown in regions which
 *			    any layer does not include. the result of this function will be
 *			    applied to MLC after calling function MES_MLC_SetTopDirtyFlag().
 *	@see        MES_MLC_SetTopPowerMode,    MES_MLC_GetTopPowerMode,
 *              MES_MLC_SetTopSleepMode,    MES_MLC_GetTopSleepMode,
 *              MES_MLC_SetTopDirtyFlag,    MES_MLC_GetTopDirtyFlag,
 *              MES_MLC_SetMLCEnable,       MES_MLC_GetMLCEnable,
 *              MES_MLC_SetFieldEnable,     MES_MLC_GetFieldEnable,
 *              MES_MLC_SetLayerPriority,   MES_MLC_SetScreenSize
 */
void	MES_MLC_SetBackground( U32 ModuleIndex, U32 color )
{
    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	pRegister->MLCBGCOLOR = color;
}


//--------------------------------------------------------------------------
//	Per Layer Operations

//------------------------------------------------------------------------------
/**
 *	@brief	    Set power control of layer's power control unit.
 *  @param[in]  layer   Select layer ( 0:RGB0, 1:RGB1, 2:Video )    
 *  @param[in]  bPower  \b CTRUE indicate that Power ON of selected layer's power control unit. \n
 *                      \b CFALSE indicate that Power OFF of selected layer's power control unit. \n
 *	@return     None.
 *  @remark	    If RGB layer is selected, 
 *                  - power control unit is Palette Table.
 *                  - Palette table can power on/off during RGB layer operation.
 *                  - Palette table only use when RGB layer is selected to palette format.
 *                  - Palette table On sequence
 *                      - Power On
 *                      - Normal Mode ( Sleep Mode disable )
 *                  - Palette table OFF sequence
 *                      - Sleep Mode
 *                      - Power Off
 *              \n
 *              If Video layer is selected, 
 *                  - Specail unit is Line buffer.\n
 *                  - Line buffer can power on/off during Video layer operation.
 *                  - Line buffer only use when Video layer's Scale Up/Down operation with biliner filter On .
 *                  - Line buffer ON sequence
 *                      - Power On
 *                      - Normal Mode ( Sleep Mode disable )
 *                  - Line buffer OFF sequence
 *                      - Sleep Mode
 *                      - Power Off
 * @code
 *              // Line Buffer or Palette Table.. On sequence, n = 0 ~ 2
 *              MES_MLC_SetLayerPowerMode( n, CTRUE );          // power on
 *              MES_MLC_SetLayerSleepMode( n, CFALSE );         // normal mode
 *              MES_MLC_SetLayerEnable( n, CTRUE );             // layer enable
 *              MES_MLC_SetDirtyFlag( n );                      // apply now
 *              ...
 *              // Line Buffer or Palette Table.. Off sequence  n = 0 ~ 2
 *              MES_MLC_SetLayerEnable( n, CFALSE );            // layer disable
 *              MES_MLC_SetDirtyFlag( n );                      // apply now
 *              while( CTRUE == MES_MLC_GetDirtyFlag( n ))      // wait until layer is disabled
 *              { CNULL; }
 *              MES_MLC_SetLayerSleepMode( n, CTRUE );          // sleep mode
 *              MES_MLC_SetLayerPowerMode( n, CFALSE );         // layer off
 * @endcode
 * @see                                    MES_MLC_GetLayerPowerMode,
 *             MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *             MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *             MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *             MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *             MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency, 
 *             MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *             MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void    MES_MLC_SetLayerPowerMode( U32 ModuleIndex, U32 layer, CBOOL bPower )
{
    const U32 PALETTEPWD_POS  = 15;
    const U32 PALETTEPWD_MASK = 1UL << PALETTEPWD_POS;

    const U32 LINEBUFF_PWD_POS   = 15;
    const U32 LINEBUFF_PWD_MASK  = 1UL << LINEBUFF_PWD_POS;
    const U32 DIRTYFLAG_MASK = 1UL << 4;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet*    pRegister;

    MES_ASSERT( 3 > layer );
    MES_ASSERT( (0==bPower) || (1==bPower) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )  // RGB
    {
        regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;

        regvalue &= ~( PALETTEPWD_MASK | DIRTYFLAG_MASK );
        regvalue |=  (bPower << PALETTEPWD_POS);    

        pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;
    }
    else if ( 2 == layer )  // Video
    {
        regvalue = pRegister->MLCCONTROL2;

        regvalue &= ~( LINEBUFF_PWD_MASK | DIRTYFLAG_MASK );
        regvalue |= (bPower << LINEBUFF_PWD_POS);

        pRegister->MLCCONTROL2 = regvalue;
    }    
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get power state of selected layer's power control unit.
 *  @param[in]  layer   Select layer ( 0:RGB0, 1:RGB1, 2:Video )    
 *	@return     \b CTRUE indicate that selected layer's power control unit is power ON.\n
 *              \b CFALSE indicate that selected layer's power control unit is power OFF.\n
 *  @remarks    If RGB layer is selected, then power control unit is Palette Table. \n
 *              If Video layer is selected, then power control unit is Line Buffer. \n
 *  @see        MES_MLC_SetLayerPowerMode,  
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *              MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency, 
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
CBOOL   MES_MLC_GetLayerPowerMode( U32 ModuleIndex, U32 layer )
{
    const U32 PALETTEPWD_POS  = 15;
    const U32 PALETTEPWD_MASK = 1UL << PALETTEPWD_POS;

    const U32 LINEBUFF_PWD_POS   = 15;
    const U32 LINEBUFF_PWD_MASK  = 1UL << LINEBUFF_PWD_POS;

    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 3 > layer );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )  // RGB
    {
        return (CBOOL)((pRegister->MLCRGBLAYER[layer].MLCCONTROL & PALETTEPWD_MASK ) >> PALETTEPWD_POS);
    }
    else if ( 2 == layer )  // Video
    {
        return (CBOOL)((pRegister->MLCCONTROL2 & LINEBUFF_PWD_MASK ) >> LINEBUFF_PWD_POS);
    }    
    
    return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set Sleep mode of layer's power control unit.
 *  @param[in]  layer   Select layer ( 0:RGB0, 1:RGB1, 2:Video )    
 *  @param[in]  bSleep  \b CTRUE indicate that \b SLEEP Mode of selected layer's power control unit. \n
 *                      \b CFALSE indicate that \b NORMAL Mode of selected layer's power control unit. \n
 *	@return     None.
 *  @remark	    If RGB layer is selected, 
 *                  - Power control unit is Palette Table.
 *                  - Palette table can power on/off during RGB layer operation.
 *                  - Palette table only use when RGB layer is selected to palette mode.
 *                  - Palette table On sequence
 *                      - Power On
 *                      - Normal Mode ( Sleep Mode disable )
 *                  - Palette table OFF sequence
 *                      - Sleep Mode
 *                      - Power Off
 *              \n
 *              If Video layer is selected, 
 *                  - Power control unit is Line buffer.\n
 *                  - Line buffer can power on/off during Video layer operation.
 *                  - Line buffer only use when Video layer's Scale Up/Down operation with biliner filter On .
 *                  - Line buffer ON sequence
 *                      - Power On
 *                      - Normal Mode ( Sleep Mode disable )
 *                  - Line buffer OFF sequence
 *                      - Sleep Mode
 *                      - Power Off
 * @code
 *              // Line Buffer or Palette Table.. On sequence, n = 0 ~ 2
 *              MES_MLC_SetLayerPowerMode( n, CTRUE );          // power on
 *              MES_MLC_SetLayerSleepMode( n, CFALSE );         // normal mode
 *              MES_MLC_SetLayerEnable( n, CTRUE );             // layer enable
 *              MES_MLC_SetDirtyFlag( n );                      // apply now
 *              ...
 *              // Line Buffer or Palette Table.. Off sequence  n = 0 ~ 2
 *              MES_MLC_SetLayerEnable( n, CFALSE );            // layer disable
 *              MES_MLC_SetDirtyFlag( n );                      // apply now
 *              while( CTRUE == MES_MLC_GetDirtyFlag( n ))      // wait until layer is disabled
 *              { CNULL; }
 *              MES_MLC_SetLayerSleepMode( n, CTRUE );          // sleep mode
 *              MES_MLC_SetLayerPowerMode( n, CFALSE );         // layer off
 * @endcode
 * @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *                                         MES_MLC_GetLayerSleepMode,
 *             MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *             MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *             MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *             MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency, 
 *             MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *             MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void    MES_MLC_SetLayerSleepMode( U32 ModuleIndex, U32 layer, CBOOL bSleep )
{
    const U32 PALETTESLD_POS  = 14;
    const U32 PALETTESLD_MASK = 1UL << PALETTESLD_POS;

    const U32 LINEBUFF_SLMD_POS   = 14;
    const U32 LINEBUFF_SLMD_MASK  = 1UL << LINEBUFF_SLMD_POS;
    const U32 DIRTYFLAG_MASK = 1UL << 4;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet*    pRegister;

    MES_ASSERT( 3 > layer );
    MES_ASSERT( (0==bSleep) || (1==bSleep) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );
	bSleep = (CBOOL)((U32)bSleep ^ 1);

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )  // RGB
    {
        regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;

        regvalue &= ~( PALETTESLD_MASK | DIRTYFLAG_MASK );
        regvalue |=  (bSleep << PALETTESLD_POS);    

        pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;
    }
    else if ( 2 == layer )  // Video
    {
        regvalue = pRegister->MLCCONTROL2;

        regvalue &= ~( LINEBUFF_SLMD_MASK | DIRTYFLAG_MASK );
        regvalue |= (bSleep << LINEBUFF_SLMD_POS);

        pRegister->MLCCONTROL2 = regvalue;
    }        
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get sleep mode state of selected layer's power control unit.
 *  @param[in]  layer   Select layer ( 0:RGB0, 1:RGB1, 2:Video )    
 *	@return     \b CTRUE indicate that selected layer's power control unit is power ON.\n
 *              \b CFALSE indicate that selected layer's power control unit is power OFF.\n
 *  @remarks    If RGB layer is selected, then power control unit is Palette Table. \n
 *              If Video layer is selected, then power control unit is Line Buffer. \n
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  
 *              MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency, 
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
CBOOL   MES_MLC_GetLayerSleepMode( U32 ModuleIndex, U32 layer )
{
    const U32 PALETTESLD_POS  = 14;
    const U32 PALETTESLD_MASK = 1UL << PALETTESLD_POS;

    const U32 LINEBUFF_SLMD_POS   = 14;
    const U32 LINEBUFF_SLMD_MASK  = 1UL << LINEBUFF_SLMD_POS;

	register struct MES_MLC_RegisterSet*    pRegister;

    MES_ASSERT( 3 > layer );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )  // RGB
    {
        return (CBOOL)(((pRegister->MLCRGBLAYER[layer].MLCCONTROL & PALETTESLD_MASK ) >> PALETTESLD_POS)^0x01); // 0: Sleep Mode 1: Normal
    }
    else if ( 2 == layer )  // Video
    {
        return (CBOOL)(((pRegister->MLCCONTROL2 & LINEBUFF_SLMD_MASK ) >> LINEBUFF_SLMD_POS)^0x01); // 0: Sleep Mode 1: Normal
    }     
    return CFALSE;   
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Apply modified register values to corresponding layer.
 *  @param[in]  layer    the layer number ( 0: RGB0, 1: RGB1, 2: Video ).
 *	@return     None.
 *	@remark     Each layer has dual register set architecture. Therefore you have to
 * 		        set a dirty flag to apply modified settings to each layer's current
 * 		        settings. If a dirty flag is set, each layer will update current
 * 		        settings to register values on a vertical blank. You can also check
 * 		        whether each layer has been updated by using function MES_MLC_GetDirtyFlag().
 * @see         MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	                                        MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void 	MES_MLC_SetDirtyFlag( U32 ModuleIndex, U32 layer )
{
    const U32 DIRTYFLG_MASK = 1UL << 4;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 3 > layer );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if ( 0==layer || 1==layer )
    {
        pRegister->MLCRGBLAYER[layer].MLCCONTROL |= DIRTYFLG_MASK;
    }
    else if( 2 == layer )
    {
        pRegister->MLCCONTROL2 |= DIRTYFLG_MASK;
    }

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Informs whether modified settings is applied to corresponding layer or not.
 *  @param[in]  layer    the layer number ( 0: RGB0, 1: RGB1, 2: Video ).
 *	@return     \b CTRUE indicates corresponding layer does not update to modified settings yet.\n
 *			    \b CFALSE indicates corresponding layer has already been updated to modified settings.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
CBOOL 	MES_MLC_GetDirtyFlag( U32 ModuleIndex, U32 layer )
{
    const U32 DIRTYFLG_POS = 4;
    const U32 DIRTYFLG_MASK = 1UL << DIRTYFLG_POS;

    register struct MES_MLC_RegisterSet* pRegister;
	
	MES_ASSERT( 3 > layer );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if ( 0==layer || 1==layer )
    {
        return (CBOOL)((pRegister->MLCRGBLAYER[layer].MLCCONTROL & DIRTYFLG_MASK ) >> DIRTYFLG_POS);
    }
    else if( 2 == layer )
    {
        return (CBOOL)((pRegister->MLCCONTROL2 & DIRTYFLG_MASK ) >> DIRTYFLG_POS);
    }

    return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Enable/Disable the layer.
 *  @param[in]  layer 	       the layer number ( 0: RGB0, 1: RGB1, 2: Video ).
 *  @param[in]  bEnb		   Set it to CTRUE to enable corresponding layer
 *	@return     None.
 *  @remark	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *                                          MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void	MES_MLC_SetLayerEnable( U32 ModuleIndex, U32 layer, CBOOL bEnb )
{
    const U32 LAYERENB_POS = 5;
    const U32 LAYERENB_MASK = 0x01 << LAYERENB_POS;

	const U32 DIRTYFLAG_POS  = 4;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 3 > layer );
    MES_ASSERT( (0==bEnb) || (1==bEnb) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )
    {
    	regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;

    	regvalue &= ~( LAYERENB_MASK | DIRTYFLAG_MASK );
    	regvalue |= (bEnb<<LAYERENB_POS);

    	pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;
    }
    else if( 2 == layer )
    {
    	regvalue = pRegister->MLCCONTROL2;

    	regvalue &= ~( LAYERENB_MASK | DIRTYFLAG_MASK );
    	regvalue |= (bEnb<<LAYERENB_POS);

    	pRegister->MLCCONTROL2 = regvalue;
    }
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Determines whether the layer is enabled or disabled.
 *  @param[in]  layer    the layer number ( 0: RGB0, 1: RGB1, 2: Video ).
 *	@return     \b CTRUE indicates the layer is enabled.\n
 *			    \b CFALSE indicates the layer is disabled.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
CBOOL	MES_MLC_GetLayerEnable( U32 ModuleIndex, U32 layer )
{
    const U32 LAYERENB_POS = 5;
    const U32 LAYERENB_MASK = 0x01 << LAYERENB_POS;

    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 3 > layer );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )
    {
        return (CBOOL)((pRegister->MLCRGBLAYER[layer].MLCCONTROL & LAYERENB_MASK ) >> LAYERENB_POS);
    }
    else if( 2 == layer )
    {
        return (CBOOL)((pRegister->MLCCONTROL2 & LAYERENB_MASK ) >> LAYERENB_POS);
    }

    return CFALSE;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the lock size for memory access.
 *  @param[in]  layer 		   the layer number ( 0: RGB0, 1: RGB1 ).
 *  @param[in]  locksize	   lock size for memory access, 4, 8, 16 are only valid.
 *	@return     None.
 *  @remark	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.\n
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *                                          MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void 	MES_MLC_SetLockSize( U32 ModuleIndex, U32 layer, U32 locksize )
{
	const U32 LOCKSIZE_MASK	= 3UL<<12;

	const U32 DIRTYFLAG_POS  = 4;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    register struct MES_MLC_RegisterSet* pRegister;
    register U32 regvalue;

    MES_ASSERT( 2 > layer );
	MES_ASSERT( 4 == locksize || 8 == locksize || 16 == locksize );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	locksize >>= 3;	// divide by 8

	regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;

	regvalue &= ~(LOCKSIZE_MASK|DIRTYFLAG_MASK);
	regvalue |= (locksize<<12);

	pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Specifies whether this layer is used for 3D graphic engine.
 *  @param[in]  layer	    the layer number ( 0: RGB0, 1: RGB1 ).
 *  @param[in]  bEnb 		Set it to CTRUE to use corresponding layer for 3D Graphic engine.
 *	@return     None.
 *	@remark	    There must be simultaneously one layer which is used for 3D graphic
 * 			    engine. The result of this function will be applied to corresponding
 * 			    layer after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 * @see         MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void 	MES_MLC_Set3DEnb( U32 ModuleIndex, U32 layer, CBOOL bEnb )
{
    const U32 GRP3DENB_POS = 8;
    const U32 GRP3DENB_MASK = 0x01 << GRP3DENB_POS;

	const U32 DIRTYFLAG_POS  = 4;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 2 > layer );
    MES_ASSERT( (0==bEnb) || (1==bEnb) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;

	regvalue &= ~( GRP3DENB_MASK | DIRTYFLAG_MASK );
	regvalue |= (bEnb<<GRP3DENB_POS);

	pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set Alpha blending.
 *  @param[in]  layer 	the layer number( 0: RGB0, 1: RGB1, 2: Video ).
 *  @param[in]  bEnb 	Set it to CTRUE to enable alpha blending.
 *  @param[in]  alpha 	alpha blending factor, 0 ~ 15.
 *              - When it is set to 0, this layer color is fully transparent.
 *              - When it is set to 15, this layer becomes fully opaque.
 *	@return     None.
 *  @remark	    The argument 'alpha' has only affect when the color format has
 * 				no alpha component. The formula for alpha blending is as follows.
 *			    - If alpha is 0 then a is 0, else a is ALPHA + 1.\n
 *			      color = this layer color * a / 16 + lower layer color * (16 - a) / 16.
 *			    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *                                          MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void	MES_MLC_SetAlphaBlending( U32 ModuleIndex, U32 layer, CBOOL bEnb, U32 alpha )
{
    const U32 BLENDENB_POS = 2;
    const U32 BLENDENB_MASK = 0x01 << BLENDENB_POS;

	const U32 DIRTYFLAG_POS  = 4;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    const U32 ALPHA_POS = 28;
    const U32 ALPHA_MASK = 0xF << ALPHA_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 3 > layer );
    MES_ASSERT( (0==bEnb) || (1==bEnb) );
    MES_ASSERT( 16 > alpha );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )
    {
    	regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;
       	regvalue &= ~( BLENDENB_MASK | DIRTYFLAG_MASK );
    	regvalue |= (bEnb<<BLENDENB_POS);
    	pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;

        regvalue = pRegister->MLCRGBLAYER[layer].MLCTPCOLOR;
        regvalue &= ~ALPHA_MASK;
        regvalue |= alpha << ALPHA_POS;
        pRegister->MLCRGBLAYER[layer].MLCTPCOLOR = regvalue;
    }
    else if( 2 == layer )
    {
    	regvalue = pRegister->MLCCONTROL2;
    	regvalue &= ~( BLENDENB_MASK | DIRTYFLAG_MASK );
    	regvalue |= (bEnb<<BLENDENB_POS);
    	pRegister->MLCCONTROL2 = regvalue;

        pRegister->MLCTPCOLOR2 = alpha << ALPHA_POS;
    }
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set transparency.
 *  @param[in]  layer	the layer number ( 0: RGB0, 1: RGB1 ).
 *  @param[in]  bEnb    Set it to CTRUE to enable transparency.
 *  @param[in]  color	Specifies the extended color to be used as the transparency color.\n
 *              		24 bit RGB format, 0xXXRRGGBB = { R[7:0], G[7:0], B[7:0] }\n
 *						You can get this argument from specific color format
 *						by using the function MES_MLC_GetExtendedColor().
 *	@return     None.
 *	@remark	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void	MES_MLC_SetTransparency( U32 ModuleIndex, U32 layer, CBOOL bEnb, U32 color )
{
    const U32 TPENB_POS = 0;
    const U32 TPENB_MASK = 0x01 << TPENB_POS;

	const U32 DIRTYFLAG_POS  = 4;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    const U32 TPCOLOR_POS = 0;
    const U32 TPCOLOR_MASK = ((1<<24)-1) << TPCOLOR_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 2 > layer );
    MES_ASSERT( (0==bEnb) || (1==bEnb) );
    MES_ASSERT( (1<<24) > color );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;
   	regvalue &= ~( TPENB_MASK | DIRTYFLAG_MASK );
	regvalue |= (bEnb<<TPENB_POS);
	pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;

    regvalue = pRegister->MLCRGBLAYER[layer].MLCTPCOLOR;
    regvalue &= ~TPCOLOR_MASK;
    regvalue |= (color & TPCOLOR_MASK);
    pRegister->MLCRGBLAYER[layer].MLCTPCOLOR = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set color inversion.
 *  @param[in]  layer	the layer number ( 0: RGB0, 1: RGB1 ).
 *  @param[in]  bEnb	Set it to CTRUE to enable color inversion.
 *  @param[in]  color	Specifies the extended color to be used for color inversion.\n
 *              		24 bit RGB format, 0xXXRRGGBB = { R[7:0], G[7:0], B[7:0] }\n
 *						You can get this argument from specific color format
 *						by using the function MES_MLC_GetExtendedColor().
 *	@return     None.
 *	@remark	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *                                          MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
void	MES_MLC_SetColorInversion( U32 ModuleIndex, U32 layer, CBOOL bEnb, U32 color )
{
    const U32 INVENB_POS = 1;
    const U32 INVENB_MASK = 0x01 << INVENB_POS;

	const U32 DIRTYFLAG_POS  = 4;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

    const U32 INVCOLOR_POS = 0;
    const U32 INVCOLOR_MASK = ((1<<24)-1) << INVCOLOR_POS;

    register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( 2 > layer );
    MES_ASSERT( (0==bEnb) || (1==bEnb) );
    MES_ASSERT( (1<<24) > color );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;
   	regvalue &= ~( INVENB_MASK | DIRTYFLAG_MASK );
	regvalue |= (bEnb<<INVENB_POS);
	pRegister->MLCRGBLAYER[layer].MLCCONTROL = regvalue;

    regvalue = pRegister->MLCRGBLAYER[layer].MLCINVCOLOR;
    regvalue &= ~INVCOLOR_MASK;
    regvalue |= (color & INVCOLOR_MASK);
    pRegister->MLCRGBLAYER[layer].MLCINVCOLOR = regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Get extended color.
 *  @param[in]  color	Specifies the color value to be converted.
 *  @param[in]  format 	the color format with RGBFMT type.
 *	@return     the color which has 24 bit RGB format\n
 * 				0xXXRRGGBB = { R[7:0], G[7:0], B[7:0] }
 *	@remark	    This function is for argument 'color' of the function
 *				MES_MLC_SetTransparency() and MES_MLC_SetColorInversion().
 *		        For example,
 *	@code
 *		MES_MLC_SetTransparency  ( layer, CTRUE, MES_MLC_GetExtendedColor(   0x0841, RGBFMT_R5G6B5 ) );
 *		MES_MLC_SetColorInversion( layer, CTRUE, MES_MLC_GetExtendedColor( 0x090909, RGBFMT_R8G8B8 ) );
 *	@endcode
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *              MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,
 *              MES_MLC_SetFormat,          MES_MLC_SetPosition
 */
U32		MES_MLC_GetExtendedColor( U32 color, MES_MLC_RGBFMT format )
{
	U32 rgb[3];
	int	bw[3], bp[3], blank, fill, i;

	MES_ASSERT( 0 == (format & 0x0000FFFFUL) );
	switch( format )
	{
	case MES_MLC_RGBFMT_R5G6B5   :  // 16bpp { R5, G6, B5 }.
		bw[0] =  5;		bw[1] =  6;		bw[2] =  5;
		bp[0] = 11; 	bp[1] =  5; 	bp[2] =  0;
		break;
	case MES_MLC_RGBFMT_B5G6R5   :  // 16bpp { B5, G6, R5 }.
		bw[0] =  5;		bw[1] =  6;		bw[2] =  5;
		bp[0] =  0; 	bp[1] =  5; 	bp[2] = 11;
		break;
	case MES_MLC_RGBFMT_X1R5G5B5 :  // 16bpp { X1, R5, G5, B5 }.
	case MES_MLC_RGBFMT_A1R5G5B5 :  // 16bpp { A1, R5, G5, B5 }.
		bw[0] =  5;		bw[1] =  5;		bw[2] =  5;
		bp[0] = 10; 	bp[1] =  5; 	bp[2] =  0;
		break;
	case MES_MLC_RGBFMT_X1B5G5R5 :  // 16bpp { X1, B5, G5, R5 }.
	case MES_MLC_RGBFMT_A1B5G5R5 :  // 16bpp { A1, B5, G5, R5 }.
		bw[0] =  5;		bw[1] =  5;		bw[2] =  5;
		bp[0] =  0; 	bp[1] =  5; 	bp[2] = 10;
		break;
	case MES_MLC_RGBFMT_X4R4G4B4 :  // 16bpp { X4, R4, G4, B4 }.
	case MES_MLC_RGBFMT_A4R4G4B4 :  // 16bpp { A4, R4, G4, B4 }.
		bw[0] =  4;		bw[1] =  4;		bw[2] =  4;
		bp[0] =  8; 	bp[1] =  4; 	bp[2] =  0;
		break;
	case MES_MLC_RGBFMT_X4B4G4R4 :  // 16bpp { X4, B4, G4, R4 }.
	case MES_MLC_RGBFMT_A4B4G4R4 :  // 16bpp { A4, B4, G4, R4 }.
		bw[0] =  4;		bw[1] =  4;		bw[2] =  4;
		bp[0] =  0; 	bp[1] =  4; 	bp[2] =  8;
		break;
	case MES_MLC_RGBFMT_X8R3G3B2 :  // 16bpp { X8, R3, G3, B2 }.
	case MES_MLC_RGBFMT_A8R3G3B2 :  // 16bpp { A8, R3, G3, B2 }.
		bw[0] =  3;		bw[1] =  3;		bw[2] =  2;
		bp[0] =  5; 	bp[1] =  2; 	bp[2] =  0;
		break;
	case MES_MLC_RGBFMT_X8B3G3R2 :  // 16bpp { X8, B3, G3, R2 }.
	case MES_MLC_RGBFMT_A8B3G3R2 :  // 16bpp { A8, B3, G3, R2 }.
		bw[0] =  2;		bw[1] =  3;		bw[2] =  3;
		bp[0] =  0; 	bp[1] =  2; 	bp[2] =  5;
		break;
	case MES_MLC_RGBFMT_R8G8B8   :  // 24bpp { R8, G8, B8 }.
//	case MES_MLC_RGBFMT_X8R8G8B8 :  // 32bpp { X8, R8, G8, B8 }.
	case MES_MLC_RGBFMT_A8R8G8B8 :  // 32bpp { A8, R8, G8, B8 }.
		bw[0] =  8;		bw[1] =  8;		bw[2] =  8;
		bp[0] = 16; 	bp[1] =  8; 	bp[2] =  0;
		break;
	case MES_MLC_RGBFMT_B8G8R8   :  // 24bpp { B8, G8, R8 }.
//	case MES_MLC_RGBFMT_X8B8G8R8 :  // 32bpp { X8, B8, G8, R8 }.
	case MES_MLC_RGBFMT_A8B8G8R8 :  // 32bpp { A8, B8, G8, R8 }.
		bw[0] =  8;		bw[1] =  8;		bw[2] =  8;
		bp[0] =  0; 	bp[1] =  8; 	bp[2] = 16;
		break;
	default : MES_ASSERT( CFALSE );
	}

	for( i=0 ; i<3 ; i++ )
	{
		rgb[i] = (color >> bp[i]) & ((U32)(1<<bw[i])-1);

		fill   = bw[i];
		blank  = 8-fill;
		rgb[i] <<= blank;
		while( blank > 0 )
		{
			rgb[i] |= (rgb[i] >> fill);
			blank  -= fill;
			fill   += fill;
		}
	}

	return (rgb[0]<<16) | (rgb[1]<<8) | (rgb[2]<<0);
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the image format of the RGB layer.
 *  @param[in]  layer	the layer number ( 0: RGB0, 1: RGB1 ).
 *  @param[in]  format 	the RGB format
 *	@return     None.
 *  @remark	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *                                          MES_MLC_SetPosition
 */
void 	MES_MLC_SetFormat( U32 ModuleIndex, U32 layer, MES_MLC_RGBFMT format )
{
	const U32 DIRTYFLAG_POS  = 4;
	const U32 DIRTYFLAG_MASK = 1UL<<DIRTYFLAG_POS;

	const U32 FORMAT_MASK	= 0xFFFF0000UL;
	register U32 regvalue;
    register struct MES_MLC_RegisterSet* pRegister;
	
	MES_ASSERT( 2 > layer );
	MES_ASSERT( 0 == (format & 0x0000FFFFUL) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	regvalue = pRegister->MLCRGBLAYER[layer].MLCCONTROL;

	regvalue &= ~(FORMAT_MASK | DIRTYFLAG_MASK);
	regvalue |= (U32)format;

	pRegister->MLCRGBLAYER[layer].MLCCONTROL	= regvalue;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the layer position.
 *  @param[in]  layer 	the layer number ( 0: RGB0, 1: RGB1, 2: Video ).
 *  @param[in]  sx   	the x-coordinate of the upper-left corner of the layer, -2048 ~ +2047.
 *  @param[in]  sy 	    the y-coordinate of the upper-left corner of the layer, -2048 ~ +2047.
 *  @param[in]  ex   	the x-coordinate of the lower-right corner of the layer, -2048 or 0 ~ +2047.
 *  @param[in]  ey 		the y-coordinate of the lower-right corner of the layer, -2048 or 0 ~ +2047.
 *	@return     None.
 *  @remark	    If layer is 2(video layer) then x, y-coordinate of the lower-right
 *				corner of the layer must be a positive value.\n
 *				The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see        MES_MLC_SetLayerPowerMode,  MES_MLC_GetLayerPowerMode,
 *              MES_MLC_SetLayerSleepMode,  MES_MLC_GetLayerSleepMode,
 *	            MES_MLC_SetDirtyFlag,       MES_MLC_GetDirtyFlag,
 *              MES_MLC_SetLayerEnable,     MES_MLC_GetLayerEnable,
 *              MES_MLC_SetLockSize,        MES_MLC_Set3DEnb,
 *              MES_MLC_SetAlphaBlending,   MES_MLC_SetTransparency,
 *              MES_MLC_SetColorInversion,  MES_MLC_GetExtendedColor,
 *              MES_MLC_SetFormat
 */
void 	MES_MLC_SetPosition( U32 ModuleIndex, U32 layer, S32 sx, S32 sy, S32 ex, S32 ey )
{
    register struct MES_MLC_RegisterSet*    pRegister;

	MES_ASSERT( 3 > layer );
	MES_ASSERT( -2048 <= sx && sx < 2048 );
	MES_ASSERT( -2048 <= sy && sy < 2048 );
	MES_ASSERT( -2048 <= ex && ex < 2048 );
	MES_ASSERT( -2048 <= ey && ey < 2048 );
	MES_ASSERT( (2!=layer) || ((ex>=0) && (ey>=0)) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == layer || 1 == layer )
    {
    	pRegister->MLCRGBLAYER[layer].MLCLEFTRIGHT	= (((U32)sx & 0xFFFUL)<<16) | ((U32)ex & 0xFFFUL);
	    pRegister->MLCRGBLAYER[layer].MLCTOPBOTTOM =  (((U32)sy & 0xFFFUL)<<16) | ((U32)ey & 0xFFFUL);
    }
    else if( 2 == layer )
    {
    	pRegister->MLCLEFTRIGHT2 = (((U32)sx & 0xFFFUL)<<16) | ((U32)ex & 0xFFFUL);
	    pRegister->MLCTOPBOTTOM2 = (((U32)sy & 0xFFFUL)<<16) | ((U32)ey & 0xFFFUL);
    }
}

//------------------------------------------------------------------------------
//	RGB Layer Specific Operations
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the invalid area of RGB Layer.
 *  @param[in]  layer 	the layer number ( 0: RGB0, 1: RGB1 ).
 *  @param[in]  region  select region ( 0 or 1 )
 *  @param[in]  sx   	the x-coordinate of the upper-left corner of the layer, 0 ~ +2047.
 *  @param[in]  sy 	    the y-coordinate of the upper-left corner of the layer, 0 ~ +2047.
 *  @param[in]  ex   	the x-coordinate of the lower-right corner of the layer, 0 ~ +2047.
 *  @param[in]  ey 		the y-coordinate of the lower-right corner of the layer, 0 ~ +2047.
 *  @param[in]  bEnb    \b CTRUE indicate that invalid region Enable,\n
 *                      \b CFALSE indicate that invalid region Disable.
 *	@return     None.
 *  @remark	    Each RGB Layer support two invalid region. so \e region argument must set to 0 or 1.\n
 *      	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see                                            MES_MLC_SetRGBLayerStride,
 *              MES_MLC_SetRGBLayerAddress,         MES_MLC_SetRGBLayerPalette
 */
void    MES_MLC_SetRGBLayerInvalidPosition( U32 ModuleIndex, U32 layer, U32 region, S32 sx, S32 sy, S32 ex, S32 ey, CBOOL bEnb )
{
    const U32 INVALIDENB_POS = 28;

    register struct MES_MLC_RegisterSet*    pRegister;

    MES_ASSERT( 2 > layer );
    MES_ASSERT( 2 > region );
	MES_ASSERT( 0 <= sx && sx < 2048 );
	MES_ASSERT( 0 <= sy && sy < 2048 );
	MES_ASSERT( 0 <= ex && ex < 2048 );
	MES_ASSERT( 0 <= ey && ey < 2048 );
    MES_ASSERT( (0==bEnb) || (1==bEnb) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    if( 0 == region )
    {
        pRegister->MLCRGBLAYER[layer].MLCINVALIDLEFTRIGHT0 = ((bEnb<<INVALIDENB_POS) | ((sx&0x7FF)<<16) | (ex&0x7FF) );
        pRegister->MLCRGBLAYER[layer].MLCINVALIDTOPBOTTOM0 = ( ((sy&0x7FF)<<16) | (ey&0x7FF) );
    }
    else
    {
        pRegister->MLCRGBLAYER[layer].MLCINVALIDLEFTRIGHT1 = ((bEnb<<INVALIDENB_POS) | ((sx&0x7FF)<<16) | (ex&0x7FF) );
        pRegister->MLCRGBLAYER[layer].MLCINVALIDTOPBOTTOM1 = ( ((sy&0x7FF)<<16) | (ey&0x7FF) );
    }
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set stride for horizontal and vertical.
 *  @param[in]  layer 		the layer number ( 0: RGB0, 1: RGB1 ).
 *  @param[in]  hstride 	the horizontal stride specifying the number of
 * 	            			bytes from one pixel to the next. Generally, this
 *		        			value has bytes per pixel. You have to set it only
 *	            			to a positive value.
 *  @param[in]  vstride		the vertical stride specifying the number of
 * 	            			bytes from one scan line of the image buffer to
 *		        			the next. Generally, this value has bytes per a
 *              			line. You can set it to a negative value for
 *              			vertical flip.
 *	@return     None.
 *  @remarks    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer. 
 *  @see        MES_MLC_SetRGBLayerInvalidPosition,
 *              MES_MLC_SetRGBLayerAddress,         MES_MLC_SetRGBLayerPalette
 */
void	MES_MLC_SetRGBLayerStride( U32 ModuleIndex, U32 layer, S32 hstride, S32 vstride )
{
    register struct MES_MLC_RegisterSet*    pRegister;

    MES_ASSERT( 2 > layer );
	MES_ASSERT( 0 <= hstride );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    pRegister->MLCRGBLAYER[layer].MLCHSTRIDE = hstride;
    pRegister->MLCRGBLAYER[layer].MLCVSTRIDE = vstride;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set an address of the image buffer.
 *  @param[in]  layer 	the layer number( 0: RGB0, 1: RGB1 ).
 *  @param[in]  addr	an address of the image buffer.
 *	@return     None.
 *	@remark	    Normally, the argument 'addr' specifies an address of upper-left
 *			    corner of the image. but you have to set it to an address of
 * 			    lower-left corner for vertical mirror.\n
 *			    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *  @see        MES_MLC_SetRGBLayerInvalidPosition, MES_MLC_SetRGBLayerStride,
 *                                                  MES_MLC_SetRGBLayerPalette
 */
void	MES_MLC_SetRGBLayerAddress( U32 ModuleIndex, U32 layer, U32 addr )
{
    register struct MES_MLC_RegisterSet* pRegister;
	
    MES_ASSERT( 2 > layer );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    pRegister->MLCRGBLAYER[layer].MLCADDRESS = addr;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set a palette table
 *  @param[in]  layer 	the layer number( 0: RGB0, 1: RGB1 ).
 *  @param[in]  Addr	Palette table address ( 0 ~ 255 )
 *  @param[in]  data	Color value ( RGB565 Format ).
 *	@return     None.
 *  @see        MES_MLC_SetRGBLayerInvalidPosition, MES_MLC_SetRGBLayerStride,
 *              MES_MLC_SetRGBLayerAddress,         MES_MLC_SetRGBLayerPalette
 */
void    MES_MLC_SetRGBLayerPalette( U32 ModuleIndex, U32 layer, U8 Addr, U16 data )
{
    const U32 PALETTEADDR_POS = 24;

    register struct MES_MLC_RegisterSet* pRegister;
	
    MES_ASSERT( 2 > layer );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    pRegister->MLCRGBLAYER[layer].MLCPALETTE = ((U32)Addr<<PALETTEADDR_POS) | (U32)data;
}


//--------------------------------------------------------------------------
// Video Layer Specific Operations
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
 *	@brief	    Set the vertical stride for video layer.
 *  @param[in]  LuStride	the vertical stride for Y component.
 *  @param[in]  CbStride	the vertical stride for Cb component.
 *  @param[in]  CrStride	the vertical stride for Cr component.
 *	@return     None.
 *	@remark	    The vertical stride specifies the number of bytes from one scan line
 *			    of the image buffer to the next. Generally, it has bytes per a line.
 *			    You have to set it only to a positive value.\n
 *			    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *	@see		                                MES_MLC_SetYUVLayerAddress,
 *              MES_MLC_SetYUVLayerScaleFactor, MES_MLC_SetYUVLayerScale,
 *              MES_MLC_SetYUVLayerLumaEnhance, MES_MLC_SetYUVLayerChromaEnhance
 */
void	MES_MLC_SetYUVLayerStride( U32 ModuleIndex, S32 LuStride, S32 CbStride, S32 CrStride )
{
    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( 0 < LuStride );
	MES_ASSERT( 0 < CbStride );
	MES_ASSERT( 0 < CrStride );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	pRegister->MLCVSTRIDE2  = LuStride;
	pRegister->MLCVSTRIDECB = CbStride;
	pRegister->MLCVSTRIDECR = CrStride;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set an address of the image buffer for video layer.
 *  @param[in]  LuAddr	an address of the Y component buffer which has 2D block addressing format.
 *  @param[in]  CbAddr	an address of the Cb component buffer which has 2D block addressing format.
 *  @param[in]  CrAddr	an address of the Cr component buffer which has 2D block addressing format.
 *	@return     None.
 *	@remark	    This function is valid when the format of video layer must has
 * 			    separated YUV format. Each color component has the buffer address.\n
 *				The 2D block addressing format is as follwings.
 *				- Addr[31:24] specifies the index of segment.
 *				- Addr[23:12] specifies the y-coordinate of upper-left corner of the
 *			  	  image in segment.
 *				- Addr[11: 0] specifies the x-coordinate of upper-left corner of the
 * 			      image in segment.
 *				.
 *			    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *	@see		MES_MLC_SetYUVLayerStride,
 *              MES_MLC_SetYUVLayerScaleFactor, MES_MLC_SetYUVLayerScale,
 *              MES_MLC_SetYUVLayerLumaEnhance, MES_MLC_SetYUVLayerChromaEnhance
 */
void	MES_MLC_SetYUVLayerAddress( U32 ModuleIndex, U32 LuAddr, U32 CbAddr, U32 CrAddr )
{
    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( 0 == (LuAddr & 7) );
	MES_ASSERT( 0 == (CbAddr & 7) );
	MES_ASSERT( 0 == (CrAddr & 7) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	pRegister->MLCADDRESS2  = LuAddr;
	pRegister->MLCADDRESSCB = CbAddr;
	pRegister->MLCADDRESSCR	= CrAddr;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Specifies the scale ratio for video layer with the scale factor.
 *  @param[in]  hscale   	the horizontal scale factor, 23 bit unsigned value.
 *  @param[in]  vscale  	the vertical scale factor, 23 bit unsigned value.
 *  @param[in]  bHEnb	    Set it CTRUE to apply Bi-linear filter for horizontal scale-up.
 *  @param[in]  bVEnb	    Set it CTRUE to apply Bi-linear filter for vertical scale-up.
 *	@return     None.
 *	@remark	    The scale factor can be calculated by following formula.
 *				If Bilinear Filter is used,
 * 			    - hscale = (source width-1) * (1<<11) / (destination width-1).
 *			    - vscale = (source height-1) * (1<<11) / (destinatin height-1).
 *			    .
 *				,else
 * 			    - hscale = source width * (1<<11) / destination width.
 *			    - vscale = source height * (1<<11) / destinatin height.
 *			    .
 *			    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *	@see		MES_MLC_SetYUVLayerStride,      MES_MLC_SetYUVLayerAddress,
 *                                              MES_MLC_SetYUVLayerScale,
 *              MES_MLC_SetYUVLayerLumaEnhance, MES_MLC_SetYUVLayerChromaEnhance
 */
void	MES_MLC_SetYUVLayerScaleFactor( U32 ModuleIndex, U32 hscale, U32 vscale, CBOOL bHEnb, CBOOL bVEnb )
{
    const U32 FILTERENB_POS = 28;
    const U32 SCALE_MASK = ((1<<23)-1);

    register struct MES_MLC_RegisterSet* pRegister;

    MES_ASSERT( (1<<23) > hscale );
    MES_ASSERT( (1<<23) > vscale );
    MES_ASSERT( (0==bHEnb) ||(1==bHEnb) );
    MES_ASSERT( (0==bVEnb) ||(1==bVEnb) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

    pRegister->MLCHSCALE = ((bHEnb << FILTERENB_POS) | (hscale & SCALE_MASK) );
    pRegister->MLCVSCALE = ((bVEnb << FILTERENB_POS) | (vscale & SCALE_MASK) );

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Specifies the scale ratio for video layer with the width and height.
 *  @param[in]  sw 		the width of the source image.
 *  @param[in]  sh 		the height of the source image.
 *  @param[in]  dw 		the width of the destination image.
 *  @param[in]  dh 		the height of the destination image.
 *  @param[in]  bHEnb	    Set it CTRUE to apply Bi-linear filter for horizontal scale-up.
 *  @param[in]  bVEnb	    Set it CTRUE to apply Bi-linear filter for vertical scale-up.
 *	@return     None.
 *	@remark	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *	@see		MES_MLC_SetYUVLayerStride,      MES_MLC_SetYUVLayerAddress,
 *              MES_MLC_SetYUVLayerScaleFactor,
 *              MES_MLC_SetYUVLayerLumaEnhance, MES_MLC_SetYUVLayerChromaEnhance
 */
void	MES_MLC_SetYUVLayerScale( U32 ModuleIndex, U32 sw, U32 sh, U32 dw, U32 dh, CBOOL bHEnb, CBOOL bVEnb )
{
    const U32 FILTERENB_POS = 28;
    const U32 SCALE_MASK = ((1<<23)-1);

  	register U32 hscale, vscale;
    register struct MES_MLC_RegisterSet* pRegister;

	MES_ASSERT( 4096 > sw );
	MES_ASSERT( 4096 > sh );
	MES_ASSERT( 4096 > dw );
	MES_ASSERT( 4096 > dh );
    MES_ASSERT( (0==bHEnb) ||(1==bHEnb) );
    MES_ASSERT( (0==bVEnb) ||(1==bVEnb) );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	if( bHEnb )
	{
		if( dw > sw )	{ sw--; dw--; }
	}

	if( bVEnb )
	{
		if( dh > sh )	{ sh--; dh--; }
    }

	hscale = (sw << 11) / dw;
	vscale = (sh << 11) / dh;

    pRegister->MLCHSCALE = ((bHEnb << FILTERENB_POS) | (hscale & SCALE_MASK) );
    pRegister->MLCVSCALE = ((bVEnb << FILTERENB_POS) | (vscale & SCALE_MASK) );

}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set brightness and contrast for video layer.
 *  @param[in]  contrast 	the contrast value, 0 ~ 7.\n
 *		        			0 : 1.0, 1 : 1.125, 2 : 1.25, 3 : 1.375,
 *		        			4 : 1.5, 5 : 1.625, 6 : 1.75, 7 : 1.875
 *  @param[in]  brightness 	the brightness value, -128 ~ +127.
 *	@return     None.
 *	@remark	    The result of this function will be applied to corresponding layer
 * 			    after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *	@see		MES_MLC_SetYUVLayerStride,      MES_MLC_SetYUVLayerAddress,
 *              MES_MLC_SetYUVLayerScaleFactor, MES_MLC_SetYUVLayerScale,
 *                                              MES_MLC_SetYUVLayerChromaEnhance
 */
void	MES_MLC_SetYUVLayerLumaEnhance( U32 ModuleIndex, U32 contrast, S32 brightness )
{
    register struct MES_MLC_RegisterSet* pRegister;
	
	MES_ASSERT( 8 > contrast );
	MES_ASSERT( -128 <= brightness && brightness < 128 );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	pRegister->MLCLUENH = (((U32)brightness & 0xFFUL)<<8) | contrast;
}

//------------------------------------------------------------------------------
/**
 *	@brief	    Set factors to control hue and satuation for video layer.
 *  @param[in]  quadrant	a quadrant to apply hue and saturation, 0 ~ 4.\n
 *  		    			Set it to 0 to apply hue and saturation on all quadrant.
 *  @param[in]  CbA 		cosine value for B-Y axis, -128 ~ +127.
 *  @param[in]  CbB 		sine value for R-Y axis, -128 ~ +127.
 *  @param[in]  CrA 		sine value for B-Y axis, -128 ~ +127.
 *  @param[in]  CrB			cosine value for R-Y axis, -128 ~ +127.
 *	@return     None.
 *	@remark	    Each quadrant has factors to control hue and satuation.
 *		        At each coordinate, HUE and saturation is applied by following formula.\n
 *		            (B-Y)' = (B-Y)*CbA + (R-Y)*CbB \n
 *		            (R-Y)' = (B-Y)*CrA + (R-Y)*CrB \n
 *		        , where \n
 *		            CbA = cos(еш) * 64 * gain, CbB = -sin(еш) * 64 * gain \n
 *		            CrA = sin(еш) * 64 * gain, CrB =  cos(еш) * 64 * gain \n
 *		            gain is 0 to 2. \n
 *		        The example for this equation is as follows.
 *	@code
 *		// HUE : phase relationship of the chrominance components, -180 ~ 180 degree
 *		// Sat : color intensity, 0 ~ 127.
 *		void SetChromaEnhance( int Hue, int Sat )
 *		{
 *			S32 sv, cv;
 *
 *			if( Sat < 0 )			Sat = 0;
 *			else if( Sat > 127 )	Sat = 127;
 *
 *			sv = (S32)(sin( (3.141592654f * Hue) / 180 ) * Sat);
 *			cv = (S32)(cos( (3.141592654f * Hue) / 180 ) * Sat);
 *
 *			MES_MLC_SetYUVLayerChromaEnhance( 0, cv, -sv, sv, cv );
 *		};
 *	@endcode
 *			The result of this function will be applied to corresponding layer
 * 			after calling function MES_MLC_SetDirtyFlag() with corresponding layer.
 *	@see		MES_MLC_SetYUVLayerStride,      MES_MLC_SetYUVLayerAddress,
 *              MES_MLC_SetYUVLayerScaleFactor, MES_MLC_SetYUVLayerScale,
 *              MES_MLC_SetYUVLayerLumaEnhance, MES_MLC_SetYUVLayerChromaEnhance
 */
void	MES_MLC_SetYUVLayerChromaEnhance( U32 ModuleIndex, U32 quadrant, S32 CbA, S32 CbB, S32 CrA, S32 CrB )
{
    register struct MES_MLC_RegisterSet* pRegister;
	register U32 temp;

	MES_ASSERT( 5 > quadrant );
	MES_ASSERT( -128 <= CbA && CbA < 128 );
	MES_ASSERT( -128 <= CbB && CbB < 128 );
	MES_ASSERT( -128 <= CrA && CrA < 128 );
	MES_ASSERT( -128 <= CrB && CrB < 128 );
	MES_ASSERT( CNULL != __g_ModuleVariables[ModuleIndex].pRegister );

	pRegister = __g_ModuleVariables[ModuleIndex].pRegister;

	temp  = (((U32)CrB & 0xFFUL)<<24)
		  | (((U32)CrA & 0xFFUL)<<16)
		  | (((U32)CbB & 0xFFUL)<< 8)
		  | (((U32)CbA & 0xFFUL)<< 0);

	if( 0 < quadrant )
	{
		pRegister->MLCCHENH[quadrant-1] = temp;
	}
	else
	{
		pRegister->MLCCHENH[0] = temp;
		pRegister->MLCCHENH[1] = temp;
		pRegister->MLCCHENH[2] = temp;
		pRegister->MLCCHENH[3] = temp;
	}
}


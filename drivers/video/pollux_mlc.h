//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module     : MLC
//	File       : mes_mlc.h
//	Description: 
//	Author     : Firmware Team
//	History    : 
//------------------------------------------------------------------------------
#ifndef __MES_MLC_H__
#define __MES_MLC_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif


#define NUMBER_OF_MLC_MODULE	1
#define OFFSET_OF_MLC_MODULE	0
//------------------------------------------------------------------------------
/// @defgroup   MLC MLC
//------------------------------------------------------------------------------
//@{

    /// @brief	a priority of layers.
    typedef enum 
    {
    	MES_MLC_PRIORITY_VIDEOFIRST		= 0UL,	///< video layer > layer0 > layer1 
    	MES_MLC_PRIORITY_VIDEOSECOND	= 1UL,	///< layer0 > video layer > layer1 
    	MES_MLC_PRIORITY_VIDEOTHIRD		= 2UL	///< layer0 > layer1 > video layer 
    
    } MES_MLC_PRIORITY ;


    /// @brief indicate block for sync of 3D layer's address change
    typedef enum
    {
      MES_MLC_3DSYNC_PRIMARY                  = 0UL,
      MES_MLC_3DSYNC_SECONDARY                = 1UL,
      MES_MLC_3DSYNC_PRIMARY_OR_SECONDARY     = 2UL,
      MES_MLC_3DSYNC_PRIMARY_AND_SECONDARY    = 3UL
        
    } MES_MLC_3DSYNC;
        
    //--------------------------------------------------------------------------
    // To remove following waring on RVDS compiler
    // 		Warning : #66-D: enumeration value is out of "int" range
    //--------------------------------------------------------------------------
    #ifdef __arm	
    #pragma diag_remark 66		// disable #66 warining
    #endif

    /// @brief	RGB layer pixel format.
    typedef enum 
    {
        MES_MLC_RGBFMT_R5G6B5    = 0x44320000UL,   ///< 16bpp { R5, G6, B5 }.
        MES_MLC_RGBFMT_B5G6R5    = 0xC4320000UL,   ///< 16bpp { B5, G6, R5 }.
    
        MES_MLC_RGBFMT_X1R5G5B5  = 0x43420000UL,   ///< 16bpp { X1, R5, G5, B5 }.
        MES_MLC_RGBFMT_X1B5G5R5  = 0xC3420000UL,   ///< 16bpp { X1, B5, G5, R5 }.
        MES_MLC_RGBFMT_X4R4G4B4  = 0x42110000UL,   ///< 16bpp { X4, R4, G4, B4 }.
        MES_MLC_RGBFMT_X4B4G4R4  = 0xC2110000UL,   ///< 16bpp { X4, B4, G4, R4 }.
        MES_MLC_RGBFMT_X8R3G3B2  = 0x41200000UL,   ///< 16bpp { X8, R3, G3, B2 }.
        MES_MLC_RGBFMT_X8B3G3R2  = 0xC1200000UL,   ///< 16bpp { X8, B3, G3, R2 }.
    
        MES_MLC_RGBFMT_A1R5G5B5  = 0x33420000UL,   ///< 16bpp { A1, R5, G5, B5 }.
        MES_MLC_RGBFMT_A1B5G5R5  = 0xB3420000UL,   ///< 16bpp { A1, B5, G5, R5 }.
        MES_MLC_RGBFMT_A4R4G4B4  = 0x22110000UL,   ///< 16bpp { A4, R4, G4, B4 }.
        MES_MLC_RGBFMT_A4B4G4R4  = 0xA2110000UL,   ///< 16bpp { A4, B4, G4, R4 }.
        MES_MLC_RGBFMT_A8R3G3B2  = 0x11200000UL,   ///< 16bpp { A8, R3, G3, B2 }.
        MES_MLC_RGBFMT_A8B3G3R2  = 0x91200000UL,   ///< 16bpp { A8, B3, G3, R2 }.
    
        MES_MLC_RGBFMT_R8G8B8    = 0x46530000UL,   ///< 24bpp { R8, G8, B8 }.
        MES_MLC_RGBFMT_B8G8R8    = 0xC6530000UL,   ///< 24bpp { B8, G8, R8 }.
    
        MES_MLC_RGBFMT_X8R8G8B8  = 0x46530000UL,   ///< 32bpp { X8, R8, G8, B8 }.
        MES_MLC_RGBFMT_X8B8G8R8  = 0xC6530000UL,   ///< 32bpp { X8, B8, G8, R8 }.
        MES_MLC_RGBFMT_A8R8G8B8  = 0x06530000UL,   ///< 32bpp { A8, R8, G8, B8 }.
        MES_MLC_RGBFMT_A8B8G8R8  = 0x86530000UL,   ///< 32bpp { A8, B8, G8, R8 }.
        MES_MLC_RGBFMT_PTR5G6B5  = 0x443A0000UL    ///< Palette Mode                
    
    } MES_MLC_RGBFMT ;

    #ifdef __arm	// for RVDS
    #pragma diag_default 66		// return to default setting for #66 warning
    #endif
    //--------------------------------------------------------------------------        
         
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_MLC_Initialize( void );
U32     MES_MLC_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
U32     MES_MLC_GetPhysicalAddress(  U32 ModuleIndex  );
U32     MES_MLC_GetSizeOfRegisterSet(  U32 ModuleIndex  );
void    MES_MLC_SetBaseAddress(  U32 ModuleIndex, U32 BaseAddress );
U32     MES_MLC_GetBaseAddress(  U32 ModuleIndex  );
CBOOL   MES_MLC_OpenModule(  U32 ModuleIndex  );
CBOOL   MES_MLC_CloseModule(  U32 ModuleIndex  );
CBOOL   MES_MLC_CheckBusy(  U32 ModuleIndex  );
CBOOL   MES_MLC_CanPowerDown(  U32 ModuleIndex  );
//@} 

//------------------------------------------------------------------------------
///  @name   Clock Control Interface
//@{
void            MES_MLC_SetClockPClkMode( U32 ModuleIndex, MES_PCLKMODE mode );
MES_PCLKMODE    MES_MLC_GetClockPClkMode( U32 ModuleIndex );
void            MES_MLC_SetClockBClkMode( U32 ModuleIndex, MES_BCLKMODE mode );
MES_BCLKMODE    MES_MLC_GetClockBClkMode( U32 ModuleIndex );
//@}

//--------------------------------------------------------------------------
///	@name	MLC Main Settings
//--------------------------------------------------------------------------
//@{
void                MES_MLC_SetTop3DAddrChangeSync( U32 ModuleIndex, MES_MLC_3DSYNC sync );
MES_MLC_3DSYNC      MES_MLC_GetTop3DAddrChangeSync( U32 ModuleIndex );

void    MES_MLC_SetTopPowerMode( U32 ModuleIndex, CBOOL bPower );
CBOOL   MES_MLC_GetTopPowerMode( U32 ModuleIndex );
void    MES_MLC_SetTopSleepMode( U32 ModuleIndex, CBOOL bSleep );
CBOOL   MES_MLC_GetTopSleepMode( U32 ModuleIndex );
void	MES_MLC_SetTopDirtyFlag( U32 ModuleIndex );
CBOOL	MES_MLC_GetTopDirtyFlag( U32 ModuleIndex );
void	MES_MLC_SetMLCEnable( U32 ModuleIndex, CBOOL bEnb );
CBOOL	MES_MLC_GetMLCEnable( U32 ModuleIndex );
void	MES_MLC_SetFieldEnable( U32 ModuleIndex, CBOOL bEnb );
CBOOL	MES_MLC_GetFieldEnable( U32 ModuleIndex );
void	MES_MLC_SetLayerPriority( U32 ModuleIndex, MES_MLC_PRIORITY priority );
void	MES_MLC_SetScreenSize( U32 ModuleIndex, U32 width, U32 height );	
void    MES_MLC_GetScreenSize( U32 ModuleIndex, U32 *pWidth, U32 *pHeight );
void	MES_MLC_SetBackground( U32 ModuleIndex, U32 color );
//@}

//--------------------------------------------------------------------------
///	@name	Per Layer Operations
//--------------------------------------------------------------------------
//@{
void    MES_MLC_SetLayerPowerMode( U32 ModuleIndex, U32 layer, CBOOL bPower ); 
CBOOL   MES_MLC_GetLayerPowerMode( U32 ModuleIndex, U32 layer );         
void    MES_MLC_SetLayerSleepMode( U32 ModuleIndex, U32 layer, CBOOL bSleep );
CBOOL   MES_MLC_GetLayerSleepMode( U32 ModuleIndex, U32 layer );
void 	MES_MLC_SetDirtyFlag( U32 ModuleIndex, U32 layer );
CBOOL 	MES_MLC_GetDirtyFlag( U32 ModuleIndex, U32 layer );
void	MES_MLC_SetLayerEnable( U32 ModuleIndex, U32 layer, CBOOL bEnb );
CBOOL	MES_MLC_GetLayerEnable( U32 ModuleIndex, U32 layer );
void 	MES_MLC_SetLockSize( U32 ModuleIndex, U32 layer, U32 locksize );                 
void 	MES_MLC_Set3DEnb( U32 ModuleIndex, U32 layer, CBOOL bEnb );                      
void	MES_MLC_SetAlphaBlending( U32 ModuleIndex, U32 layer, CBOOL bEnb, U32 alpha );   
void	MES_MLC_SetTransparency( U32 ModuleIndex, U32 layer, CBOOL bEnb, U32 color );    
void	MES_MLC_SetColorInversion( U32 ModuleIndex, U32 layer, CBOOL bEnb, U32 color );  
U32		MES_MLC_GetExtendedColor( U32 color, MES_MLC_RGBFMT format );           
void 	MES_MLC_SetFormat( U32 ModuleIndex, U32 layer, MES_MLC_RGBFMT format );
void 	MES_MLC_SetPosition( U32 ModuleIndex, U32 layer, S32 sx, S32 sy, S32 ex, S32 ey );
//@}

//--------------------------------------------------------------------------
/// @name	RGB Layer Specific Operations
//--------------------------------------------------------------------------
//@{
void    MES_MLC_SetRGBLayerInvalidPosition( U32 ModuleIndex, U32 layer, U32 region, S32 sx, S32 sy, S32 ex, S32 ey, CBOOL bEnb );   
void	MES_MLC_SetRGBLayerStride( U32 ModuleIndex, U32 layer, S32 hstride, S32 vstride );
void	MES_MLC_SetRGBLayerAddress( U32 ModuleIndex, U32 layer, U32 addr );
void    MES_MLC_SetRGBLayerPalette( U32 ModuleIndex, U32 layer, U8 Addr, U16 data );       
//@}

//--------------------------------------------------------------------------
/// @name	Video Layer Specific Operations
//--------------------------------------------------------------------------
//@{
void	MES_MLC_SetYUVLayerStride( U32 ModuleIndex, S32 LuStride, S32 CbStride, S32 CrStride );  
void	MES_MLC_SetYUVLayerAddress( U32 ModuleIndex, U32 LuAddr, U32 CbAddr, U32 CrAddr );     
void	MES_MLC_SetYUVLayerScaleFactor( U32 ModuleIndex, U32 hscale, U32 vscale, CBOOL bHEnb, CBOOL bVEnb );   
void	MES_MLC_SetYUVLayerScale( U32 ModuleIndex, U32 sw, U32 sh, U32 dw, U32 dh, CBOOL bHEnb, CBOOL bVEnb );
void	MES_MLC_SetYUVLayerLumaEnhance( U32 ModuleIndex, U32 contrast, S32 brightness );
void	MES_MLC_SetYUVLayerChromaEnhance( U32 ModuleIndex, U32 quadrant, S32 CbA, S32 CbB, S32 CrA, S32 CrB );
//@}

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_MLC_H__

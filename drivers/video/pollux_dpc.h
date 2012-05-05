//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module     : DPC
//	File       : mes_dpc.h
//	Description: 
//	Author     : 
//	History    : 
//------------------------------------------------------------------------------
#ifndef __MES_DPC_H__
#define __MES_DPC_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif

#define NUMBER_OF_DPC_MODULE	2
#define OFFSET_OF_DPC_MODULE	0x400
#define INTNUM_OF_DPC0_MODULE   0       /* PRI */
#define INTNUM_OF_DPC1_MODULE   1       /* SECOND */
//------------------------------------------------------------------------------
/// @defgroup   DPC DPC
//------------------------------------------------------------------------------
//@{
 
         /// @brief	the data output format.
        typedef	enum 
        {
        	MES_DPC_FORMAT_RGB555     	=  0UL,	///< RGB555 Format
        	MES_DPC_FORMAT_RGB565     	=  1UL,	///< RGB565 Format
        	MES_DPC_FORMAT_RGB666     	=  2UL,	///< RGB666 Format
        	MES_DPC_FORMAT_RGB888     	=  3UL,	///< RGB888 Format
        	MES_DPC_FORMAT_MRGB555A   	=  4UL,	///< MRGB555A Format
        	MES_DPC_FORMAT_MRGB555B   	=  5UL,	///< MRGB555B Format
        	MES_DPC_FORMAT_MRGB565    	=  6UL,	///< MRGB565 Format
        	MES_DPC_FORMAT_MRGB666    	=  7UL,	///< MRGB666 Format
        	MES_DPC_FORMAT_MRGB888A   	=  8UL,	///< MRGB888A Format
        	MES_DPC_FORMAT_MRGB888B   	=  9UL,	///< MRGB888B Format
        	MES_DPC_FORMAT_CCIR656    	= 10UL,	///< ITU-R BT.656 / 601(8-bit)
        	MES_DPC_FORMAT_CCIR601A		= 12UL,	///< ITU-R BT.601A
        	MES_DPC_FORMAT_CCIR601B		= 13UL,	///< ITU-R BT.601B
            MES_DPC_FORMAT_4096COLOR    = 1UL,  ///< 4096 Color Format
            MES_DPC_FORMAT_16GRAY       = 3UL   ///< 16 Level Gray Format
        
        } MES_DPC_FORMAT ;
        
        
        /// @brief	the data output order in case of ITU-R BT.656 / 601.
        typedef enum 
        {
        	MES_DPC_YCORDER_CbYCrY		= 0UL,	///< Cb, Y, Cr, Y
        	MES_DPC_YCORDER_CrYCbY		= 1UL,	///< Cr, Y, Cb, Y
        	MES_DPC_YCORDER_YCbYCr		= 2UL,	///< Y, Cb, Y, Cr
        	MES_DPC_YCORDER_YCrYCb		= 3UL	///< Y, Cr, Y, Cb
        
        } MES_DPC_YCORDER ;
        
        ///	@brief the PAD output clock.
        typedef enum 
        {
        	MES_DPC_PADCLK_VCLK		= 0UL,	///< VCLK
        	MES_DPC_PADCLK_VCLK2	= 1UL	///< VCLK2
        
        } MES_DPC_PADCLK ;
 
         /// @brief	RGB dithering mode.
        typedef	enum 
        {
        	MES_DPC_DITHER_BYPASS	= 0UL,	///< Bypass mode.
        	MES_DPC_DITHER_4BIT		= 1UL,	///< 8 bit -> 4 bit mode.       
        	MES_DPC_DITHER_5BIT		= 2UL,	///< 8 bit -> 5 bit mode.
        	MES_DPC_DITHER_6BIT		= 3UL	///< 8 bit -> 6 bit mode.
        
        } MES_DPC_DITHER ;

        ///	@brief	Video Broadcast Standards.
        typedef enum 
        {
        	MES_DPC_VBS_NTSC_M		= 0UL,	///< NTSC-M             59.94 Hz(525)
        	MES_DPC_VBS_NTSC_N		= 1UL,	///< NTSC-N             50 Hz(625)
        	MES_DPC_VBS_NTSC_443	= 2UL,	///< NTSC-4.43          60 Hz(525)
        	MES_DPC_VBS_PAL_M		= 3UL,	///< PAL-M              59.952 Hz(525)
        	MES_DPC_VBS_PAL_N		= 4UL,	///< PAL-combination N  50 Hz(625)
        	MES_DPC_VBS_PAL_BGHI	= 5UL,	///< PAL-B,D,G,H,I,N    50 Hz(625)
        	MES_DPC_VBS_PSEUDO_PAL	= 6UL,	///< Pseudo PAL         60 Hz(525)
        	MES_DPC_VBS_PSEUDO_NTSC = 7UL	///< Pseudo NTSC        50 Hz (625)
        
        } MES_DPC_VBS ; 
 
        ///	@brief Luma/Chroma bandwidth control
        typedef enum 
        {
        	MES_DPC_BANDWIDTH_LOW		= 0UL,	///< Low bandwidth
        	MES_DPC_BANDWIDTH_MEDIUM	= 1UL,	///< Medium bandwidth
        	MES_DPC_BANDWIDTH_HIGH		= 2UL	///< High bandwidth
        
        } MES_DPC_BANDWIDTH ; 
  
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_DPC_Initialize( void );
U32     MES_DPC_SelectModule( U32 ModuleIndex );
U32     MES_DPC_GetCurrentModule( void );
U32     MES_DPC_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
U32     MES_DPC_GetPhysicalAddress( void );
U32     MES_DPC_GetSizeOfRegisterSet( void );
void    MES_DPC_SetBaseAddress( U32 BaseAddress );
U32     MES_DPC_GetBaseAddress( void );
CBOOL   MES_DPC_OpenModule( void );
CBOOL   MES_DPC_CloseModule( void );
CBOOL   MES_DPC_CheckBusy( void );
CBOOL   MES_DPC_CanPowerDown( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Interrupt Interface
//@{
S32     MES_DPC_GetInterruptNumber( void );
void    MES_DPC_SetInterruptEnable( S32 IntNum, CBOOL Enable );
CBOOL   MES_DPC_GetInterruptEnable( S32 IntNum );
CBOOL   MES_DPC_GetInterruptPending( S32 IntNum );
void    MES_DPC_ClearInterruptPending( S32 IntNum );
void    MES_DPC_SetInterruptEnableAll( CBOOL Enable );
CBOOL   MES_DPC_GetInterruptEnableAll( void );
CBOOL   MES_DPC_GetInterruptPendingAll( void );
void    MES_DPC_ClearInterruptPendingAll( void );
S32     MES_DPC_GetInterruptPendingNumber( void );  // -1 if None
//@} 

//------------------------------------------------------------------------------
///  @name   Clock Control Interface
//@{

void            MES_DPC_SetClockPClkMode( MES_PCLKMODE mode );
MES_PCLKMODE    MES_DPC_GetClockPClkMode( void );
void            MES_DPC_SetClockSource( U32 Index, U32 ClkSrc );
U32             MES_DPC_GetClockSource( U32 Index );
void            MES_DPC_SetClockDivisor( U32 Index, U32 Divisor );
U32             MES_DPC_GetClockDivisor( U32 Index );
void            MES_DPC_SetClockOutInv( U32 Index, CBOOL OutClkInv );
CBOOL           MES_DPC_GetClockOutInv( U32 Index );
void            MES_DPC_SetClockOutEnb( U32 Index, CBOOL OutClkEnb );
CBOOL           MES_DPC_GetClockOutEnb( U32 Index );
void            MES_DPC_SetClockOutDelay( U32 Index, U32 delay );
U32             MES_DPC_GetClockOutDelay( U32 Index );
void            MES_DPC_SetClockDivisorEnable( CBOOL Enable );
CBOOL           MES_DPC_GetClockDivisorEnable( void );

//@}

//--------------------------------------------------------------------------
///	@name	Display Controller(TFT/STN) Operations
//--------------------------------------------------------------------------
//@{
void	MES_DPC_SetDPCEnable( CBOOL bEnb );
CBOOL	MES_DPC_GetDPCEnable( void );

void	MES_DPC_SetDelay( U32 DelayRGB_PVD, U32 DelayHS_CP1, U32 DelayVS_FRAM, U32 DelayDE_CP2 );
void	MES_DPC_GetDelay( U32 *pDelayRGB_PVD, U32 *pDelayHS_CP1, U32 *pDelayVS_FRAM, U32 *pDelayDE_CP2 );

void	MES_DPC_SetDither( MES_DPC_DITHER DitherR, MES_DPC_DITHER DitherG, MES_DPC_DITHER DitherB );
void	MES_DPC_GetDither( MES_DPC_DITHER *pDitherR, MES_DPC_DITHER *pDitherG, MES_DPC_DITHER *pDitherB );

void    MES_DPC_SetHorizontalUpScaler( CBOOL bEnb, U32 sourceWidth, U32 destWidth );
void    MES_DPC_GetHorizontalUpScaler( CBOOL* pbEnb, U32* psourceWidth, U32* pdestWidth );
//@}

//--------------------------------------------------------------------------
/// @name TFT LCD Specific Control Function
//--------------------------------------------------------------------------
//@{
void	MES_DPC_SetMode( MES_DPC_FORMAT format, 
    					 CBOOL bInterlace, CBOOL bInvertField, 
    					 CBOOL bRGBMode, CBOOL bSwapRB, 
	        			 MES_DPC_YCORDER ycorder, CBOOL bClipYC, CBOOL bEmbeddedSync, 
			        	 MES_DPC_PADCLK clock, CBOOL bInvertClock );

void	MES_DPC_GetMode( MES_DPC_FORMAT *pFormat, 
		        		 CBOOL *pbInterlace, CBOOL *pbInvertField, 
				         CBOOL *pbRGBMode, CBOOL *pbSwapRB, 
    					 MES_DPC_YCORDER *pYCorder, CBOOL *pbClipYC, CBOOL *pbEmbeddedSync, 
	        			 MES_DPC_PADCLK *pClock, CBOOL *pbInvertClock );

void	MES_DPC_SetHSync( U32 AVWidth, U32 HSW, U32 HFP, U32 HBP, CBOOL bInvHSYNC );
void	MES_DPC_GetHSync( U32 *pAVWidth, U32 *pHSW, U32 *pHFP, U32 *pHBP, CBOOL *pbInvHSYNC );

void	MES_DPC_SetVSync( U32 AVHeight, U32 VSW, U32 VFP, U32 VBP, CBOOL bInvVSYNC,
    					  U32 EAVHeight, U32 EVSW, U32 EVFP, U32 EVBP );

void	MES_DPC_GetVSync( U32 *pAVHeight, U32 *pVSW, U32 *pVFP, U32 *pVBP, CBOOL *pbInvVSYNC,
	        			  U32 *pEAVHeight, U32 *pEVSW, U32 *pEVFP, U32 *pEVBP );

void	MES_DPC_SetVSyncOffset( U32 VSSOffset, U32 VSEOffset, U32 EVSSOffset, U32 EVSEOffset );
void	MES_DPC_GetVSyncOffset( U32 *pVSSOffset, U32 *pVSEOffset, U32 *pEVSSOffset, U32 *pEVSEOffset );
//@}

//--------------------------------------------------------------------------
/// @name STN LCD Specific Control Function
//--------------------------------------------------------------------------
//@{
void	MES_DPC_SetSTNMode( MES_DPC_FORMAT Format, CBOOL bRGBMode, U32 BusWidth, CBOOL bInvertVM, CBOOL bInvertClock );
void	MES_DPC_GetSTNMode( MES_DPC_FORMAT *pFormat, CBOOL *pbRGBMode, U32 *pBusWidth, CBOOL* pbInvertVM, CBOOL *pbInvertClock );

void	MES_DPC_SetSTNHSync( MES_DPC_FORMAT Format, U32 BusWidth, U32 AVWidth, U32 CP1HW, U32 CP1ToCP2DLY, U32 CP2CYC );
void	MES_DPC_GetSTNHSync( U32* pAVWidth, U32* pCP1HW, U32* pCP1ToCP2DLY, U32* pCP2CYC );

void    MES_DPC_SetSTNVSync( U32 ActiveLine, U32 BlankLine );
void    MES_DPC_GetSTNVSync( U32* pActiveLine, U32* pBlankLine );

void    MES_DPC_SetSTNRandomGenerator( U32 initShiftValue, U32 formula, U8 subData );
void    MES_DPC_GetSTNRandomGenerator( U32* pinitShiftValue, U32* pformula, U8* psubData );

void    MES_DPC_SetSTNRandomGeneratorEnable( CBOOL bEnb );
CBOOL   MES_DPC_GetSTNRandomGeneratorEnable( void );

void    MES_DPC_SetSTNDitherTable( U8 addr, U16 red, U16 green, U16 blue );
//@}

//--------------------------------------------------------------------------
///	@name	Internal Video encoder operations
//--------------------------------------------------------------------------
//@{
void    MES_DPC_SetSecondaryDPCSync( CBOOL bEnb );
CBOOL   MES_DPC_GetSecondaryDPCSync( void );

void	MES_DPC_SetENCEnable( CBOOL bEnb );
CBOOL	MES_DPC_GetENCEnable( void );
void	MES_DPC_SetVideoEncoderPowerDown( CBOOL bEnb );
CBOOL	MES_DPC_GetVideoEncoderPowerDown( void );

void	MES_DPC_SetVideoEncoderMode( MES_DPC_VBS vbs, CBOOL bPedestal );
void	MES_DPC_SetVideoEncoderSCHLockControl( CBOOL bFreeRun );
CBOOL	MES_DPC_GetVideoEncoderSCHLockControl( void );

void	MES_DPC_SetVideoEncoderBandwidth( MES_DPC_BANDWIDTH Luma, MES_DPC_BANDWIDTH Chroma );
void	MES_DPC_GetVideoEncoderBandwidth( MES_DPC_BANDWIDTH *pLuma, MES_DPC_BANDWIDTH *pChroma );
void	MES_DPC_SetVideoEncoderColorControl( S8 SCH, S8 HUE, S8 SAT, S8 CRT, S8 BRT );              
void	MES_DPC_GetVideoEncoderColorControl( S8 *pSCH, S8 *pHUE, S8 *pSAT, S8 *pCRT, S8 *pBRT );    
void	MES_DPC_SetVideoEncoderFSCAdjust( S16 adjust );
U16		MES_DPC_GetVideoEncoderFSCAdjust( void );
void	MES_DPC_SetVideoEncoderTiming( U32 HSOS, U32 HSOE, U32 VSOS, U32 VSOE );
void	MES_DPC_GetVideoEncoderTiming( U32 *pHSOS, U32 *pHSOE, U32 *pVSOS, U32 *pVSOE );
//@}

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_DPC_H__

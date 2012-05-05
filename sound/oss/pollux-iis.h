//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module     : AUDIO
//	File       : mes_audio.h
//	Description: 
//	Author     : 
//	History    : 
//------------------------------------------------------------------------------
#ifndef __MES_AUDIO_H__
#define __MES_AUDIO_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif

// ghcstop: mes_chip_lf100.h, 의미없는 define....
#define NUMBER_OF_AUDIO_MODULE 1
#define OFFSET_OF_AUDIO_MODULE 0 

//------------------------------------------------------------------------------
/// @defgroup   AUDIO AUDIO
//------------------------------------------------------------------------------
//@{
 
     /// @brief  Audio Operation Mode
    typedef enum
    {
        MES_AUDIO_IF_I2S    =   0,  ///< I2S Mode
        MES_AUDIO_IF_LEFT   =   2,  ///< Left-Justified Mode.
        MES_AUDIO_IF_RIGHT  =   3   ///< Right-Justified Mode.
    } MES_AUDIO_IF ;
 
     /// @brief I2S Controller's State
    typedef enum 
    {
        MES_AUDIO_I2SSTATE_IDLE       =   0x01,       ///< All registers excepts I2S_CTRL are reset value. 
        MES_AUDIO_I2SSTATE_READY      =   0x02,       ///< I2S link is inactive, ready to communicate with codec.
        MES_AUDIO_I2SSTATE_RUN        =   0x04        ///< I2S link is active.
    
    }MES_AUDIO_I2SSTATE;
    
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_AUDIO_Initialize( void );
U32     MES_AUDIO_SelectModule( U32 ModuleIndex );
U32     MES_AUDIO_GetCurrentModule( void );
U32     MES_AUDIO_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
U32     MES_AUDIO_GetPhysicalAddress( void );
U32     MES_AUDIO_GetSizeOfRegisterSet( void );
void    MES_AUDIO_SetBaseAddress( U32 BaseAddress );
U32     MES_AUDIO_GetBaseAddress( void );
CBOOL   MES_AUDIO_OpenModule( void );
CBOOL   MES_AUDIO_CloseModule( void );
CBOOL   MES_AUDIO_CheckBusy( void );
CBOOL   MES_AUDIO_CanPowerDown( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Interrupt Interface
//@{
S32     MES_AUDIO_GetInterruptNumber( void );
void    MES_AUDIO_SetInterruptEnable( S32 IntNum, CBOOL Enable );
CBOOL   MES_AUDIO_GetInterruptEnable( S32 IntNum );
CBOOL   MES_AUDIO_GetInterruptPending( S32 IntNum );
void    MES_AUDIO_ClearInterruptPending( S32 IntNum );
void    MES_AUDIO_SetInterruptEnableAll( CBOOL Enable );
CBOOL   MES_AUDIO_GetInterruptEnableAll( void );
CBOOL   MES_AUDIO_GetInterruptPendingAll( void );
void    MES_AUDIO_ClearInterruptPendingAll( void );
S32     MES_AUDIO_GetInterruptPendingNumber( void );  // -1 if None
//@} 

//------------------------------------------------------------------------------
///  @name   DMA Interface
//@{
U32     MES_AUDIO_GetDMAIndex_PCMIn( void );
U32     MES_AUDIO_GetDMAIndex_PCMOut( void );
U32     MES_AUDIO_GetDMABusWidth( void );
//@}

//------------------------------------------------------------------------------
///  @name   Clock Control Interface
//@{
void            MES_AUDIO_SetClockPClkMode( MES_PCLKMODE mode );
MES_PCLKMODE    MES_AUDIO_GetClockPClkMode( void );
void            MES_AUDIO_SetClockSource( U32 Index, U32 ClkSrc );
U32             MES_AUDIO_GetClockSource( U32 Index );
void            MES_AUDIO_SetClockDivisor( U32 Index, U32 Divisor );
U32             MES_AUDIO_GetClockDivisor( U32 Index );
void            MES_AUDIO_SetClockOutInv( U32 Index, CBOOL OutClkInv );
CBOOL           MES_AUDIO_GetClockOutInv( U32 Index );
void            MES_AUDIO_SetClockOutEnb( U32 Index, CBOOL OutClkEnb );
CBOOL           MES_AUDIO_GetClockOutEnb( U32 Index );
void            MES_AUDIO_SetClockDivisorEnable( CBOOL Enable );
CBOOL           MES_AUDIO_GetClockDivisorEnable( void );
//@}

//--------------------------------------------------------------------------
/// @name Audio Configuration Function
//--------------------------------------------------------------------------
//@{
void            MES_AUDIO_SetI2SMasterMode( CBOOL Enable );
CBOOL           MES_AUDIO_GetI2SMasterMode( void );
void            MES_AUDIO_SetInterfaceMode( MES_AUDIO_IF mode );
MES_AUDIO_IF    MES_AUDIO_GetInterfaceMode( void );
void            MES_AUDIO_SetSyncPeriod( U32 period );   
U32             MES_AUDIO_GetSyncPeriod( void );
//@}

//--------------------------------------------------------------------------
/// @name Audio Control Function
//--------------------------------------------------------------------------
//@{
void  MES_AUDIO_SetI2SLinkOn( void );   
CBOOL MES_AUDIO_GetI2SLinkOn( void );
void  MES_AUDIO_SetI2SControllerReset( CBOOL Enable );
CBOOL MES_AUDIO_GetI2SControllerReset( void );   
void  MES_AUDIO_SetI2SOutputEnable( CBOOL Enable );
CBOOL MES_AUDIO_GetI2SOutputEnable( void );
void  MES_AUDIO_SetI2SInputEnable( CBOOL Enable );
CBOOL MES_AUDIO_GetI2SInputEnable( void );
void  MES_AUDIO_SetI2SLoopBackEnable( CBOOL Enable );
CBOOL MES_AUDIO_GetI2SLoopBackEnable( void );
//@}

//--------------------------------------------------------------------------
/// @name Audio Buffer Function
//--------------------------------------------------------------------------
//@{
void  MES_AUDIO_SetAudioBufferPCMOUTEnable( CBOOL Enable ); 
CBOOL MES_AUDIO_GetAudioBufferPCMOUTEnable( void ); 
void  MES_AUDIO_SetAudioBufferPCMINEnable( CBOOL Enable ); 
CBOOL MES_AUDIO_GetAudioBufferPCMINEnable( void ); 
void  MES_AUDIO_SetPCMOUTDataWidth( U32 DataWidth );
U32   MES_AUDIO_GetPCMOUTDataWidth( void );    
void  MES_AUDIO_SetPCMINDataWidth( U32 DataWidth );
U32   MES_AUDIO_GetPCMINDataWidth( void );
CBOOL MES_AUDIO_IsPCMInBufferReady( void );    
CBOOL MES_AUDIO_IsPCMOutBufferReady( void );        
//@}

//--------------------------------------------------------------------------
/// @name   Audio State Function
//--------------------------------------------------------------------------
//@{
MES_AUDIO_I2SSTATE    MES_AUDIO_GetI2SState( void );
//@}

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_AUDIO_H__

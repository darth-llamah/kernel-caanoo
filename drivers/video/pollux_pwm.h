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
#ifndef __MES_PWM_H__
#define __MES_PWM_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif

#define NUMBER_OF_PWM_MODULE	2
#define OFFSET_OF_PWM_MODULE	0x400

//------------------------------------------------------------------------------
/// @defgroup   PWM PWM
//------------------------------------------------------------------------------
//@{
 
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_PWM_Initialize( void );
U32     MES_PWM_SelectModule( U32 ModuleIndex );
U32     MES_PWM_GetCurrentModule( void );
U32     MES_PWM_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
U32     MES_PWM_GetPhysicalAddress( void );
U32     MES_PWM_GetSizeOfRegisterSet( void );
void    MES_PWM_SetBaseAddress( U32 BaseAddress );
U32     MES_PWM_GetBaseAddress( void );
CBOOL   MES_PWM_OpenModule( void );
CBOOL   MES_PWM_CloseModule( void );
CBOOL   MES_PWM_CheckBusy( void );
CBOOL   MES_PWM_CanPowerDown( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Clock Control Interface
//@{
void            MES_PWM_SetClockPClkMode( MES_PCLKMODE mode );
MES_PCLKMODE    MES_PWM_GetClockPClkMode( void );
void            MES_PWM_SetClockDivisorEnable( CBOOL Enable );
CBOOL           MES_PWM_GetClockDivisorEnable( void );
void            MES_PWM_SetClockSource( U32 Index, U32 ClkSrc );
U32             MES_PWM_GetClockSource( U32 Index );
void            MES_PWM_SetClockDivisor( U32 Index, U32 Divisor );
U32             MES_PWM_GetClockDivisor( U32 Index );
//@}

//------------------------------------------------------------------------------
///  @name  PWM Operation.
//@{
void    MES_PWM_SetPreScale( U32 channel, U32 prescale );
U32     MES_PWM_GetPreScale( U32 channel );
void    MES_PWM_SetPolarity( U32 channel, CBOOL bypass );
CBOOL   MES_PWM_GetPolarity( U32 channel );
void    MES_PWM_SetPeriod( U32 channel, U32 period );
U32     MES_PWM_GetPeriod( U32 channel );
void    MES_PWM_SetDutyCycle( U32 channel, U32 duty );
U32     MES_PWM_GetDutyCycle( U32 channel );
//@} 

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_PWM_H__

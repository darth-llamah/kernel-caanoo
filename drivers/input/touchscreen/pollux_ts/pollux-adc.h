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
#ifndef __MES_ADC_H__
#define __MES_ADC_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif


#define NUMBER_OF_ADC_MODULE 1
#define OFFSET_OF_ADC_MODULE 0 
#define INTNUM_OF_ADC_MODULE 25
//------------------------------------------------------------------------------
/// @defgroup   ADC ADC
//------------------------------------------------------------------------------
//@{
 
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_ADC_Initialize( void );
U32     MES_ADC_SelectModule( U32 ModuleIndex );
U32     MES_ADC_GetCurrentModule( void );
U32     MES_ADC_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
U32     MES_ADC_GetPhysicalAddress( void );
U32     MES_ADC_GetSizeOfRegisterSet( void );
void    MES_ADC_SetBaseAddress( U32 BaseAddress );
U32     MES_ADC_GetBaseAddress( void );
CBOOL   MES_ADC_OpenModule( void );
CBOOL   MES_ADC_CloseModule( void );
CBOOL   MES_ADC_CheckBusy( void );
CBOOL   MES_ADC_CanPowerDown( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Interrupt Interface
//@{
S32     MES_ADC_GetInterruptNumber( void );
void    MES_ADC_SetInterruptEnable( S32 IntNum, CBOOL Enable );
CBOOL   MES_ADC_GetInterruptEnable( S32 IntNum );
CBOOL   MES_ADC_GetInterruptPending( S32 IntNum );
void    MES_ADC_ClearInterruptPending( S32 IntNum );
void    MES_ADC_SetInterruptEnableAll( CBOOL Enable );
CBOOL   MES_ADC_GetInterruptEnableAll( void );
CBOOL   MES_ADC_GetInterruptPendingAll( void );
void    MES_ADC_ClearInterruptPendingAll( void );
S32     MES_ADC_GetInterruptPendingNumber( void );  // -1 if None
//@} 

//------------------------------------------------------------------------------
///  @name  ADC Operation.
//@{
void  	MES_ADC_SetClockPClkMode( MES_PCLKMODE mode );
void    MES_ADC_SetPrescalerValue( U32 value );
U32     MES_ADC_GetPrescalerValue( void );
void    MES_ADC_SetPrescalerEnable( CBOOL enable );
CBOOL   MES_ADC_GetPrescalerEnable( void );
void    MES_ADC_SetInputChannel( U32 channel );
U32     MES_ADC_GetInputChannel( void );
void    MES_ADC_SetStandbyMode( CBOOL enable );
CBOOL   MES_ADC_GetStandbyMode( void );
void    MES_ADC_Start( void );
CBOOL   MES_ADC_IsBusy( void );
U32     MES_ADC_GetConvertedData( void );
//@} 

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_ADC_H__

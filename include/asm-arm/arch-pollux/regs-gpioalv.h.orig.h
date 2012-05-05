//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module     : ALIVE
//	File       : mes_alive.h
//	Description: 
//	Author     : 
//	History    : 
//------------------------------------------------------------------------------
#ifndef __MES_ALIVE_H__
#define __MES_ALIVE_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif

// ghcstop: mes_chip_lf100.h, 의미없는 define....
#define NUMBER_OF_ALIVE_MODULE 1
#define OFFSET_OF_ALIVE_MODULE 0 


//------------------------------------------------------------------------------
/// @defgroup   ALIVE   ALIVE
//------------------------------------------------------------------------------
//@{
 
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_ALIVE_Initialize( void );
U32     MES_ALIVE_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
U32     MES_ALIVE_GetPhysicalAddress( U32 ModuleIndex );
U32     MES_ALIVE_GetSizeOfRegisterSet( void );
void    MES_ALIVE_SetBaseAddress( U32 ModuleIndex, U32 BaseAddress );
U32     MES_ALIVE_GetBaseAddress( U32 ModuleIndex );
CBOOL   MES_ALIVE_OpenModule( U32 ModuleIndex );
CBOOL   MES_ALIVE_CloseModule( U32 ModuleIndex );
CBOOL   MES_ALIVE_CheckBusy( U32 ModuleIndex );
CBOOL   MES_ALIVE_CanPowerDown( U32 ModuleIndex );
//@} 

//------------------------------------------------------------------------------
///  @name  Alive Operation.
//@{
void    MES_ALIVE_SetWriteEnable( U32 ModuleIndex, CBOOL Enable );
CBOOL   MES_ALIVE_GetWriteEnable( U32 ModuleIndex );
void    MES_ALIVE_SetVDDPower( U32 ModuleIndex, CBOOL Enable );
CBOOL   MES_ALIVE_GetVDDPower( U32 ModuleIndex );
void    MES_ALIVE_SetAliveGpio( U32 ModuleIndex, U32 GpioNum, CBOOL Enable );
CBOOL   MES_ALIVE_GetAliveGpio( U32 ModuleIndex, U32 GpioNum );
void    MES_ALIVE_SetScratchReg( U32 ModuleIndex, U32 Data );
U32     MES_ALIVE_GetScratchReg( U32 ModuleIndex );
CBOOL   MES_ALIVE_GetVddPowerToggle( U32 ModuleIndex );
//@} 

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_GPIOALIVE_H__

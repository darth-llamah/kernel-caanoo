//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module     : I2C
//	File       : mes_i2c.h
//	Description: 
//	Author     : 
//	History    : 
//------------------------------------------------------------------------------
#ifndef __MES_I2C_H__
#define __MES_I2C_H__

#include <asm/arch/wincetype.h> // ghcstop add

#ifdef  __cplusplus
extern "C"
{
#endif

#define NUMBER_OF_I2C_MODULE 2 // ghcstop add

//------------------------------------------------------------------------------
/// @defgroup   I2C I2C
//------------------------------------------------------------------------------
//@{
 
     /// @brief Select I2C Mode 
    typedef enum 
    {
        MES_I2C_TXRXMODE_SLAVE_RX    = 0,     ///< Slave Receive Mode
        MES_I2C_TXRXMODE_SLAVE_TX    = 1,     ///< Slave Transmit Mode
        MES_I2C_TXRXMODE_MASTER_RX   = 2,     ///< Master Receive Mode
        MES_I2C_TXRXMODE_MASTER_TX   = 3     ///< Master Transmit Mode

    } MES_I2C_TXRXMODE ;

    /// @brief  Start/Stop signal
    typedef enum
    {
        MES_I2C_SIGNAL_STOP  = 0,            ///< Stop signal generation
        MES_I2C_SIGNAL_START = 1             ///< Start signal generation
        
    } MES_I2C_SIGNAL ;
 
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_I2C_Initialize( void );
U32     MES_I2C_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
//U32     MES_I2C_GetPhysicalAddress( U32 ModuleIndex );  // ghcstop delete
U32     MES_I2C_GetSizeOfRegisterSet( U32 ModuleIndex );
void    MES_I2C_SetBaseAddress( U32 ModuleIndex, U32 BaseAddress );
U32     MES_I2C_GetBaseAddress( U32 ModuleIndex );
CBOOL   MES_I2C_OpenModule( U32 ModuleIndex );
CBOOL   MES_I2C_CloseModule( U32 ModuleIndex );
CBOOL   MES_I2C_CheckBusy( U32 ModuleIndex );
CBOOL   MES_I2C_CanPowerDown( U32 ModuleIndex );
//@} 

//------------------------------------------------------------------------------
///  @name   Interrupt Interface
//@{
//S32     MES_I2C_GetInterruptNumber( U32 ModuleIndex ); // ghcstop delete
void    MES_I2C_SetInterruptEnable( U32 ModuleIndex, S32 IntNum, CBOOL Enable );
CBOOL   MES_I2C_GetInterruptEnable( U32 ModuleIndex, S32 IntNum );
CBOOL   MES_I2C_GetInterruptPending( U32 ModuleIndex, S32 IntNum );
void    MES_I2C_ClearInterruptPending( U32 ModuleIndex, S32 IntNum );
void    MES_I2C_SetInterruptEnableAll( U32 ModuleIndex, CBOOL Enable );
CBOOL   MES_I2C_GetInterruptEnableAll( U32 ModuleIndex );
CBOOL   MES_I2C_GetInterruptPendingAll( U32 ModuleIndex );
void    MES_I2C_ClearInterruptPendingAll( U32 ModuleIndex );
S32     MES_I2C_GetInterruptPendingNumber( U32 ModuleIndex );  // -1 if None
//@} 

//------------------------------------------------------------------------------
///  @name   Clock Control Interface
//@{
void            MES_I2C_SetClockPClkMode( U32 ModuleIndex, MES_PCLKMODE mode );
MES_PCLKMODE    MES_I2C_GetClockPClkMode( U32 ModuleIndex );

//@}

//--------------------------------------------------------------------------
/// @name Configuration Function
//--------------------------------------------------------------------------
//@{
void    MES_I2C_SetClockPrescaler( U32 ModuleIndex, U32 PclkDivider, U32 Prescaler );
void    MES_I2C_GetClockPrescaler( U32 ModuleIndex, U32* pPclkDivider, U32* pPrescaler );
void    MES_I2C_SetExtendedIRQEnable( U32 ModuleIndex, CBOOL bEnable );
CBOOL   MES_I2C_GetExtendedIRQEnable( U32 ModuleIndex );
void    MES_I2C_SetSlaveAddress( U32 ModuleIndex, U8 Address);
U8      MES_I2C_GetSlaveAddress(U32 ModuleIndex);
void    MES_I2C_SetDataDelay( U32 ModuleIndex, U32 delay );
U32     MES_I2C_GetDataDelay (U32 ModuleIndex);
//@}

//------------------------------------------------------------------------------
/// @name Operation Function
//------------------------------------------------------------------------------
//@{
void        MES_I2C_SetAckGenerationEnable( U32 ModuleIndex, CBOOL bAckGen );
CBOOL       MES_I2C_GetAckGenerationEnable( U32 ModuleIndex );
void        MES_I2C_ClearOperationHold  ( U32 ModuleIndex );
void        MES_I2C_ControlMode ( U32 ModuleIndex, MES_I2C_TXRXMODE TxRxMode, MES_I2C_SIGNAL Signal );
CBOOL       MES_I2C_IsBusy( U32 ModuleIndex );
void        MES_I2C_WriteByte ( U32 ModuleIndex, U8 WriteData);
U8          MES_I2C_ReadByte (U32 ModuleIndex );
void        MES_I2C_BusDisable( U32 ModuleIndex );
//@}

//------------------------------------------------------------------------------
/// @name Checking Function ( extra Interrupt source )
//------------------------------------------------------------------------------
//@{    
CBOOL       MES_I2C_IsSlaveAddressMatch( U32 ModuleIndex );
void        MES_I2C_ClearSlaveAddressMatch( U32 ModuleIndex );    
CBOOL       MES_I2C_IsGeneralCall( U32 ModuleIndex );
void        MES_I2C_ClearGeneralCall( U32 ModuleIndex );
CBOOL       MES_I2C_IsSlaveRxStop( U32 ModuleIndex );
void        MES_I2C_ClearSlaveRxStop( U32 ModuleIndex );
CBOOL       MES_I2C_IsBusArbitFail( U32 ModuleIndex );
CBOOL       MES_I2C_IsACKReceived( U32 ModuleIndex );
//@}

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_I2C_H__

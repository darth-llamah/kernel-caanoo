//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module     : Clock & Power Manager
//	File       : mes_clkpwr.h
//	Description: 
//	Author     : 
//	History    : 
//------------------------------------------------------------------------------
#ifndef __MES_CLKPWR_H__
#define __MES_CLKPWR_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif

#define NUMBER_OF_CLKPWR_MODULE	2
#define OFFSET_OF_CLKPWR_MODULE	0
#define INTNUM_OF_CLKPWR_MODULE 5
//------------------------------------------------------------------------------
/// @defgroup   CLKPWR Clock & Power Manager
//------------------------------------------------------------------------------
//@{
 
 	/// @brief reset status
    typedef enum 
    {
        MES_CLKPWR_RESETSTATUS_POWERON		= (1UL<<0),	///< Power on reset
        MES_CLKPWR_RESETSTATUS_WATCHDOG	    = (1UL<<1), ///< Watchdog reset
        MES_CLKPWR_RESETSTATUS_GPIOSW		= (1UL<<2),	///< GPIO wakeup or Software reset
        MES_CLKPWR_RESETSTATUS_ALL			= (7UL)     ///< All reset

    }MES_CLKPWR_RESETSTATUS;
 
 	/// @brief	Power modes
   	typedef enum {
		MES_CLKPWR_POWERMODE_NORMAL 	= 0,    ///< normal 
		MES_CLKPWR_POWERMODE_IDLE 		= 1,    ///< idle (only cpu clock is off)
		MES_CLKPWR_POWERMODE_STOP 		= 2     ///< stop (all clock is off)

	}MES_CLKPWR_POWERMODE;
 
     /// @brief  Bus type
    typedef enum
    {
        MES_CLKPWR_BUSPAD_DDR_CNTL        = 30,    ///< DDR SDRAM Control Pad
        MES_CLKPWR_BUSPAD_DDR_ADDR        = 28,    ///< DDR SDRAM Address Pad
        MES_CLKPWR_BUSPAD_DDR_DATA        = 26,    ///< DDR SDRAM Data Pad
        MES_CLKPWR_BUSPAD_DDR_CLK         = 24,    ///< DDR SDRAM Clock Pad
        MES_CLKPWR_BUSPAD_STATIC_CNTL     = 22,    ///< Static Control Pad
        MES_CLKPWR_BUSPAD_STATIC_ADDR     = 20,    ///< Static Address Pad
        MES_CLKPWR_BUSPAD_STATIC_DATA     = 18,    ///< Static Data Pad
        MES_CLKPWR_BUSPAD_VSYNC           = 6,     ///< Vertical Sync Pad
        MES_CLKPWR_BUSPAD_HSYNC           = 4,     ///< Horizontal Sync Pad
        MES_CLKPWR_BUSPAD_DE              = 2,     ///< Data Enable Pad
        MES_CLKPWR_BUSPAD_PVCLK           = 0      ///< Primary Video Clock Pad

    } MES_CLKPWR_BUSPAD;

 
//------------------------------------------------------------------------------
/// @name   Module Interface
//@{
CBOOL   MES_CLKPWR_Initialize( void );
U32     MES_CLKPWR_SelectModule( U32 ModuleIndex );
U32     MES_CLKPWR_GetCurrentModule( void );
U32     MES_CLKPWR_GetNumberOfModule( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Basic Interface
//@{
U32     MES_CLKPWR_GetPhysicalAddress( void );
U32     MES_CLKPWR_GetSizeOfRegisterSet( void );
void    MES_CLKPWR_SetBaseAddress( U32 BaseAddress );
U32     MES_CLKPWR_GetBaseAddress( void );
CBOOL   MES_CLKPWR_OpenModule( void );
CBOOL   MES_CLKPWR_CloseModule( void );
CBOOL   MES_CLKPWR_CheckBusy( void );
CBOOL   MES_CLKPWR_CanPowerDown( void );
//@} 

//------------------------------------------------------------------------------
///  @name   Interrupt Interface
//@{
S32     MES_CLKPWR_GetInterruptNumber( void );
void    MES_CLKPWR_SetInterruptEnable( S32 IntNum, CBOOL Enable );
CBOOL   MES_CLKPWR_GetInterruptEnable( S32 IntNum );
CBOOL   MES_CLKPWR_GetInterruptPending( S32 IntNum );
void    MES_CLKPWR_ClearInterruptPending( S32 IntNum );
void    MES_CLKPWR_SetInterruptEnableAll( CBOOL Enable );
CBOOL   MES_CLKPWR_GetInterruptEnableAll( void );
CBOOL   MES_CLKPWR_GetInterruptPendingAll( void );
void    MES_CLKPWR_ClearInterruptPendingAll( void );
S32     MES_CLKPWR_GetInterruptPendingNumber( void );  // -1 if None
//@} 

//--------------------------------------------------------------------------
/// @name   Clock Management
//--------------------------------------------------------------------------
//@{
void 	MES_CLKPWR_SetPLLPMS ( U32 pllnumber, U32 PDIV, U32 MDIV, U32 SDIV );
void 	MES_CLKPWR_SetPLLPowerOn ( CBOOL On );
void 	MES_CLKPWR_DoPLLChange ( void );
CBOOL 	MES_CLKPWR_IsPLLStable ( void );
void 	MES_CLKPWR_SetClockCPU    ( U32 ClkSrc, U32 CoreDivider, U32 BusDivider );
void 	MES_CLKPWR_SetClockBCLK    ( U32 ClkSrc, U32 BCLKDivider );
//@}

//--------------------------------------------------------------------------
/// @name   VDD Power Toggle Wakeup Management
//--------------------------------------------------------------------------
//@{
void  	MES_CLKPWR_SetVDDPowerToggleWakeUpEnable ( CBOOL Enable );
CBOOL	MES_CLKPWR_GetVDDPowerToggleWakeUpEnable ( void );
void  	MES_CLKPWR_SetVDDPowerToggleWakeUpFallEdgeEnable ( CBOOL Enable );
CBOOL 	MES_CLKPWR_GetVDDPowerToggleWakeUpFallEdgeEnable ( void );
void  	MES_CLKPWR_SetVDDPowerToggleWakeUpRiseEdgeEnable ( CBOOL Enable ); 
CBOOL 	MES_CLKPWR_GetVDDPowerToggleWakeUpRiseEdgeEnable ( void );               
CBOOL 	MES_CLKPWR_GetVDDPowerToggleWakeUpPending ( void );       
void  	MES_CLKPWR_ClearVDDPowerToggleWakeUpPending ( void );
//@}

//--------------------------------------------------------------------------
/// @name   GPIO Wakeup Management
//--------------------------------------------------------------------------
//@{
void  	MES_CLKPWR_SetGPIOWakeUpEnable ( U32 Group, U32 BitNumber, CBOOL Enable );
CBOOL	MES_CLKPWR_GetGPIOWakeUpEnable ( U32 Group, U32 BitNumber );
void  	MES_CLKPWR_SetGPIOWakeUpFallEdgeEnable ( U32 Group, U32 BitNumber, CBOOL Enable );
CBOOL 	MES_CLKPWR_GetGPIOWakeUpFallEdgeEnable ( U32 Group, U32 BitNumber );
void  	MES_CLKPWR_SetGPIOWakeUpRiseEdgeEnable ( U32 Group, U32 BitNumber, CBOOL Enable );
CBOOL 	MES_CLKPWR_GetGPIOWakeUpRiseEdgeEnable ( U32 Group, U32 BitNumber );
CBOOL 	MES_CLKPWR_GetGPIOWakeUpPending ( U32 Group, U32 BitNumber );       
void  	MES_CLKPWR_ClearGPIOWakeUpPending ( U32 Group, U32 BitNumber );
//@}

//--------------------------------------------------------------------------
/// @name   RTC Wakeup Management
//--------------------------------------------------------------------------
//@{
void  	MES_CLKPWR_SetRTCWakeUpEnable ( CBOOL Enable );
CBOOL	MES_CLKPWR_GetRTCWakeUpEnable ( void );
//@}

//--------------------------------------------------------------------------
/// @name	Reset Management
//--------------------------------------------------------------------------
//@{
U32 	MES_CLKPWR_GetResetStatus ( void );
void	MES_CLKPWR_ClearResetStatus ( U32 rst );
CBOOL	MES_CLKPWR_CheckPowerOnReset( void );
CBOOL	MES_CLKPWR_CheckWatchdogReset( void );
CBOOL 	MES_CLKPWR_CheckGPIOSWReset( void );
CBOOL   MES_CLKPWR_CheckBatteryFaultStatus( void );
void	MES_CLKPWR_ClearPowerOnReset( void );
void	MES_CLKPWR_ClearWatchdogReset( void );
void	MES_CLKPWR_ClearGPIOSWReset( void );
void    MES_CLKPWR_ClearBatteryFaultStatus( void );
// Software Reset
void 	MES_CLKPWR_DoSoftwareReset( void );
// GPIO & Software reset
void  	MES_CLKPWR_SetGPIOSWResetEnable ( CBOOL Enable );
CBOOL 	MES_CLKPWR_GetGPIOSWResetEnable ( void );
//@}

//--------------------------------------------------------------------------
/// @name	Power Management
//--------------------------------------------------------------------------
//@{
MES_CLKPWR_POWERMODE 	MES_CLKPWR_GetLastPowerMode ( void );
MES_CLKPWR_POWERMODE 	MES_CLKPWR_GetCurrentPowerMode ( void );
void		            MES_CLKPWR_SetCurrentPowerMode ( MES_CLKPWR_POWERMODE powermode );
//@}

//--------------------------------------------------------------------------
/// @name	PAD Strength Management
//--------------------------------------------------------------------------
//@{
void    MES_CLKPWR_SetGpioPadStrength( U32 Group, U32 BitNumber, U32 mA );
U32     MES_CLKPWR_GetGpioPadStrength( U32 Group, U32 BitNumber );
void    MES_CLKPWR_SetBusPadStrength( MES_CLKPWR_BUSPAD Bus, U32 mA );
U32     MES_CLKPWR_GetBusPadStrength( MES_CLKPWR_BUSPAD Bus );
//@}

//@} 

#ifdef  __cplusplus
}
#endif


#endif // __MES_CLKPWR_H__


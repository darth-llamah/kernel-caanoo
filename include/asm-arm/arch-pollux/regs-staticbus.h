/*
 * linux/include/asm-arm/arch-mp2530/regs-pwm.h
 *
 * Copyright. 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/mp2530/index.html
 * 
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho)     - main programmer
 * amphell(Bang Chang Hyeok)    - Hardware Engineer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */ 
#ifndef __ASM_ARM_REGS_PWM_H
#define __ASM_ARM_REGS_PWM_H

#include <asm/arch/wincetype.h>


///	@brief	Static BUS ID
enum SBUSID
{
	SBUSID_STATIC0		=  0, 	///< Static BUS 0
	SBUSID_STATIC1		=  1, 	///< Static BUS 1
	SBUSID_STATIC2		=  2, 	///< Static BUS 2
	SBUSID_STATIC3		=  3, 	///< Static BUS 3
	SBUSID_STATIC4		=  4, 	///< Static BUS 4
	SBUSID_STATIC5		=  5, 	///< Static BUS 5
	SBUSID_NAND			=  7, 	///< NAND Flash
	SBUSID_FORCEU32		= 0x7FFFFFFF
};

///	@brief	wait control
enum WAITMODE
{
	WAITMODE_DISABLE    	= 1, 	///< wait control is disabled
	WAITMODE_ACTIVEHIGH 	= 2, 	///< nSWAIT is active high
	WAITMODE_ACTIVELOW  	= 3, 	///< nSWAIT is active low
	WAITMODE_FORCEU32   	= 0x7FFFFFFF
};

/// @brief  Burst Mode control
enum BURSTMODE
{
	BURSTMODE_DISABLE		= 0,	///< Disable burst access
	BURSTMODE_4BYTEBURST	= 1,	///< 4 byte burst access
	BURSTMODE_8BYTEBURST	= 2,	///< 8 byte burst access
	BURSTMODE_16BYTEBURST	= 3,	///< 16 byte burst access
	BURSTMODE_FORCEU32		= 0x7FFFFFFF
};

/// @brief	Nand Flash address type.
enum NFTYPE
{
	NFTYPE_SBADDR3	= 0, 	///< Small block 3 address type 
	NFTYPE_SBADDR4	= 1, 	///< Small block 4 address type
	NFTYPE_LBADDR4	= 2, 	///< Large block 4 address type
	NFTYPE_LBADDR5	= 3, 	///< Large block 5 address type
	NFTYPE_FORCEU32	= 0x7FFFFFFF
};


enum {
	STATICWAIT_DISABLE    	= 1,
	STATICWAIT_ACTIVEHIGH 	= 2,
	STATICWAIT_ACTIVELOW  	= 3
};

enum {
	STATICBST_DISABLE		= 0,
	STATICBST_4BYTE			= 1,
	STATICBST_8BYTE			= 2,
	STATICBST_16BYTE		= 3
};


//------------------------------------------------------------------------------
/// @brief	MCUS03 register set structure
//------------------------------------------------------------------------------
struct MES_MCUS03_RegisterSet
{
	volatile U32 MEMBW			;	///< 00h : Memory Bus Width Register
	volatile U32 MEMTIMEACS		;	///< 04h : Memory Timing for tACS Register
	volatile U32 MEMTIMECOS		;	///< 08h : Memory Timing for tCOS Register
	volatile U32 MEMTIMEACCL	;	///< 0Ch : Memory Timing for tACC Low Register
	volatile U32 MEMTIMEACCH	;	///< 10h : Memory Timing for tACC High Register (Reserved for future use)
	volatile U32 MEMTIMESACCL	;	///< 14h : Memory Timing for tSACC Low Register
	volatile U32 MEMTIMESACCH	;	///< 18h : Memory Timing for tSACC High Register (Reserved for future use)
	const    U32 __Reserved00[2];	///< 1Ch, 20h : Reserved for future use.
	volatile U32 MEMTIMEOCH		;	///< 24h : Memory Timing for tOCH Register
	volatile U32 MEMTIMECAH		;	///< 28h : Memory Timing for tCAH Register
	volatile U32 MEMBURSTL		;	///< 2Ch : Memory Burst Control Low Register
	volatile U32 MEMBURSTH		;	///< 30h : Memory Burst Control High Register (Reserved for future use)
	volatile U32 MEMWAIT		;	///< 34h : Memory Wait Control Register
	const    U32 __Reserved01[15];	///< 38h ~ 73h : Reserved for future use.
	volatile U32 NFCONTROL		;	///< 74h : Nand Flash Control Register
	volatile U32 NFECC			;	///< 78h : Nand Flash ECC Register
	volatile U32 NFDATACNT		;	///< 7Ch : Nand Flash Data Counter Register
};

//------------------------------------------------------------------------------
/// @class	MES_MCUS03
/// @brief	Memory control unit for static bus
///	@nosubgrouping
//------------------------------------------------------------------------------

typedef struct _MES_MCUS03 {
	volatile struct  MES_MCUS03_RegisterSet *m_pRegister;	
	
	void 	(*SetStaticBUSConfig)  ( struct _MES_MCUS03 *this, enum SBUSID Id, U32 BitWidth,U32 tACS,U32 tCAH,U32 tCOS,U32 tOCH,U32 tACC,U32 tSACC,enum WAITMODE  WaitMode,enum BURSTMODE BurstRead,enum BURSTMODE BurstWrite);
	void 	(*GetStaticBUSConfig)  ( struct _MES_MCUS03 *this, enum SBUSID Id, U32* BitWidth, U32* tACS, U32* tCAH, U32* tCOS, U32* tOCH, U32* tACC, U32* tSACC, enum WAITMODE*  WaitMode, enum BURSTMODE* BurstRead, enum BURSTMODE* BurstWrite );

	void 	(*SetNFType) ( struct _MES_MCUS03 *this, enum NFTYPE type );
	enum NFTYPE (*GetNFType)  ( struct _MES_MCUS03 *this );
	void 	(*SetNFBootEnable) ( struct _MES_MCUS03 *this, CBOOL bEnb);
	CBOOL 	(*IsNFBootEnable)  ( struct _MES_MCUS03 *this );
	void 	(*SetNFBank) (struct _MES_MCUS03 *this, U32 Bank);
	U32 	(*GetNFBank)  ( struct _MES_MCUS03 *this );
	CBOOL 	(*IsNFReady)  ( struct _MES_MCUS03 *this );
	U32 	(*GetNFECC) ( struct _MES_MCUS03 *this );
	U32 	(*GetNFDataCount) ( struct _MES_MCUS03 *this );

	U32 (*SetValue) ( U32 OldValue, U32 BitValue, U32 MSB, U32 LSB );
	U32 (*GetValue) ( U32 Value, U32 MSB, U32 LSB );
	U32 (*SetBit) ( U32 OldValue, U32 BitValue, U32 BitPos );
	U32 (*SetBit1) ( U32 OldValue, U32 BitPos );
	U32 (*ClearBit) ( U32 OldValue, U32 BitPos );
	U32 (*GetBit) ( U32 Value, U32 BitPos );

} MES_MCUS03;

void MES_MCUS03_INIT(MES_MCUS03 *this, unsigned long regbase);


#endif // __MES_MCUS03_H__


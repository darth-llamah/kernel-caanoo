/*
 * linux/include/asm-arm/arch-pollux/regs-timer.h
 *
 * Copyright. 2003-2008 AESOP-Embedded project
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
#ifndef __ASM_ARM_REGS_TIMER_H
#define __ASM_ARM_REGS_TIMER_H
 
#include <asm/hardware.h>

#define TMRCOUNT0		    __REG32(0xC0001800)	// timer 0 counter register
#define TMRMATCH0		    __REG32(0xC0001804)	// timer 0 match register
#define TMRCONTROL0		    __REG32(0xC0001808)	// timer 0 control register
#define TMRCLKENB0		    __REG32(0xC0001840)	// timer 0 clock generation enable register
#define TMRCLKGEN0		    __REG32(0xC0001844)	// timer 0 clock generation control register

#define TMRCOUNT1		    __REG32(0xC0001880)	// timer 1 counter register
#define TMRMATCH1		    __REG32(0xC0001884)	// timer 1 match register
#define TMRCONTROL1		    __REG32(0xC0001888)	// timer 1 control register
#define TMRCLKENB1		    __REG32(0xC00018C0)	// timer 1 clock generation enable register
#define TMRCLKGEN1		    __REG32(0xC00018C4)	// timer 1 clock generation control register

#define TMRCOUNT2		    __REG32(0xC0001900)	// timer 2 counter register
#define TMRMATCH2		    __REG32(0xC0001904)	// timer 2 match register
#define TMRCONTROL2		    __REG32(0xC0001908)	// timer 2 control register
#define TMRCLKENB2		    __REG32(0xC0001940)	// timer 2 clock generation enable register
#define TMRCLKGEN2		    __REG32(0xC0001944)	// timer 2 clock generation control register

#define TMRCOUNT3		    __REG32(0xC0001980)	// timer 3 counter register
#define TMRMATCH3		    __REG32(0xC0001984)	// timer 3 match register
#define TMRCONTROL3		    __REG32(0xC0001988)	// timer 3 control register
#define TMRCLKENB3		    __REG32(0xC00019C0)	// timer 3 clock generation enable register
#define TMRCLKGEN3		    __REG32(0xC00019C4)	// timer 3 clock generation control register

// Timer Control Register( TMRCONTROL )
#define TMRCONTROL_READ_COUNTER		(1<<6)  
#define TMRCONTROL_INTPEND			(1<<5)  
#define TMRCONTROL_INTENB			(1<<4)
#define TMRCONTROL_RUN				(1<<3)
#define TMRCONTROL_WATCHDOG_ENB		(1<<2)
#define TMRCONTROL_SET_TCLOCK		(3<<0)
	#define TMRCONTROL_TCLOCK_DIV2	(0<<0)  // TCLK = Source clock / 2
	#define TMRCONTROL_TCLOCK_DIV4	(1<<0)  // TCLK = Source clock / 4
	#define TMRCONTROL_TCLOCK_DIV8	(2<<0)  // TCLK = Source clock / 8
	#define TMRCONTROL_TCLOCK_DIV1	(3<<0)  // TCLK = Source clock

// Timer Clock Gerneration Enable Register ( TMRCLKENB )
#define	TMRCLKENB_TCLKMODE			(1<<3) 	
#define	TMRCLKENB_CLKGENENB			(1<<2) 	

#define TMRCLKGEN_CLKDIV  			(0xff<<4)
#define TMRCLKGEN_CLKDIV_SHIFT		(4)

//#define TMRCLKGEN_CLKDIV0			(32) // pll0: 384 Mhz
//#define TMRCLKGEN_CLKDIV0			(25) // pll1: 147 Mhz
//#define TMRCLKGEN_CLKDIV0			(25) // pll1: 132.75 Mhz
//#define TMRCLKGEN_CLKDIV0			(23) // pll1: 124.2Mhz
//#define TMRCLKGEN_CLKDIV0			(19) // pll1: 100 Mhz
#define TMRCLKGEN_CLKDIV0			(50) // pll1: 261 Mhz

#define TMRCLKGEN_CLKSRCSEL			(7<<1)
#define TMRCLKGEN_CLKSRCSEL_SHIFT	(1)
	#define TMRCLKGEN_CLKSRC_PLL0	(0<<1)
	#define TMRCLKGEN_CLKSRC_PLL1	(1<<1)


#endif /* __REGS_TIMER_H */

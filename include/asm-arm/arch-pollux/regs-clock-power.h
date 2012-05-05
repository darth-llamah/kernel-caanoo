/*
 * linux/include/asm-arm/arch-pollux/regs-clock-power.h
 *
 * Copyright. 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/pollux/index.html
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
#ifndef __ASM_ARM_REGS_CLOCK_POWER_H
#define __ASM_ARM_REGS_CLOCK_POWER_H

#include <asm/hardware.h>

/*
 * Chapter 3 
 * Clocks and Power Manager
 */
#define CLKMODEREG		    __REG32(0xC000F000)	// clock mode register  
#define PLLSETREG0		    __REG32(0xC000F004)	// pll 0 setting register
#define PLLSETREG1		    __REG32(0xC000F008)	// pll 1 setting register
//#define PLLSETREG2		    __REG32(0xC000F00C)	// pll 2 setting register, only for mp2530
#define GPIOWAKEUPENB	    __REG32(0xC000F040)	// gpio wakeup enable register
#define RTCWAKEUPENB	    __REG32(0xC000F044)	// rtc  wakeup enable register
#define GPIOWAKEUPRISEENB	__REG32(0xC000F048)	// rising  edge detect register
#define GPIOWAKEUPFALLENB	__REG32(0xC000F04C)	// falling edge detect register
#define GPIOPEND	        __REG32(0xC000F050)	// wakeup source detect pending register
#define INTPENDSPAD	        __REG32(0xC000F058)	// interrup pending & scratch pad register
#define PWRRSTSTATUS	    __REG32(0xC000F05C)	// reset status register
#define INTENB	            __REG32(0xC000F060)	// interrupt enable register
#define PWMMODE	            __REG32(0xC000F07C)	// power mode control register

// only for pollux
#define PADSTRENGTHGPIOAL   __REG32(0xC000F100) // PAD STRENGTH GPIOA LOW REGISTER
#define PADSTRENGTHGPIOAH   __REG32(0xC000F104) // PAD STRENGTH GPIOA HIGH REGISTER 
#define PADSTRENGTHGPIOBL   __REG32(0xC000F108) // PAD STRENGTH GPIOB LOW REGISTER
#define PADSTRENGTHGPIOBH   __REG32(0xC000F10C) // PAD STRENGTH GPIOB HIGH REGISTER
#define PADSTRENGTHGPIOCL   __REG32(0xC000F110) // PAD STRENGTH GPIOC LOW REGISTER
#define PADSTRENGTHGPIOCH   __REG32(0xC000F114) // PAD STRENGTH GPIOC HIGH REGISTER
#define PADSTRENGTHBUS      __REG32(0xC000F118) // PAD STRENGTH BUS REGISTER
 
 
#define CLKMODEREG_PLLPWDN1			(1<<30)
#define CLKMODEREG_CLKSELBCLK       (3<<24)     
#define CLKMODEREG_CLKDIV1BCLK		(15<<20)   
#define CLKMODEREG_CLKDIV2CPU0      (15<<6)    
#define CLKMODEREG_CLKSELCPU0       (3<<4)      
#define CLKMODEREG_CLKDIVCPU0       (15<<0)    

#define PLLSET_0_PDIV			(0x3f<<18)
#define PLLSET_0_PDIV_SHIFT	    (18)
#define PLLSET_0_MDIV			(0x3ff<<8)
#define PLLSET_0_MDIV_SHIFT	    (8)
#define PLLSET_0_SDIV			(0xff<<0)
#define PLLSET_0_SDIV_SHIFT	    (0)

#define PLLSET_1_PDIV			(0xff<<16)
#define PLLSET_1_PDIV_SHIFT	    (16)
#define PLLSET_1_MDIV			(0xff<<8)
#define PLLSET_1_MDIV_SHIFT	    (8)
#define PLLSET_1_SDIV			(0xff<<0)
#define PLLSET_1_SDIV_SHIFT	    (0)

#ifndef __ASSEMBLY__

struct pollux_clkmode_reg_t
{
	int pll_pdown1;
	int bclk_src_pll;
	int bclk_div;
	int cpu0_AHB_div;
	int cpu0_src_pll;
	int cpu0_div;
};	



// arch/arm/mach-pollux/pollux.c
extern unsigned long pollux_get_cpu0_fclk   (void);
extern unsigned long pollux_get_cpu0_AHB_clk(void);
extern unsigned long pollux_get_bclk        (void);
extern unsigned long pollux_get_pclk        (void);
extern unsigned long pollux_get_dramclk     (void);
extern unsigned long get_pll_clk( int pllsrc ); 
extern int           pollux_calc_divider(unsigned int pll_hz, unsigned int desired_hz);

#endif /* __ASSEMBLY__ */

#endif /* __REGS_CLOCK_POWER_H */

/*
 * linux/include/asm-arm/arch-mp2530/system.h
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
#ifndef __ASM_ARM_SYSTEML_H
#define __ASM_ARM_SYSTEML_H

#include <asm/hardware.h>
#include <asm/arch/regs-clock-power.h>
#include <asm/arch/regs-timer.h>

static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode)
{
	u32 regvalue		 = 0;
	
	if (mode == 's') 
	{
		cpu_reset(0);
	} 
	
	// ghcstop: not active.....어디 문제가 있군...	
	regvalue	=	TMRCLKGEN1;
	regvalue	&= ~( TMRCLKGEN_CLKSRCSEL << 1 );   
 	regvalue  |=  ( TMRCLKGEN_CLKSRC_PLL0    );
 	TMRCLKGEN1	=  regvalue; 	
 	
 	regvalue	=	TMRCLKGEN1;
 	regvalue 	&= ~( TMRCLKGEN_CLKDIV         ); 
	regvalue	|=  ( TMRCLKGEN_CLKDIV0 - 1 ) << ( TMRCLKGEN_CLKDIV_SHIFT );
	TMRCLKGEN1	=  regvalue;


	regvalue	=	TMRCLKENB1;
	regvalue 	&=  ~( TMRCLKENB_TCLKMODE);
	regvalue 	|=   ( TMRCLKENB_TCLKMODE ); 		
	TMRCLKENB1	=	 regvalue;
	
	regvalue	=	TMRCLKENB1;
	regvalue	 &= ~( TMRCLKENB_CLKGENENB ); 		
	regvalue	 |=  ( TMRCLKENB_CLKGENENB );
	TMRCLKENB1	 =	 regvalue;
	
	regvalue	=	TMRCONTROL1;
	regvalue 	&= ~( TMRCONTROL_INTPEND | TMRCONTROL_SET_TCLOCK );
	regvalue 	|=  ( TMRCONTROL_TCLOCK_DIV1                     ); 
	TMRCONTROL1  =	regvalue;
				
	TMRCOUNT1	=   (u32)0;
		
	regvalue 	=	(u32)( ( get_pll_clk(CLKSRC_PLL0) / (unsigned long)TMRCLKGEN_CLKDIV0 ) / (unsigned long)(10) );
	TMRMATCH1	=   (u32)regvalue;
 	
 	regvalue	=	TMRCONTROL1;
 	regvalue 	|=  ( TMRCONTROL_WATCHDOG_ENB ); 
	TMRCONTROL1 =   regvalue;					
	
	
	regvalue	=	TMRCONTROL1;
	regvalue |=  ( TMRCONTROL_RUN     );
	TMRCONTROL1 = regvalue;
	
	mdelay(1000);
	cpu_reset(0);	
}


#endif

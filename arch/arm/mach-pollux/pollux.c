/* 
 *  linux/arch/arm/mach-pollux/pollux.c
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
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/timex.h>		
#include <asm/arch/regs-clock-power.h>	


static unsigned int InfoLsb;
static unsigned int InfoMsb;
static unsigned int PwrOff = 0;

void Pwr_Off_Enable(void)
{
    PwrOff = 1;
}
EXPORT_SYMBOL(Pwr_Off_Enable);

unsigned int Get_Pwr_Status(void)
{
    return PwrOff;
}
EXPORT_SYMBOL(Get_Pwr_Status);

void SetInfo(unsigned int info, int flag)
{
    if(flag) InfoLsb = info;
    else InfoMsb = info;
}
EXPORT_SYMBOL(SetInfo);

unsigned int GetInfo(int flag)
{
    if(flag) return InfoLsb;
    return InfoMsb;
}

EXPORT_SYMBOL(GetInfo);


// pll0 
inline int get_0_pdiv(unsigned long x) 
{
	return (int)((x>>PLLSET_0_PDIV_SHIFT)&0xff); // 6 bits
}

inline int get_0_mdiv(unsigned long x) 
{
	return (int)((x>>PLLSET_0_MDIV_SHIFT)&0x3ff); // 10 bits
}

inline int get_0_sdiv(unsigned long x) 
{
	return (int)((x)&0xff);
}

// pll1
inline int get_1_pdiv(unsigned long x) 
{
	return (int)((x>>PLLSET_1_PDIV_SHIFT)&0xff);  // 8 bits
}

inline int get_1_mdiv(unsigned long x) 
{
	return (int)((x>>PLLSET_1_MDIV_SHIFT)&0xff); // 8 bits
}

inline int get_1_sdiv(unsigned long x) 
{
	return (int)((x)&0xff);
}



/*#define CLKMODEREG_PLLPWDN1			(1<<30)
#define CLKMODEREG_CLKSELBCLK       (3<<24)     
#define CLKMODEREG_CLKDIV1BCLK		(15<<20)   
#define CLKMODEREG_CLKDIV2CPU0      (15<<6)    
#define CLKMODEREG_CLKSELCPU0       (3<<4)      
#define CLKMODEREG_CLKDIVCPU0       (15<<0)    

struct pollux_clkmode_reg_t
{
	int pll_pdown1;
	int bclk_src_pll;
	int bclk_div;
	int cpu0_AHB_div;
	int cpu0_src_pll;
	int cpu0_div;
};	
*/

inline void get_clkmode_register(unsigned long cmreg, struct pollux_clkmode_reg_t *p) 
{
	p->cpu0_div       = (int)(cmreg&0xf) ;
	p->cpu0_src_pll   = (int)( (cmreg>>4)&0x3) ;
	p->cpu0_AHB_div   = (int)( (cmreg>>6)&0xf) ;
	p->bclk_div       = (int)( (cmreg>>20)&0xf) ;
	p->bclk_src_pll   = (int)( (cmreg>>24)&0x3) ;
	p->pll_pdown1     = (int)( (cmreg>>30)&0x1) ;
}


//	PLL0,1=(m * Fin)/(p * 2^s)
//	PLL0	56<=MDIV<=1023,		1<=PDIV<=63,	SDIV = 0, 1, 2, 3
//	PLL1	13<=MDIV<=255,		1<=PDIV<=63,	SDIV = 0, 1, 2, 3
#define PLLFREQ( FIN, PDIV, MDIV, SDIV )	\
				((__int64)((__int64)(MDIV) * (__int64)(FIN))/((PDIV) * (1UL<<SDIV)))


/* 
 * pollux pll calculation is different mp2530f, it is like mp2530f's pll2 calculation
 * 
 * pollux    pll0, pll1 setting
 * fout = (m * fin ) / ( p * 2^s )
 */
unsigned long get_pll_clk( int pllsrc )
{
	unsigned int v, m, p, s;
	unsigned int c = 0;
	
	switch(pllsrc)
	{
		case CLKSRC_PLL0:
			v = PLLSETREG0;
			p = get_0_pdiv(v); m = get_0_mdiv(v);  s = get_0_sdiv(v);
			//p = 24; m = 948; s = 1; // 533Mhz
			//p = 9; m = 256; s = 1; // 384Mhz
			//c = (unsigned long)( (unsigned long long)( (m) * CLOCK_TICK_RATE ) / (unsigned long long)( (p) * (1<<s) ) ); 
			c = ((((CLOCK_TICK_RATE)/p)/(1<<s))*m);	// CLOCK_TICK_RATE(27Mhz) is from timex.h, overflow때문에 이렇게 계산한다.		 
			//c = 380000000;
			break;
		case CLKSRC_PLL1:
			v = PLLSETREG1;
			//p = get_1_pdiv(v); m = get_1_mdiv(v);  s = get_1_sdiv(v); 
			p = get_0_pdiv(v); m = get_0_mdiv(v);  s = get_0_sdiv(v); // ==> because of Databook bug.......
			//c = ( (m) * CLOCK_TICK_RATE ) / ( (p) * (1<<s) );
			c = ((((CLOCK_TICK_RATE)/p)/(1<<s))*m);	// CLOCK_TICK_RATE(27Mhz) is from timex.h, overflow때문에 이렇게 계산한다.		 
			//c = 147000000;
			break;
		default:
			break;
	}
	
	return c;
}		

/*
 * fclk: cpu0(arm926) clock
 * hclk: cpu0's AHB bus
 * bclk: system bus or core clock
 * pclk: peripheral bus clock == bclk/2
 */

/**
 *	@brief	    Set clock for CPU0.
 *  @param[in]  ClkSrc  	Source clock for CPU0.              
 *  @param[in]  CoreDivider	Divider for CPU0 core clock, 0 ~ 15.
 *  @param[in]  BusDivider	Divider for CPU0 Bus clock, 0 ~ 1.  
 *	@return	    None.
 *	@remark	    \n
 *			    - CPU0 core clock = Source clock / (CoreDivider + 1), CoreDivider = 0 ~ 15
 *			    - CPU0 bus clock = CPU0 core clock / (BusDivider + 1), BusDivider = 0 ~ 1
 *	@see	    SetClockCPU1, SetClockBCLK, SetClockSDRAM
 */

static struct pollux_clkmode_reg_t pollux_cmr;


unsigned long pollux_get_cpu0_fclk(void)
{
	unsigned long fclk, srcpll;
	
	srcpll = get_pll_clk(pollux_cmr.cpu0_src_pll);
	fclk = srcpll/(pollux_cmr.cpu0_div+1);
	
	return fclk;
}

unsigned long pollux_get_cpu0_AHB_clk(void)
{
	unsigned long fclk, srcpll;
	
	srcpll = get_pll_clk(pollux_cmr.cpu0_src_pll);
	fclk = srcpll/(pollux_cmr.cpu0_div+1);
	fclk /= (pollux_cmr.cpu0_AHB_div + 1);
	
	return fclk;
}


/**
 *	@brief	    Set clock for BCLK and PCLK.
 *  @param[in]  ClkSrc  	Source clock for BCLK and PCLK.
 *  @param[in]  BCLKDivider	Divider for BCLK, 0 ~ 15.      
 *  @param[in]  PCLKDivider	Divider for PCLK, 0 ~ 1.       
 *	@return	    None.
 *	@remark	    BCLK is system bus clock and PCLK is IO bus clock.
 *			    - BCLK = Source clock / (BCLKDivider + 1), BCLKDivider = 0 ~ 15
 *			    - PCLK = BCLK / (PCLKDivider + 1), PCLKDivider = 0 ~ 1
 *    
 *	@see	    SetClockCPU0, SetClockCPU1, SetClockSDRAM
 */
unsigned long pollux_get_bclk(void)
{
	unsigned long fclk, srcpll;
	
	srcpll = get_pll_clk(pollux_cmr.bclk_src_pll);
	fclk = srcpll/(pollux_cmr.bclk_div+1);
	
	return fclk;
}

unsigned long pollux_get_pclk(void)
{
	unsigned long fclk, srcpll;
	
	srcpll = get_pll_clk(pollux_cmr.bclk_src_pll);
	fclk = srcpll/(pollux_cmr.bclk_div+1);
	fclk /= (2);
	
	return fclk;
}


/**
 *	@brief	    Set clock for SDRAM.
 * bclk's half
 */
unsigned long pollux_get_dramclk(void)
{
	return pollux_get_bclk();
}


/* generate a suggested divider value from a PLL rate */
int pollux_calc_divider(unsigned int pll_hz, unsigned int desired_hz)
{
	int div, rem;

	if(pll_hz == 0 || desired_hz == 0 || desired_hz > pll_hz)
		return -1;

	div = pll_hz/desired_hz;
	rem = pll_hz%desired_hz;

	/*
	 * if 'rem' is greater than (desired_hz / 2) round up
	 * as 'div+1' is a more accurate divisor
	 */
	if ((desired_hz / 2) < rem)
		div++;
	
	return(div);
}
EXPORT_SYMBOL(pollux_calc_divider);



void __init pollux_show_clk(void)
{
	unsigned long c;
	unsigned long clkm = CLKMODEREG;
	get_clkmode_register(clkm, &pollux_cmr); 

    c = get_pll_clk(CLKSRC_PLL0);
	c = get_pll_clk(CLKSRC_PLL1);
	
	c = pollux_get_cpu0_fclk();
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
	printk("ARM926 fclk      = %9lu Hz\n", c);
#endif	
	c = pollux_get_cpu0_AHB_clk();
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE	
	printk("ARM926 AHB clock = %9lu Hz\n", c);
#endif
	c = pollux_get_bclk();
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE	
	printk("       BCLK      = %9lu Hz\n", c);
#endif	
	c = pollux_get_pclk();
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE	
	printk("       PCLK      = %9lu Hz\n", c);
#endif	
	c = pollux_get_dramclk();
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE	
	printk("   DRAM CLK      = %9lu Hz\n", c);
#endif
}
	
EXPORT_SYMBOL(get_pll_clk);	
EXPORT_SYMBOL(pollux_get_pclk);

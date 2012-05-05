/*
 * arch/arm/mach-mp2530/clock_control.c
 *
 * Driver for general clock control on the Magiceyes MP2530
 *
 * Based on Magiceyes's WinCE BSP mes_iclockcontrol.cpp
 *
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho)     - main programmer
 * amphell(Bang Chang Hyeok)    - Hardware Engineer
 *
 * 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/mp2530/index.html
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
 
#include <asm/arch/regs-clock-control.h>


static struct clock_regset_t *m_pClockControlRegister = NULL;


void mp2530f_set_clock_base_register(unsigned long clkbase)
{
	m_pClockControlRegister = (struct clock_regset_t *)clkbase;
}

void mp2530f_unset_clock_base_register(void)
{
	m_pClockControlRegister = NULL;
}


//------------------------------------------------------------------------------
/**
 *  @brief      Set Periperal Bus Clock's operation Mode
 *  @param[in]  ClkMode     PCLK Mode
 *  @return     None.
 *  @see        GetClockPClkMode
 */
void mp2530f_set_clock_pclk_mode( PCLKMODE ClkMode )
{
	U32 clkmode;
	U32 CLKENB;
	
	switch(ClkMode)
	{
		case PCLKMODE_ONLYWHENCPUACCESS: clkmode = 0;		break;
		case PCLKMODE_ALWAYS:  			 clkmode = 1;		break;
		default: printk("input value error\n"); return;
	}
	
	CLKENB = m_pClockControlRegister->CLKENB;
	CLKENB &= ~(1<<3);
	CLKENB |= (clkmode & 0x1) << 3;
	m_pClockControlRegister->CLKENB = CLKENB;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set System Bus Clock's operation Mode
 *  @param[in]  ClkMode     BCLK Mode
 *  @return     None.
 *  @see        GetClockBClkMode
 */
void mp2530f_set_clock_bclk_mode( BCLKMODE ClkMode )
{
	U32 clkmode;
	U32 CLKENB;
	
	switch(ClkMode)
	{
	case BCLKMODE_DISABLE: 	clkmode = 0;		break;
	case BCLKMODE_DYNAMIC:	clkmode = 2;		break;
	case BCLKMODE_ALWAYS:	clkmode = 3;		break;
	default: printk("input value error\n"); return;
	}
	
	CLKENB = m_pClockControlRegister->CLKENB;
	CLKENB &= ~(0x3);
	CLKENB |= clkmode & 0x3;
	m_pClockControlRegister->CLKENB = CLKENB;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set of clock Divisor's Usage ( Enable or Disable )
 *  @param[in]  Enable  CTRUE indicate clock divisor use, CFASE indicate clock divisor Not use.
 *  @return     None.
 *  @see        GetClockDivisorEnable
 */
void mp2530f_set_clock_divisor_enable(CBOOL Enable )
{
	
	U32 CLKENB = m_pClockControlRegister->CLKENB;
	CLKENB &= ~(0x1<<2);
	CLKENB |= ((Enable) ? (1<<2) : 0);
	m_pClockControlRegister->CLKENB = CLKENB;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock divisor's value.
 *  @param[in]  Index       Select clock generator
 *  @param[in]  Divisor     Divide value of clock
 *  @return     None.
 *  @remarks    Each module have different number of clock generator and range of Divider So
 *              identify each module's state.
 *  @see        GetClockDivisor
 */
void mp2530f_set_clock_divisor( U32 Index, U32 Divisor ) // ghcstop: index가 0만 되는 넘이 있고, 1까지 되는 넘이 있으니 조심할 것.
{
	U32 CLKGEN = m_pClockControlRegister->CLKGEN[Index];
	CLKGEN &= ~0x00000FF0UL;
	CLKGEN |= (Divisor-1) << 4;
	m_pClockControlRegister->CLKGEN[Index] = CLKGEN;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set clock source for operation
 *  @param[in]  Index       Select clock generator
 *  @param[in]  ClkSrc      Select clock source
 *  @return     None.
 *  @remarks    Each module have different number of clock generator and clock source. So
 *              identify each module's state.
 *  @see        GetClockSource
 */
void mp2530f_set_clock_source( U32 Index, U32 ClkSrc ) // clksrc must be <= 7
{
	U32 CLKGEN = m_pClockControlRegister->CLKGEN[Index];
	CLKGEN &= ~(0x7<<1);
	CLKGEN |= (ClkSrc & 0x7) << 1;
	m_pClockControlRegister->CLKGEN[Index] = CLKGEN;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set output clock's polarity
 *  @param[in]  Index       Select clock generator
 *  @param[in]  OutClkInv   CTRUE indicate clock polarity invert.\n
 *                          CFALSe indicate clock polarity bypass.
 *  @return     None.
 *  @remarks    Each module have different number of clock generator. So
 *              identify each module's state.
 *  @see        GetClockOutInv
 */
void mp2530f_set_clock_out_inv( U32 Index, CBOOL OutClkInv )
{
	U32 CLKGEN = m_pClockControlRegister->CLKGEN[Index];
	CLKGEN &= ~0x1;
	CLKGEN |= (OutClkInv ? 1 : 0);
	m_pClockControlRegister->CLKGEN[Index] = CLKGEN;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set output clock's operation
 *  @param[in]  Index       Select clock generator
 *  @param[in]  OutClkEnb   CTRUE indicate output clock enable.\n
 *                          CFALSE indicate output clock disable.
 *  @return     None.
 *  @remarks    Each module have different number of clock generator. So
 *              identify each module's state.
 *  @see        GetClockOutEnb
 */
void mp2530f_set_clock_out_enb( U32 Index, CBOOL OutClkEnb )
{
	U32 CLKGEN = m_pClockControlRegister->CLKGEN[Index];
	CLKGEN &= ~(0x1<<15);
	CLKGEN |= ((OutClkEnb) ? 0 : 1<<15);
	m_pClockControlRegister->CLKGEN[Index] = CLKGEN;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Set output clock's delay time.
 *  @param[in]  Index   Select clock generator
 *  @param[in]  delay   delay time of output clock
 *  @return     None.
 *  @remarks    Each module have different number of clock generator. So
 *              identify each module's state.
 *  @see        GetClockOutDelay
 */
void mp2530f_set_clock_out_delay( U32 Index, U32 delay ) // delay must be <=8
{
	U32 CLKGEN = m_pClockControlRegister->CLKGEN[Index];
	CLKGEN &= ~(0x7<<12);
	CLKGEN |= ( ((U32)(delay)) & 0x7 ) << 12;
	m_pClockControlRegister->CLKGEN[Index] = CLKGEN;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get Periperal Bus Clock's operation Mode
 *  @return     PCLK Mode
 *  @see        SetClockPClkMode
 */
PCLKMODE mp2530f_get_clock_pclk_mode( void )
{
	U32 PCLKMODE	= 1UL<<3;
	if( m_pClockControlRegister->CLKENB & PCLKMODE )
		return PCLKMODE_ALWAYS;
		
	return PCLKMODE_ONLYWHENCPUACCESS;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get System Bus Clock's operation Mode
 *  @return     BCLK Mode
 *  @see        SetClockBClkMode
 */
BCLKMODE mp2530f_get_clock_bclk_mode ( void )
{
	U32 BCLKMODE	= 3UL<<0;
	switch( m_pClockControlRegister->CLKENB & BCLKMODE )
	{
	case 0 :	return BCLKMODE_DISABLE;
	case 2 :	return BCLKMODE_DYNAMIC;
	case 3 :	return BCLKMODE_ALWAYS;
	}
	return BCLKMODE_DISABLE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get of clock Divisor's Usage 
 *  @return     CTRUE indicate clock divisor is enabled.\n
 *              CFALSE indicate clock divisor is disabled.
 *  @see        SetClockDivisorEnable
 */
CBOOL mp2530f_get_clock_divisor_enable( void )
{
	U32 CLKGENENB	= 1UL<<2;
	return (m_pClockControlRegister->CLKENB & CLKGENENB) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock divisor's value.
 *  @param[in] Index       Select clock generator
 *  @return     Divide value of selected clock generator 
 *  @see        GetClockDivisor
 */
U32 mp2530f_get_clock_divisor( U32 Index )
{
	U32 CLKDIV_POS	= 4;
	U32 CLKDIV_MASK	= 0xFFUL<<CLKDIV_POS;
	return (U32)((m_pClockControlRegister->CLKGEN[Index] & CLKDIV_MASK)>>CLKDIV_POS) + 1;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get clock source of clock generator
 *  @param[in]  Index       Select clock generator
 *  @return     Clock source
 *  @see        SetClockSource
 */
U32 mp2530f_get_clock_source( U32 Index )
{
	U32 CLKSRCSEL_POS		= 1;
	U32 CLKSRCSEL_MASK	= 0x7UL<<CLKSRCSEL_POS;
	return (U32)((m_pClockControlRegister->CLKGEN[Index] & CLKSRCSEL_MASK)>>CLKSRCSEL_POS);
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get output clock's polarity
 *  @param[in]  Index       Select clock generator
 *  @return     CTRUE indicate clock polarity is inverted.\n
 *              CFALSe indicate clock polarity is bypassed.
 *  @see        SetClockOutInv
 */
CBOOL mp2530f_get_clock_out_inv( U32 Index )
{
	U32 OUTCLKINV	= 1UL<<0;
	return (m_pClockControlRegister->CLKGEN[Index] & OUTCLKINV) ? CTRUE : CFALSE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get output clock's operation
 *  @param[in]  Index       Select clock generator
 *  @return     CTRUE indicate output clock is enabled.\n
 *              CFALSE indicate output clock is disabled.
 *  @see        SetClockOutEnb
 */
CBOOL mp2530f_get_clock_out_enb( U32 Index )
{
	U32 OUTCLKENB	= 1UL<<15;
	return (m_pClockControlRegister->CLKGEN[Index] & OUTCLKENB) ? CFALSE : CTRUE;
}

//------------------------------------------------------------------------------
/**
 *  @brief      Get output clock's delay time.
 *  @param[in]  Index   Select clock generator
 *  @return     Delay time of output clock.
 *  @see        SetClockOutDelay
 */
U32 mp2530f_get_clock_out_delay( U32 Index )
{
	U32 OUTCLKDELAY_POS	= 12;
	U32 OUTCLKDELAY_MASK	= 7UL<<OUTCLKDELAY_POS;
	return ((m_pClockControlRegister->CLKGEN[Index] & OUTCLKDELAY_MASK)>>OUTCLKDELAY_POS);
}



EXPORT_SYMBOL(mp2530f_set_clock_base_register);
EXPORT_SYMBOL(mp2530f_unset_clock_base_register);
EXPORT_SYMBOL(mp2530f_set_clock_pclk_mode);
EXPORT_SYMBOL(mp2530f_set_clock_bclk_mode);
EXPORT_SYMBOL(mp2530f_set_clock_divisor_enable);
EXPORT_SYMBOL(mp2530f_set_clock_divisor); // ghcstop: index가 0만 되는 넘이 있고, 1까지 되는 넘이 있으니 조심할 것.
EXPORT_SYMBOL(mp2530f_set_clock_source); // clksrc must be <= 7
EXPORT_SYMBOL(mp2530f_set_clock_out_inv);
EXPORT_SYMBOL(mp2530f_set_clock_out_enb);
EXPORT_SYMBOL(mp2530f_set_clock_out_delay); // delay must be <=8
EXPORT_SYMBOL(mp2530f_get_clock_pclk_mode);
EXPORT_SYMBOL(mp2530f_get_clock_bclk_mode);
EXPORT_SYMBOL(mp2530f_get_clock_divisor_enable);
EXPORT_SYMBOL(mp2530f_get_clock_divisor);
EXPORT_SYMBOL(mp2530f_get_clock_source);
EXPORT_SYMBOL(mp2530f_get_clock_out_inv);
EXPORT_SYMBOL(mp2530f_get_clock_out_enb);
EXPORT_SYMBOL(mp2530f_get_clock_out_delay);


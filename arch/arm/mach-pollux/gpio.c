/*
 * arch/arm/mach-pollux/gpio.c
 *
 * Driver for onboard GPIO on the Magiceyes POLLUX
 *
 * Based on arch/arm/mach-s3c2410/gpio.c
 *
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho)     - main programmer
 * amphell(Bang Chang Hyeok)    - Hardware Engineer
 *
 * 2003-2008 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/pollux/index.html
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/arch/regs-gpio.h>

/* completly not tested ==> all function test later */

// set pin to input function: gpio/alt1/alt2
void pollux_set_gpio_func(unsigned int pin, enum POLLUX_GPIO_MODE function)
{
	void __iomem *base = (void __iomem *)POLLUX_GPIO_BASE(pin);
	void __iomem *regs;
	int offset = POLLUX_GPIO_OFFSET(pin);
	unsigned long mask;
	unsigned long con;
	unsigned long flags;
	
	
	if( offset < 16 ) // 0 ~ 15 pin
	{
		regs = (base+POLLUX_GPIO_FUNC0);
	}
	else // >= 16 pin
	{
		regs    = (base+POLLUX_GPIO_FUNC1);
		offset -= 16;
	}

	mask       = 3 << (offset*2);  // mask for 2bit field control
	function  &= 3;                // input value has control 2 bit.
	function <<= (offset)*2;

	local_irq_save(flags);

	con  = __raw_readl(regs);   
	con &= ~mask;    // 2bit clear
	con |= function; // 2bit setting

	__raw_writel(con, regs);

	local_irq_restore(flags);
}


EXPORT_SYMBOL(pollux_set_gpio_func);

unsigned long pollux_get_gpio_func(unsigned int pin)
{
	void __iomem *base = (void __iomem *)POLLUX_GPIO_BASE(pin);
	void __iomem *regs;
	int offset = POLLUX_GPIO_OFFSET(pin);
	unsigned long val;

	if( offset < 16 ) // 0 ~ 15 pin
	{
		regs = (base+POLLUX_GPIO_FUNC0);
	}
	else // >= 16 
	{
		regs    = (base+POLLUX_GPIO_FUNC1);
		offset -= 16;
	}

	val = __raw_readl(regs);

	val >>= (offset*2);
	val &= 3;

	return val;
}

EXPORT_SYMBOL(pollux_get_gpio_func);

void pollux_gpio_pullup(unsigned int pin, unsigned int to)
{
	void __iomem *base = (void __iomem *)POLLUX_GPIO_BASE(pin);
	unsigned long offs = POLLUX_GPIO_OFFSET(pin);
	unsigned long flags;
	unsigned long up;

	local_irq_save(flags);

	up = __raw_readl(base + POLLUX_GPIO_PUENB);
	up &= ~(1L << offs);
	up |= (to << offs);
	__raw_writel(up, base + POLLUX_GPIO_PUENB);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(pollux_gpio_pullup);


void pollux_gpio_set_inout(unsigned int pin, unsigned int to)
{
	void __iomem *base = (void __iomem *)POLLUX_GPIO_BASE(pin);
	unsigned long offs = POLLUX_GPIO_OFFSET(pin);
	unsigned long flags;
	unsigned long dat;

	local_irq_save(flags);

	dat = __raw_readl(base + POLLUX_GPIO_OUTENB);
	dat &= ~(1 << offs);
	dat |= to << offs;
	__raw_writel(dat, base + POLLUX_GPIO_OUTENB);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(pollux_gpio_set_inout);


void pollux_gpio_setpin(unsigned int pin, unsigned int to)
{
	void __iomem *base = (void __iomem *)POLLUX_GPIO_BASE(pin);
	unsigned long offs = POLLUX_GPIO_OFFSET(pin);
	unsigned long flags;
	unsigned long dat;

	local_irq_save(flags);

	dat = __raw_readl(base + POLLUX_GPIO_OUT);
	dat &= ~(1 << offs);
	dat |= to << offs;
	__raw_writel(dat, base + POLLUX_GPIO_OUT);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(pollux_gpio_setpin);

unsigned int pollux_gpio_getpin(unsigned int pin)
{
	void __iomem *base = (void __iomem *)POLLUX_GPIO_BASE(pin);
	unsigned long offs = POLLUX_GPIO_OFFSET(pin);

    //return __raw_readl(base + POLLUX_GPIO_PAD) & (1<< offs); // return valus is  0 or 1
	
	if( __raw_readl(base + POLLUX_GPIO_PAD) & (1<< offs) )
	{
		return 1;
	}
	else
		return 0; // return is 0 or 1
}

EXPORT_SYMBOL(pollux_gpio_getpin);

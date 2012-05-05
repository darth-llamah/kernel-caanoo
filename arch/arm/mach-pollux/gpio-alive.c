/*
 * arch/arm/mach-pollux/gpioalv.c
 *
 * Driver for onboard GPIO alive on the Magiceyes POLLUX
 *
 * Based on Magiceyes wince code
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

#include <asm/arch/regs-gpioalv.h>


/*
 * Get status of Alive GPIO is writable or Not.
 * 
 * 1: indicate that writing data to Alive GPIO is Enable. 
 * 0: indicate that writing data to Alive GPIO is Disable.
 */
void pollux_gpioalv_set_write_enable( int enable ) 
{
	GPIOALV_PWRGATE  =  ( (u32)enable & 0x01 );
}

/*
 * Get status of Alive GPIO is writable or Not.
 *
 * return      1: indicate that Writing data to Alive GPIO is enabled.
 *             0: indicate that Writing data to Alive GPIO is disabled.
 */
int   pollux_gpioalv_get_write_enable( void )
{
	int val;
	
	val = GPIOALV_PWRGATE & (1<<0);
	
	return val;
}

/*
 *  Set VDD Power's on/off
 *  input     1 indicate that VDD Power ON. 
 *            0 indicate that VDD Power OFF.
 */
void pollux_gpioalv_set_VDD_power(int enable )
{
	GPIOALV_GPIOSET = 0;
	GPIOALV_GPIORST = 0;

	if( enable )	GPIOALV_GPIOSET = (1<<7);
	else			GPIOALV_GPIORST = (1<<7);

	GPIOALV_GPIOSET = 0;
	GPIOALV_GPIORST = 0;
}

 
/*
 * Get VDD Power's status
 *
 * return: 1 indicate that VDD Power is on. 
 *         0 indicate that VDD Power is off.
 */
int pollux_gpioalv_get_VDD_power(void)
{
	return (int)( (GPIOALV_GPIOREAD&(1<<7)) >> 7 ); 
}

 
/*
 * Set Alive Gpio's Output value.
 * 
 * Input
 *  num   : Alive GPIO Number( 0 ~ 6 ).
 *  enabme: 1 indicate that Alive GPIO's output value is High value.  
 *          0 indicate that Alive GPIO's output value is Low value.
 */
void pollux_gpioalv_set_pin(int num, int enable)
{
	if( num >= 7 )
	{
		printk(KERN_ERR "pollux_set_gpioalv_pin(): gpioalv pin number error ");
		return ;
	}

	GPIOALV_GPIOSET = 0;
	GPIOALV_GPIORST = 0;

	if( enable )	GPIOALV_GPIOSET = (1<<num);
	else			GPIOALV_GPIORST = (1<<num);

	GPIOALV_GPIOSET = 0;
	GPIOALV_GPIORST = 0;
}

 
/*
 * Get Alive Gpio's Output status.
 * 
 * Input : num Alive GPIO Number( 0 ~ 6 ).
 * Return: 1 indicate that Alive GPIO's output value is High value.  
 *         0 indicate that Alive GPIO's output value is Low value.
 */
int pollux_gpioalv_get_pin( int num )
{
	int val;
	
	if( num >= 7 )
	{
		printk(KERN_ERR "pollux_set_gpioalv_pin(): gpioalv pin number error ");
		return -1;
	}
	
	val = (GPIOALV_GPIOREAD >> num ) & 0x01 ;

	return val;
}

 
/*
 * Set scratch register
 */
void pollux_gpioalv_set_scratchreg( u32 val )
{
	GPIOALV_SCRATCHSET = val;
	GPIOALV_SCRATCHRST = ~val;    
	GPIOALV_SCRATCHSET = 0x0;
	GPIOALV_SCRATCHRST = 0x0;    
}

 
/*
 * Get data of scratch register.
 */
u32 pollux_gpioalv_get_scratchreg(void)
{
	u32 val;
	
	val = GPIOALV_SCRATCHREAD;
	
	return val;
}

 
/*
 * Get VDD power toggle status.
 *
 * Return: 1 indicate that VDDPWRTOGGLE PAD is toggled.  
 *         0 indicate that VDDPWRTOGGLE PAD is   NOT toggled. 
 */

int pollux_gpioalv_get_vdd_power_toggled(void)
{
	u32 mask = (1 << 8);
	int val;
	
	val = (GPIOALV_GPIOREAD & mask ) >> 8;

	return val;
}


EXPORT_SYMBOL(pollux_gpioalv_set_write_enable);
EXPORT_SYMBOL(pollux_gpioalv_get_write_enable);
EXPORT_SYMBOL(pollux_gpioalv_set_VDD_power);
EXPORT_SYMBOL(pollux_gpioalv_get_VDD_power);
EXPORT_SYMBOL(pollux_gpioalv_set_pin);
EXPORT_SYMBOL(pollux_gpioalv_get_pin);
EXPORT_SYMBOL(pollux_gpioalv_set_scratchreg);
EXPORT_SYMBOL(pollux_gpioalv_get_scratchreg);
EXPORT_SYMBOL(pollux_gpioalv_get_vdd_power_toggled);

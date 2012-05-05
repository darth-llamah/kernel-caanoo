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
 * This program is free software  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation  either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program  if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __ASM_ARM_REGS_GPIOALV_H
#define __ASM_ARM_REGS_GPIOALV_H
 
#include <asm/hardware.h>


#define GPIOALV_PWRGATE      __REG32(0xC0019000)      // 0x00 : Alive Power Gating Register.
#define GPIOALV_GPIORST      __REG32(0xC0019004)      // 0x04 : Alive GPIO Reset Register.
#define GPIOALV_GPIOSET      __REG32(0xC0019008)      // 0x08 : Alive GPIO Set Register.
#define GPIOALV_GPIOREAD     __REG32(0xC001900c)      // 0x0C : Alive GPIO Read Register.
#define GPIOALV_SCRATCHRST   __REG32(0xC0019010)      // 0x10 : Alive Scratch Reset Register.
#define GPIOALV_SCRATCHSET   __REG32(0xC0019014)      // 0x14 : Alive Scratch Set Register.
#define GPIOALV_SCRATCHREAD  __REG32(0xC0019018)      // 0x18 : Alive Scratch Read Register.


void pollux_gpioalv_set_write_enable(int enable);
int  pollux_gpioalv_get_write_enable(void);
void pollux_gpioalv_set_VDD_power(int enable );
int  pollux_gpioalv_get_VDD_power(void);
void pollux_gpioalv_set_pin(int num, int enable);
int  pollux_gpioalv_get_pin( int num );
void pollux_gpioalv_set_scratchreg( u32 val );
u32  pollux_gpioalv_get_scratchreg(void);
int  pollux_gpioalv_get_vdd_power_toggled(void);


#endif /* __ASM_ARM_REGS_GPIOALV_H */

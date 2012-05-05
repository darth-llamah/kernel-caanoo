/*
 * linux/include/asm-arm/arch-pollux/regs-interrupt.h
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
#ifndef __ASM_ARM_REGS_INTERRUPT_H
#define __ASM_ARM_REGS_INTERRUPT_H

#include <asm/hardware.h>

/*
 * Chapter 7 
 * Interrupt Controller
 */
#define INTMODEL		    __REG32(0xC0000808)	// interrupt mode register low(IRQ냐 FIQ냐 결정)
#define INTMODEH		    __REG32(0xC000080C)	// interrupt mode register high(used LSB 10bit)
#define INTMASKL		    __REG32(0xC0000810)	// interrupt mask register low
#define INTMASKH		    __REG32(0xC0000814)	// interrupt mask register high(used LSB 10bit)
#define PRIORDER		    __REG32(0xC0000818)	// priority order register
#define INTPENDL		    __REG32(0xC0000820)	// interrupt pending register low
#define INTPENDH		    __REG32(0xC0000824)	// interrupt pending register high(used LSB 10bit)





#endif /* __REGS_INTERRUPT_H */

/* 
 * linux/include/asm-arm/arch-mp2530/io.h
 *  from linux/include/asm-arm/arch-rpc/io.h
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
#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

//#include <asm/hardware.h>

#define IO_SPACE_LIMIT 0xffffffff

#define PCIO_BASE            0

/*
 * We don't actually have real ISA nor PCI buses, but there is so many
 * drivers out there that might just work if we fake them...
 */
#define __io(a)			(PCIO_BASE + (a))
#define __mem_pci(a)	((unsigned long)(a))
#define __mem_isa(a)	((unsigned long)(a))

/*
 * Genenric virtual read/write
 */
//#define __arch_getw(a)		(*(volatile unsigned short *)(a))
//#define __arch_putw(v,a)	(*(volatile unsigned short *)(a) = (v))

#define iomem_valid_addr(iomem,sz)	(1)
#define iomem_to_phys(iomem)		(iomem)


#endif

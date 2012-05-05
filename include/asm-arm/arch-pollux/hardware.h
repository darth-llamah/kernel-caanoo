/* 
 * linux/include/asm-arm/arch-mp2530/hardware.h
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
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#ifndef __ASM_HARDWARE_H
#error "Do not include this directly, instead #include <asm/hardware.h>"
#endif

#ifndef __ASSEMBLY__
	/* bit masking: ghcstop add from LF1000 kernel */
	#define BIT_SET(v,b)	(v |= (1<<(b)))
	#define BIT_CLR(v,b)	(v &= ~(1<<(b)))
	#define IS_SET(v,b)	(v & (1<<(b)))
	#define IS_CLR(v,b)	!(v & (1<<(b)))
	//#define BIT_MASK(b)	((1<<(b))-1)

#endif /* __ASSEMBLY__ */

#include <asm/sizes.h>
#include <asm/arch/map.h>


#endif /* __ASM_ARCH_HARDWARE_H */

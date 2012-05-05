/*
 * linux/include/asm-arm/arch-mp2530/memory.h
 *  from linux/include/asm-arm/arch-rpc/memory.h
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
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#include <asm/hardware.h>

/*
 * DRAM starts at 0x00000000 for pollux shadow == 1
 * DRAM starts at 0x80000000 for pollux shadow == 0
 */
#ifdef CONFIG_POLLUX_SHADOW_ONE
	#define PHYS_OFFSET	UL(0x00000000) // ghcstop fix
#else
	#define PHYS_OFFSET	UL(0x80000000) // ghcstop fix
#endif	


/*
 * These are exactly the same on the S3C2410 as the
 * physical memory view.
*/

#define __virt_to_bus(x) __virt_to_phys(x)
#define __bus_to_virt(x) __phys_to_virt(x)


/*
 * The nodes are matched with the physical SDRAM banks as follows:
 *
 * 	node 0:  0x00000000-0x3fffffff	-->  0xc0000000-0xc3ffffff
 * 	node 1:  0xa4000000-0xa7ffffff	-->  0xc4000000-0xc7ffffff
 * 	node 2:  0xa8000000-0xabffffff	-->  0xc8000000-0xcbffffff
 * 	node 3:  0xac000000-0xafffffff	-->  0xcc000000-0xcfffffff
 *
 * This needs a node mem size of 26 bits.
 */
 
#define NODE_MEM_SIZE_BITS	27 //ghcstop_numa ==> can not applied bacause of mp2530's memory range


#endif

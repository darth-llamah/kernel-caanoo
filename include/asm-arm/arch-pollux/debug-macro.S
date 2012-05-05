/* 
 * linux/include/asm-arm/arch-mp2530/debug-macro.S
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
#include <asm/hardware.h>

#define UTRSTAT_TX_EMPTY    (0x4)

        .macro  addruart,rx
        mrc p15, 0, \rx, c1, c0
        tst \rx, #1         @ MMU enabled?
        moveq   \rx, #0xc0000000    @ physical
        movne   \rx, #0xf0000000    @ virtual
        orr \rx, \rx, #0x00016000   @ uart 0 base address
        orr \rx, \rx, #0x00000080   @ uart 1 base address
        @orr \rx, \rx, #0x00000880   @ uart 3 base address
        .endm

        .macro  senduart,rd,rx
        strh    \rd, [\rx, #0x10]
        .endm

        .macro  busyuart,rd,rx
        nop
        .endm

        .macro  waituart,rd,rx
        mov \rd, \rx
1001:   ldrh    \rd, [\rx, #0x8]
        and \rd, \rd, #UTRSTAT_TX_EMPTY
        tst \rd, #UTRSTAT_TX_EMPTY
        beq 1001b
        .endm




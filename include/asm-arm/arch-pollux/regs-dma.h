/*
 * include/asm-arm/arch-mp2530/regs-dma.h
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
#ifndef __ASM_ARM_REGS_DMA_H
#define __ASM_ARM_REGS_DMA_H

#include <asm/arch/wincetype.h>

/* DMA */
#define POLLUX_DMA_BASE(x)   (POLLUX_VA_DMA+(x)*0x80)

#define POLLUX_DMA_SRCADDR   (0x00)
#define POLLUX_DMA_DSTADDR   (0x04)
#define POLLUX_DMA_LENGTH    (0x08)
#define POLLUX_DMA_REQESTID  (0x0a) 
#define POLLUX_DMA_MODE      (0x0c) 


#define POLLUX_DMA_INTPEND   (1<<17)
#define POLLUX_DMA_INTMASK   (1<<18)


#define POLLUX_NUM_DMA       (8) 

#define MAX_TRANSFER_SIZE    (0x10000)

/// @brief	DMA Transfer Mode.
enum OPMODE
{
	OPMODE_MEM_TO_MEM	= 0,	///< Memory to Memory operation
	OPMODE_MEM_TO_IO	= 1,	///< Memory to Peripheral operation
	OPMODE_IO_TO_MEM	= 2,	///< Peripheral to Memory operation
	OPMODE_FORCED32		= 0x7FFFFFFF
};

//------------------------------------------------------------------------------
/// @brief	DMA03 register list.
//------------------------------------------------------------------------------
struct dma_regset_t
{
	volatile U32 DMASRCADDR;	///< 0x00 : Source Address Register
	volatile U32 DMADSTADDR;	///< 0x04 :	Destination Address Register
	volatile U16 DMALENGTH;		///< 0x08 :	Transfer Length Register
	volatile U16 DMAREQID;		///< 0x0A : Peripheral ID Register
	volatile U32 DMAMODE;		///< 0x0C : DMA Mode Register
};


//------------------------------------------------------------------------------
// DMA peripheral index of modules for the DMA controller.
//------------------------------------------------------------------------------
enum {
	DMAINDEX_OF_UART0_MODULE_TX 		=  0,
	DMAINDEX_OF_UART0_MODULE_RX 		=  1,
	DMAINDEX_OF_UART1_MODULE_TX 		=  2,
	DMAINDEX_OF_UART1_MODULE_RX 		=  3,
	DMAINDEX_OF_UART2_MODULE_TX 		=  4,
	DMAINDEX_OF_UART2_MODULE_RX	 		=  5,
	DMAINDEX_OF_UART3_MODULE_TX 		=  6,
	DMAINDEX_OF_UART3_MODULE_RX 		=  7,
	DMAINDEX_OF_USBDEVICE_MODULE_EP1 	= 12,
	DMAINDEX_OF_USBDEVICE_MODULE_EP2 	= 13,
	DMAINDEX_OF_SDHC0_MODULE 			= 16,
	DMAINDEX_OF_SSPSPI0_MODULE_TX 		= 18,
	DMAINDEX_OF_SSPSPI0_MODULE_RX 		= 19,
	DMAINDEX_OF_SSPSPI1_MODULE_TX 		= 20,
	DMAINDEX_OF_SSPSPI1_MODULE_RX 		= 21,
	DMAINDEX_OF_AUDIO_MODULE_PCMOUT 	= 24,
	DMAINDEX_OF_AUDIO_MODULE_PCMIN 		= 26,
	DMAINDEX_OF_SDHC1_MODULE 			= 30,
	DMAINDEX_OF_MCUS_MODULE 			= 31

};
	


#endif // __ASM_ARM_REGS_DMA_H


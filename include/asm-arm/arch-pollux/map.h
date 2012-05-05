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
#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H

#include <linux/autoconf.h>


#define HEX_1M    0x00100000
#define HEX_512K  0x00080000
#define HEX_256K  0x00040000
#define HEX_128K  0x00020000
#define HEX_64K   0x00010000

#if 0
#define CLKSRC_PLL0 0
#define CLKSRC_PLL1 1
#define CLKSRC_PLL2 2
#endif


/*
 * mp2530 internal I/O mappings
 *
 * We have the following mapping:
 *
 * 	phys		virt
 * 	c0000000	f0000000
 *
 */
#define VIO_BASE	0xf0000000 /* virtual start of IO space */
#define PIO_BASE	0xc0000000 /* physical start of IO space */

#define io_p2v(x)	((x) | 0x30000000)
#define io_v2p(x)	((x) & ~0x30000000)


#define NF_PIO_BASE		0x9c000000 /* physical start of NAND flash IO space */
#define nf_p2v(x)	((x) | 0x60000000)
#define nf_v2p(x)	((x) & ~0x60000000)



#ifndef __ASSEMBLY__

/*
 * This __REG() version gives the same results as the one above,  except
 * that we are fooling gcc somehow so it generates far better and smaller
 * assembly code for access to contigous registers.  It's a shame that gcc
 * doesn't guess this by itself
 */
#include <asm/types.h>
typedef struct { volatile u32 offset[4096]; } __regbase;

#define __REGP(x)	((__regbase *)((x)&~4095))->offset[((x)&4095)>>2]
#define __REG(x)	__REGP(io_p2v(x))


#define __REG32(x)	(*(volatile u32 *)io_p2v(x))
#define __REG16(x)	(*(volatile u16 *)io_p2v(x))
#define __REG8(x)	(*(volatile u8 *)io_p2v(x))	
#define __PREG(x)	(io_v2p((u32)&(x))) // register as physical address(using pointer)

#else // pollux not use this routine

#define __REG(x)	io_p2v(x)
#define __REG32(x)	io_p2v(x)
#define __REG16(x)	io_p2v(x)
#define __REG8(x)	io_p2v(x)
#define __PREG(x)	io_v2p(x)

#endif /* __ASSEMBLY__ */


#ifndef __ASSEMBLY__
	// ghcstop: 040716, from mes_enum.h
	/// @brief 	Clock Sources
	enum CLKSRC
	{
		CLKSRC_PLL0 	= 0,	///< PLL0
		CLKSRC_PLL1 	= 1,	///< PLL1
		CLKSRC_FORCE32  = 0x7fffffff,
		
		// mes_dpc03
		CLKSRC_FPLL     	= 0UL,	///< PLL0
		CLKSRC_UPLL     	= 1UL,	///< PLL1
		CLKSRC_APLL     	= 2UL,	///< PLL2
		CLKSRC_PSVCLK   	= 3UL,	///< P(S)VCLK pad (when VCLK is input)
		CLKSRC_INVPSVCLK	= 4UL,	///< inversion of P(S)VCLK pad
		CLKSRC_XTI			= 5UL,	///< XTI (Main Crystal input)
		CLKSRC_AVCLK    	= 6UL,	///< AVCLK pad
		CLKSRC_CLKGEN0		= 7UL,	///< CLKGEN0 output, this is only vaild for CLKGEN1
        
		// mes_pwm03
		CLKSRC_INVALID     = 3, 
	};

	#define POLLUX_ADDR(x)	  (VIO_BASE + (x))
#else
	#define POLLUX_ADDR(x)	  (VIO_BASE + (x))
#endif	

#ifdef CONFIG_POLLUX_SHADOW_ONE 
    /* nand boot */
	#define POLLUX_DRAM_PA       (0x00000000)
	#define POLLUX_CS_BASE_PA    (0x80000000)
#else                   
    /* NOR boot */
	#define POLLUX_DRAM_PA       (0x80000000)
	#define POLLUX_CS_BASE_PA    (0x00000000)
#endif	


#define POLLUX_PA_CS0  (POLLUX_CS_BASE_PA+0x00000000)
#define POLLUX_PA_CS1  (POLLUX_CS_BASE_PA+0x04000000)
#define POLLUX_PA_CS2  (POLLUX_CS_BASE_PA+0x08000000)
#define POLLUX_PA_CS3  (POLLUX_CS_BASE_PA+0x0c000000)
#define POLLUX_PA_CS4  (POLLUX_CS_BASE_PA+0x10000000)
#define POLLUX_PA_CS5  (POLLUX_CS_BASE_PA+0x14000000)
#define POLLUX_PA_CS6  (POLLUX_CS_BASE_PA+0x18000000)
#define POLLUX_PA_CS7  (POLLUX_CS_BASE_PA+0x1c000000)
#define POLLUX_PA_CS8  (POLLUX_CS_BASE_PA+0x20000000)
#define POLLUX_PA_CS9  (POLLUX_CS_BASE_PA+0x24000000)
#define POLLUX_PA_CS10 (POLLUX_CS_BASE_PA+0x2C000000) /* NAND area */


/* UARTs */
#define POLLUX_VA_UART	   POLLUX_ADDR(0x00016000)
#define POLLUX_PA_UART	   (0xC0016000) 

/* GPIO */
#define POLLUX_VA_GPIO	   POLLUX_ADDR(0x0000A000)
#define POLLUX_PA_GPIO	   (0xC000A000) 

/* SDI */
#define POLLUX_VA_SDI0	   POLLUX_ADDR(0x00009800)
#define POLLUX_PA_SDI0	   (0xC0009800) 
#define POLLUX_VA_SDI1	   POLLUX_ADDR(0x0000C800)
#define POLLUX_PA_SDI1	   (0xC000C800) 

/* IIS/sound */
#define POLLUX_VA_SOUND	   POLLUX_ADDR(0x0000D800)
#define POLLUX_PA_SOUND	   (0xC000D800) 

/* DPC */
#define POLLUX_VA_DPC_PRI	   POLLUX_ADDR(0x00003000)
#define POLLUX_PA_DPC_PRI	   (0xC0003000) 
#define POLLUX_VA_DPC_SEC	   POLLUX_ADDR(0x00003400)
#define POLLUX_PA_DPC_SEC	   (0xC0003400) 

/* MLC */
#define POLLUX_VA_MLC_PRI	   POLLUX_ADDR(0x00004000)
#define POLLUX_PA_MLC_PRI	   (0xC0004000) 
#define POLLUX_VA_MLC_SEC	   POLLUX_ADDR(0x00004400)
#define POLLUX_PA_MLC_SEC	   (0xC0004400) 
#define	POLLUX_FB_NAME		   "pollux-fb"

/* pwm */
#define POLLUX_VA_PWM	   POLLUX_ADDR(0x0000C000)
#define POLLUX_PA_PWM	   (0xC000C000) 

/* DMA */
#define POLLUX_VA_DMA	   POLLUX_ADDR(0x00000000)
#define POLLUX_PA_DMA	   (0xC0000000) // DMA channel 0 base

/* USB-HOST: not active in NOR-boot ==> SoC Bug.... */
#define POLLUX_VA_OHCI	   POLLUX_ADDR(0x0000D000)
#define POLLUX_PA_OHCI	   (0xC000D000) 
#define	POLLUX_HCD_NAME	  "pollux-hcd"

/* GPIOALV */
#define POLLUX_VA_ALVGPIO	   POLLUX_ADDR(0x00019000)
#define POLLUX_PA_ALVGPIO	   (0xC0019000) 

/* RTC */
#define POLLUX_VA_RTC	   POLLUX_ADDR(0x0000F080)
#define POLLUX_PA_RTC	   (0xC000F080) 

/* SPI0 */
#define POLLUX_VA_SPI0	   POLLUX_ADDR(0x00007800)
#define POLLUX_PA_SPI0	   (0xC0007800) 

/* SPI1 */
#define POLLUX_VA_SPI1	   POLLUX_ADDR(0x00008000)
#define POLLUX_PA_SPI1	   (0xC0008000) 

/* SPI2: Recommended that spi channel 2 is not used. gpiob[0~5] is used for SDIO 0 */
#define POLLUX_VA_SPI2	   POLLUX_ADDR(0x00008800)
#define POLLUX_PA_SPI2	   (0xC0008800) 

/* UDC: usb device controller */
#define POLLUX_VA_UDC	   POLLUX_ADDR(0x00018000)
#define POLLUX_PA_UDC	   (0xC0018000) 

/* ADC */
#define POLLUX_VA_ADC    POLLUX_ADDR(0x00005000)
#define POLLUX_PA_ADC    POLLUX_ADDR(0xC0005000)

/* MCU-S */
#define POLLUX_VA_STATICBUS	   POLLUX_ADDR(0x00015800)
#define POLLUX_PA_STATICBUS	   (0xC0015800) 

/* IDCT */
#define POLLUX_VA_IDCT    POLLUX_ADDR(0x0000F800)
#define POLLUX_PA_IDCT    POLLUX_ADDR(0xC000F800)

/* 3D */
#define POLLUX_VA_3D    POLLUX_ADDR(0x0001A000)
#define POLLUX_PA_3D    POLLUX_ADDR(0xC001A000)

#define POLLUX_VA_CLKPWR    POLLUX_ADDR(0x0000F000)
#define POLLUX_PA_CLKPWR    POLLUX_ADDR(0xC000F000)

/* NAND */
#define POLLUX_PA_NAND	   POLLUX_PA_CS10
#define POLLUX_VA_NAND	   0xf5000000UL

#define POLLUX_VA_NANDECC	   POLLUX_ADDR(0x00015874)
#define POLLUX_PA_NANDECC	   0xC0015874

/* UBI9302 (HOST) */
#define POLLUX_PA_UBI9302	   		POLLUX_PA_CS0
#define POLLUX_VA_UBI9302	   		0xf3000000UL

#endif /* __ASM_ARCH_MAP_H */



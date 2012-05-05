/*
 * linux/include/asm-arm/arch-pollux/regs-serial.h
 *
 * Copyright. 2003-2008 AESOP-Embedded project
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
#ifndef __ASM_ARM_REGS_SERIAL_H
#define __ASM_ARM_REGS_SERIAL_H

#include <linux/serial_core.h>

#define POLLUX_UART_NR_PORTS	(4)
//#define UART_BASE_DIV            (18) // (100022727(PLL1) / 18 ) / 1843200(16x115200) = 3.00 -1
//#define UART_BASE_DIV            (24) // 132.750Mhz, (132750000(PLL1) / 24 ) / 1843200(16x115200) = 3.00 -1 
//#define UART_BASE_DIV            (23) // 130.050Mhz, (130050000(PLL1) / 23 ) / 1843200(16x115200) = 3.00 -1 
//#define UART_BASE_DIV             (24)  // (132750000(PLL1) / 24 ) / 1843200(16x115200) = 3.00 -1 
//#define UART_BASE_DIV            (22) // 124.2Mhz
#define UART_BASE_DIV            (46) // 261Mhz


	#define POLLUX_VA_UART0      (POLLUX_VA_UART)
	#define POLLUX_VA_UART1      (POLLUX_VA_UART + 0x80 )
	#define POLLUX_VA_UART2      (POLLUX_VA_UART + 0x800 )
	#define POLLUX_VA_UART3      (POLLUX_VA_UART + 0x880 )

#define POLLUX_ULCON			(0x00)
#define POLLUX_UCON				(0x02)
#define POLLUX_UFCON			(0x04)
#define POLLUX_UMCON			(0x06)
#define POLLUX_UTRSTAT			(0x08)
#define POLLUX_UERSTAT			(0x0a)
#define POLLUX_UFSTAT			(0x0c)
#define POLLUX_UMSTAT			(0x0e)
#define POLLUX_UTXH				(0x10)
#define POLLUX_URXH				(0x12)
#define POLLUX_UBRDIV			(0x14)
#define POLLUX_UTOUT			(0x16)
#define POLLUX_UINTSTAT			(0x18)

#define POLLUX_UCLKENB			(0x40)
#define POLLUX_UCLKGEN			(0x44)


/* 
 * uart line control register
 */
#define POLLUX_LCON_SYNCPENDCLR	(1<<7)           
#define POLLUX_LCON_CFGMASK		((0xF<<3)|(0x3)) 
#define POLLUX_LCON_IRM			(1<<6)           
#define POLLUX_LCON_PNONE		(0x0)            
#define POLLUX_LCON_PEVEN		(0x5 << 3)       
#define POLLUX_LCON_PODD		(0x4 << 3)       
#define POLLUX_LCON_PMASK		(0x7 << 3)       
#define POLLUX_LCON_STOPB		(1<<2)
#define POLLUX_LCON_CS5			(0x0)
#define POLLUX_LCON_CS6			(0x1)
#define POLLUX_LCON_CS7			(0x2)
#define POLLUX_LCON_CS8			(0x3)
#define POLLUX_LCON_CSMASK		(0x3)


#define POLLUX_UCON_SBREAK        (1<<4)   
#define POLLUX_UCON_TXILEVEL	  (1<<9)   
#define POLLUX_UCON_RXILEVEL	  (1<<8)   
#define POLLUX_UCON_TXIRQMODE	  (1<<2)   
#define POLLUX_UCON_RXIRQMODE	  (1<<0)   
#define POLLUX_UCON_RXFIFO_TOI	  (1<<7)   

#define POLLUX_UCON_DEFAULT	  (POLLUX_UCON_TXILEVEL  | \
				   POLLUX_UCON_RXILEVEL  | \
				   POLLUX_UCON_TXIRQMODE | \
				   POLLUX_UCON_RXIRQMODE | \
				   POLLUX_UCON_RXFIFO_TOI)

/*
 * uart fifo control register
 */
#define POLLUX_UFCON_FIFOMODE	  (1<<0)
#define POLLUX_UFCON_TXTRIG0	  (0<<6)
#define POLLUX_UFCON_TXTRIG4	  (1<<6)
#define POLLUX_UFCON_TXTRIG8	  (2<<6)
#define POLLUX_UFCON_TXTRIG12	  (3<<6)
#define POLLUX_UFCON_RXTRIG1	  (0<<4)
#define POLLUX_UFCON_RXTRIG4	  (1<<4)
#define POLLUX_UFCON_RXTRIG8	  (2<<4)
#define POLLUX_UFCON_RXTRIG12	  (3<<4)

#define POLLUX_UFCON_RESETBOTH	  (3<<1)
#define POLLUX_UFCON_RESETTX	  (1<<2)
#define POLLUX_UFCON_RESETRX	  (1<<1)

#define POLLUX_UFCON_DEFAULT	  (POLLUX_UFCON_FIFOMODE | \
				   POLLUX_UFCON_TXTRIG0  | \
				   POLLUX_UFCON_RXTRIG8 )


/*
 * uart modem control register
 */

/* ghcstop add 20080316 for uart/iso7816 control: UART로 사용할 경우(NULL modem)는 1, 1, 0 가 되어야 한다.  */
#define	POLLUX_HALF_CH_ENB        (1<<7) /* 0: Full uart, 1: half uart           */
#define	POLLUX_SCRXENB            (1<<6) /* 0: iso 7816 Tx, 1: UART/iso 7816 rx  */
#define	POLLUX_SCTXENB            (1<<5) /* 0: UART / ISO7816 RX, 1 : ISO7816 Tx */

#define	POLLUX_UMCOM_AFC	      (1<<4)
#define	POLLUX_UMCOM_RTS_LOW	  (1<<0) 
#define	POLLUX_UMCOM_DTRACT	      (1<<1) 
#define	POLLUX_UMCOM_SCTXENB	  (1<<5) 
#define	POLLUX_UMCOM_SCRXENB	  (1<<6) 
#define	POLLUX_UMCOM_HALFUART	  (1<<7) 

/*
 * uart tx/rx status register
 */
#define POLLUX_UTRSTAT_TXE	  (1<<2) 
#define POLLUX_UTRSTAT_TXFE	  (1<<1) 
#define POLLUX_UTRSTAT_RXDR	  (1<<0) 



/* 
 * uart error status register
 */
#define POLLUX_UERSTAT_OVERRUN	  (1<<0)
#define POLLUX_UERSTAT_FRAME	  (1<<2)
#define POLLUX_UERSTAT_BREAK	  (1<<3)
#define POLLUX_UERSTAT_ANY	  (POLLUX_UERSTAT_OVERRUN | \
				   POLLUX_UERSTAT_FRAME | \
				   POLLUX_UERSTAT_BREAK)


/*
 * uart fifo status register
 */
#define POLLUX_UFSTAT_RXFIFOERR	  (1<<10)  
#define POLLUX_UFSTAT_TXFULL	  (1<<9)
#define POLLUX_UFSTAT_RXFULL	  (1<<8)
#define POLLUX_UFSTAT_TXMASK	  (15<<4)
#define POLLUX_UFSTAT_TXSHIFT	  (4)
#define POLLUX_UFSTAT_RXMASK	  (15<<0)
#define POLLUX_UFSTAT_RXSHIFT	  (0)



/*
 * uart modem status register
 */
#define POLLUX_UMSTAT_CTS	      (1<<0)
#define POLLUX_UMSTAT_DeltaCTS	  (1<<4) 

// 2530에만 있는 필드들
#define POLLUX_UMSTAT_DeltaDCD	  (1<<7) 
#define POLLUX_UMSTAT_DeltaRI	  (1<<6) 
#define POLLUX_UMSTAT_DeltaDSR	  (1<<5) 
#define POLLUX_UMSTAT_DCD	      (1<<3) 
#define POLLUX_UMSTAT_RI	      (1<<2) 
#define POLLUX_UMSTAT_DSR	      (1<<1) 


/*
 * uart interrupt status register
 */
#define POLLUX_UINTSTAT_MENB      (1<<7)
#define POLLUX_UINTSTAT_ERRENB    (1<<6)
#define POLLUX_UINTSTAT_IRQRXENB  (1<<5)
#define POLLUX_UINTSTAT_IRQTXENB  (1<<4)
#define POLLUX_UINTSTAT_MPEND     (1<<3)
#define POLLUX_UINTSTAT_ERRPEND   (1<<2)
#define POLLUX_UINTSTAT_RXENB     (1<<1)
#define POLLUX_UINTSTAT_TXENB     (1<<0)

/*
 * uart clock enable register
 */
#define	POLLUX_UCLKENB_PCLKMODE			(1<<3) 	
#define	POLLUX_UCLKENB_CLKGENENB		(1<<2) 	


/*
 * uart clock generate register
 */
#define POLLUX_UCLKGEN_CLKDIV_MASK		(0x3f<<4) // 6bit
#define POLLUX_UCLKGEN_CLKDIV_SHIFT		(4)

#define POLLUX_UCLKGEN_CLKSRCSEL_MASK	(7<<1)
#define POLLUX_UCLKGEN_CLKSRCSEL_SHIFT	(1)
	#define POLLUX_UCLKGEN_CLKSRC_PLL0	(0<<1)
	#define POLLUX_UCLKGEN_CLKSRC_PLL1	(1<<1)
	#define POLLUX_UCLKGEN_CLKSRC_PLL2	(2<<1)






#ifndef __ASSEMBLY__

struct pollux_uart_clksrc {
	const char	*name;
	unsigned int	 divisor;
	unsigned int	 min_baud;
	unsigned int	 max_baud;
};

struct pollux_uartcfg {
	unsigned char	   hwport;	 /* hardware port number */
	unsigned char	   unused;
	unsigned short	   flags;
	upf_t		   uart_flags;	 /* default uart flags */

	unsigned long	   ucon;	 /* value of ucon for port */
	unsigned long	   ulcon;	 /* value of ulcon for port */
	unsigned long	   ufcon;	 /* value of ufcon for port */

};

extern struct platform_device *pollux_uart_devs[ POLLUX_UART_NR_PORTS ];

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARM_REGS_SERIAL_H */


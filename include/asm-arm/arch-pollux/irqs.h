/* 
 * linux/include/asm-arm/arch-mp2530/irqs.h
 *
 * Copyright. 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/mp2530/index.html
 *
 * This code from mp2520f linux kernel(by Oh Yeah Whan - hellofjoy)
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
#ifndef _ASM_ARCH_IRQS_H
#define _ASM_ARCH_IRQS_H

//-- Main irqs -------------------------------------------------------------------
enum
{
	IRQ_PDISPLAY	=	0, 			//  0. Primary Display Interrupt	
	IRQ_SDISPLAY,					//  1. Secondary Display Interrupt
	IRQ_NA_2,						//  2. N/A
	IRQ_DMA,						//  3. DMA Interrupt
	IRQ_TIMER0,						//  4. Timer0 Interrupt
	IRQ_SYSCTRL,					//  5. System Control Interrupt
	IRQ_NA_6,					    //  6. N/A
	IRQ_NA_7,						//  7. N/A
	IRQ_NA_8,					    //  8. N/A
	IRQ_NA_9,					    //  9. N/A
	IRQ_UART0,						// 10. UART 0 Interrput 
	IRQ_TIMER1,						// 11. Timer 1 Interrupt 
	IRQ_SSPSPI0,					// 12. SSP/SPI0 Interrupt 
	IRQ_GPIO,						// 13. GPIO Interrupt
	IRQ_SDMMC0,						// 14. SD/MMC Interrupt, differ to mp2530f
	IRQ_TIMER2,						// 15. Timer 2 Interrupt 
	IRQ_NA_16,						// 16. N/A
	IRQ_NA_17,						// 17. N/A
	IRQ_NA_18,						// 18. N/A
	IRQ_NA_19,						// 19. N/A
	IRQ_UDC,						// 20. USB Device 2.0 Interrupt
	IRQ_TIMER3,						// 21. Timer 3 Interrupt 
	IRQ_NA_22,						// 22. N/A
	IRQ_NA_23,						// 23. N/A
	IRQ_AUDIOIF,					// 24. Audio Interface Interrupt( AC97/IIS )
	IRQ_ADC,						// 25. ADC Interrupt
	IRQ_MCUSTATIC,					// 26. MCU-S Interrupt
	IRQ_GRP3D,						// 27. 3D Graphic Controller Interrupt
	IRQ_UHC,						// 28. USB Host Interrupt
	IRQ_NA_29,						// 29. N/A
	IRQ_NA_30,						// 30. N/A
	IRQ_RTC,						// 31. RTC Interrupt
	IRQ_IIC0,						// 32. IIC 0 Interrupt
	IRQ_IIC1,						// 33. IIC 1 Interrupt
	IRQ_UART1,						// 34. UART1 Interrupt
	IRQ_UART2,						// 35. UART2 Interrupt
	IRQ_UART3,						// 36. UART3 Interrupt36
	IRQ_UART4,						// 37. N/A
	IRQ_UART5,						// 38. N/A
	IRQ_SSPSPI1,					// 39. SSP/SPI 1 Interrupt
	IRQ_SSPSPI2,					// 40. SSO/SPI 2 Interrupt
	IRQ_CSC,						// 41. Color Space Conventer 
	IRQ_SDMMC1,						// 42. SD/MMC 1 interrupt, differ to mp2530f
	IRQ_TIMER4						// 43. Timer 4 interrrupt, differ to mp2530f 
};

#define MAIN_IRQ_LOW_END 	32
#define MAIN_IRQ_BASE		0    
#define MAIN_IRQ_END		( MAIN_IRQ_BASE + 44)


//-- UART sub irqs -----------------------------------------------------------------
#define UART_SUB_IRQ_BASE	MAIN_IRQ_END			/* UART 0~1 : **** */
#define IRQ_UART_SUB(x)		(UART_SUB_IRQ_BASE + (x))
#define IRQ_UART0_OFFSET	(0)
#define IRQ_TXD0			IRQ_UART_SUB(0)   // 44
#define IRQ_RXD0			IRQ_UART_SUB(1)   // 45
#define IRQ_ERROR0			IRQ_UART_SUB(2)	  // 46
#define IRQ_MODEM0			IRQ_UART_SUB(3)   // 47

#define IRQ_UART1_OFFSET	(4)
#define IRQ_TXD1			IRQ_UART_SUB(4)		//48
#define IRQ_RXD1			IRQ_UART_SUB(5)		//49
#define IRQ_ERROR1			IRQ_UART_SUB(6)		//50
#define IRQ_MODEM1			IRQ_UART_SUB(7)		//51

#define IRQ_UART2_OFFSET	(8)
#define IRQ_TXD2			IRQ_UART_SUB(8)		//52
#define IRQ_RXD2			IRQ_UART_SUB(9)		//53
#define IRQ_ERROR2			IRQ_UART_SUB(10)	//54
#define IRQ_MODEM2			IRQ_UART_SUB(11)	//55

#define IRQ_UART3_OFFSET	(12)
#define IRQ_TXD3			IRQ_UART_SUB(12)	//56
#define IRQ_RXD3			IRQ_UART_SUB(13)    //44+13 57
#define IRQ_ERROR3			IRQ_UART_SUB(14)	//58
#define IRQ_MODEM3			IRQ_UART_SUB(15)    //59

#define UART_SUB_IRQ_END	IRQ_UART_SUB(16)	//60


//-- DMA sub irqs ------------------------------------------------------------------
#define DMA_SUB_IRQ_BASE	UART_SUB_IRQ_END    // 60 ~ 67
#define IRQ_DMA_SUB(x)		(DMA_SUB_IRQ_BASE + (x))			/* DMA channel 0~7 :  */
#define DMA_SUB_IRQ_END		IRQ_DMA_SUB(8)      // 68

#define IRQ_TO_DMA(x)		((x) - DMA_SUB_IRQ_BASE)

/* 69 ~ 79: empty irq number */

//-- GPIO sub irqs ------------------------------------------------------------------
#define GPIO_SUB_IRQ_BASE 	(0x50)	// 80				        /* GPIOA[31:0]~GPIOC[31:0]  */ // irq 추출을 빨리 하기 위해서
#define IRQ_GPIO_A(x)		(GPIO_SUB_IRQ_BASE + (x))			// 0x50 ~ 0x6f,  80 ~ 111, 0 ~ 31
#define IRQ_GPIO_B(x)		(GPIO_SUB_IRQ_BASE + 0x20 + (x))	// 0x70 ~ 0x8f, 112 ~ 143, 0 ~ 31
#define IRQ_GPIO_C(x)		(GPIO_SUB_IRQ_BASE + 0x40 + (x))	// 0x90 ~ 0xa4, 144 ~ 164, 0 ~ 20
#define GPIO_SUB_IRQ_END 	(IRQ_GPIO_C(20)+1)
//-----------------------------------------------------------------------------------
#define IRQ_TO_GPIO(x)		((x) - GPIO_SUB_IRQ_BASE)


#define NR_IRQS				GPIO_SUB_IRQ_END	


#endif /* _ASM_ARCH_IRQS_H */

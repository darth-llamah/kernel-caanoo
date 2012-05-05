/*
 * linux/arch/arm/mach-pollux/devs.c
 *
 * Base S3C24XX platform device definitions
 *
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho)     - main programmer
 * amphell(Bang Chang Hyeok)    - Hardware Engineer
 *
 * 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/pollux/index.html
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
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/arch/regs-serial.h>

#include "devs.h"
#include "cpu.h"

static struct resource pollux_uart0_resource[] = {
	[0] = {
		.start = POLLUX_VA_UART0,
		.end   = POLLUX_VA_UART0 + 0x7f,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_TXD0,
		.end   = IRQ_MODEM0,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource pollux_uart1_resource[] = {
	[0] = {
		.start = POLLUX_VA_UART1,
		.end   = POLLUX_VA_UART1 + 0x7f,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_TXD1,
		.end   = IRQ_MODEM1,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource pollux_uart2_resource[] = {
	[0] = {
		.start = POLLUX_VA_UART2,
		.end   = POLLUX_VA_UART2 + 0x7f,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_TXD2,
		.end   = IRQ_MODEM2,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource pollux_uart3_resource[] = {
	[0] = {
		.start = POLLUX_VA_UART3,
		.end   = POLLUX_VA_UART3 + 0x7f,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_TXD3,
		.end   = IRQ_MODEM3,
		.flags = IORESOURCE_IRQ,
	}
};





struct pollux_uart_resources pollux_uart_resources[] __initdata = {
	[0] = {
		.resources	= pollux_uart0_resource,
		.nr_resources	= ARRAY_SIZE(pollux_uart0_resource),
	},
	[1] = {
		.resources	= pollux_uart1_resource,
		.nr_resources	= ARRAY_SIZE(pollux_uart1_resource),
	},
	[2] = {
		.resources	= pollux_uart2_resource,
		.nr_resources	= ARRAY_SIZE(pollux_uart2_resource),
	},
	[3] = {
		.resources	= pollux_uart3_resource,
		.nr_resources	= ARRAY_SIZE(pollux_uart3_resource),
	},
	
};

/* uart devices */

static struct platform_device pollux_uart_device0 = {
	.id		= 0,
};

static struct platform_device pollux_uart_device1 = {
	.id		= 1,
};

static struct platform_device pollux_uart_device2 = {
	.id		= 2,
};

static struct platform_device pollux_uart_device3 = {
	.id		= 3,
};


struct platform_device *pollux_uart_src[POLLUX_UART_NR_PORTS] = {
	&pollux_uart_device0,
	&pollux_uart_device1,
	&pollux_uart_device2,
	&pollux_uart_device3,
};

// 위의 pollux_uart_src를 여기다 다시 담는다.... cpu.c의 pollux_init_uartdevs()를 이용해서
struct platform_device *pollux_uart_devs[POLLUX_UART_NR_PORTS] = {
};

//===============================================================
// USB host
//===============================================================
static struct resource ohci_hcd_pollux_resources[] = {

	[0] = {
		.start  = POLLUX_VA_OHCI, 
		.end    = (POLLUX_VA_OHCI + 0x00000fff), // see manual 
		.flags  = IORESOURCE_IO,
	},

	[1] = {
		.start  = IRQ_UHC,
		.end    = IRQ_UHC,
		.flags  = IORESOURCE_IRQ,
	},	
};


static u64 pollux_device_usb_dmamask = 0xffffffffUL;

struct platform_device ohci_hcd_pollux_device = {
	.name		= POLLUX_HCD_NAME,
	.id		= 0,
	.dev            = {
		.dma_mask = &pollux_device_usb_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	},
	.num_resources  = ARRAY_SIZE(ohci_hcd_pollux_resources),
	.resource       = ohci_hcd_pollux_resources,
};

EXPORT_SYMBOL(ohci_hcd_pollux_device);


//===============================================================
// FrameBuffer
//===============================================================
struct platform_device pollux_fb_device = {
	.name	= POLLUX_FB_NAME,
	.id	= 0,			
};
EXPORT_SYMBOL(pollux_fb_device);



//===============================================================
// RTC
//===============================================================
struct platform_device pollux_rtc_device = {
	.name		= "pollux-rtc",
	.id			= 0,
};

EXPORT_SYMBOL(pollux_rtc_device);

//===============================================================
// SPI
//===============================================================
/* Recommend: spi channel 2 is not used. gpiob[0~5] is used for SDIO 0 */
struct resource pollux_spi_resources[] = {
	[0] = {
		.start		= POLLUX_VA_SPI0,
		.end		= POLLUX_VA_SPI0+0x44,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_SSPSPI0,
		.end		= IRQ_SSPSPI0,
		.flags		= IORESOURCE_IRQ,
	},
	[2] = {
		.start		= POLLUX_VA_SPI1,
		.end		= POLLUX_VA_SPI1+0x44,
		.flags		= IORESOURCE_MEM,
	},
	[3] = {
		.start		= IRQ_SSPSPI1,
		.end		= IRQ_SSPSPI1,
		.flags		= IORESOURCE_IRQ,
	},
	[4] = {
		.start		= POLLUX_VA_SPI2,
		.end		= POLLUX_VA_SPI2+0x44,
		.flags		= IORESOURCE_MEM,
	},
	[5] = {
		.start		= IRQ_SSPSPI2,
		.end		= IRQ_SSPSPI2,
		.flags		= IORESOURCE_IRQ,
	},
};

struct platform_device pollux_spi_device = {
	.name			= "pollux-spi",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(pollux_spi_resources),
	.resource		= pollux_spi_resources,
};

EXPORT_SYMBOL(pollux_spi_device);

//===============================================================
// UDC: USB Device Controller
//===============================================================
struct resource pollux_udc_resources[] = {
	[0] = {
		.start		= POLLUX_VA_UDC,
		.end		= POLLUX_VA_UDC+0x880,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_UDC,
		.end		= IRQ_UDC,
		.flags		= IORESOURCE_IRQ,
	},
};

struct platform_device pollux_udc_device = {
	.name			= "pollux-usbgadget",
	.id			= -1,
	.num_resources		= ARRAY_SIZE(pollux_udc_resources),
	.resource		= pollux_udc_resources,
};

EXPORT_SYMBOL(pollux_udc_device);

struct resource pollux_3d_resources = {
		.start		= POLLUX_VA_3D,
		.end		= POLLUX_VA_3D+0x1FFF,
		.flags		= IORESOURCE_MEM,
};

struct platform_device pollux_3d_device = {
	.name			= "pollux-ga3d",
	.id			= -1,
	.num_resources		= 1,
	.resource		= &pollux_3d_resources,
};

EXPORT_SYMBOL(pollux_3d_device);








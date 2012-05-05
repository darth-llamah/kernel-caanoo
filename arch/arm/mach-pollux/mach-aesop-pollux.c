/* 
 *  linux/arch/arm/mach-pollux/mach-mes-navikit.c
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
#include <asm/hardware/iomd.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <linux/irq.h>
#include <asm/arch/aesop-pollux.h> 
#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-gpio.h> 

#include "pollux.h"
#include "devs.h"
#include "cpu.h"

 
#define UCON  (POLLUX_UCON_DEFAULT)
#define ULCON (POLLUX_LCON_CS8 | POLLUX_LCON_PNONE )
#define UFCON (POLLUX_UFCON_RXTRIG1 | POLLUX_UFCON_TXTRIG4 | POLLUX_UFCON_FIFOMODE) 

static struct pollux_uartcfg aesoppollux_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[3] = {
		.hwport	     = 3,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	
};

static struct map_desc aesoppollux_iodesc[] __initdata = {
	[0] = {
		.virtual	= (unsigned long)POLLUX_FB_VIO_BASE,
		.pfn		= __phys_to_pfn(POLLUX_FB_PIO_BASE),
		.length		= (TOTAL_DMA_MAP_SIZE),  
		.type		= MT_DEVICE
	},

	[1] = {
		.virtual	= (unsigned long)POLLUX_VA_NAND,
		.pfn		= __phys_to_pfn(POLLUX_PA_NAND),
		.length		= 0x10000,  
		.type		= MT_DEVICE
	},	

    [2] = {
		.virtual	= (unsigned long)POLLUX_MPEGDEC_VIO_BASE,
		.pfn		= __phys_to_pfn(POLLUX_MPEGDEC_PIO_BASE),
		.length		= (POLLUX_MPEGDEC_MAP_SIZE),  
		.type		= MT_DEVICE
	},
    [3] = {
		.virtual	= (unsigned long)POLLUX_3D_VIO_BASE,
		.pfn		= __phys_to_pfn(POLLUX_3D_PIO_BASE),
		.length		= (POLLUX_3D_MAP_SIZE),  
		.type		= MT_DEVICE
	},

};

struct resource lf1000_nand_resource = {
	.start			= POLLUX_PA_NAND,
	.end			= POLLUX_PA_NAND+0x18,
	.flags			= IORESOURCE_MEM,
};

struct platform_device lf1000_nand_device = {
	.name			= "lf1000-nand",
	.id			= -1,
	.num_resources		= 1,
	.resource		= &lf1000_nand_resource,
};


static struct platform_device *aesoppollux_devices[] __initdata = {
	&lf1000_nand_device,
	&pollux_fb_device,
	&ohci_hcd_pollux_device,
	&pollux_rtc_device,
	&pollux_udc_device,
    &pollux_3d_device,
};

static struct pollux_board aesoppollux_board __initdata = {
	.devices       = aesoppollux_devices,
	.devices_count = ARRAY_SIZE(aesoppollux_devices)
};

static struct map_desc standard_io_desc[] __initdata = {
	[0] = {
		.virtual	= (unsigned long)VIO_BASE,
		.pfn		= __phys_to_pfn(PIO_BASE),
		.length		= HEX_128K,
		.type		= MT_DEVICE
	},
};

static void __init pollux_map_io(void)
{
	iotable_init(standard_io_desc, ARRAY_SIZE(standard_io_desc));
	iotable_init(aesoppollux_iodesc, ARRAY_SIZE(aesoppollux_iodesc));
	pollux_show_clk();
	pollux_init_uarts(aesoppollux_uartcfgs, ARRAY_SIZE(aesoppollux_uartcfgs)); 
	pollux_set_board(&aesoppollux_board); 
}


MACHINE_START(MESPOLLUX, "AESOP_POLLUX")	
/* Maintainer: godori(Ko Hyun Chul) <ghcstop@gmail.com> */
	.phys_io	= POLLUX_PA_UART,
	.io_pg_offst	= (((u32)POLLUX_VA_UART) >> 18) & 0xfffc,
	.boot_params	= POLLUX_DRAM_PA + 0x100,
	.map_io		= pollux_map_io,
	.init_irq	= pollux_init_irq,
	.timer		= &pollux_timer,
MACHINE_END

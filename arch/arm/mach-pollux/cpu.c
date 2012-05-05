/* 
 * linux/arch/arm/mach-pollux/cpu.c
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h> // ghcstop add

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/arch/regs-serial.h>

#include "cpu.h"
#include "devs.h"
#include "pollux.h"

#if 0 // ghcstop debug
extern void printascii(const char *); // debug.S

static void
gdebug(const char *fmt, ...)
{
	va_list va;
	char buff[256];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	printascii(buff);
}
#else /* no debug */
	#if 1
		#define gdebug(x...) do {} while(0)
	#else
		#define gdebug(fmt, x... )  printk( "%s: " fmt, __FUNCTION__ , ## x)
	#endif
#endif


/* board information */

int __init pollux_init(void);

static struct pollux_board *board;

void pollux_set_board(struct pollux_board *b)
{
	int i;

	board = b;
}

/* uart management */

static int nr_uarts __initdata = 0;
static struct pollux_uartcfg uart_cfgs[ POLLUX_UART_NR_PORTS ];

/* pollux_init_uartdevs
 *
 * copy the specified platform data and configuration into our central
 * set of devices, before the data is thrown away after the init process.
 *
 * This also fills in the array passed to the serial driver for the
 * early initialisation of the console.
*/

// pollux.c의 pollux_init_uarts()에서 호출
void __init pollux_init_uartdevs(char *name,
				  struct pollux_uart_resources *res,
				  struct pollux_uartcfg *cfg, int no)
{
	struct platform_device *platdev;
	struct pollux_uartcfg *cfgptr = uart_cfgs;
	struct pollux_uart_resources *resp;
	int uart;

	memcpy(cfgptr, cfg, sizeof(struct pollux_uartcfg) * no);

	for (uart = 0; uart < no; uart++, cfg++, cfgptr++) {
		platdev = pollux_uart_src[cfgptr->hwport];

		resp = res + cfgptr->hwport;

		pollux_uart_devs[uart] = platdev;

		platdev->name = name;
		platdev->resource = resp->resources;
		platdev->num_resources = resp->nr_resources;

		platdev->dev.platform_data = cfgptr; // 나중에 초기값들을 가져오기위해.
	}

	nr_uarts = no;
}

void __init pollux_init_uarts(struct pollux_uartcfg *cfg, int no)
{
	//smdk2410_uartcfgs, ARRAY_SIZE(smdk2410_uartcfgs);
	//pollux_init_uarts(cfg, no); // pollux.c
	
	// cpu->init_uarts로 호출하는것을 다음처럼 다이렉트로 호출..	
	pollux_init_uartdevs("pollux-uart", pollux_uart_resources, cfg, no );	
}

static int __init pollux_arch_init(void)
{
	int ret;

gdebug("polluxf_arch_init 1\n");

	// do the correct init for cpu
	ret = pollux_init();
	
	if (ret != 0)
		return ret;

gdebug("polluxf_arch_init 2\n");
	ret = platform_add_devices(pollux_uart_devs, nr_uarts);
	if (ret != 0)
		return ret;

gdebug("polluxf_arch_init 3\n");
	if (board != NULL) {
		struct platform_device **ptr = board->devices;
		int i;

		for (i = 0; i < board->devices_count; i++, ptr++) 
		{
			ret = platform_device_register(*ptr);

			if (ret) 
			{
				gdebug(KERN_ERR "pollux: failed to add board device %s (%d) @%p\n", (*ptr)->name, ret, *ptr);
			}
		}

		ret = 0;
	}
gdebug("polluxf_arch_init 4\n");
	return ret;
}

arch_initcall(pollux_arch_init); // init call level 3


#define pollux_suspend NULL
#define pollux_resume  NULL

/* Since the S3C2442 and S3C2440 share  items, put both sysclasses here */

struct sysdev_class pollux_sysclass = {
	set_kset_name("pollux-core"),
	.suspend	= pollux_suspend,
	.resume		= pollux_resume
};

static struct sys_device pollux_sysdev = {
	.cls		= &pollux_sysclass,
};

static int __init pollux_core_init(void)
{
	gdebug("pollux_core_init\n");
	return sysdev_class_register(&pollux_sysclass);
}

core_initcall(pollux_core_init); // init call level 1

int __init pollux_init(void)
{
	gdebug("POLLUX: Initialising architecture\n");

	return sysdev_register(&pollux_sysdev);
}

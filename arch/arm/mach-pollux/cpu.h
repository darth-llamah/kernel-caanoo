/* 
 * arch/arm/mach-pollux/cpu.h
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
/* todo - fix when rmk changes iodescs to use `void __iomem *` */
#ifndef MHZ
#define MHZ (1000*1000)
#endif

#define print_mhz(m) ((m) / MHZ), ((m / 1000) % 1000)

/* forward declaration */
struct pollux_uart_resources;
struct platform_device;
struct pollux_uartcfg;
struct map_desc;

/* core initialisation functions */
extern void pollux_init_uarts(struct pollux_uartcfg *cfg, int no);
extern void pollux_init_uartdevs(char *name,
				  struct pollux_uart_resources *res,
				  struct pollux_uartcfg *cfg, int no);

struct pollux_board {
	struct platform_device  **devices;
	unsigned int              devices_count;
};

extern void pollux_set_board(struct pollux_board *board);

/* system device classes */
extern struct sysdev_class pollux_sysclass;

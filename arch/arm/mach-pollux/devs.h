/* 
 * arch/arm/mach-pollux/devs.h
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
#ifndef __DEVS_H
#define __DEVS_H

#include <linux/platform_device.h>

struct pollux_uart_resources {
	struct resource		*resources;
	unsigned long		 nr_resources;
};

//extern struct pollux_uart_resources s3c2410_uart_resources[];
extern struct pollux_uart_resources pollux_uart_resources[];
extern struct platform_device *pollux_uart_devs[];
extern struct platform_device *pollux_uart_src[];

// USB-HOST
extern struct platform_device ohci_hcd_pollux_device;

// FrameBuffer
extern struct platform_device pollux_fb_device; 

// RTC
extern struct platform_device pollux_rtc_device;

// SPI
extern struct platform_device pollux_spi_device;

// UDC: USB Device Controller
extern struct platform_device pollux_udc_device;

// 3D
extern struct platform_device pollux_3d_device;

#endif

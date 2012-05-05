/*
 * linux/drivers/usb/gadget/lf1000_udc.c
 *
 * Based on:
 * linux/drivers/usb/gadget/s3c2410_udc.c
 * Samsung on-chip full speed USB device controllers
 *
 * Copyright (C) 2004-2006 Herbert Pötzl - Arnaud Patard
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
 * TODO:
 *
 * Move done() call from write_req to handle_ep and make it interrupt driven
 * based on TX bit.
 *
 * Use consistent locking scheme
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/clk.h>

#include <linux/usb.h>
#include <linux/usb/gadget.h>


#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>

#include <asm/mach-types.h>
#include <asm/arch/regs-gpio.h>
//#include <asm/arch/gpio.h>

#include "lf1000_udc.h"

#define ENABLE_SYSFS

#define DRIVER_DESC     "LF1000 USB Device Controller Gadget"
#define DRIVER_VERSION  "03 Jul 2007"
#define DRIVER_AUTHOR	"Brian Cavagnolo <brian@cozybit.com>"


#if 1
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif



static const char       gadget_name [] = "lf1000_udc";
static const char	driver_desc [] = DRIVER_DESC;

static struct lf1000_udc	*the_controller;
static void __iomem 		*base_addr;
static u64			rsrc_start;
static u64			rsrc_len;

static inline u32 udc_readl(u32 reg)
{
	return ioread32(base_addr+reg);
}
static inline void udc_writel(u32 value, u32 reg)
{
	iowrite32(value,base_addr+reg);
}

static inline u16 udc_readh(u32 reg)
{
	return ioread16(base_addr+reg);
}
static inline void udc_writeh(u16 value, u32 reg)
{
	iowrite16(value,base_addr+reg);
}

static inline void udc_setbits(u16 bits, u32 reg) {
	unsigned long tmp;
	tmp = udc_readh(reg);
	tmp |= bits;
	udc_writeh(tmp, reg);
}

static inline void udc_clearbits(u16 bits, u32 reg) {
	unsigned long tmp;
	tmp = udc_readh(reg);
	tmp &= ~(bits);
	udc_writeh(tmp, reg);
}

#ifndef BIT_SET // ghcstop: defined in include/asm/arch/hardware.h
	#define BIT_SET(v,b)    ((v) |= (1<<(b)))
	#define BIT_CLR(v,b)    ((v) &= ~(1<<(b)))
	#define IS_SET(v,b)     ((v) & (1<<(b)))
	#define IS_CLR(v,b)     !((v) & (1<<(b)))
#endif

/*************************** DEBUG FUNCTION ***************************/
#define DEBUG_NORMAL	1
#define DEBUG_VERBOSE	2

//#define CONFIG_USB_LF1000_DEBUG // ghcstop fix
#ifdef CONFIG_USB_LF1000_DEBUG
//#define USB_LF1000_DEBUG_LEVEL 2
#define USB_LF1000_DEBUG_LEVEL 1

static uint32_t lf1000_ticks=0;

static int dprintk(int level, const char *fmt, ...)
{
	static char printk_buf[1024];
	static long prevticks;
	static int invocation;
	va_list args;
	int len;

	if (level > USB_LF1000_DEBUG_LEVEL)
		return 0;

	if (lf1000_ticks != prevticks) {
		prevticks = lf1000_ticks;
		invocation = 0;
	}

	len = scnprintf(printk_buf, \
			sizeof(printk_buf), "%1lu.%02d USB: ", \
			prevticks, invocation++);

	va_start(args, fmt);
	len = vscnprintf(printk_buf+len, \
			sizeof(printk_buf)-len, fmt, args);
	va_end(args);

	return printk("%s", printk_buf);
}
#else
static int dprintk(int level, const char *fmt, ...)  { return 0; }
#endif
#ifdef ENABLE_SYSFS
static struct lf1000_udc udc_dev;

static ssize_t lf1000udc_vbus_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", udc_dev.vbus);
}

/* This is a hack.  The watchdog should be in the mass storage driver so that it
 * can be disarmed when that driver is enabled.  But the interface is already in
 * the sysfs for this driver.  So it goes.
 */
void usb_gadget_watchdog_cancel(void)
{
	unsigned long flags;
	spin_lock_irqsave(&udc_dev.lock, flags);
	del_timer(&udc_dev.watchdog);
	udc_dev.watchdog_expired = 0;
	spin_unlock_irqrestore(&udc_dev.lock, flags);	
}
EXPORT_SYMBOL(usb_gadget_watchdog_cancel);

static ssize_t watchdog_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	unsigned long flags;
	spin_lock_irqsave(&udc_dev.lock, flags);
	i = udc_dev.watchdog_seconds;
	spin_unlock_irqrestore(&udc_dev.lock, flags);
	return sprintf(buf, "%d\n", i);
}

static ssize_t watchdog_expired_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int i;
	unsigned long flags;
	spin_lock_irqsave(&udc_dev.lock, flags);
	i = udc_dev.watchdog_expired;
	spin_unlock_irqrestore(&udc_dev.lock, flags);
	return sprintf(buf, "%d\n", i);
}

static void watchdog_handler(unsigned long data)
{
	unsigned long flags;
	spin_lock_irqsave(&udc_dev.lock, flags);
	udc_dev.watchdog_expired = 1;
	spin_unlock_irqrestore(&udc_dev.lock, flags);
}

static ssize_t watchdog_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int i, running;
	unsigned long flags;

	if (sscanf(buf, "%d", &i) != 1 || i < 0)
		return -EINVAL;

	spin_lock_irqsave(&udc_dev.lock, flags);
	udc_dev.watchdog_seconds = i;
	running = del_timer(&udc_dev.watchdog);
	udc_dev.watchdog_expired = 0;
	if(i != 0 && running && udc_dev.driver && udc_dev.driver->is_enabled &&
	   !udc_dev.driver->is_enabled(&udc_dev.gadget)) {
		udc_dev.watchdog.expires = 
			jiffies + udc_dev.watchdog_seconds*HZ;
		add_timer(&udc_dev.watchdog);
	}
	spin_unlock_irqrestore(&udc_dev.lock, flags);

	return count;
}

static ssize_t lf1000udc_regs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 epindex, epint, epinten, funcaddr, framenum, epdir, test, sysstat;
	u32 sysclt, ep0stat, ep0ctl, epstat, epctl, brcr, bwcr, mpr, dcr, dtcr;
	u32 dfcr, dttcr, clken, clkgen;

#ifdef CPU_LF1000
	u32 ciksel, vbusintenb, vbuspend, por, suspend, user0, user1, plicr, pcr;
#endif

	epindex = udc_readh(UDC_EPINDEX);
	epint = udc_readh(UDC_EPINT);
	epinten = udc_readh(UDC_EPINTEN);
	funcaddr = udc_readh(UDC_FUNCADDR);
	framenum = udc_readh(UDC_FRAMENUM);
	epdir = udc_readh(UDC_EPDIR);
	test = udc_readh(UDC_TEST);
	sysstat = udc_readh(UDC_SYSSTAT);
	sysclt = udc_readh(UDC_SYSCTL);
	ep0stat = udc_readh(UDC_EP0STAT);
	ep0ctl = udc_readh(UDC_EP0CTL);
	epstat = udc_readh(UDC_EPSTAT);
	epctl = udc_readh(UDC_EPCTL);
	brcr = udc_readh(UDC_BRCR);
	bwcr = udc_readh(UDC_BWCR);
	mpr = udc_readh(UDC_MPR);
	dcr = udc_readh(UDC_DCR);
	dtcr = udc_readh(UDC_DTCR);
	dfcr = udc_readh(UDC_DFCR);
	dttcr = udc_readh(UDC_DTTCR);

#ifdef CPU_LF1000
	plicr = udc_readh(UDC_PLICR);
	pcr = udc_readh(UDC_PCR);

	ciksel = udc_readh(UDC_CIKSEL);
	vbusintenb = udc_readh(UDC_VBUSINTENB);
	vbuspend = udc_readh(UDC_VBUSPEND);
	por = udc_readh(UDC_POR);
	suspend = udc_readh(UDC_SUSPEND);
	user0 = udc_readh(UDC_USER0);
	user1 = udc_readh(UDC_USER1);
#endif

	clken = udc_readh(UDC_CLKEN);
	clkgen = udc_readh(UDC_CLKGEN);

	return snprintf(buf, PAGE_SIZE,        \
			"UDC_EPINDEX:		0x%04X\n"
			"UDC_EPINT:		0x%04X\n"
			"UDC_EPINTEN:		0x%04X\n"
			"UDC_FUNCADDR:		0x%04X\n"
			"UDC_FRAMENUM:		0x%04X\n"
			"UDC_EPDIR:		0x%04X\n"
			"UDC_TEST:		0x%04X\n"
			"UDC_SYSSTAT:		0x%04X\n"
			"UDC_SYSCTL:		0x%04X\n"
			"UDC_EP0STAT:		0x%04X\n"
			"UDC_EP0CTL:		0x%04X\n"
			"UDC_EPSTAT:		0x%04X\n"
			"UDC_EPCTL:		0x%04X\n"
			"UDC_BRCR:		0x%04X\n"
			"UDC_BWCR:		0x%04X\n"
			"UDC_MPR:		0x%04X\n"
			"UDC_DCR:		0x%04X\n"
			"UDC_DTCR:		0x%04X\n"
			"UDC_DFCR:		0x%04X\n"
			"UDC_DTTCR:		0x%04X\n"
#ifdef CPU_LF1000
			"UDC_PLICR:		0x%04X\n"
			"UDC_PCR:		0x%04X\n"
			"UDC_CIKSEL:		0x%04X\n"
			"UDC_VBUSINTENB:		0x%04X\n"
			"UDC_VBUSPEND:		0x%04X\n"
			"UDC_POR:		0x%04X\n"
			"UDC_SUSPEND:		0x%04X\n"
			"UDC_USER0:		0x%04X\n"
			"UDC_USER1:		0x%04X\n"
#endif
			"UDC_CLKEN:		0x%04X\n"
			"UDC_CLKGEN:		0x%04X\n",
			epindex, epint, epinten, funcaddr, framenum, epdir,
			test, sysstat, sysclt, ep0stat, ep0ctl, epstat, epctl,
			brcr, bwcr, mpr, dcr, dtcr, dfcr, dttcr,
#ifdef CPU_LF1000
			plicr, pcr, ciksel, vbusintenb, vbuspend, por, suspend,
			user0, user1,
#endif
			clken, clkgen);
}

static DEVICE_ATTR(regs, 0444,
		   lf1000udc_regs_show,
		   NULL);
static DEVICE_ATTR(vbus, 0444, lf1000udc_vbus_show, NULL);
static DEVICE_ATTR(watchdog_seconds, 0644, watchdog_show, watchdog_store);
static DEVICE_ATTR(watchdog_expired, 0444, watchdog_expired_show, NULL);
#endif
/*------------------------- I/O ----------------------------------*/

static void udc_disable(struct lf1000_udc *dev);
static void udc_enable(struct lf1000_udc *dev);

#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
static irqreturn_t lf1000_vbus_irq(enum gpio_port port, enum gpio_pin pin,
				   void *_dev);
#endif

#define LF1000_UDC_VBUS_INIT 0
#define LF1000_UDC_VBUS_ENABLE 1
#define LF1000_UDC_VBUS_DISABLE 2
#define LF1000_UDC_VBUS_RESET 3
#define LF1000_UDC_VBUS_SHUTDOWN 4

static int lf1000_vbus_command(unsigned char cmd)
{
	int retval = 0;
	struct lf1000_udc *udc = the_controller;
	unsigned long flags;
#if defined(CONFIG_MACH_ME_LF1000) || defined(CONFIG_MACH_LF_LF1000)
	unsigned long tmp;
#endif
	switch (cmd) {
	case LF1000_UDC_VBUS_INIT:
		udc->vbus = 0;
#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
		gpio_set_fn(VBUS_SIG_PORT, VBUS_SIG_PIN, GPIO_GPIOFN);
		gpio_set_out_en(VBUS_SIG_PORT, VBUS_SIG_PIN, 1);
		gpio_set_val(VBUS_SIG_PORT, VBUS_SIG_PIN, 0);
		retval = gpio_request_irq(VBUS_DET_PORT, VBUS_DET_PIN,
					  lf1000_vbus_irq, udc);
		if(retval != 0) {
			return retval;
		}
		gpio_set_fn(VBUS_DET_PORT, VBUS_DET_PIN, GPIO_GPIOFN);
		gpio_set_out_en(VBUS_DET_PORT, VBUS_DET_PIN, 0);
		gpio_set_int_mode(VBUS_DET_PORT, VBUS_DET_PIN,
				  GPIO_IMODE_HIGH_LEVEL);
#endif
		spin_lock_irqsave(&udc->lock, flags);
		del_timer(&udc->watchdog);
		init_timer(&udc->watchdog);
		udc->watchdog.data = 0;
		udc->watchdog.function = watchdog_handler;
		udc->watchdog_expired = 0;
		spin_unlock_irqrestore(&udc->lock, flags);
		break;
	case LF1000_UDC_VBUS_ENABLE:
		spin_lock_irqsave(&udc->lock, flags);
		del_timer(&udc->watchdog);
		udc->watchdog_expired = 0;
		spin_unlock_irqrestore(&udc->lock, flags);

#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
		gpio_set_int(VBUS_DET_PORT, VBUS_DET_PIN, 1);
#else
		/* enable vbus detect interrupts */
		tmp = udc_readh(UDC_SYSCTL);
		tmp |= ((1<<VBUSOFFEN)|(1<<VBUSONEN));
		udc_writeh(tmp, UDC_SYSCTL);

		/* set up the VBUSENB bit. */
		tmp = udc_readh(UDC_USER1);
		tmp |= (1<<VBUSENB);
		udc_writeh(tmp, UDC_USER1);
#endif
		break;
	case LF1000_UDC_VBUS_DISABLE:
#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
		gpio_set_int(VBUS_DET_PORT, VBUS_DET_PIN, 0);
#else
		/* disable the VBUSENB bit. */
		tmp = udc_readh(UDC_USER1);
		tmp &= ~(1<<VBUSENB);
		udc_writeh(tmp, UDC_USER1);

		/* disable vbus detect interrupts */
		tmp = udc_readh(UDC_SYSCTL);
		tmp &= ~((1<<VBUSOFFEN)|(1<<VBUSONEN));
		udc_writeh(tmp, UDC_SYSCTL);
#endif
		spin_lock_irqsave(&udc->lock, flags);
		udc->watchdog_expired = 0;
		del_timer(&udc->watchdog);
		spin_unlock_irqrestore(&udc->lock, flags);
		break;
	case LF1000_UDC_VBUS_SHUTDOWN:
		lf1000_vbus_command(LF1000_UDC_VBUS_DISABLE);
#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
		gpio_free_irq(VBUS_DET_PORT, VBUS_DET_PIN, lf1000_vbus_irq);
#endif
		break;
	case LF1000_UDC_VBUS_RESET:
		/* What to do here? */
		break;
	default:
		break;
	}
	return retval;
}

static int lf1000_udc_vbus_session(struct usb_gadget *_gadget, int is_active)
{
	struct lf1000_udc  *udc;
	unsigned long flags;
	
	dprintk(DEBUG_NORMAL, "lf1000_udc_vbus_session()\n");
	udc = container_of (_gadget, struct lf1000_udc, gadget);

	spin_lock_irqsave(&udc->lock, flags);

	if (udc->driver && udc->driver->vbus_session) {
		udc->driver->vbus_session(&udc->gadget, is_active);
	}

	if(is_active) {
		udc->vbus = 1;
#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
		gpio_set_val(VBUS_SIG_PORT, VBUS_SIG_PIN, 1);
#endif
		del_timer(&udc->watchdog);
		udc->watchdog_expired = 0;
		if(udc->watchdog_seconds && udc->driver &&
		   udc->driver->is_enabled && 
		   !udc->driver->is_enabled(&udc->gadget)) {
			udc->watchdog.expires =
				jiffies + udc->watchdog_seconds*HZ;
			add_timer(&udc->watchdog);
		}

	} else {
		if (udc->gadget.speed != USB_SPEED_UNKNOWN) {
			if (udc->driver && udc->driver->disconnect)
				udc->driver->disconnect(&udc->gadget);
		}
#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
		gpio_set_val(VBUS_SIG_PORT, VBUS_SIG_PIN, 0);
#endif
		udc->vbus = 0;
		del_timer(&udc->watchdog);
		udc->watchdog_expired = 0;
	}
	spin_unlock_irqrestore(&udc->lock, flags);
	return 0;
}

#if defined(CONFIG_MACH_ME_MP2530F) || defined(CONFIG_MACH_LF_MP2530F)
static irqreturn_t lf1000_vbus_irq(enum gpio_port port, enum gpio_pin pin,
				   void *_dev)
{
	struct lf1000_udc *dev = _dev;
	int mode;

	if(gpio_get_pend(VBUS_DET_PORT, VBUS_DET_PIN)) {
		mode = gpio_get_int_mode(VBUS_DET_PORT, VBUS_DET_PIN);
		switch (mode) {
		case GPIO_IMODE_HIGH_LEVEL:
			/* VBUS just went high */
			dprintk(DEBUG_VERBOSE, "VBUS on IRQ\n\n\n");
			if(dev->vbus == 0)
				lf1000_udc_vbus_session(&dev->gadget, 1);
			break;

		case GPIO_IMODE_LOW_LEVEL:
			/* VBUS just went low */
			dprintk(DEBUG_VERBOSE, "VBUS off IRQ\n\n\n");
			if(dev->vbus == 1)
				lf1000_udc_vbus_session(&dev->gadget, 0);
			break;
		default:
			/* Should never get here! */
			break;
		}
		/* Clear the interrupt */
		gpio_toggle_int_mode(VBUS_DET_PORT, VBUS_DET_PIN);
		gpio_clear_pend(VBUS_DET_PORT, VBUS_DET_PIN);
	}

	return IRQ_HANDLED;
}
#endif

/* Only call done while holding the eplock */
static void done(struct lf1000_ep *ep, struct lf1000_request *req, int status)
{
	unsigned halted = ep->halted;

	list_del_init(&req->queue);

	if (likely (req->req.status == -EINPROGRESS))
		req->req.status = status;

	ep->halted = 1; /* do we need this? */
	if(req->req.complete)
		req->req.complete(&ep->ep, &req->req);
	ep->halted = halted;
}

/* Only call nuke while holding the eplock or with interrupts disabled */
static void nuke(struct lf1000_udc *udc, struct lf1000_ep *ep, int status)
{
	while (!list_empty (&ep->queue)) {
		struct lf1000_request  *req;
		req = list_entry (ep->queue.next, struct lf1000_request, queue);
		done(ep, req, status);
	}
}

static inline void clear_ep_state (struct lf1000_udc *dev)
{
	unsigned i;
	unsigned long flags;

	spin_lock_irqsave(&dev->eplock, flags);
	for (i = 1; i < LF1000_ENDPOINTS; i++)
		nuke(dev, &dev->ep[i], -ECONNRESET);
	spin_unlock_irqrestore(&dev->eplock, flags);
}

/*
 * 	write_packet
 */
static inline int
write_packet(int fifo, struct lf1000_request *req, unsigned max)
{
	unsigned	len;
	u8		*buf;

	buf = req->req.buf + req->req.actual;
	prefetch(buf);

	len = min(req->req.length - req->req.actual, max);
	dprintk(DEBUG_VERBOSE, "write_packet %d %d %d ",req->req.actual,req->req.length,len);
	req->req.actual += len;
	dprintk(DEBUG_VERBOSE, "%d\n",req->req.actual);

	writesb(base_addr+fifo, buf, len);
	return len;
}

static void udc_reinit(struct lf1000_udc *dev);

/*
 * 	write_fifo
 *
 * send what we can.
 */
static int write_fifo(struct lf1000_ep *ep, char *buf, int len)
{
	int idx = ep->bEndpointAddress&0x7F;
	unsigned long fifo_reg = UDC_EPBUFS + 2*idx;
	int bytes_written = 0;
	unsigned short tmp;
	int fifo_nonempty;

	udc_writeh(idx, UDC_EPINDEX);

	fifo_nonempty = (udc_readh(UDC_EPSTAT)>>PSIF) & 0x3;
	if((ep->num != 0) && fifo_nonempty)
		return 0;

	if(len > ep->ep.maxpacket)
		len = ep->ep.maxpacket;

	udc_writeh(len, UDC_BWCR);

	if(unlikely((int)buf & 0x1)) {
		/* source buffer is not aligned.  Yuck. */
		while( len > 1) {
			tmp = (*buf++)&0xff;
			tmp |= ((*buf++)<<8);
			udc_writeh(tmp, fifo_reg);
			bytes_written += 2;
			len -= 2;
		}
	} else {
		while( len > 1) {
			udc_writeh(*((short *)buf), fifo_reg);
			buf += 2;
			bytes_written += 2;
			len -= 2;
		}
	}

	if(len) {
		/* write odd byte */
		tmp = *buf++;
		udc_writeh(tmp&0xff, fifo_reg);
		bytes_written += 1;
		len -= 1;
	}
	return bytes_written;
}

/* do not call unless you're holding the eplock */
static int read_fifo(struct lf1000_ep *ep, char *buf, int len)
{

	int bytes_read = 0;
	int fifo_count = 0;
	int idx = ep->bEndpointAddress&0x7F;
	unsigned long fifo_reg = UDC_EPBUFS + 2*idx;
	unsigned short tmp;
	int flush = 0;
	int fifo_empty;

	udc_writeh(idx, UDC_EPINDEX);

	/* We can't read the BRCR unless there's a valid packet. */
	fifo_empty = !((udc_readh(UDC_EPSTAT)>>PSIF) & 0x3);
	if((ep->num != 0) && fifo_empty)
		return 0;

	fifo_count = 2*udc_readh(UDC_BRCR);

	if(fifo_count == 0)
		return 0;

	if(((ep->num == 0) && (IS_SET(udc_readh(UDC_EP0STAT), EP0LWO))) ||
	   ((ep->num != 0) && (IS_SET(udc_readh(UDC_EPSTAT), EPLWO))))
		fifo_count--;

	if(len < fifo_count) {
		fifo_count = len;
		if(ep->num != 0)
			flush = 1;
	}

	if(unlikely((int)buf & 0x1)) {
		/* destination buffer is not aligned.  Yuck. */
		while( fifo_count > 1) {
			tmp = udc_readh(fifo_reg);
			*buf++ = (char)(tmp & 0xff);
			*buf++ = (char)((tmp >> 8) & 0xff);
			bytes_read += 2;
			fifo_count -= 2;
		}
	} else {
		while( fifo_count > 1) {
			*((short *)buf) = udc_readh(fifo_reg);
			buf += 2;
			bytes_read += 2;
			fifo_count -= 2;
		}
	}

	if(fifo_count) {
		/* read odd byte */
		tmp = udc_readh(fifo_reg);
		*buf++ = (char)(tmp & 0xff);
		bytes_read++;
		fifo_count--;
	}

	/* A packet must not cross a req boundary.  Flush any extra data. */
	if(flush)
		udc_writeh((1<<FLUSH), UDC_EPCTL);

	return bytes_read;
}

static int lf1000_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gf);
static int lf1000_set_halt(struct usb_ep *_ep, int value);

static int lf1000_get_status(struct lf1000_udc *dev, struct usb_ctrlrequest  *crq)
{
	u8 ep_num = crq->wIndex & 0x7F;

	switch(crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			/* All interface status bits are reserved.  Return 0. */
			dev->ifstatus = 0;
			dev->statreq.req.buf = &dev->ifstatus;
			break;
		case USB_RECIP_DEVICE:
			dev->statreq.req.buf = &dev->devstatus;
			break;
		case USB_RECIP_ENDPOINT:
			if (ep_num > LF1000_ENDPOINTS || crq->wLength > 2)
				return 1;
			udc_writeh(ep_num, UDC_EPINDEX);

			dev->ep[ep_num].status = dev->ep[ep_num].halted;
			dev->statreq.req.buf = &dev->ep[ep_num].status;
			break;
		default:
			return 1;
	}

	dev->statreq.req.length = 2;
	dev->statreq.req.actual = 0;
	dev->statreq.req.status = -EINPROGRESS;
	dev->statreq.req.complete = NULL;
	dev->statreq.req.zero = 0;
	lf1000_queue(&dev->ep[0].ep, &dev->statreq.req, GFP_ATOMIC);
	return 0;
}

static int lf1000_set_feature(struct lf1000_udc *dev, struct usb_ctrlrequest  *crq)
{
	u8 ep_num = crq->wIndex & 0x7F;
	struct lf1000_ep *ep = &dev->ep[ep_num];

	switch(crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			/* How will we support interface features? */
			return 1;
			break;
		case USB_RECIP_DEVICE:
			if (crq->wValue != USB_DEVICE_REMOTE_WAKEUP)
				return 1;
			/* What do we do here? */
			break;
		case USB_RECIP_ENDPOINT:
			if (crq->wValue != USB_ENDPOINT_HALT ||
			    ep_num > LF1000_ENDPOINTS)
				return 1;
			if (!ep->desc)
				return 1;
			if ((crq->wIndex & USB_DIR_IN)) {
				if (!ep->is_in)
					return 1;
			} else if (ep->is_in)
				return 1;
			lf1000_set_halt(&ep->ep, 1);
			break;

		default:
			return 1;
	}

	return 0;
}

static int lf1000_clear_feature(struct lf1000_udc *dev, struct usb_ctrlrequest  *crq)
{
	u8 ep_num = crq->wIndex & 0x7F;
	struct lf1000_ep *ep = &dev->ep[ep_num];

	switch(crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			return 1;
			break;
		case USB_RECIP_DEVICE:
			if (crq->wValue != USB_DEVICE_REMOTE_WAKEUP)
				return 1;
			/* What do we do here? */
			break;
		case USB_RECIP_ENDPOINT:
			if (crq->wValue != USB_ENDPOINT_HALT ||
			    ep_num > LF1000_ENDPOINTS)
				return 1;
			udc_writeh(ep_num, UDC_EPINDEX);
			if (!ep->desc)
				return 1;
			if ((crq->wIndex & USB_DIR_IN)) {
				if (!ep->is_in)
					return 1;
			} else if (ep->is_in)
				return 1;
			lf1000_set_halt(&ep->ep, 0);
			break;

		default:
			return 1;
	}

	return 0;
}

/*------------------------- usb helpers -------------------------------*/

static void clear_ep_stall(int ep)
{
	if(ep == 0) {
		udc_writeh(1<<SHT, UDC_EP0STAT);
		udc_clearbits((1<<EP0ESS), UDC_EP0CTL);
		udc_writeh(1<<SHT, UDC_EP0STAT);
	} else if(ep < LF1000_ENDPOINTS) {
		udc_writeh(ep, UDC_EPINDEX);
		udc_setbits((1<<FLUSH), UDC_EPCTL);
		udc_clearbits((1<<ESS)|(1<<INPKTHLD)|(1<<OUTPKTHLD), UDC_EPCTL);
		udc_writeh((1<<FSC)|(1<<FFS), UDC_EPSTAT);
		udc_writeh((1<<ep), UDC_EPINT);
	} else {
		printk(KERN_ERR "No such endpoint: %d\n", ep);
	}
}

static void set_ep_stall(int ep)
{
	if(ep == 0) {
		udc_setbits((1<<EP0ESS), UDC_EP0CTL);
	} else if(ep < LF1000_ENDPOINTS) {
		udc_writeh(ep, UDC_EPINDEX);
		udc_setbits((1<<ESS), UDC_EPCTL);
	} else {
		printk(KERN_ERR "No such endpoint: %d\n", ep);
	}
}

/*------------------------- usb state machine -------------------------------*/

/* return 1 for complete, 0 for incomplete */
/* do not call unless you're holding the eplock */
int write_req(struct lf1000_ep *ep, struct lf1000_request *req)
{
	int res = req->req.length - req->req.actual;
	char *src = req->req.buf + req->req.actual;
	if(res > (ep->ep.maxpacket&0x7ff))
		res = ep->ep.maxpacket&0x7ff;
	dprintk(DEBUG_VERBOSE, "Writing %d/%d bytes.\n", res + req->req.actual,
		req->req.length);
	req->req.actual += write_fifo(ep, src, res);
	if(req->req.actual == req->req.length) {
		dprintk(DEBUG_VERBOSE, "Transfer complete.\n");
		/* FIXME: We should call done on this packet when we see its TX
		 * interrupt.  But wehen we try that, we end up with some weird
		 * inconsistent list state.  Why?
		 */
		done(ep, req, 0);
		return 1;
	}
	return 0;
}

/* return 1 for complete, 0 for incomplete */
/* do not call unless you're holding the eplock */
int read_req(struct lf1000_ep *ep, struct lf1000_request *req)
{
	int res = req->req.length - req->req.actual;
	int pkt_len;
	char *dst = req->req.buf + req->req.actual;

	pkt_len = read_fifo(ep, dst, res);
	req->req.actual += pkt_len;
	dprintk(DEBUG_VERBOSE, "Read %d/%d bytes.\n", req->req.actual,
		req->req.length);

	if(pkt_len == 0)
		return 0;

	if((req->req.actual == req->req.length) ||
	   (pkt_len < ep->ep.maxpacket)) {
		dprintk(DEBUG_VERBOSE, "Transfer complete.\n");
		/* FIXME: We should call done on this packet when we see its TX
		 * interrupt.  But wehen we try that, we end up with some weird
		 * inconsistent list state.  Why?
		 */
		done(ep, req, 0);
		return 1;
	}
	return 0;
}

/* Return the recommended next state */
static int handle_setup(struct lf1000_udc *udc, struct lf1000_ep *ep, u32 csr)
{
	unsigned rxcount;
	struct usb_ctrlrequest creq;
	int next_state = EP0_IDLE, status = 0;
	u8 is_in;

	/* read SETUP; hard-fail for bogus packets */
	rxcount = read_fifo(ep, (char *)&creq, 8);
	if(unlikely(rxcount != 8)) {
		// REVISIT this happens sometimes under load; why??
		printk(KERN_ERR "setup: len %d, csr %08x\n", rxcount, csr);
		/* set stall */
		return EP0_STALL;
	}

	dprintk(DEBUG_VERBOSE, "setup: bRequestType: %02x\n"
		"bRequest: %02x\nwValue: %04x\nwIndex: %04x\nwLength: %04x\n",
		creq.bRequestType, creq.bRequest, creq.wValue, creq.wIndex,
		creq.wLength);

	is_in = creq.bRequestType & USB_DIR_IN;
	/* either invoke the driver or handle the request. */
	switch (creq.bRequest) {

	case USB_REQ_GET_STATUS:
		dprintk(DEBUG_VERBOSE, "get device status\n");
		if(lf1000_get_status(udc, &creq))
			return EP0_STALL;
		return EP0_IN_DATA_PHASE;

	case USB_REQ_CLEAR_FEATURE:
		dprintk(DEBUG_VERBOSE, "clear feature\n");
		if(lf1000_clear_feature(udc, &creq))
			return EP0_STALL;
		return EP0_STATUS_PHASE;

	case USB_REQ_SET_FEATURE:
		dprintk(DEBUG_VERBOSE, "set feature\n");
		if(lf1000_set_feature(udc, &creq))
			return EP0_STALL;
		return EP0_STATUS_PHASE;

	case USB_REQ_SET_ADDRESS:
		dprintk(DEBUG_VERBOSE, "set address\n");
		/* do we take action here?  Confirm address? */
		return EP0_STATUS_PHASE;

	case USB_REQ_GET_DESCRIPTOR:
	case USB_REQ_GET_CONFIGURATION:
	case USB_REQ_GET_INTERFACE:
		dprintk(DEBUG_VERBOSE, "get descriptor/config/interface\n");
		next_state = EP0_IN_DATA_PHASE;
		/* Invoke driver */
		break;

	case USB_REQ_SET_DESCRIPTOR:
		dprintk(DEBUG_VERBOSE, "set descriptor\n");
		next_state = EP0_OUT_DATA_PHASE;
		/* Invoke driver */
		break;

	case USB_REQ_SET_CONFIGURATION:
	case USB_REQ_SET_INTERFACE:
		dprintk(DEBUG_VERBOSE, "set config/interface\n");
		/* Invoke driver */
		next_state = EP0_STATUS_PHASE;
		break;
	default:
		/* Possibly a vendor-specific request. */
		if(is_in)
			next_state = EP0_IN_DATA_PHASE;
		else
			next_state = EP0_STATUS_PHASE;
	}

	/* pass request up to the gadget driver */
	if(udc->driver) {
		status = udc->driver->setup(&udc->gadget, &creq);
		if(status < 0) {
			if(status != -EOPNOTSUPP) {
				printk(KERN_ERR "Unknown USB request: ERR: %d\n",
				       status);
			}
			next_state = EP0_STALL;
		}
	} else {
		next_state = EP0_STALL;
	}

	return next_state;
}

static void handle_ep0(struct lf1000_udc *udc)
{
	u32			ep0csr;
	struct lf1000_ep	*ep = &udc->ep[0];
	struct lf1000_request	*req;

	udc_writeh(0, UDC_EPINDEX);
	ep0csr = udc_readh(UDC_EP0STAT);

	if(unlikely(IS_SET(ep0csr, SHT))) {
		dprintk(DEBUG_VERBOSE, "Clearing stall...\n");
		nuke(udc, ep, -EPIPE);
	    	dprintk(DEBUG_NORMAL, "... clear SENT_STALL ...\n");
	    	clear_ep_stall(0);
		udc->ep0state = EP0_IDLE;
	}

	if((!IS_SET(ep0csr, RSR)) && (!IS_SET(ep0csr, TST))) {
		dprintk(DEBUG_NORMAL,
			"Unknown ep0 interrupt: ep0 state: 0x%04x\n", ep0csr);
		return;
	}

	while(1) {

		if(list_empty(&ep->queue))
			req = NULL;
		else
			req = list_entry(ep->queue.next,
					 struct lf1000_request, queue);

		dprintk(DEBUG_VERBOSE, "ep0state %s\n",
			ep0states[udc->ep0state]);

		switch (udc->ep0state) {
		case EP0_IDLE:
			if (IS_SET(ep0csr, TST)) {
				/* just finished an IN data transfer */
				udc_writeh(1<<TST, UDC_EP0STAT);
				return;
			} else if (IS_SET(ep0csr, RSR)) {
				udc->ep0state = handle_setup(udc, ep, ep0csr);
				udc_writeh(1<<RSR, UDC_EP0STAT);
			} else {
				/* This means all TSTs and RSRs have been
				 * handled, and somebody changed the state to
				 * IDLE.  Time to return from interrupt.
				 */
				return;
			}
			break;

		case EP0_IN_DATA_PHASE:
			/* We're either here because we are launching or
			 * continuing a control data transer to the host.
			 */
			if (IS_SET(ep0csr, TST)) {
				/* successfully tx'd a fragment of current req. */
				udc_writeh(1<<TST, UDC_EP0STAT);
			}

			if(req != NULL) {
				if(write_req(ep, req))
					udc->ep0state = EP0_STATUS_PHASE;
				else
					/* wait until TST interrupt */
					return;
			} else {
				dprintk(DEBUG_NORMAL, "Probably don't wanna be here.\n");
				return;
			}
			break;

		case EP0_OUT_DATA_PHASE:
			/* We're either here because we are launching or
			 * continuing a control data transer from the host.
			 */
			/* Note: If we receive a setup packet in this phase, we
			 * must handle it.  This may be a bit tricky.
			 */
			printk(KERN_WARNING "Unsupported OUT transfer.\n");
			udc->ep0state=EP0_STALL;
			break;

		case EP0_STATUS_PHASE:
			if (IS_SET(ep0csr, TST)) {
				/* successfully tx'd a fragment of current req. */
				udc_writeh(1<<TST, UDC_EP0STAT);
			}
			udc->ep0state=EP0_IDLE;
			return;

		case EP0_STALL:
			/* If we stall, next invocation should be either a new
			 * setup transaction or a stall sent interrupt.
			 */
			set_ep_stall(0);
			udc->ep0state=EP0_IDLE;
			return;
		}
	}
}

/*
 * 	handle_ep - Manage I/O endpoints
 */
static void handle_ep(struct lf1000_ep *ep)
{
	struct lf1000_request	*req;
	u32 ep_status; /* name ambiguous with ep int status */
	u32			idx;

	req = NULL;
	if (likely(!list_empty(&ep->queue)))
		req = list_entry(ep->queue.next,
				 struct lf1000_request, queue);

	idx = (u32)(ep->bEndpointAddress&0x7F);
	udc_writeh(idx, UDC_EPINDEX);
	ep_status = udc_readh(UDC_EPSTAT);
	dprintk(DEBUG_VERBOSE, "ep%01d status:%04x.  Req pending: %s\n",
		idx, ep_status, req ? "yes" : "no");
	if(unlikely(IS_SET(ep_status, FUDR))) {
		/* handle fifo underflow */
		/* Only for ISO endpoints.  Not supported yet. */
		udc_writeh(1<<FUDR, UDC_EPSTAT);
	}

	if(unlikely(IS_SET(ep_status, FOVF))) {
		/* handle fifo overflow */
		/* Only for ISO endpoints.  Not supported yet. */
		udc_writeh(1<<FOVF, UDC_EPSTAT);
	}

	if(unlikely(IS_SET(ep_status, FFS))) {
		/* handle fifo flushed.  We probably just init'd an endpoint */
		udc_writeh(1<<FFS, UDC_EPSTAT);
	}

	if(IS_SET(ep_status, TPS)) {
		/* handle IN success */
		udc_writeh(1<<TPS, UDC_EPSTAT);
		if(req != NULL) {
			if(write_req(ep, req)) {
				/* if we just completed a req, and there is
				 * another pending, we must launch it.
				 */
				if(!list_empty(&ep->queue)) {
					req = list_entry(ep->queue.next,
							 struct lf1000_request,
							 queue);
					write_req(ep, req);
				}
			}
		} else {
			/* should we stall? */
		}
	}
	if(IS_SET(ep_status, RPS)) {
		/* handle OUT success */
		if(req != NULL)
			read_req(ep, req);

		if(ep->halted == 1) {
			dprintk(DEBUG_VERBOSE,
				"Dumping packet for halted ep\n");
			/* stall the ep and flush the data */
			set_ep_stall(ep->num);
			udc_writeh(idx, UDC_EPINDEX);
			udc_setbits((1<<FLUSH), UDC_EPCTL);
		}

		/* If the request is NULL, but the ep is configured, we leave
		 * the data in the queue. */
		udc_writeh(1<<RPS, UDC_EPSTAT);
		return;
	}
}

/*
 *      lf1000_udc_irq - interrupt handler
 */
static irqreturn_t lf1000_udc_irq(int irq, void *_dev)
{

	struct lf1000_udc      *dev = _dev;
	int usb_status, tmp;
	int ep_int, ep_status;
	int test_reg;
	int ep0csr;
	int     i;
	u32	idx;

	/* Driver connected ? */
	if (!dev->driver) {
		/* Clear interrupts */
		tmp = udc_readh(UDC_EPINT);
		udc_writeh(tmp, UDC_EPINT);
		tmp = udc_readh(UDC_TEST);
		udc_writeh(tmp, UDC_TEST);
		tmp = udc_readh(UDC_SYSSTAT);
		udc_writeh(tmp, UDC_SYSSTAT);
	}

	idx = udc_readh(UDC_EPINDEX);

	/* Read status registers */
	usb_status = udc_readh(UDC_SYSSTAT);
	ep_int = udc_readh(UDC_EPINT);
	test_reg = udc_readh(UDC_TEST);
	ep_status = udc_readh(UDC_EPSTAT);
	ep0csr = udc_readh(UDC_EP0STAT);

	dprintk(DEBUG_NORMAL, "usbs=%04x, ep_int=%04x, test=%04x "
		"ep0csr=%04x epstat=%04x\n",
		usb_status, ep_int, test_reg, ep0csr, ep_status);

#if defined(CONFIG_MACH_ME_LF1000) || defined(CONFIG_MACH_LF_LF1000)
	if (IS_SET(usb_status, VBUSOFF)) {
		dprintk(DEBUG_VERBOSE, "VBUS off IRQ\n");
		if(dev->vbus == 1)
			lf1000_udc_vbus_session(&dev->gadget, 0);
		udc_writeh(1<<VBUSOFF, UDC_SYSSTAT);
		goto irq_done;
	}

	if (IS_SET(usb_status, VBUSON)) {
		dprintk(DEBUG_VERBOSE, "VBUS on IRQ\n");
		if(dev->vbus == 0)
			lf1000_udc_vbus_session(&dev->gadget, 1);
		udc_writeh(1<<VBUSON, UDC_SYSSTAT);
		goto irq_done;
	}
#endif

	/* RESET */
	if (IS_SET(usb_status, HFRES)) {
		dprintk(DEBUG_NORMAL, "USB reset csr %x\n", ep0csr);
		if (dev->driver && dev->driver->disconnect)
			dev->driver->disconnect(&dev->gadget);
		nuke(dev, &dev->ep[0], -ECONNRESET);
		udc_writeh(1<<TST, UDC_EP0STAT);
		dev->gadget.speed = USB_SPEED_UNKNOWN;
		udc_writeh(0x00, UDC_EPINDEX);
		udc_writeh(dev->ep[0].ep.maxpacket&0x7ff, UDC_MPR);
		dev->address = 0;
		dev->ep0state = EP0_IDLE;
		clear_ep_state(dev);
		/* clear interrupt */
		udc_writeh(1<<HFRES, UDC_SYSSTAT);
		goto irq_done;
	}

	/* Speed Detect */
	if (IS_SET(usb_status, SDE)) {
		dprintk(DEBUG_NORMAL, "USB speed detect\n");
		if(dev->gadget.speed == USB_SPEED_UNKNOWN) {
			if(IS_SET(usb_status, HSP))
				dev->gadget.speed = USB_SPEED_HIGH;
			else
				dev->gadget.speed = USB_SPEED_FULL;
		}
		/* clear interrupt */
		udc_writeh(1<<SDE, UDC_SYSSTAT);
	}

	/* RESUME */
	if (IS_SET(usb_status, HFRM)) {

		dprintk(DEBUG_NORMAL, "USB resume\n");

		/* clear interrupt */
		udc_writeh(1<<HFRM, UDC_SYSSTAT);
		if (dev->gadget.speed != USB_SPEED_UNKNOWN
		    && dev->driver
		    && dev->driver->resume)
			dev->driver->resume(&dev->gadget);
	}

	/* SUSPEND */
	if (IS_SET(usb_status, HFSUSP)) {
		dprintk(DEBUG_NORMAL, "USB suspend\n");

		/* clear interrupt */
		udc_writeh(1<<HFSUSP, UDC_SYSSTAT);
		if (dev->gadget.speed != USB_SPEED_UNKNOWN
				&& dev->driver
				&& dev->driver->suspend)
			dev->driver->suspend(&dev->gadget);

		dev->ep0state = EP0_IDLE;
	}

	/* EP */
	if (IS_SET(ep_int, EP0INT)) {
		dprintk(DEBUG_VERBOSE, "USB ep0 irq\n");
		udc_writeh(1<<EP0INT, UDC_EPINT);
		handle_ep0(dev);
		goto irq_done;
	}
	/* endpoint data transfers */
	for (i = 1; i < LF1000_ENDPOINTS; i++) {
		if(IS_SET(ep_int, i)) {
			dprintk(DEBUG_VERBOSE, "USB ep%d irq\n", i);
			udc_writeh((1<<i), UDC_EPINT);
			handle_ep(&dev->ep[i]);
		}
	}

 irq_done:
	dprintk(DEBUG_VERBOSE,"irq: %d done.\n\n\n", irq);
	udc_writeh(idx, UDC_EPINDEX);

	return IRQ_HANDLED;
}

/*------------------------- lf1000_ep_ops ----------------------------------*/

/*
 * 	lf1000_ep_enable
 */
static int
lf1000_ep_enable (struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	struct lf1000_udc	*dev;
	struct lf1000_ep	*ep;
	unsigned int		max, tmp;
	unsigned long		flags;

	ep = container_of (_ep, struct lf1000_ep, ep);
	if (!_ep || !desc || ep->desc || _ep->name == ep0name
	    || desc->bDescriptorType != USB_DT_ENDPOINT)
		return -EINVAL;

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	max = le16_to_cpu(desc->wMaxPacketSize) & 0x1fff;

	_ep->maxpacket = max & 0x7ff;
	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;

	spin_lock_irqsave(&ep->dev->eplock, flags);

	tmp = udc_readh(UDC_EPINTEN);
	tmp &= ~(1<<ep->num);
	udc_writeh(tmp, UDC_EPINTEN);
	udc_writeh((1<<ep->num), UDC_EPINT);

	/* reset the endpoint */
	udc_writeh(ep->num, UDC_EPINDEX);
	tmp = udc_readh(UDC_EPDIR);
	tmp &= ~(1<<ep->num);
	udc_writeh(tmp, UDC_EPDIR);
	tmp = udc_readh(UDC_DCR);
	tmp &= ~(1<<DEN);
	udc_writeh(tmp, UDC_DCR);
	udc_writeh((1<<FLUSH)|(1<<CDP)|(3<<TNPMF), UDC_EPCTL);
	udc_writeh(0, UDC_EPCTL);

	/* set type, direction, address; reset fifo counters */
	if(desc->bEndpointAddress & USB_DIR_IN) {
		tmp = udc_readh(UDC_EPDIR);
		tmp |= 1<<ep->num;
		udc_writeh(tmp, UDC_EPDIR);
		ep->is_in = 1;
	} else {
		tmp = udc_readh(UDC_EPDIR);
		tmp &= ~(1<<ep->num);
		udc_writeh(tmp, UDC_EPDIR);
		ep->is_in = 0;
	}

	tmp = udc_readh(UDC_EPCTL);
	tmp |= (1<<FLUSH);
	udc_writeh(tmp, UDC_EPCTL);

	switch(desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_ISOC:
		tmp = udc_readh(UDC_EPCTL);
		tmp |= (1<<IME)|(3<<TNPMF);
		udc_writeh(tmp, UDC_EPCTL);
		break;

	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
	default:
		tmp = udc_readh(UDC_EPCTL);
		tmp &= ~(1<<IME);
		udc_writeh(tmp, UDC_EPCTL);
	}

	udc_writeh(max, UDC_MPR);

	clear_ep_stall(ep->num);

	/* clear any pending interrupts, and enable */
	tmp = (1<<FUDR)|(1<<FOVF)|(1<<OSD)|(1<<DTCZ)|(1<<SPT)|(1<<FFS);
	tmp |= (1<<FSC)|(1<<TPS);
	/* I think we should clear this, but ME recommends otherwise. */
	/* tmp |= (1<<RPS); */
	udc_setbits(tmp, UDC_EPSTAT);
	udc_setbits((1<<ep->num), UDC_EPINT);
	udc_setbits((1<<ep->num), UDC_EPINTEN);

	spin_unlock_irqrestore(&ep->dev->eplock, flags);

	/* print some debug message */
	tmp = desc->bEndpointAddress;
	dprintk (DEBUG_NORMAL, "enable %s(%d) ep%x%s-blk max %02x\n",
		_ep->name,ep->num, tmp, desc->bEndpointAddress & USB_DIR_IN ? "in" : "out", max);

	return 0;
}

/*
 * lf1000_ep_disable
 */
static int lf1000_ep_disable (struct usb_ep *_ep)
{
	struct lf1000_ep *ep = container_of(_ep, struct lf1000_ep, ep);
	unsigned long flags, tmp;

	if (!_ep || !ep->desc) {
		dprintk(DEBUG_NORMAL, "%s not enabled\n",
			_ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	dprintk(DEBUG_NORMAL, "ep_disable: %s\n", _ep->name);

	ep->desc = NULL;
	ep->halted = 1;

	nuke(ep->dev, ep, -ESHUTDOWN);

	/* disable irqs */
	spin_lock_irqsave(&ep->dev->eplock, flags);
	tmp = udc_readh(UDC_EPINTEN);
	tmp |= 1<<ep->num;
	udc_writeh(tmp, UDC_EPINTEN);
	spin_unlock_irqrestore(&ep->dev->lock,flags);

	dprintk(DEBUG_NORMAL, "%s disabled\n", _ep->name);

	return 0;
}

/*
 * lf1000_alloc_request
 */
static struct usb_request *
lf1000_alloc_request (struct usb_ep *_ep, gfp_t mem_flags)
{
	struct lf1000_ep	*ep;
	struct lf1000_request	*req;

    	dprintk(DEBUG_VERBOSE,"lf1000_alloc_request(ep=%p,flags=%d)\n", _ep, mem_flags);

	ep = container_of (_ep, struct lf1000_ep, ep);
	if (!_ep)
		return NULL;

	req = kzalloc (sizeof *req, mem_flags);
	if (!req)
		return NULL;
	INIT_LIST_HEAD (&req->queue);
	return &req->req;
}

/*
 * lf1000_free_request
 */
static void
lf1000_free_request (struct usb_ep *_ep, struct usb_request *_req)
{
	struct lf1000_ep	*ep;
	struct lf1000_request	*req;

	dprintk(DEBUG_VERBOSE, "lf1000_free_request(ep=%p,req=%p)\n", _ep, _req);

	ep = container_of (_ep, struct lf1000_ep, ep);
	if (!ep || !_req || (!ep->desc && _ep->name != ep0name))
		return;

	req = container_of (_req, struct lf1000_request, req);
	WARN_ON (!list_empty (&req->queue));
	kfree (req);
}

/*
 * 	lf1000_alloc_buffer
 */
static void *
lf1000_alloc_buffer (
	struct usb_ep *_ep,
	unsigned bytes,
	dma_addr_t *dma,
	gfp_t mem_flags)
{
	char *retval;

    	dprintk(DEBUG_VERBOSE,"lf1000_alloc_buffer()\n");

	if (!the_controller->driver)
		return NULL;
	retval = kmalloc (bytes, mem_flags);
	*dma = (dma_addr_t) retval;
	return retval;
}

/*
 * lf1000_free_buffer
 */
static void lf1000_free_buffer (struct usb_ep *_ep, void *buf, dma_addr_t dma,
				unsigned bytes)
{
    	dprintk(DEBUG_VERBOSE, "lf1000_free_buffer()\n");

	if (bytes)
		kfree (buf);
}

/*
 * 	lf1000_queue
 */
static int lf1000_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gf)
{
	struct lf1000_request	*req;
	struct lf1000_ep	*ep;
	struct lf1000_udc	*udc;
	unsigned long		flags;
	int			launch = 0;

	ep = container_of(_ep, struct lf1000_ep, ep);
	if (unlikely (!_req || !_ep || (!ep->desc && ep->ep.name != ep0name))) {
		dprintk(DEBUG_NORMAL, "lf1000_queue: invalid arguments.\n");
		return -EINVAL;
	}
	req = container_of(_req, struct lf1000_request, req);

	udc = ep->dev;
	if(unlikely (!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN)) {
		return -ESHUTDOWN;
	}

	if (unlikely(!_req->buf)) {
		dprintk(DEBUG_NORMAL, "lf1000_queue: Buffer not allocated!\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&ep->dev->eplock, flags);
	_req->status = -EINPROGRESS;
	_req->actual = 0;

	dprintk(DEBUG_VERBOSE,"lf1000_queue: ep%x len %d\n",
		ep->bEndpointAddress, _req->length);

	/* pio or dma irq handler advances the queue.  Don't bother with
	 * zero-length packets.  Those are handled automatically by our
	 * hardware.
	 */
	if(_req->length == 0) {
		if(_req->complete) {
			_req->status = 0;
			_req->complete(_ep, _req);
		}
	} else {
		if(list_empty(&ep->queue) &&
		   (ep->bEndpointAddress != 0)) {
			/* In this case we must launch the first tranfer.
			 * Otherwise, the interrupts that result from other
			 * transfers will launch ours.
			 */
			launch = 1;
		}
		list_add_tail(&req->queue, &ep->queue);
		if(launch == 1) {
			if(ep->bEndpointAddress & USB_DIR_IN) {
				write_req(ep, req);
			} else {
				read_req(ep, req);
			}
		}
	}
	spin_unlock_irqrestore(&ep->dev->eplock, flags);

	return 0;
}

/*
 * 	lf1000_dequeue
 */
static int lf1000_dequeue (struct usb_ep *_ep, struct usb_request *_req)
{
	int			retval = -EINVAL;
	struct lf1000_ep	*ep;
	struct lf1000_udc	*udc;
	unsigned long		flags;
	struct lf1000_request	*req = NULL;

    	dprintk(DEBUG_VERBOSE,"lf1000_dequeue(ep=%p,req=%p)\n", _ep, _req);

	if (!the_controller->driver)
		return -ESHUTDOWN;

	if (!_ep || !_req)
		return retval;
	ep = container_of (_ep, struct lf1000_ep, ep);
	udc = container_of (ep->gadget, struct lf1000_udc, gadget);

	spin_lock_irqsave(&ep->dev->eplock, flags);

	list_for_each_entry (req, &ep->queue, queue) {
		if (&req->req == _req) {
			list_del_init (&req->queue);
			_req->status = -ECONNRESET;
			retval = 0;
			break;
		}
	}

	if (retval == 0) {
		dprintk(DEBUG_VERBOSE, "dequeued req %p from %s, len %d buf %p\n",
				req, _ep->name, _req->length, _req->buf);

		done(ep, req, -ECONNRESET);
	}
	spin_lock_irqsave(&ep->dev->eplock, flags);
	return retval;
}


/*
 * lf1000_set_halt
 */
static int
lf1000_set_halt(struct usb_ep *_ep, int value)
{
	int			retval = -EINVAL;
	struct lf1000_ep	*ep;
	struct lf1000_udc	*udc;
	unsigned long		flags, len;

	if (!the_controller->driver)
		return -ESHUTDOWN;

	if (!_ep)
		return retval;

	ep = container_of (_ep, struct lf1000_ep, ep);
	udc = container_of (ep->gadget, struct lf1000_udc, gadget);

	spin_lock_irqsave(&ep->dev->eplock, flags);

	ep->halted = value;

	udc_writeh(ep->num, UDC_EPINDEX);
	len = 2*udc_readh(UDC_BRCR);
	if(ep->is_in && (!list_empty(&ep->queue) || len))
		retval = -EAGAIN;
	else {
		if (value) {
			dprintk(DEBUG_VERBOSE, "setting halt on ep%d\n",
				ep->num);
			set_ep_stall(ep->num);
		} else {
			dprintk(DEBUG_VERBOSE, "clearing halt on ep%d\n",
				ep->num);
			clear_ep_stall(ep->num);
		}
		retval = 0;
	}
	spin_lock_irqsave(&ep->dev->eplock, flags);
	return retval;
}

/* All ep ops must hold the ep spinlock to do anything. */
static const struct usb_ep_ops lf1000_ep_ops = {
	.enable         = lf1000_ep_enable,
	.disable        = lf1000_ep_disable,

	.alloc_request  = lf1000_alloc_request,
	.free_request   = lf1000_free_request,

	.alloc_buffer   = lf1000_alloc_buffer,
	.free_buffer    = lf1000_free_buffer,

	.queue          = lf1000_queue,
	.dequeue        = lf1000_dequeue,

	.set_halt       = lf1000_set_halt,
};

/*------------------------- usb_gadget_ops ----------------------------------*/

static int lf1000_get_frame (struct usb_gadget *_gadget)
{
	int tmp;
	unsigned long flags;
	struct lf1000_udc  *udc;

	dprintk(DEBUG_VERBOSE, "lf1000_get_frame()\n");

	udc = container_of (_gadget, struct lf1000_udc, gadget);
	spin_lock_irqsave(&udc->lock, flags);
	tmp = udc_readh(UDC_FRAMENUM) & 0x7ff;
	spin_unlock_irqrestore(&udc->lock, flags);

	return tmp;
}

/*
 * 	lf1000_wakeup
 */
static int lf1000_wakeup (struct usb_gadget *_gadget)
{

	dprintk(DEBUG_NORMAL,"lf1000_wakeup()\n");

	return 0;
}

/*
 * 	lf1000_set_selfpowered
 */
static int lf1000_set_selfpowered(struct usb_gadget *_gadget, int value)
{
	struct lf1000_udc  *udc;
	unsigned long flags;

	dprintk(DEBUG_NORMAL, "lf1000_set_selfpowered()\n");

	udc = container_of (_gadget, struct lf1000_udc, gadget);

	spin_lock_irqsave(&udc->lock, flags);
	if (value)
		udc->devstatus |= (1 << USB_DEVICE_SELF_POWERED);
	else
		udc->devstatus &= ~(1 << USB_DEVICE_SELF_POWERED);
	spin_unlock_irqrestore(&udc->lock, flags);
	return 0;
}

/* All gadget ops must hold the dev spinlock to do anything. */
static const struct usb_gadget_ops lf1000_ops = {
	.get_frame          = lf1000_get_frame,
	.wakeup             = lf1000_wakeup,
	.set_selfpowered    = lf1000_set_selfpowered,
	.pullup             = NULL,
	.vbus_session	    = lf1000_udc_vbus_session,
};

/*------------------------- gadget driver handling---------------------------*/
/*
 * udc_disable
 */
static void udc_disable(struct lf1000_udc *dev)
{
	int tmp;
	int i;

	dprintk(DEBUG_NORMAL, "udc_disable called\n");

	lf1000_vbus_command(LF1000_UDC_VBUS_DISABLE);

#ifdef CPU_LF1000
	/* disable phy block */
	tmp = udc_readh(UDC_PCR);
	tmp |= 1<<PCE;
	udc_writeh(tmp, UDC_PCR);
#endif

	/* Disable all interrupts */
	udc_writeh((1<<RRDE), UDC_SYSCTL);
	udc_writeh(0, UDC_EPINTEN);

	/* Clear the interrupt registers */
	udc_writeh(0xFFFF, UDC_SYSSTAT);
	udc_writeh(0xFFFF, UDC_EPINT);
	tmp = udc_readh(UDC_TEST);
	udc_writeh(tmp, UDC_TEST);

	/* Reset endpoint 0 */
	udc_writeh((1<<RSR)|(1<<TST)|(1<<SHT), UDC_EP0STAT);
	udc_writeh((1<<TTS), UDC_EP0CTL); /* Why? */
	udc_writeh((dev->ep[0].ep.maxpacket&0x7ff), UDC_MPR);

	/* Reset endpoints */
	udc_writeh(0, UDC_EPINTEN);
	udc_writeh(0xFFFF, UDC_EPINT);
	udc_writeh(0, UDC_EPDIR);
	for(i=1; i<LF1000_ENDPOINTS; i++) {
		udc_writeh(i, UDC_EPINDEX);
		tmp = udc_readh(UDC_DCR);
		tmp &= ~(1<<DEN);
		udc_writeh(tmp, UDC_DCR);
		udc_writeh((1<<FLUSH)|(1<<CDP)|(3<<TNPMF), UDC_EPCTL);
	}

	/* Set speed to unknown */
	dev->gadget.speed = USB_SPEED_UNKNOWN;
}
/*
 * udc_reinit
 */
static void udc_reinit(struct lf1000_udc *dev)
{
	u32     i;

	/* device/ep0 records init */
	INIT_LIST_HEAD (&dev->gadget.ep_list);
	INIT_LIST_HEAD (&dev->gadget.ep0->ep_list);
	dev->ep0state = EP0_IDLE;


	for (i = 0; i < LF1000_ENDPOINTS; i++) {
		struct lf1000_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail (&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->dev = dev;
		ep->desc = NULL;
		ep->halted = 1;
		INIT_LIST_HEAD (&ep->queue);
	}
}

/*
 * udc_enable
 */
static void udc_enable(struct lf1000_udc *dev)
{
	int i;
	int tmp;

	dprintk(DEBUG_NORMAL, "udc_enable called\n");

	/* dev->gadget.speed = USB_SPEED_UNKNOWN; */
	dev->gadget.speed = USB_SPEED_FULL;

	/* Reverse byte order */
	tmp = udc_readh(UDC_SYSCTL);
	BIT_SET(tmp, RRDE);
	udc_writeh(tmp, UDC_SYSCTL);

	/* Set MAXP for all endpoints */
	for (i = 0; i < LF1000_ENDPOINTS; i++) {
		udc_writeh(i, UDC_EPINDEX);
		if(i==0)
			udc_writeh((1<<RSR)|(1<<TST)|(1<<SHT), UDC_EP0STAT);
		else {
			tmp = udc_readh(UDC_DCR);
			BIT_CLR(tmp, DEN);
			udc_writeh(tmp, UDC_DCR);
			udc_writeh((1<<CDP)|(1<<FLUSH)|(3<<TNPMF), UDC_EPCTL);
		}
		udc_writeh((dev->ep[i].ep.maxpacket&0x7ff), UDC_MPR);
	}

#ifdef CPU_LF1000
	/* disable phy block */
	tmp = udc_readh(UDC_PCR);
	tmp |= 1<<PCE;
	udc_writeh(tmp, UDC_PCR);

	/* enable phy block */
	tmp = udc_readh(UDC_PCR);
	tmp &= ~(1<<PCE);
	udc_writeh(tmp, UDC_PCR);
#endif

	/* Clear and enable reset and suspend interrupt interrupts */
	udc_writeh(0xff, UDC_SYSSTAT);
	tmp = udc_readh(UDC_SYSCTL);
	BIT_SET(tmp, HSUSPE);
	BIT_SET(tmp, HRESE);
	BIT_SET(tmp, SPDEN);
	udc_writeh(tmp, UDC_SYSCTL);

	/* Clear and enable ep0 interrupt */
	udc_writeh(0xff, UDC_EPINT);
	tmp = udc_readh(UDC_EPINTEN);
	BIT_SET(tmp, EP0INTEN);
	udc_writeh(tmp, UDC_EPINTEN);

	/* enable the vbus interrupt and let the vbus signal through */
	lf1000_vbus_command(LF1000_UDC_VBUS_ENABLE);

}

/*
 * 	nop_release
 */
static void nop_release (struct device *dev)
{
	dprintk(DEBUG_NORMAL, "%s %s\n", __FUNCTION__, dev->bus_id);
}
/*
 *	usb_gadget_register_driver
 */
int
usb_gadget_register_driver (struct usb_gadget_driver *driver)
{
	struct lf1000_udc *udc = the_controller;
	int		retval;

    	dprintk(DEBUG_NORMAL, "usb_gadget_register_driver() '%s'\n",
		driver->driver.name);

	/* Sanity checks */
	if (!udc)
		return -ENODEV;
	if (udc->driver)
		return -EBUSY;
	if (!driver->bind || !driver->setup) {
		printk(KERN_ERR "Invalid driver : bind %p setup %p\n",
		       driver->bind, driver->setup);
		return -EINVAL;
	}
#if defined(MODULE)
	if (!driver->unbind) {
		printk(KERN_ERR "Invalid driver : no unbind method\n");
		return -EINVAL;
	}
#endif

	/* Hook the driver */
	udc->driver = driver;
	udc->gadget.dev.driver = &driver->driver;

	/*Bind the driver */
	device_add(&udc->gadget.dev);
	dprintk(DEBUG_NORMAL, "binding gadget driver '%s'\n", driver->driver.name);
	if ((retval = driver->bind(&udc->gadget)) != 0) {
		printk(KERN_ERR "Failed to bind gadget driver: %d\n", retval);
		device_del(&udc->gadget.dev);
		udc->driver = NULL;
		udc->gadget.dev.driver = NULL;
		return retval;
	}

	/* Enable udc */
	udc_enable(udc);
	return 0;
}


/*
 * 	usb_gadget_unregister_driver
 */
int
usb_gadget_unregister_driver (struct usb_gadget_driver *driver)
{
	struct lf1000_udc *udc = the_controller;

	if (!udc)
		return -ENODEV;
	if (!driver || driver != udc->driver)
		return -EINVAL;

    	dprintk(DEBUG_NORMAL,"usb_gadget_register_driver() '%s'\n",
		driver->driver.name);

	if (driver->disconnect)
		driver->disconnect(&udc->gadget);

	if (driver->unbind)
		driver->unbind (&udc->gadget);

	device_del(&udc->gadget.dev);
	udc->driver = NULL;

	/* Disable udc */
	udc_disable(udc);

	return 0;
}

/*---------------------------------------------------------------------------*/
static struct lf1000_udc udc_dev = {

	.gadget = {
		.ops		= &lf1000_ops,
		.ep0		= &udc_dev.ep[0].ep,
		.name		= gadget_name,
		.dev = {
			.bus_id		= "gadget",
			.release	= nop_release,
		},
	},

	/* control endpoint */
	.ep[0] = {
		.num		= 0,
		.ep = {
			.name		= ep0name,
			.ops		= &lf1000_ep_ops,
			.maxpacket	= EP0_FIFO_SIZE,
		},
		.dev		= &udc_dev,
		.is_in = 1,
	},

	/* first group of endpoints */
	.ep[1] = {
		.num		= 1,
		.ep = {
			.name		= "ep1-bulk",
			.ops		= &lf1000_ep_ops,
			.maxpacket	= EP_FIFO_SIZE,
		},
		.dev		= &udc_dev,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = 1,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},
	.ep[2] = {
		.num		= 2,
		.ep = {
			.name		= "ep2-bulk",
			.ops		= &lf1000_ep_ops,
			.maxpacket	= EP_FIFO_SIZE,
		},
		.dev		= &udc_dev,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = 2,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},
#if defined(CPU_MF2530F)
	.ep[3] = {
		.num		= 3,
		.ep = {
			.name		= "ep3-bulk",
			.ops		= &lf1000_ep_ops,
			.maxpacket	= EP_FIFO_SIZE,
		},
		.dev		= &udc_dev,
		.fifo_size	= EP_FIFO_SIZE,
		.bEndpointAddress = 3,
		.bmAttributes	= USB_ENDPOINT_XFER_BULK,
	},
#endif
};

/*
 * 	lf1000_udc_remove
 */
static int lf1000_udc_remove(struct platform_device *pdev)
{
	struct lf1000_udc *udc = platform_get_drvdata(pdev);

	dprintk(DEBUG_NORMAL, "lf1000_udc_remove\n");

#ifdef ENABLE_SYSFS
	/* remove device files */
	device_remove_file(&pdev->dev, &dev_attr_regs);
	device_remove_file(&pdev->dev, &dev_attr_vbus);
	device_remove_file(&pdev->dev, &dev_attr_watchdog_seconds);
	device_remove_file(&pdev->dev, &dev_attr_watchdog_expired);
#endif

	lf1000_vbus_command(LF1000_UDC_VBUS_SHUTDOWN);

	if(base_addr != 0) {
		udc_writeh(0, UDC_CLKEN);
#if 0		
		iounmap(base_addr);
		release_mem_region(rsrc_start, rsrc_len);
#endif		
	}

	platform_set_drvdata(pdev, NULL);

	usb_gadget_unregister_driver(udc->driver);

	free_irq(udc->irq, udc);

    pollux_gpio_setpin(POLLUX_GPB12, 0);  /* lcd vbus off */
    
	return 0;
}

/*
 *	probe - binds to the platform device
 */
static int lf1000_udc_probe(struct platform_device *pdev)
{
	struct lf1000_udc *udc = &udc_dev;
	int retval = 0;
	struct resource *res;

	dprintk(DEBUG_NORMAL,"lf1000_udc_probe\n");

	/* set up driver basics */
	the_controller = udc;
	platform_set_drvdata(pdev, udc);

	/* get iomem */
	base_addr = 0;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "lf1000_udc_probe: failed to get resource\n");
		retval = -ENXIO;
		goto fail;
	}

#if 0
	rsrc_start = res->start;
	rsrc_len   = (res->end - res->start) + 1;

	if(!request_mem_region(rsrc_start, rsrc_len, "lf1000_udc")) {
		printk(KERN_ERR "lf1000_udc_probe: failed to get io region\n");
		retval = -EBUSY;
		goto fail;
	}

	base_addr = ioremap(rsrc_start, rsrc_len);

	if(base_addr == NULL) {
		printk(KERN_ERR "lf1000_udc_probe: failed to ioremap\n");
		retval = -ENOMEM;
		goto fail;
	}
#else
	base_addr = (void __iomem *)(res->start);
	gprintk("base addr = %x\n", base_addr);
#endif	

	/* Set up clock */
	udc_writeh((0<<UDC_CLKDIV)|(0x3<<UDC_CLKSRCSEL), UDC_CLKGEN);
	udc_writeh((1<<UDC_PCLKMODE)|(1<<UDC_CLKGENENB)|(3<<UDC_CLKENB),
		   UDC_CLKEN);

	/* Add the usb gadget bus */
	spin_lock_init(&udc->lock);
	spin_lock_init(&udc->eplock);
	device_initialize(&udc->gadget.dev);
	udc->gadget.dev.parent = &pdev->dev;
	udc->gadget.dev.dma_mask = pdev->dev.dma_mask;
	udc->gadget.is_dualspeed = 1;
	udc->devstatus = 0;

	udc_disable(udc);
	udc_reinit(udc);

	/* irq setup after old hardware state is cleaned up */
	udc->irq = platform_get_irq(pdev, 0);
	if(udc->irq < 0) {
		printk(KERN_ERR "%s: can't get IRQ\n", gadget_name);
		retval = udc->irq;
		goto fail;
	}
	retval = request_irq(udc->irq, lf1000_udc_irq,
			IRQF_DISABLED, gadget_name, udc);

	if (retval != 0) {
		printk(KERN_ERR "%s: can't get irq %i, err %d\n",
			gadget_name, udc->irq, retval);
		retval = -EBUSY;
		goto fail;
	}
	dprintk(DEBUG_VERBOSE, "%s: got irq %i\n", gadget_name, udc->irq);

	/* init the vbus signal */
	retval = lf1000_vbus_command(LF1000_UDC_VBUS_INIT);
	if(retval != 0) {
		printk(KERN_ALERT "Failed to init vbus pin\n");
		goto fail;
	}

#ifdef ENABLE_SYSFS
	/* create device files */
	device_create_file(&pdev->dev, &dev_attr_regs);
	device_create_file(&pdev->dev, &dev_attr_vbus);
	device_create_file(&pdev->dev, &dev_attr_watchdog_seconds);
	device_create_file(&pdev->dev, &dev_attr_watchdog_expired);
#endif

    pollux_gpio_setpin(POLLUX_GPB12, 1);  /* lcd vbus on */
    
	return 0;

 fail:
	lf1000_udc_remove(pdev);
	return retval;
}

#ifdef CONFIG_PM
static int lf1000_udc_suspend(struct platform_device *pdev, pm_message_t message)
{
#if 0
	struct lf1000_udc *udc = platform_get_drvdata(pdev);

	if (udc_info && udc_info->udc_command)
		udc_info->udc_command(LF1000_UDC_P_DISABLE);
#endif
	return 0;
}

static int lf1000_udc_resume(struct platform_device *pdev)
{
#if 0
	struct lf1000_udc *udc = platform_get_drvdata(pdev);

	if (udc_info && udc_info->udc_command)
		udc_info->udc_command(LF1000_UDC_P_ENABLE);
#endif
	return 0;
}
#else
#define lf1000_udc_suspend      NULL
#define lf1000_udc_resume       NULL
#endif


static struct platform_driver lf1000_udc_driver = {
	.driver		= {
		//.name 	= "lf1000-usbgadget",
		.name 	= "pollux-usbgadget",
		.owner	= THIS_MODULE,
	},
	.probe          = lf1000_udc_probe,
	.remove         = lf1000_udc_remove,
	.suspend	= lf1000_udc_suspend,
	.resume		= lf1000_udc_resume,
};

static int __init udc_init(void)
{
    
	dprintk(DEBUG_NORMAL, "%s: version %s\n", gadget_name, DRIVER_VERSION);
	printk("%s: version %s\n", gadget_name, DRIVER_VERSION);

	return platform_driver_register(&lf1000_udc_driver);
}

static void __exit udc_exit(void)
{
	platform_driver_unregister(&lf1000_udc_driver);
}


EXPORT_SYMBOL (usb_gadget_unregister_driver);
EXPORT_SYMBOL (usb_gadget_register_driver);

module_init(udc_init);
module_exit(udc_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");


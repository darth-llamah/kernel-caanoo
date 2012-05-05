/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * Bus Glue for pollux
 *
 * This file is licenced under the GPL.
 */

#include <linux/device.h>
#include <linux/signal.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>
#include <asm/hardware.h>
#include <asm/arch/regs-clock-power.h>	



/* ghcstop: Debugging stuff */
#if 0
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif




#if 0
static int pollux_start_hc(struct platform_device *pdev)
{
	unsigned long regbase = pdev->resource[0].start;
	unsigned long val;

	gprintk("pollux_start_hc\n");
	val = 0xf;
	__raw_writel( val, regbase + 0x1C0);
	val = ((1 << 4) | (2<<1));
	__raw_writel( val, regbase + 0x1C4);
	

	return 0;
}

static void pollux_stop_hc(struct platform_device *pdev)
{
	unsigned long regbase = pdev->resource[0].start;
	unsigned long val;
	
	gprintk("pollux_stop_hc\n");

	val = 0x0;
	__raw_writel( val, regbase + 0x1C0);
}
#else
static int pollux_start_hc(void)
{
	unsigned long c;
	int div;
	
	
	c = get_pll_clk(CLKSRC_PLL0);
	
	div = c/48000000;

	gprintk("1 div = %d\n", div);
	
	if( c%48000000 )
	{
		printk(KERN_ERR "pollux_start_hc: USB clock div error\n");
		return -1;
	}
	
	
	div -= 1;
	
	gprintk("2 div = %d\n", div);

#if 1	
	*(u32 *)(POLLUX_VA_OHCI + 0xC4) = (div<<4); 
	*(u32 *)(POLLUX_VA_OHCI + 0xC0) = ((1 << 3) | (1<<2) | (3<<0));
	*(u32 *)(POLLUX_VA_OHCI + 0x80) = (3 << 3);
#else	
	//*(u32 *)(POLLUX_VA_OHCI + 0xC4) = (7<<4); // pll0 384Mhz, div = 8: 8 - 1 = 7
	*(u32 *)(POLLUX_VA_OHCI + 0xC4) = (10<<4); // pll0 528Mhz, div = 11: 11 - 1 = 10
	*(u32 *)(POLLUX_VA_OHCI + 0xC0) = ((1 << 3) | (1<<2) | (3<<0));
	*(u32 *)(POLLUX_VA_OHCI + 0x80) = (3 << 3);
#endif	
	

	return 0;
}

static void pollux_stop_hc(void)
{
	gprintk("pollux_stop_hc\n");

	*(u32 *)(POLLUX_VA_OHCI + 0x1C0) = 0x0;
}
#endif


/**
 * usb_hcd_pollux_probe - initialize pollux-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 */
int usb_hcd_pollux_probe (const struct hc_driver *driver, struct platform_device *pdev)
{
	int retval;
	struct usb_hcd *hcd;

	gprintk("ohci_hcd_pollux_probe, %p, %p\n", driver, driver->start);

	/* allocate memory for usb_hcd struct. defined usb/ocre/hcd.c */
	hcd = usb_create_hcd (driver, &pdev->dev, "pollux");
	if (!hcd)
	{
		gprintk("pollux-hcd register error\n");
		return -ENOMEM;
	}

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len   = pdev->resource[0].end - pdev->resource[0].start + 1;
	//hcd->rsrc_len   = 0x00000fff;
	
	gprintk("hcd->rsrc_len = %d\n", hcd->rsrc_len);
	gprintk("pdev->resource[0].end = %x\n", pdev->resource[0].end);
	gprintk("aastart = %x, %x\n", hcd->rsrc_start, pdev->resource[0].end);

	hcd->regs = (void __iomem *)((unsigned long)hcd->rsrc_start);

	if ((retval = pollux_start_hc()) < 0) {
		pr_debug("pollux_start_hc failed");
		goto err1;
	}

	gprintk("a\n");
	ohci_hcd_init(hcd_to_ohci(hcd));
	gprintk("b\n");

	retval = usb_add_hcd(hcd, pdev->resource[1].start, IRQF_DISABLED);
gprintk("c\n");	
	if (retval == 0)
		return retval;

	pollux_stop_hc();
//err2:
//	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return retval;
}


/**
 * usb_hcd_pollux_remove - shutdown processing for pollux-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_pollux_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 */
void usb_hcd_pollux_remove (struct usb_hcd *hcd, struct platform_device *pdev)
{
	gprintk("ohci_hcd_pollux_remove\n");

	usb_remove_hcd(hcd);
	pollux_stop_hc();
	usb_put_hcd(hcd);
}

static int __devinit ohci_pollux_start (struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ret;

gprintk("0\n");
	//ohci_dbg (ohci, "ohci_pollux_start, ohci:%p", ohci);
	gprintk("ohci_pollux_start, ohci:%p\n", (ohci)?ohci:NULL);

	/* The value of NDP in roothub_a is incorrect on this hardware */
	ohci->num_ports = 3;
	
	gprintk("1\n");

	if ((ret = ohci_init(ohci)) < 0)
	{
		gprintk("2\n");
		return ret;
	}
	gprintk("3\n");

	if ((ret = ohci_run (ohci)) < 0) 
	{
		gprintk("4\n");
		err ("can't start %s", hcd->self.bus_name);
		ohci_stop (hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver ohci_pollux_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"POLLUX OHCI",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =		ohci_pollux_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
	.hub_irq_enable =	ohci_rhsc_enable,
#ifdef  CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};


static int ohci_hcd_pollux_drv_probe(struct platform_device *pdev)
{
	gprintk("ohci_hcd_pollux_drv_probe\n");

	return usb_hcd_pollux_probe(&ohci_pollux_hc_driver, pdev);
}

static int ohci_hcd_pollux_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	gprintk("ohci_hcd_pollux_drv_remove\n");

	usb_hcd_pollux_remove(hcd, pdev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef	CONFIG_PM
static int ohci_hcd_pollux_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);

	gprintk("ohci_hcd_pollux_drv_suspend\n");

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	pollux_stop_hc();
	hcd->state = HC_STATE_SUSPENDED;
	pdev->dev.power.power_state = PMSG_SUSPEND;

	return 0;
}

static int ohci_hcd_pollux_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int status;

	gprintk("ohci_hcd_pollux_drv_resume\n");

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	if ((status = pollux_start_hc()) < 0)
		return status;

	pdev->dev.power.power_state = PMSG_ON;
	usb_hcd_resume_root_hub(hcd);

	return 0;
}
#endif


static struct platform_driver ohci_hcd_pollux_driver = {
	.probe		= ohci_hcd_pollux_drv_probe,
	.remove		= ohci_hcd_pollux_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
#ifdef CONFIG_PM
	.suspend	= ohci_hcd_pollux_drv_suspend,
	.resume		= ohci_hcd_pollux_drv_resume,
#endif
	.driver		= {
		.name	= POLLUX_HCD_NAME,
	},
};


#if 0
static int __init ohci_hcd_pollux_init (void)
{
	int ret = 0;

	gprintk("ohci_hcd_pollux_init\n");

	/* register platform driver, exec platform_driver probe */
	ret = platform_driver_register(&ohci_hcd_pollux_driver);
	if(ret)
	{
		printk(KERN_ERR "failed to add platrom driver %s (%d) \n", ohci_hcd_pollux_driver.driver.name, ret);
	}

	return ret;
}

static void __exit ohci_hcd_pollux_exit (void)
{
	gprintk("ohci_hcd_pollux_exit\n");
	platform_driver_unregister(&ohci_hcd_pollux_driver);
}

module_init (ohci_hcd_pollux_init);
module_exit (ohci_hcd_pollux_exit);
#endif
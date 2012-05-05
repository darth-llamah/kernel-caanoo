/* 
 * drivers/pollux/spi/main.c
 *
 * POLLUX SPI/SSP Driver
 *
 * Copyright 2007 LeapFrog Enterprises Inc.
 *
 * Scott Esters <sesters@leapfrog.com>
 * Andrey Yurovsky <andrey@cozybit.com>
 *
 * TODO: integrate with kernel SPI API, see drivers/spi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-clock-power.h>

#include <asm/arch/regs-spi.h>


#ifdef CONFIG_SPI_POLLUX_DEBUG
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif

/* ghcstop_caution: This driver is not formed for Linux SPI master stack. Just works......like bulk driver */

/* clock settings */
#define SPI_SRC_HZ      10000000
#define SPI_CLK_SRC     CLKSRC_PLL1


struct pollux_spi_device
{
    void           *mem;
    int             irq;        // assigned IRQ number
    spinlock_t      spi_spinlock;   // serialize spi bus access
    int             isBusy;     // 1 = spi bus is busy
    wait_queue_head_t wqueue;   // waiting for spi bus lock
    wait_queue_head_t intqueue; // waiting for an interrupt
#ifdef CONFIG_PROC_FS
    struct proc_dir_entry *proc_port;
#endif
};

#define SPI_CHANNEL	CONFIG_SPI_POLLUX_CHANNEL

/* ghcstop_caution: re-check hardware pin-map */
#if   (SPI_CHANNEL == 0)
	#define GPIO_SSPFRM   POLLUX_GPB12
	#define GPIO_SSPCLK   POLLUX_GPB13
	#define GPIO_SSPRXD   POLLUX_GPB14
	#define GPIO_SSPTXD   POLLUX_GPB15
	#define SPI_GPIO_FUNC POLLUX_GPIO_MODE_ALT1  // except for SSPSRM, SSPSRM set to gpioout
#elif (SPI_CHANNEL == 1)
	#define GPIO_SSPFRM   POLLUX_GPC3
	#define GPIO_SSPCLK   POLLUX_GPC4
	#define GPIO_SSPRXD   POLLUX_GPC5
	#define GPIO_SSPTXD   POLLUX_GPC6
	#define SPI_GPIO_FUNC POLLUX_GPIO_MODE_ALT1
#elif (SPI_CHANNEL == 2)
	#define GPIO_SSPFRM   POLLUX_GPB5
	#define GPIO_SSPCLK   POLLUX_GPB0
	#define GPIO_SSPRXD   POLLUX_GPB1
	#define GPIO_SSPTXD   POLLUX_GPB2
	#define SPI_GPIO_FUNC POLLUX_GPIO_MODE_ALT2
#else
	#error CONFIG_SPI_POLLUX_CHANNEL must be in 0 ... 2
#endif

struct pollux_spi_device spi;

/*******************
 * sysfs Interface *
 *******************/

#ifdef CONFIG_SPI_POLLUX_DEBUG
static ssize_t
show_registers(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t         len = 0;

    len += sprintf(buf + len, "SPI_CONT0  = 0x%04X\n", ioread16(spi.mem + SSPSPICONT0));
    len += sprintf(buf + len, "SPI_CONT1  = 0x%04X\n", ioread16(spi.mem + SSPSPICONT1));
    len += sprintf(buf + len, "SPI_DATA   = 0x%04X\n", ioread16(spi.mem + SSPSPIDATA));
    len += sprintf(buf + len, "SPI_STATE  = 0x%04X\n", ioread16(spi.mem + SSPSPISTATE));
    len += sprintf(buf + len, "SPI_CLKENB = 0x%08X\n", ioread32(spi.mem + SSPSPICLKENB));
    len += sprintf(buf + len, "SPI_CLKGEN = 0x%04X\n", ioread16(spi.mem + SSPSPICLKGEN));

    return len;
}

static          DEVICE_ATTR(registers, S_IRUSR | S_IRGRP, show_registers, NULL);

static struct attribute *spi_attributes[] = {
    &dev_attr_registers.attr,
    NULL
};

static struct attribute_group spi_attr_group = {
    .attrs = spi_attributes
};
#endif                          /* CONFIG_SPI_POLLUX_DEBUG */

/**********************
 * Interrupt Handling *
 *********************/

static void
pollux_spi_interrupt(u8 en)
{
    u16             reg = ioread16(spi.mem + SSPSPISTATE);

    if (en)
    {
        BIT_SET(reg, IRQEENB);  // RX, TX completed
        BIT_SET(reg, IRQWENB);  // TX Buffer Empty
        BIT_SET(reg, IRQRENB);  // RX Buffer Full
        BIT_SET(reg, IRQE);     // clear status bits
        BIT_SET(reg, IRQW);
        BIT_SET(reg, IRQR);
    }
    else
    {
        BIT_CLR(reg, IRQEENB);  // RX, TX completed
        BIT_CLR(reg, IRQWENB);  // TX Buffer Empty
        BIT_CLR(reg, IRQRENB);  // RX Buffer Full
    }
    iowrite16(reg, spi.mem + SSPSPISTATE);
}

static irqreturn_t spi_irq(int irq, void *dev_id)
{
    u16             reg;

    reg = ioread16(spi.mem + SSPSPISTATE);

    if (IS_CLR(reg, IRQE))
    {                           /* interrupt not from SPI */
        return (IRQ_NONE);
    }

    pollux_spi_interrupt(0);
    wake_up_interruptible(&spi.intqueue);   /* wake any tasks */
    return (IRQ_HANDLED);
}


static int
spi_bus_available(void)
{
    unsigned long   flags;
    int             ret;

    spin_lock_irqsave(&spi.spi_spinlock, flags);
    ret = spi.isBusy ? 0 : 1;
    spin_unlock_irqrestore(&spi.spi_spinlock, flags);
    return (ret);
}

/*
 * spi_xfer -- transfer data via SPI
 *
 * Write 16 bits, read 16 bits.
 *
 * Serialize access to SPI bus.  Clear any data in FIFOs, TX data then RX data
 */
int
pollux_spi_xfer(u16 data)
{
    unsigned long   flags;

    if (spi.mem == NULL)
    {
        printk(KERN_ALERT "SPI register pointer is NULL\n");
        return -1;
    }

    /*
     *  wait for our turn on the SPI bus
     */

    spin_lock_irqsave(&spi.spi_spinlock, flags);
    while (spi.isBusy)
    {
        spin_unlock_irqrestore(&spi.spi_spinlock, flags);

        if (wait_event_interruptible(spi.wqueue, (spi_bus_available())))
            return -ERESTARTSYS;
        spin_lock_irqsave(&spi.spi_spinlock, flags);
    }

    spi.isBusy = 1;             /* got the bus */
    spin_unlock_irqrestore(&spi.spi_spinlock, flags);

    /*
     * Clear TX and RX data from SPI device 
     */

    if (IS_CLR(ioread16(spi.mem + SSPSPISTATE), WFFEMPTY))
    {
        /*
         * wait for tx buffer empty 
         */
        pollux_spi_interrupt(1);
        if (wait_event_interruptible(spi.intqueue, (IS_SET(ioread16(spi.mem + SSPSPISTATE), WFFEMPTY))))
        {
            printk(KERN_ERR "SPI: TX Empty error\n");
            spi.isBusy = 0;
            spin_unlock_irqrestore(&spi.spi_spinlock, flags);
            return -ERESTARTSYS;
        }
    }

    while (!IS_SET(ioread16(spi.mem + SSPSPISTATE), RFFEMPTY))
    {
        ioread16(spi.mem + SSPSPIDATA);
    }

    /*
     * Transmit Data 
     */
    //gpio_set_val(SPI_GPIO_PORT, SPI_SSPFRM_PIN, 0);
   	pollux_gpio_setpin(GPIO_SSPFRM, POLLUX_GPIO_LOW_LEVEL);   // --> Low
    pollux_spi_interrupt(1);
    iowrite16(data, spi.mem + SSPSPIDATA);

    /*
     * Wait for Receive Data 
     */
    if (wait_event_interruptible(spi.intqueue, (IS_CLR(ioread16(spi.mem + SSPSPISTATE), RFFEMPTY))))
    {
        printk(KERN_ERR "SPI: RX error\n");
        spi.isBusy = 0;
        spin_unlock_irqrestore(&spi.spi_spinlock, flags);
        return -ERESTARTSYS;
    }
    //gpio_set_val(SPI_GPIO_PORT, SPI_SSPFRM_PIN, 1);
    pollux_gpio_setpin(GPIO_SSPFRM, POLLUX_GPIO_HIGH_LEVEL);  // --> High

    /*
     * release SPI bus 
     */
    spin_lock_irqsave(&spi.spi_spinlock, flags);
    spi.isBusy = 0;
    spin_unlock_irqrestore(&spi.spi_spinlock, flags);

    wake_up_interruptible(&spi.wqueue);
    return (ioread16(spi.mem + SSPSPIDATA));
}

EXPORT_SYMBOL(pollux_spi_xfer);

/****************************************
 *  module functions and initialization *
 ****************************************/

static int
pollux_spi_probe(struct platform_device *pdev)
{
    int             ret = 0;
    int             div;
    struct resource *res;

    res = platform_get_resource(pdev, IORESOURCE_MEM, SPI_CHANNEL); //0, 1, 2, get io base memory
    if (!res)
    {
        printk(KERN_ERR "spi: failed to get resource\n");
        return -ENXIO;
    }

    div = pollux_calc_divider(get_pll_clk(SPI_CLK_SRC), SPI_SRC_HZ); // pll1, 10000000
    if (div < 0)
    {
        printk(KERN_ERR "spi: failed to find a clock divider!\n");
        return -EFAULT;
    }

	gprintk("res->start = 0x%x\n", res->start );

    spi.mem = (void *)(res->start);
    if (spi.mem == NULL)
    {
        printk(KERN_ERR "spi: failed to ioremap\n");
        ret = -ENOMEM;
        goto fail;
    }

    /*
     * set SPI for 1 MHz devices 
     */
    iowrite16(((div - 1) << SPI_CLKDIV) | (SPI_CLK_SRC << SPI_CLKSRCSEL), spi.mem + SSPSPICLKGEN);
    /*
     * start clock generation 
     */
    iowrite32((1 << SPI_PCLKMODE) | (1 << SPI_CLKGENENB), spi.mem + SSPSPICLKENB);
    /*
     * output only for transmission, enable, 16 bits, divide internal clock by 20 to get ~1MHz 
     */
    iowrite16((0 << ZENB) | (0 << RXONLY) | (1 << ENB) | ((16 - 1) << NUMBIT) | (20 << DIVCNT), spi.mem + SSPSPICONT0);
    /*
     * invert SPI clock, Format B, SPI type 
     */
    iowrite16((0 << SCLKPOL) | (1 << SCLKSH) | (1 << TYPE), spi.mem + SSPSPICONT1);
    /*
     * clear status flags 
     */
    iowrite16(0x0000, spi.mem + SSPSPISTATE);

    /*
     * configure SPI IO pins 
     *
     * SSFRM  --> gpio out
     * others --> ALT1
     */
	pollux_set_gpio_func( GPIO_SSPFRM, POLLUX_GPIO_MODE_GPIO);
	pollux_gpio_set_inout(GPIO_SSPFRM, POLLUX_GPIO_OUTPUT_MODE); 
	pollux_gpio_setpin(   GPIO_SSPFRM, POLLUX_GPIO_HIGH_LEVEL); 
	pollux_set_gpio_func( GPIO_SSPCLK, SPI_GPIO_FUNC);
	pollux_set_gpio_func( GPIO_SSPRXD, SPI_GPIO_FUNC);
	pollux_set_gpio_func( GPIO_SSPTXD, SPI_GPIO_FUNC);
	
#ifdef CONFIG_SPI_POLLUX_DEBUG	
	gprintk("GPIO_SSPFRM = %d, function = %d\n", GPIO_SSPFRM, (int)pollux_get_gpio_func(GPIO_SSPFRM) );
	gprintk("GPIO_SSPCLK = %d, function = %d\n", GPIO_SSPCLK, (int)pollux_get_gpio_func(GPIO_SSPCLK) );
	gprintk("GPIO_SSPRXD = %d, function = %d\n", GPIO_SSPRXD, (int)pollux_get_gpio_func(GPIO_SSPRXD) );
	gprintk("GPIO_SSPTXD = %d, function = %d\n", GPIO_SSPTXD, (int)pollux_get_gpio_func(GPIO_SSPTXD) );
#endif	
	
	

    init_waitqueue_head(&spi.wqueue);
    init_waitqueue_head(&spi.intqueue);
    spi.isBusy = 0;
    spi.spi_spinlock = SPIN_LOCK_UNLOCKED;

    spi.irq = platform_get_irq(pdev, SPI_CHANNEL);
    if (spi.irq < 0)
    {
        printk(KERN_ERR "spi: failed to get an IRQ\n");
        ret = spi.irq;
        goto fail;
    }
    ret = request_irq(spi.irq, spi_irq, SA_INTERRUPT | SA_SAMPLE_RANDOM, "spi", NULL);
    if (ret)
    {
        printk(KERN_INFO "spi: requesting IRQ failed\n");
        goto fail;
    }

#ifdef CONFIG_SPI_POLLUX_DEBUG
    sysfs_create_group(&pdev->dev.kobj, &spi_attr_group);
#endif

    return 0;

  fail:
  	ret = -EFAULT;
  	
    return ret;
}

static int
pollux_spi_remove(struct platform_device *pdev)
{
    struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

#ifdef CONFIG_SPI_POLLUX_DEBUG
    sysfs_remove_group(&pdev->dev.kobj, &spi_attr_group);
#endif
    if (spi.irq != -1)
        free_irq(spi.irq, NULL);

    return 0;
}

static struct platform_driver pollux_spi_driver = {
    .probe = pollux_spi_probe,
    .remove = pollux_spi_remove,
    .driver = {
               .name = "pollux-spi",
               .owner = THIS_MODULE,
               },
};


static int __init
spi_init(void)
{
    return platform_driver_register(&pollux_spi_driver);
}


static void __exit
spi_cleanup(void)
{
    platform_driver_unregister(&pollux_spi_driver);
}


module_init(spi_init);
module_exit(spi_cleanup);

MODULE_AUTHOR("Scott Esters");
MODULE_VERSION("1:1.0");
MODULE_LICENSE("GPL");

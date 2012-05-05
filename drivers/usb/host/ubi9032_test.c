/*********************************************************************************************************
* file name	: ubi9032_test.c
*
* UBISYS TECHNOLOGY CO., LTD
*
* author	: Ryojin KIM
*
* desc		: This test code checks 2 part as below.
*				- UBI9032 register access
*				- UBI9032 interrupt
*
* history	:
*		OCT 31, 2008		first version								by Ryojin KIM
*		SEP 4, 2009			register read/write test 추가				by Ryojin KIM
*							interrupt level 선택 추가
*		SEP 11, 2009		linux version별 관리 code 추가				by Ryojin KIM
*		JAN 8, 2010			PM mode switching 기능 추가					by Ryojin KIM, KM Lee
**********************************************************************************************************/
#if 0	// choose header files depend on Linux kernel version
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <asm/io.h>
#else
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h> 
#include <linux/platform_device.h>
#include <asm/io.h>
#endif

#include <asm/arch/regs-gpio.h> 

#include "u9032reg.h"

/* GamePark holdings pin define */ 
#define UBI9032_DECT_IRQ		IRQ_GPIO_A(14)
#define UBI9032_DECT_DECT	    POLLUX_GPA14
#define UBI9032_RESET			POLLUX_GPA13
#define HOST_POWER_ENABLE		POLLUX_GPC16
#define HIGH_CUREENT_CONTROL	POLLUX_GPB7


//#define UBI_BIG_ENDIAN			(1)				// little-endian : not define / big-endian : define
//#define UBI_BASE_START_ADDRESS		(0x10000000)	// UBI903x base address
#define UBI_BASE_START_ADDRESS		POLLUX_PA_UBI9302	// UBI903x base address
#define UBI_CLK_SRC_CRYSTAL			(1)				// oscillator : not define / crystal : define
//#define UBI_CLK_SELECT_24			(1)				// 12 [MHz] : not define / 24[Mhz] : define
//#define UBI_IRQ_NUMBER				(1)				// irq number
#define UBI_IRQ_NUMBER				UBI9032_DECT_IRQ	// irq number
#define UBI_IRQ_ACTIVE_LOW			(1)					// active high : not define / active low : define


#define DRIVER_VERSION		"JAN-8-2010"
#define DRIVER_AUTHOR		"Ryojin KIM"
#define DRIVER_DESC			"UBI9032 test module for register access and interrupt occurrence"

static const char g_pszHCDName[] = "UBI9032 Test HCD";

static struct timer_list	g_stUbiTimer;

static void UBITimer(unsigned long ulUBIHost);
#if 1	// choose function format depend on Linux kernel version
static irqreturn_t UBI9032ISR(int irq, void *ptr, struct pt_regs *regs);
#else
static irqreturn_t UBI9032ISR(int irq, void *ptr);
#endif
static int UBI9032Init(void);
static void UBI9032SetGPIO(void);
static void UBI9032Reset(void);


static void UBITimer(unsigned long ulUBIHost)
{
#ifdef UBISYS_BIG_ENDIAN
	if((rcPM_Control_0 & 0x0300) == 0x0300)
	{
		rcPM_Control_0	= 0x8000;
		rcMainIntEnb	= 0x0100;
	}
	else
	{
		rcPM_Control_0	= 0x4000;
		rcMainIntEnb	= 0x0100;
	}
#else
	if((rcPM_Control_0 & 0x0003) == 0x0003)
	{
		rcPM_Control_0	= 0x0080;
		rcMainIntEnb	= 0x0001;
	}
	else
	{
		rcPM_Control_0	= 0x0040;
		rcMainIntEnb	= 0x0001;
	}
#endif

	mod_timer(&g_stUbiTimer, jiffies + msecs_to_jiffies(1000));
}


#if 1	// choose function format depend on Linux kernel version
static irqreturn_t UBI9032ISR(int irq, void *ptr, struct pt_regs *regs)
#else
static irqreturn_t UBI9032ISR(int irq, void *ptr)
#endif
{
	unsigned short	usMainIntStatus;
	irqreturn_t	ret	= IRQ_HANDLED;
	
	usMainIntStatus	= rcMainIntStat;
	rcMainIntStat	= 0xFFFF;

#ifdef UBISYS_BIG_ENDIAN
	if((rcPM_Control_0 & 0x0300) == 0x0300)
	{
		printk("[UBI9032_ISR] UBI9032 is in active state.\n");
	}
	else
	{
		printk("[UBI9032_ISR] UBI9032 is in sleep state.\n");
	}
#else
	if((rcPM_Control_0 & 0x0003) == 0x0003)
	{
		printk("[UBI9032_ISR] UBI9032 is in active state.\n");
	}
	else
	{
		printk("[UBI9032_ISR] UBI9032 is in sleep state.\n");
	}
#endif

	return(ret);
}


static int UBI9032Init(void)
{
	unsigned short	usTempVal;
	unsigned int	i;

	
	rcChipReset		= 0x0001;
	udelay(20);
	rcChipReset		= 0x0000;
	udelay(200);
	
	for(i = 0; i < 0x10000; i++)
	{
		rcWakeupTim_H	= i;
		if(rcWakeupTim_H != i)
		{
			printk("i:(0x%04x) , [UBI9032Init] Register access test error.(0x%04x)\n", i, rcWakeupTim_H);
			return(-1);
		}
	}

	rcWakeupTim_H	= 0xCCBB;
	usTempVal		= rcWakeupTim_H;
	printk("[UBI9032Init] Wakeup Time : 0x%04x\n", usTempVal);

#ifdef UBI_CLK_SRC_CRYSTAL
#ifdef UBI_CLK_SELECT_24
	rcClkSelect		= 0x0001;	// clock source : crystal, frequency : 24[MHz]
#else

	rcClkSelect		= 0x0000;	// clock source : crystal, frequency : 12[MHz]
#endif
#else
#ifdef UBI_CLK_SELECT_24
	rcClkSelect		= 0x0081;	// clock source : oscillator, frequency : 24[MHz]
#else
	rcClkSelect		= 0x0080;	// clock source : oscillator, frequency : 12[MHz]
#endif
#endif
	mdelay(1);

#ifdef 	UBISYS_BIG_ENDIAN
	for(i = 0; i < 3; i++)
	{
#ifdef UBI_IRQ_ACTIVE_LOW
		rcChipConfig	= 0x0000;
#else
		rcChipConfig	= 0x0080;
#endif

		usTempVal		= rcDummyRead;
		usTempVal		= rcRevisionNum;
		if(usTempVal == 0x1000)
			break;

		mdelay(1);
	}

	rcModeProtect	= 0x0000;

	rcHostDeviceSel	= 0x0001;

	printk("[UBI9032Init] Revision Number(Big Endian) = 0x%04x\n", usTempVal);
	if(usTempVal == 0x1000)
	{
		printk("[UBI9032Init] UBI9032 Init is OK.\n");
	}
	else
	{
		printk("[UBI9032Init] UBI9032 Init is failed.\n");

		return(-1);
	}
#else	// UBISYS_LITTLE_ENDIAN
	for(i = 0; i < 3; i++)
	{
#ifdef UBI_IRQ_ACTIVE_LOW
		rcChipConfig	= 0x0004;
#else
		rcChipConfig	= 0x0084;
#endif

		usTempVal		= rcDummyRead;
		usTempVal		= rcRevisionNum;
		if(usTempVal == 0x0010)
			break;
		mdelay(1);
	}

	rcModeProtect	= 0x0000;
	rcHostDeviceSel	= 0x0100;

	printk("[UBI9032Init] Revision Number(Little Endian) = 0x%04x\n", usTempVal);
	
	if(usTempVal == 0x0010)
	{
		printk("[UBI9032Init] UBI9032 Init is OK.\n");
	}
	else
	{
		printk("[UBI9032Init] UBI9032 Init is failed.\n");

		return(-1);
	}
#endif

	return(0);
}

static void HostPowerEnable(void)
{
	pollux_gpio_setpin(HOST_POWER_ENABLE, 1);
	pollux_gpio_setpin(HIGH_CUREENT_CONTROL, 1);	
	mdelay(1);
}

static void HostPowerDisable(void)
{
	pollux_gpio_setpin(HIGH_CUREENT_CONTROL, 0);
	pollux_gpio_setpin(HOST_POWER_ENABLE, 0);		
}

static void UBI9032SetGPIO(void)
{
	
	// set interrupt pin connected to UBI9032		
	// set reset pin connected to UBI9032
	// set the memory controller connected to UBI9032	
}

static void UBI9032Reset(void)
{
	// reset UBI9032 using GPIO pin
	pollux_gpio_setpin(UBI9032_RESET, 1);
	udelay(100);
	pollux_gpio_setpin(UBI9032_RESET, 0);
	udelay(100);
	pollux_gpio_setpin(UBI9032_RESET, 1);
	udelay(100);			
}

static int __init ubisysh_probe(struct device *dev)
{
	struct platform_device	*pdev;
	struct resource			*addr;
	int		irq;
	int		status;
	int		ret;
	
	printk("[ubisysh probe] %s, %s\n", g_pszHCDName, DRIVER_VERSION);
	
	pdev	= container_of(dev, struct platform_device, dev);
	addr	= platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq		= platform_get_irq(pdev, 0);
	printk("[ubisysh probe] addr->start : 0x%x\n", (unsigned int)(addr->start));
	printk("[ubisysh probe] irq : %d\n", irq);
	if((!addr) || (irq < 0))
	{
		printk("[ubisysh probe] error > platform_get_resource/platform_get_irq\n");
		return(-ENODEV);
	}

	if(!request_mem_region(addr->start, 1, g_pszHCDName))
	{
		printk("[ubisysh probe] error > request_mem_region\n");
		return(-EBUSY);
	}
	
	g_uiUBI9032BaseAddress	= (unsigned int)ioremap(addr->start, 0x200);
	printk("[ubisysh probe] g_uiUBI9032BaseAddress : 0x%x\n", g_uiUBI9032BaseAddress);
		

#ifdef UBI_IRQ_ACTIVE_LOW	
	set_irq_type(irq, IRQT_FALLING);
#else
	set_irq_type(irq, IRQT_FALLING);
#endif	
	status	= request_irq(irq, UBI9032ISR, IRQF_DISABLED, g_pszHCDName, NULL);
	if(status < 0){
		printk("[ubisysh_probe] request_irq() is failed.\n");		
		goto fail_ubi;
	}
		
	HostPowerEnable();
	UBI9032SetGPIO();
	UBI9032Reset();
	
	ret	= UBI9032Init();
	if(ret < 0)
	{
		printk("[ubisysh_probe] UBI9032Init() is failed.\n");
		status = ret;
		goto fail;
	}
	
	
	init_timer(&g_stUbiTimer);
	g_stUbiTimer.function	= UBITimer;
	g_stUbiTimer.data		= (unsigned long)0;
	
	mod_timer(&g_stUbiTimer, jiffies + msecs_to_jiffies(1000));
	return(0);

fail_ubi:
	free_irq(irq, NULL);
fail:
	
	iounmap((void *)g_uiUBI9032BaseAddress);
	
	//HostPowerDisable();
	return status;
}

struct device_driver ubisysh_driver =
{
	.name		= (char *)g_pszHCDName,
	.bus		= &platform_bus_type,
	.probe		= ubisysh_probe,
};


#define DRIVER_INFO DRIVER_VERSION " " DRIVER_DESC

MODULE_DESCRIPTION (DRIVER_INFO);
MODULE_AUTHOR (DRIVER_AUTHOR);
MODULE_LICENSE ("GPL");


struct resource ubisys_hcd_resources[] =
{
	[0] 	=
	{
		.start	= UBI_BASE_START_ADDRESS,
		.end	= (UBI_BASE_START_ADDRESS + 0x1FF),
		.flags	= IORESOURCE_MEM,
	},
	
	[1]	=
	{
		.start 	= UBI_IRQ_NUMBER,
		.end	= UBI_IRQ_NUMBER,
		.flags	= IORESOURCE_IRQ,
	},
};


struct platform_device ubisys_platform_device =
{
	.name			= (char *)g_pszHCDName,
	.id				= -1,
	.num_resources	= ARRAY_SIZE(ubisys_hcd_resources),
	.resource		= ubisys_hcd_resources,
};


void ubisys_pdev_release(struct device *dev)
{
	printk("[ubisys_pdev_release]\n");
}

static int __init ubisysh_init(void) 
{
	int ret;
	
	printk("==================================================================\n");
	printk(" UBI9032 test module for register access and interrupt occurrence\n");
	printk("==================================================================\n");
	
	printk("[ubisysh_init] %s, %s\n", g_pszHCDName, DRIVER_VERSION);
	
	ret	= driver_register(&ubisysh_driver);
	if(ret < 0)
	{
		printk("[ubisysh_init] driver registeration error! ret : %d\n", ret);
		return(ret);
	}

	ubisys_platform_device.dev.release = ubisys_pdev_release;
	ret	= platform_device_register(&ubisys_platform_device);
	if (ret < 0)
	{
		printk("[ubisysh_init] device registeration error! ret : %d\n", ret);
		driver_unregister (&ubisysh_driver);
	}
	
	return(ret);
}
module_init(ubisysh_init);


static void __exit ubisysh_cleanup(void) 
{
	iounmap((void *)g_uiUBI9032BaseAddress);
	release_mem_region(g_uiUBI9032BaseAddress, 1);
	
	free_irq(UBI_IRQ_NUMBER, NULL);

	platform_device_unregister(&ubisys_platform_device);

	driver_unregister(&ubisysh_driver);
	
	HostPowerDisable();
	printk("[ubisysh_cleanup] UBI9032 test module is removed.\n");
}
module_exit(ubisysh_cleanup);


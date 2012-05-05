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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/completion.h>
#include <linux/delay.h>

#include <asm/uaccess.h>
#include <asm/rtc.h>
#include <asm/io.h>

#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/mach/time.h>

#include <asm/arch/regs-rtc.h>

#if 0
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif


int pollux_rtc_checkbusy(void)
{
	int busy;
	
	busy = (pollux_rtc_check_busy_alarm() || pollux_rtc_check_busy_rtc());
	return  busy;
}

int pollux_rtc_set_alarm_counter( u32 alarm ) 
{
	if( pollux_rtc_check_busy_alarm() )
		return -1;
	
    RTCALARM = alarm;
    
    return 0;
}


u32 pollux_rtc_get_alarm_counter(void)
{
	int val;
	
	val = RTCALARM;
	return val;
}

	
int pollux_rtc_check_busy_alarm ( void )
{
	u32 ALARMCNTWAIT	= (1UL<<3);
	return (RTCCONTROL & ALARMCNTWAIT) ? 1 : 0;
}


int pollux_rtc_set_counter(u32 val)
{
	if( pollux_rtc_check_busy_rtc() )
		return -1;
	
    RTCCNTWRITE = val;	
    
    return 0;
}


u32 pollux_rtc_get_counter(void)
{
    return RTCCNTREAD;
}
	
	
int pollux_rtc_check_busy_rtc ( void )
{
	u32 RTCCNTWAIT	= (1UL<<4);
	return (RTCCONTROL & RTCCNTWAIT) ? 1 : 0;
}


void pollux_rtc_set_write_enable(int enable)
{
	u32 WRITEENB = (1UL<<0);
	
	if( enable ) 
		RTCCONTROL |=  WRITEENB;
	else
		RTCCONTROL &= ~WRITEENB;
}	



static int
pollux_rtc_readtime(struct device *dev, struct rtc_time *tm)
{
    u32             time;
    int             tout = 0x10000;
    
    pollux_rtc_set_write_enable(1);
	
	while( pollux_rtc_check_busy_rtc() && tout--);
	if( tout == 0 )
	{
		gprintk("busy timeout\n");
		return -EDEADLOCK;
	}
	
	time = pollux_rtc_get_counter();
	pollux_rtc_set_write_enable(0);

    rtc_time_to_tm(time, tm);

    gprintk("%4d-%02d-%02d %02d:%02d:%02d\n", 1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    if( (1900 + tm->tm_year) < 2000)
    {
        tm->tm_year = 110;			
		tm->tm_mon = 0;
		tm->tm_mday =1;
		tm->tm_wday =1;        
    }
    
    return 0;
}

static int
pollux_rtc_settime(struct device *dev, struct rtc_time *tm)
{
    unsigned long   time;
    int             ret = 0;
    int             tout = 0x10000;	// TODO:  must redefine for oem over 2 clock of 32768Hz rtc clock

    gprintk("%4d-%02d-%02d %02d:%02d:%02d\n", 1900 + tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    
    rtc_tm_to_time(tm, &time);

    if( time > 0xFFFFFFFF )
        printk(KERN_ALERT "time value %ld is out of range\n", time);

    time = time & 0xFFFFFFFF;   

	pollux_rtc_set_write_enable(1);
	pollux_rtc_set_counter( (u32)time );
	while( pollux_rtc_check_busy_rtc() && tout--);
	if( tout == 0)
	{
		gprintk("RTC write timeout\n");
		return -EDEADLOCK;
	}
	pollux_rtc_set_write_enable(0);

    return ret;
}

static int pollux_rtc_set_mmss(struct device *dev, unsigned long secs)
{
    int             tout = 0x10000;	// TODO:  must redefine for oem over 2 clock of 32768Hz rtc clock
	
	pollux_rtc_set_write_enable(1);
	pollux_rtc_set_counter( secs );
	while( pollux_rtc_check_busy_rtc() && tout--);
	if( tout == 0)
	{
		gprintk("RTC write timeout\n");
		return -EIO;
	}
	pollux_rtc_set_write_enable(0);
	
	return 0;
}

static const struct rtc_class_ops pollux_rtc_ops = {
    .read_time = pollux_rtc_readtime,
    .set_time  = pollux_rtc_settime,
	.set_mmss  = pollux_rtc_set_mmss,    
};

static int
pollux_rtc_probe(struct platform_device *pdev)
{
    struct rtc_device *rtc;
    int             ret;

    pollux_rtc_set_write_enable(0); // disable access
	RTCALARM  = 0;    
	RTCINTENB = 0;
	RTCINTPEND = 0;
	

    rtc = rtc_device_register(pdev->name, &pdev->dev, &pollux_rtc_ops, THIS_MODULE);
    if (IS_ERR(rtc))
    {
        ret = PTR_ERR(rtc);
        goto fail_end;
    }
    platform_set_drvdata(pdev, rtc);

    printk(KERN_INFO "POLLUX Real Time Clock driver initialized.\n");
    return 0;

  fail_end:
  	ret =  -ENXIO;

    return ret;
}

static int
pollux_rtc_remove(struct platform_device *pdev)
{
    struct rtc_device *rtc = platform_get_drvdata(pdev);

    rtc_device_unregister(rtc);
    platform_set_drvdata(pdev, NULL);

    return 0;
}

#ifdef CONFIG_PM
static int
pollux_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}

static int
pollux_rtc_resume(struct platform_device *pdev)
{
    return 0;
}
#else
#    define pollux_rtc_suspend NULL
#    define pollux_rtc_resume  NULL
#endif

static struct platform_driver pollux_rtc_driver = {
    .probe = pollux_rtc_probe,
    .remove = pollux_rtc_remove,
    .suspend = pollux_rtc_suspend,
    .resume = pollux_rtc_resume,
    .driver = {
               .name = "pollux-rtc",
               .owner = THIS_MODULE,
               },
};

static int __init
pollux_rtc_init(void)
{
    return platform_driver_register(&pollux_rtc_driver);
}

static void __exit
pollux_rtc_exit(void)
{
    platform_driver_unregister(&pollux_rtc_driver);
}

module_init(pollux_rtc_init);
module_exit(pollux_rtc_exit);

MODULE_AUTHOR("godori, www.aesop-embedded.org");
MODULE_DESCRIPTION("RTC driver for POLLUX");
MODULE_LICENSE("GPL");

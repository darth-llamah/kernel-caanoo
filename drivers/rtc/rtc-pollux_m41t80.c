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
#include <linux/bcd.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/m41t00.h>
#include <linux/irq.h>
#include <linux/sched.h>

#include <asm/uaccess.h>
#include <asm/rtc.h>
#include <asm/io.h>

#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/mach/time.h>
#include <asm/arch/regs-gpio.h>

//#include <asm/arch/regs-rtc.h>


#if 0
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif


#define M41T80_ID         0xD0 

struct m41t00_chip_info {
	u8	type;
	char	*name;
	u8	read_limit;
	u8	sec;		/* Offsets for chip regs */
	u8	min;
	u8	hour;
	u8	wday;
	u8	day;
	u8	mon;
	u8	year;
	u8	alarm_mon;
	u8  alarm_day;
	u8	alarm_hour;
	u8  alarm_min;
	u8  alarm_sec;
	u8	sqw;
	u8	sqw_freq;
};

static struct m41t00_chip_info m41t00_chip_info_tbl = {
	.type		= M41T00_TYPE_M41T81,
	.name		= "m41t80",
	.read_limit	= 1,
	.sec		= 1,
	.min		= 2,
	.hour		= 3,
	.wday       = 4,
	.day		= 5,
	.mon		= 6,
	.year		= 7,
	.alarm_mon	= 0xa,
	.alarm_day	= 0xb,
	.alarm_hour	= 0xc,
	.alarm_min	= 0xd,
	.alarm_sec	= 0xe,
	.sqw		= 0x13,
};

static struct m41t00_chip_info *m41t00_chip = &m41t00_chip_info_tbl;
struct work_struct	alarm_work;

#define IRQ_GPIO_ALARM                	IRQ_GPIO_B(30)

#define GPIO_SCL    POLLUX_GPA28
#define GPIO_SDA    POLLUX_GPA29

#define SCCB_SCL_INPUT	    pollux_gpio_set_inout(GPIO_SCL, POLLUX_GPIO_INPUT_MODE)
#define SCCB_SCL_OUTPUT	    pollux_gpio_set_inout(GPIO_SCL, POLLUX_GPIO_OUTPUT_MODE)
#define SCCB_SCL_LOW	    pollux_gpio_setpin(GPIO_SCL, 0)
#define SCCB_SCL_HIGH	    pollux_gpio_setpin(GPIO_SCL, 1)
#define SCCB_SCL_DATA	    pollux_gpio_getpin(GPIO_SCL)
                                
#define SCCB_SDA_INPUT	    pollux_gpio_set_inout(GPIO_SDA, POLLUX_GPIO_INPUT_MODE)
#define SCCB_SDA_OUTPUT	    pollux_gpio_set_inout(GPIO_SDA, POLLUX_GPIO_OUTPUT_MODE)
#define SCCB_SDA_LOW	    pollux_gpio_setpin(GPIO_SDA, 0)
#define SCCB_SDA_HIGH	    pollux_gpio_setpin(GPIO_SDA, 1)
#define SCCB_SDA_DATA	    pollux_gpio_getpin(GPIO_SDA)



int I2C_failed(void)
{
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	SCCB_SDA_OUTPUT;
	mdelay(5);
	SCCB_SCL_HIGH;
	mdelay(5);
	SCCB_SDA_HIGH;
	
	SCCB_SCL_INPUT;
	SCCB_SDA_INPUT;		
	
	return 0;
}

int	I2C_wbyte( unsigned char id, unsigned char addr, unsigned char data ) 
{
	int i;

	SCCB_SDA_HIGH;
	SCCB_SCL_HIGH;
	SCCB_SCL_OUTPUT;
	SCCB_SDA_OUTPUT;
	mdelay(1);

	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;

	mdelay(1);
	
    // START
	SCCB_SDA_LOW;
	mdelay(1);	
	SCCB_SCL_LOW;
    
	// Write Slave ID
	for( i=7 ; i>=0 ; i-- )
	{
		if( id & (1<<i) )			SCCB_SDA_HIGH;
		else						SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	
	if( SCCB_SDA_DATA )		
	{
		printk("I2C_wbyte : No Ack at id(0x%02X) transfer\r\n", id );
		I2C_failed();
	}
	mdelay(1);	
	
	// Write Slave Address
	SCCB_SCL_LOW;
	SCCB_SDA_OUTPUT;
	for( i=7 ; i>=0 ; i-- )
	{
		if( addr & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_wbyte : No Ack at addr(0x%02X) transfer\r\n", addr );
		I2C_failed();
	}
	mdelay(1);	

	// Write Slave data
	SCCB_SCL_LOW;
	SCCB_SDA_OUTPUT;
	for( i=7 ; i>=0 ; i-- )
	{
		if( data & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_wbyte : No Ack at data(0x%02X) transfer\r\n", data );
		I2C_failed();
	}
	mdelay(1);	
	
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;
	
	SCCB_SCL_INPUT;
	SCCB_SDA_INPUT;
	
	return 1;
}


//------------------------------------------------------------------------------
int I2C_rbyte( unsigned char id, unsigned char addr)
{
	unsigned char temp;
	int i;
	
	SCCB_SDA_HIGH;
	SCCB_SCL_HIGH;
	SCCB_SCL_OUTPUT;
	SCCB_SDA_OUTPUT;
	mdelay(1);

	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;

	mdelay(1);
	
	// START
	SCCB_SDA_LOW;
	mdelay(1);	
	SCCB_SCL_LOW;
	
	// Write Slave ID
	for( i=7 ; i>=0 ; i-- )
	{
		if( id & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )		
	{
		printk("I2C_rbyte : No Ack at id(0x%02X) transfer\r\n", id );
		I2C_failed();
	}
	mdelay(1);	

	// Write Slave Address
	SCCB_SCL_LOW;
	SCCB_SDA_OUTPUT;
	for( i=7 ; i>=0 ; i-- )
	{
		if( addr & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_rbyte : No Ack at addr(0x%02X) transfer\r\n", addr );
		I2C_failed();
	}
	mdelay(1);	
	
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	SCCB_SDA_OUTPUT;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;

	mdelay(1);
	
	// START
	SCCB_SDA_LOW;
	mdelay(1);	
	SCCB_SCL_LOW;

	// Write Slave ID
	id |= 1;
	for( i=7 ; i>=0 ; i-- )
	{
		if( id & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_rbyte : No Ack at Repeated Start(0x%02X) transfer\r\n", id );
		I2C_failed();
	}
	mdelay(1);	
	
	// Read Data
	SCCB_SCL_LOW;
	temp = 0;
	for( i=7 ; i>=0 ; i-- )
	{
		mdelay(2);
		SCCB_SCL_HIGH;
		if( SCCB_SDA_DATA )	temp |= (1<<i);
		else				temp |= (0<<i);
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_HIGH;		// No ACK
	SCCB_SDA_OUTPUT;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;
	
	SCCB_SCL_INPUT;
	SCCB_SDA_INPUT;
	
    return temp;
}



static irqreturn_t pollux_rtc_m41t80_interrupt(int irq, void *dev_id)
{
    printk("alarm interrupt 111111 \n");
    if (!(I2C_rbyte(M41T80_ID, m41t00_chip->sec) & 0x7f))
		return IRQ_NONE;
    
    printk("alarm interrupt GO GO GO \n");

#if 0    
    schedule_work(&alarm_work);
#endif    
    
    return IRQ_HANDLED;
}


static void alarm_work_handler(struct work_struct *work)
{
    char *argv[3], **envp, *buf, *scratch;
    int i = 0,value;
    
    if (!(envp = (char **) kmalloc(20 * sizeof(char *), GFP_KERNEL))) {
		    printk(KERN_ERR "input.c: not enough memory allocating hotplug environment\n");
		    return;
	}
    
    if (!(buf = kmalloc(1024, GFP_KERNEL))) {
		    kfree (envp);
		    printk(KERN_ERR "pwrbtn: not enough memory allocating hotplug environment\n");
		    return;
	}
    
    argv[0] = "/etc/hotplug.d/default/default.hotplug";
	argv[1] = "alarm_on";
	argv[2] = NULL;
    
    envp[i++] = "HOME=/";
	envp[i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
    
	scratch = buf;

	envp[i++] = scratch;
	scratch += sprintf(scratch, "ACTION=%s", "change") + 1;
    
    envp[i++] = NULL;
   
    value = call_usermodehelper(argv [0], argv, envp, 0);
    
    kfree(buf);
	kfree(envp);
}

static int pollux_rtc_m41t80_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    s32 sec, min, hour, day;
	u8 sec1, min1, hour1, day1;
    u8 AFE = 0;
    
    sec = BIN2BCD(alrm->time.tm_sec);
    min = BIN2BCD(alrm->time.tm_min);
    hour = BIN2BCD(alrm->time.tm_hour);
    day = BIN2BCD(alrm->time.tm_mday);
    
    sec1 = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_sec) & 0x7f;
    min1 = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_min) & 0x7f;
    hour1 = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_hour) & 0x3f;
    day1 = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_day) & 0x3f;
	
	if (alrm->enabled)  AFE = 0x80;
	
	I2C_wbyte(M41T80_ID, m41t00_chip->alarm_sec, ((sec1 & ~0x7f) | (sec & 0x7f)) );
    I2C_wbyte(M41T80_ID, m41t00_chip->alarm_min, ((min1 & ~0x7f) | (min & 0x7f)) );
    I2C_wbyte(M41T80_ID, m41t00_chip->alarm_hour, ((hour1 & ~0x3f) | (hour & 0x3f)) );
	I2C_wbyte(M41T80_ID, m41t00_chip->alarm_day, ((day1 & ~0x3f) | (day & 0x3f)) );
	I2C_wbyte(M41T80_ID, m41t00_chip->alarm_mon, AFE );
    
    return 0;
}

static int pollux_rtc_m41t80_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
    s32 sec, min, hour, day, mon, year;
    sec = min = hour = day = mon = year = 0;
    
    sec = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_sec) & 0x7f;
    min = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_min) & 0x7f;
    hour = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_hour) & 0x3f;
    day = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_day) & 0x3f;
	mon = I2C_rbyte(M41T80_ID, m41t00_chip->alarm_mon) & 0x80;
	
	alrm->enabled = (mon & 0x80) ? 1 : 0;    
    
    alrm->time.tm_sec = BCD2BIN(sec);
	alrm->time.tm_min = BCD2BIN(min);
	alrm->time.tm_hour = BCD2BIN(hour);
	alrm->time.tm_mday = BCD2BIN(day);
    alrm->time.tm_mon = BCD2BIN(mon);
    
    return 0;
}


static int
pollux_rtc_m41t80_readtime(struct device *dev, struct rtc_time *dt)
{
    s32 sec, min, hour, day, mon, year ,wday;
    sec = min = hour = day = mon = year = wday = 0;
    
    sec = I2C_rbyte(M41T80_ID, m41t00_chip->sec) & 0x7f;
    min = I2C_rbyte(M41T80_ID, m41t00_chip->min) & 0x7f;
    hour = I2C_rbyte(M41T80_ID, m41t00_chip->hour) & 0x3f;
    day = I2C_rbyte(M41T80_ID, m41t00_chip->day) & 0x3f;
	mon = I2C_rbyte(M41T80_ID, m41t00_chip->mon) & 0x1f;
	year = I2C_rbyte(M41T80_ID, m41t00_chip->year) & 0x1f;
    wday = I2C_rbyte(M41T80_ID, m41t00_chip->wday) & 0x07;
        
    dt->tm_sec = BCD2BIN(sec);
	dt->tm_min = BCD2BIN(min);
	dt->tm_hour = BCD2BIN(hour);
	dt->tm_wday = BCD2BIN(wday);
	dt->tm_mday = BCD2BIN(day);
	dt->tm_mon = BCD2BIN(mon);
	dt->tm_year = BCD2BIN(year);

    
#if 0
    dt->tm_year += 1900;
    if (dt->tm_year < 1970)
        dt->tm_year += 100;
#else
    if ((dt->tm_year + 1900) < 1970)
        dt->tm_year += 100;
#endif

#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
    printk("%4d-%02d-%02d %02d:%02d:%02d\n", 1900 + dt->tm_year, dt->tm_mon + 1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
#endif

    return 0;
}

static int
pollux_rtc_m41t80_settime(struct device *dev, struct rtc_time *dt)
{
    s32 sec, min, hour, day, mon, year ,wday;
	u8 sec1, min1, hour1, day1, mon1, year1 ,wday1;
    int ret = 0,rc, i;
    
    //printk("%4d-%02d-%02d %02d:%02d:%02d\n", 1900 + dt->tm_year, dt->tm_mon, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
    
    //dt->tm_year = (dt->tm_year - 1900) % 100;
    sec = BIN2BCD(dt->tm_sec);
    min = BIN2BCD(dt->tm_min);
    hour = BIN2BCD(dt->tm_hour);
    wday = BIN2BCD(dt->tm_wday);
    day = BIN2BCD(dt->tm_mday);
    mon = BIN2BCD(dt->tm_mon);
    year = BIN2BCD(dt->tm_year);

    
    sec1 = I2C_rbyte(M41T80_ID, m41t00_chip->sec) & 0x7f;
    min1 = I2C_rbyte(M41T80_ID, m41t00_chip->min) & 0x7f;
    hour1 = I2C_rbyte(M41T80_ID, m41t00_chip->hour) & 0x3f;
    wday1 = I2C_rbyte(M41T80_ID, m41t00_chip->wday) & 0x07;
    day1 = I2C_rbyte(M41T80_ID, m41t00_chip->day) & 0x3f;
	mon1 = I2C_rbyte(M41T80_ID, m41t00_chip->mon) & 0x1f;
	
    I2C_wbyte(M41T80_ID, m41t00_chip->sec, ((sec1 & ~0x7f) | (sec & 0x7f)) );
    I2C_wbyte(M41T80_ID, m41t00_chip->min, ((min1 & ~0x7f) | (min & 0x7f)) );
    I2C_wbyte(M41T80_ID, m41t00_chip->hour, ((hour1 & ~0x3f) | (hour & 0x3f)) );
	I2C_wbyte(M41T80_ID, m41t00_chip->day, ((day1 & ~0x3f) | (day & 0x3f)) );
	I2C_wbyte(M41T80_ID, m41t00_chip->wday, ((wday1 & ~0x07) | (wday & 0x07)) );
	I2C_wbyte(M41T80_ID, m41t00_chip->mon, ((mon1 & ~0x1f) | (mon & 0x1f)) );
	I2C_wbyte(M41T80_ID, m41t00_chip->year, year);

    
    if ((rc = I2C_rbyte(M41T80_ID, m41t00_chip->sec)) < 0)
        printk(" stop clear bit error \n");
        
     return ret;

}

static const struct rtc_class_ops pollux_rtc_m41t80_ops = {
    .read_time = pollux_rtc_m41t80_readtime,
    .set_time  = pollux_rtc_m41t80_settime,
    .read_alarm	= pollux_rtc_m41t80_read_alarm,
	.set_alarm	= pollux_rtc_m41t80_set_alarm,
};


static int
pollux_rtc_m41t80_probe(struct platform_device *pdev)
{
    struct rtc_device *rtc;
    int ret,rc;

    if (m41t00_chip->type != M41T00_TYPE_M41T00) 
    {
		/* If asked, disable SQW, set SQW frequency & re-enable */
		
		if (m41t00_chip->sqw_freq)
			if (((rc = I2C_rbyte(M41T80_ID,
					m41t00_chip->alarm_mon)) < 0)
			 || ((rc = I2C_rbyte(M41T80_ID,
					m41t00_chip->alarm_mon)) <0)
			 || ((rc = I2C_wbyte(M41T80_ID,
					m41t00_chip->sqw,
					m41t00_chip->sqw_freq)) < 0)
			 || ((rc = I2C_wbyte(M41T80_ID,
					m41t00_chip->alarm_mon, rc | 0x40)) <0))
				goto sqw_err;

		
		/* Make sure HT (Halt Update) bit is cleared */
		if ((rc = I2C_rbyte(M41T80_ID,
				m41t00_chip->alarm_hour)) < 0)
			goto ht_err;
        
        if (rc & 0x40)
			if ((rc = I2C_wbyte(M41T80_ID,
					m41t00_chip->alarm_hour, rc & ~0x40))<0)
				goto ht_err;
	}
    
    /* Make sure ST (stop) bit is cleared */
	if ((rc = I2C_rbyte(M41T80_ID, m41t00_chip->sec)) < 0)
		goto st_err;
    
    
	if (rc & 0x80)
		if ((rc = I2C_wbyte(M41T80_ID, m41t00_chip->sec,rc & ~0x80)) < 0)
			goto st_err;


    #if 0
    set_irq_type( IRQ_GPIO_ALARM, IRQT_FALLING);
	if (request_irq( IRQ_GPIO_ALARM , &pollux_rtc_m41t80_interrupt, SA_INTERRUPT, "GPIO", NULL ) != 0)
	{
        printk( "cannot get alarm interrup irq %d\n", IRQ_GPIO_ALARM);
        return -1;
    }
    
    //INIT_WORK(&alarm_work, alarm_work_handler);
    #endif 
                    
    rtc = rtc_device_register(pdev->name, &pdev->dev, &pollux_rtc_m41t80_ops, THIS_MODULE);
    if (IS_ERR(rtc))
    {
        ret = PTR_ERR(rtc);
        goto fail_end;
    }
    platform_set_drvdata(pdev, rtc);
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
    printk(KERN_INFO "Real Time m41t80 Clock driver initialized.\n");
#endif 
    return 0;


st_err:
	printk("m41t00_probe: Can't clear ST bit\n");
	goto attach_err;
ht_err:
	printk("m41t00_probe: Can't clear HT bit\n");
	goto attach_err;
sqw_err:
	printk("m41t00_probe: Can't set SQW Frequency\n");
attach_err:
fail_end:
  	ret =  -ENXIO;

    return ret;
}

static int
pollux_rtc_m41t80_remove(struct platform_device *pdev)
{
    struct rtc_device *rtc = platform_get_drvdata(pdev);
    
    flush_scheduled_work();
    rtc_device_unregister(rtc);
    platform_set_drvdata(pdev, NULL);

    return 0;
}

#ifdef CONFIG_PM
static int
pollux_rtc_m41t80_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}

static int
pollux_rtc_m41t80_resume(struct platform_device *pdev)
{
    return 0;
}
#else
#    define pollux_rtc_m41t80_suspend NULL
#    define pollux_rtc_m41t80_resume  NULL
#endif

static struct platform_driver pollux_rtc_m41t80_driver = {
    .probe = pollux_rtc_m41t80_probe,
    .remove = pollux_rtc_m41t80_remove,
    .suspend = pollux_rtc_m41t80_suspend,
    .resume = pollux_rtc_m41t80_resume,
    .driver = {
               .name = "pollux-rtc",
               .owner = THIS_MODULE,
               },
};

static int __init
pollux_rtc_m41t80_init(void)
{
    pollux_set_gpio_func(POLLUX_GPA28, POLLUX_GPIO_MODE_GPIO);
	pollux_gpio_set_inout(POLLUX_GPA28, POLLUX_GPIO_INPUT_MODE);
    pollux_set_gpio_func(POLLUX_GPA29, POLLUX_GPIO_MODE_GPIO);
	pollux_gpio_set_inout(POLLUX_GPA29, POLLUX_GPIO_INPUT_MODE);

#if 0	
	pollux_set_gpio_func(POLLUX_GPB30, POLLUX_GPIO_MODE_GPIO);
	pollux_gpio_set_inout(POLLUX_GPB30, POLLUX_GPIO_INPUT_MODE);
#endif
	
    return platform_driver_register(&pollux_rtc_m41t80_driver);
}

static void __exit
pollux_rtc_m41t80_exit(void)
{
    
    platform_driver_unregister(&pollux_rtc_m41t80_driver);
}

module_init(pollux_rtc_m41t80_init);
module_exit(pollux_rtc_m41t80_exit);

MODULE_AUTHOR("godori, www.aesop-embedded.org");
MODULE_DESCRIPTION("RTC driver for POLLUX");
MODULE_LICENSE("GPL");

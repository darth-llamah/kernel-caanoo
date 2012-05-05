/*
 * pollux_pwrbtn.c
 *
 * Copyright    (C) 2007 MagicEyes Digital Co., Ltd.
 * 
 * godori(Ko Hyun Chul), omega5                 							- project manager
 * gtgkjh(Kim Jung Han), choi5946(Choi Hyun Jin), nautilus_79(Lee Jang Ho) 	- main programmer
 * amphell(Bang Chang Hyeok)                        						- Hardware Engineer
 *
 * 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/mp2530/index.html
 *
 * This code is released under both the GPL version 2 and BSD licenses.
 * Either license may be used.  The respective licenses are found at
 * below.
 *
 * - GPL -
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
 *
 * - BSD -
 *
 *    In addition:
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 *   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *   IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#if defined(MODVERSIONS)
#include <linux/modversions.h>
#endif

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <asm/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/pollux.h>
#include "pollux_pwrbtn.h"

#define DRV_NAME "pollux_pwrbtn"

static int end_chk = 0;

extern asmlinkage long sys_umount(char* name, int flags);

//#define GDEBUG                  
#ifdef  GDEBUG
#    define gprintk(fmt, x... )  printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
#    define gprintk( x... )
#endif

#define BD_VER_LSB              POLLUX_GPC18
#define BD_VER_MSB              POLLUX_GPC17
#define PWR_TIME_CNT            25


static void pwr_timechk_timer(unsigned long data)
{
	struct power_switch *pwrsw = (struct power_switch *)data;

	pwrsw->timer_done = 1; 
	pwrsw->power_switch_off = 0;
	if( pollux_gpio_getpin(POWER_SWITCH_DECT) )
	{	
		if(!end_chk){
		    end_chk = 1;
		    schedule_work(&pwrsw->work);
	    }
	}   
}       
        

static void pwr_timechk_work_handler(struct work_struct *work)
{
	struct power_switch *pwrsw = container_of(work, struct power_switch, work);
	struct platform_device *pdev = pwrsw->pdev;
	char *argv[3], **envp, *buf, *scratch;
	int i = 0,value;
	
	
#if 0
	kobject_uevent(&pdev->dev.kobj, KOBJ_CHANGE);
    	//kill_cad_pid(SIGINT, 1);
#else
    
    Pwr_Off_Enable();

    
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
	argv[1] = "power_off";
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
	
#endif	
}


irqreturn_t pollux_pwrbtn_interrupt( int irq, void *dev_id )
{	
	struct power_switch *pwrsw = (struct power_switch *)dev_id;
	
	if( (pollux_gpio_getpin(POWER_SWITCH_DECT)) && !pwrsw->power_switch_off )
	{
		
		if(pwrsw->timer_done ){
			del_timer_sync(&pwrsw->pwr_chkTimer);
			pwrsw->timer_done = 0;
		}
		
		mod_timer(&pwrsw->pwr_chkTimer, jiffies + PWR_TIME_CNT); 
        pwrsw->power_switch_off = 1;
    }
	
	return IRQ_HANDLED;
}


static int __init pollux_pwrbtn_probe(struct platform_device *pdev)
{
	struct power_switch *pwrsw; 
	int ret;
	
	pwrsw = kzalloc(sizeof(struct power_switch), GFP_KERNEL);
	if (unlikely(!pwrsw))
		return -ENOMEM;
	
	set_irq_type(POWER_SWITCH_DECT_IRQ, IRQT_RISING);
	if (request_irq( POWER_SWITCH_DECT_IRQ, &pollux_pwrbtn_interrupt, SA_INTERRUPT, "GPIO", pwrsw ) != 0)
	{
        printk( "cannot get irq %d\n", POWER_SWITCH_DECT_IRQ );
        goto err;
    	} 
	
	INIT_WORK(&pwrsw->work, pwr_timechk_work_handler);
	init_timer(&pwrsw->pwr_chkTimer);

	pwrsw->pwr_chkTimer.function = pwr_timechk_timer;
	pwrsw->pwr_chkTimer.data = (unsigned long)pwrsw;
	pwrsw->power_switch_off = 0;
	pwrsw->timer_done = 0;
	pwrsw->pdev = pdev;
	platform_set_drvdata(pdev, pwrsw);
	
	return 0;
err:
	kfree(pwrsw);
	return ret;
}

static int pollux_pwrbtn_remove(struct platform_device *pdev)
{
	struct power_switch *pwrsw = platform_get_drvdata(pdev);
	
	platform_set_drvdata(pdev, NULL);
	flush_scheduled_work();
	
	free_irq(POWER_SWITCH_DECT_IRQ, NULL);
	kfree(pwrsw);
	
	return 0;
}


static struct platform_driver pollux_pwrbtn_driver = {
       .driver         = {
	       .name   = DRV_NAME,
	       .owner  = THIS_MODULE,
       },
       .probe          = pollux_pwrbtn_probe,
       .remove         = pollux_pwrbtn_remove,
};

static struct platform_device pollux_pwrbtn_device = {
	.name	= DRV_NAME,
	.id	= 0,
};

static const struct file_operations pwrbtn_fops = {
	.owner		= THIS_MODULE,
};

static struct miscdevice pwrbtn_misc_device = {
	PWRBTN_MINOR,
	DRV_NAME,
	&pwrbtn_fops,
};



static struct class *pwrbtn_class;

int __init pollux_pwrbtn_init(void)
{
	int res;
        
    res = platform_device_register(&pollux_pwrbtn_device);
	if(res)
	{
		printk("fail : platform device %s (%d) \n", pollux_pwrbtn_device.name, res);
		return res;
	}

	res =  platform_driver_register(&pollux_pwrbtn_driver);
	if(res)
	{
		printk("fail : platrom driver %s (%d) \n", pollux_pwrbtn_driver.driver.name, res);
		return res;
	}
	
	if (misc_register (&pwrbtn_misc_device)) {
		printk (KERN_WARNING "pwrbton: Couldn't register device 10, "
				"%d.\n", PWRBTN_MINOR);
		platform_driver_unregister(&pollux_pwrbtn_driver);
    		platform_device_unregister(&pollux_pwrbtn_device);
		return -EBUSY;
	}


#if 0
    pwrbtn_class = class_create(THIS_MODULE, DRV_NAME);
    device_create(pwrbtn_class, NULL, NULL, DRV_NAME);
#endif
    
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE	
	printk("pollux power off button check device driver install\n");
#endif
	
	return 0;	
}

void __exit
pollux_pwrbtn_exit(void)
{
	
    free_irq(POWER_SWITCH_DECT_IRQ, NULL);
    platform_driver_unregister(&pollux_pwrbtn_driver);
    platform_device_unregister(&pollux_pwrbtn_device);
	misc_deregister (&pwrbtn_misc_device);
	printk("pollux power off button check device driver uinstall\n");
    
}

module_init(pollux_pwrbtn_init);
module_exit(pollux_pwrbtn_exit);

MODULE_AUTHOR("godori working");
MODULE_DESCRIPTION("pollux touchscreen driver");
MODULE_LICENSE("GPL"); 

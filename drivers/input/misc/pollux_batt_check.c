/*
 * pollux_pollux_batt.c
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
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <linux/types.h>
#include <linux/sched.h>

#include <linux/mmc/host.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/pollux.h>
#include <asm/arch/regs-gpioalv.h>

#include "../touchscreen/pollux_ts/pollux-adc.h"

#define DRV_NAME "pollux_batt"
#define POLLUX_BATT_MINOR       159

#define LED_ON_OFF_TIME             (HZ)

#define GPIO_USB_DETECT	        POLLUX_GPC15  // USB3P3

struct batt_timer {
	struct timer_list	batt_chkTimer;
	struct timer_list	led_onTimer;
	int power_switch_off; 
	int timer_done;
	struct work_struct	work;
    struct work_struct	work_led;
    int startTimer;
    int offCnt;
    int is_off;
	int batt_led_status;		// On or blink status only
    int check_time;
    int led_flag;    
};

struct batt_timer *bTimer; 

#define LIMIT3_9V 0x390
#define LIMIT3_7V 0x36a //0x365     //diff with 3_9 = 38
#define LIMIT3_5V 0x34a //0x345		//diff with 3_7 = 32

#define BATTRY_CUT_OFF 0x335  		//diff with 3_5 = 21 

enum {
	BATT_LED_ON,
	BATT_LED_BLINK,
}BATT_LED_STATUS;

#define CHECK_BATTRY_HIGH_TIME (30 * HZ) 	//3000 //1000
#define CHECK_BATTRY_MID_TIME (20 * HZ)    //2000    
#define CHECK_BATTRY_LOW_TIME (10 * HZ)    //2000    
#define CHECK_BATTRY_EMPTY_TIME (5 * HZ)    //2000    

enum {
	BATT_LEVEL_INIT = 0,
    BATT_LEVEL_HIGH = 1,
    BATT_LEVEL_MID = 2,
    BATT_LEVEL_LOW = 3,
    BATT_LEVEL_EMPTY = 4,
}BATT_LEVEL_STATUS;    

static int check_time_table[] = {
	0,
	CHECK_BATTRY_HIGH_TIME,
	CHECK_BATTRY_MID_TIME,
	CHECK_BATTRY_LOW_TIME,
	CHECK_BATTRY_EMPTY_TIME
};

#define BATT_LEVEL_CHANGE_DIFF	15 //0x15
#define BATT_LIMIT_LED_BITNUM  5

static unsigned short cur_adc = 0;
static int current_batt_level = BATT_LEVEL_INIT;
static int batt_empty = 0;
static int open_cnt = 0;

static int usb_connection_status(void)
{
	return pollux_gpio_getpin(GPIO_USB_DETECT);
}

static int pollux_adc_value(int channel) 
{
	int ret_val;
    unsigned char Timeout = 0xff;
    
    MES_ADC_SetInputChannel(channel);
	MES_ADC_Start();
	
	while(!MES_ADC_IsBusy() && Timeout--);	// for check between start and busy time
	while(MES_ADC_IsBusy());
	ret_val = (U16)MES_ADC_GetConvertedData();
    return ret_val;
}
EXPORT_SYMBOL(pollux_adc_value);

static int get_battADC(unsigned short* adc)
{
	/*
    unsigned char Timeout = 0xff;
    
    MES_ADC_SetInputChannel(3);
	MES_ADC_Start();
	
	while(!MES_ADC_IsBusy() && Timeout--);	// for check between start and busy time
	while(MES_ADC_IsBusy());
	*adc = (U16)MES_ADC_GetConvertedData();
	*/
	*adc = pollux_adc_value(3);
    return 0;
}

#define BATTRY_READ_CNT 5
static unsigned short battary_adc_value(void)
{
	unsigned long sum = 0;
	unsigned short r[BATTRY_READ_CNT];
	unsigned short min, max;
	unsigned short read_val;
	unsigned short ret_val;
	int i;
   
	for(i = 0; i < BATTRY_READ_CNT ; i++) {
        get_battADC(&read_val);
        r[i]= read_val; 
    }
   	
	min = max = r[0];   
    
    for(i = 0 ; i < BATTRY_READ_CNT ; i++) {
		if(r[i] < min)
			min = r[i];
		if(max < r[i])
			max = r[i];
        sum += r[i];
    }
    
	sum -= (min + max);
    ret_val = (unsigned short) (sum / ( BATTRY_READ_CNT - 2 ));
	return ret_val;
}

static void set_battary_level(unsigned short input_adc_val)
{
	unsigned int batt_level;
	unsigned short adc_val;
	unsigned short diff = 0;

	adc_val = input_adc_val;

    if(adc_val > LIMIT3_9V) batt_level = BATT_LEVEL_HIGH;
	else if(adc_val > LIMIT3_7V) batt_level = BATT_LEVEL_MID;
	else if(adc_val > LIMIT3_5V) batt_level = BATT_LEVEL_LOW;
	else batt_level = BATT_LEVEL_EMPTY;	 // 0x1A4 ~ 0x19C 

	if(current_batt_level == BATT_LEVEL_INIT) {
		current_batt_level = batt_level;
		cur_adc = adc_val; 
		bTimer->check_time = check_time_table[batt_level];
	}
	
	if(cur_adc > adc_val) diff = cur_adc - adc_val;
	else if( adc_val > cur_adc) diff = adc_val - cur_adc;
	else diff = 0;
			
	if(current_batt_level != batt_level) {
		if(BATT_LEVEL_CHANGE_DIFF < diff) {
			//level changing is completed.. update current adc value and level..
			current_batt_level = batt_level;
			bTimer->check_time = check_time_table[batt_level];
			cur_adc = adc_val;
		}
	} else {
		//No level change... -> update current adc value
		cur_adc = adc_val;
	}
}

static ssize_t battary_show(struct device * dev, struct device_attribute * attr, 
		                            char * buffer)
{
    int adc, level;

	adc = battary_adc_value();
	level = current_batt_level;

	return sprintf(buffer, "%d %d 0", adc, level); 
}

DEVICE_ATTR(battary_val, S_IWUSR | S_IRUGO, battary_show, NULL);
static struct attribute * batt_attrs[] = {
	&dev_attr_battary_val.attr,
	NULL
};
static struct attribute_group battary_attr_group = {
	.attrs = batt_attrs,
};

static void excute_power_off_hotplug(void)
{
	int i = 0, value;
    char *argv[3], **envp, *buf, *scratch;

	printk("battry low level => power off\n");
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
}

#define BATT_CUTOFF_COUNT 5
static void check_battary_cut_off(unsigned short input_adc_value, struct batt_timer *batt_timer)
{
	int adc_val;
	int usb_connect;
	
	if ( !batt_empty && (current_batt_level == BATT_LEVEL_EMPTY) ) {
		batt_empty = 1;
		bTimer->offCnt = 0;
	} else if (batt_empty && (current_batt_level != BATT_LEVEL_EMPTY) ) {
		batt_empty = 0;
		return;
	}
	
	adc_val = input_adc_value;
	if(adc_val <= BATTRY_CUT_OFF) {
		if(BATT_CUTOFF_COUNT <= bTimer->offCnt) {
			usb_connect = usb_connection_status();
			if(!usb_connect) {
				bTimer->is_off = 1;
				excute_power_off_hotplug();
			}
		} else {
			bTimer->offCnt++;		
		}
	} else {
		if(bTimer->offCnt) {
			bTimer->offCnt--;
		}
	}
}

static void low_batt_led_ctrl(void)
{
	switch( bTimer->batt_led_status )
    {
    	case BATT_LED_ON : 
			if(!batt_empty) 
				return;
            bTimer->batt_led_status = BATT_LED_BLINK;
            bTimer->led_flag = 0;
            mod_timer(&bTimer->led_onTimer, jiffies + LED_ON_OFF_TIME);
            return;                       

        case BATT_LED_BLINK :
			if(batt_empty) 
				return;
            bTimer->batt_led_status = BATT_LED_ON;            
            return;
    }   /* bTimer->low_batt_led_state */ 
}

static void battary_check_core(void)
{
	struct batt_timer *btimer = bTimer; 
	unsigned short batt_adc_val;
	
	batt_adc_val = battary_adc_value();
	set_battary_level(batt_adc_val);	 
	//printk("[%ld] %s, adc_read = %d, 0x%03x, level = %d\n", jiffies / HZ  ,__func__, batt_adc_val, batt_adc_val, current_batt_level );	
	check_battary_cut_off(batt_adc_val, btimer);
	low_batt_led_ctrl();
}

static void batt_level_chk_timer(unsigned long data)
{
	struct batt_timer *btimer = (struct batt_timer *)data;
	
    schedule_work(&btimer->work);
}

static void batt_chk_time_work_handler(struct work_struct *work)
{
	battary_check_core();
	if(!bTimer->is_off)
	    mod_timer(&bTimer->batt_chkTimer, jiffies + bTimer->check_time);
}

static void led_onoff_timer(unsigned long data)
{
    struct batt_timer *btimer = (struct batt_timer *)data;   
    schedule_work(&btimer->work_led);
}


static void batt_led_control(int on_off_command)
{
	int command = on_off_command;
	if(command) 		//for prevent wrong value
		command = 1;
	else
		command = 0;
	
	pollux_gpioalv_set_write_enable(1);
	pollux_gpioalv_set_pin(BATT_LIMIT_LED_BITNUM, command);
	pollux_gpioalv_set_write_enable(0);
}

static void led_handler(struct work_struct *work)
{	
    if( bTimer->batt_led_status == BATT_LED_ON )
    {
		batt_led_control(1);
        return;
    }
    
    if(bTimer->led_flag){
		batt_led_control(1);
	    bTimer->led_flag = 0;
	}
	else {
		batt_led_control(0);
	    bTimer->led_flag = 1;
	}    
    mod_timer(&bTimer->led_onTimer, jiffies + LED_ON_OFF_TIME);
}

static int pollux_batt_open(struct inode *inode, struct file *filp)
{
	unsigned short read_adc_value;
	
	if(bTimer->startTimer == 0) {
		//below codes are excuted when irqbattary is excuted....
		read_adc_value =  battary_adc_value();
		set_battary_level(read_adc_value);
	    mod_timer(&bTimer->batt_chkTimer, jiffies + bTimer->check_time);
		//FIXME-lars : why does below code exist??
		pollux_sdi_probe1();	 
        bTimer->startTimer = 1;
        bTimer->batt_led_status = BATT_LED_ON;
    } 
    
    batt_empty = 0;
    open_cnt++;
    return 0;
}

static int pollux_batt_release(struct inode *inode, struct file *filp)
{
	open_cnt--;
	return 0;
}

static int pollux_batt_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos)
{
	unsigned short ret_val;

	ret_val = current_batt_level;
	//printk("[%d] %s, batt level = %d\n", jiffies / HZ  ,__func__, ret_val);	
	if( copy_to_user (buffer, &ret_val, sizeof(unsigned short)) )
        return -EFAULT;

    return sizeof(unsigned short);
}

static struct file_operations pollux_batt_fops = {
	.owner		= THIS_MODULE,
    .open       = pollux_batt_open,
    .read		= pollux_batt_read,
    .release    = pollux_batt_release,
};

static struct miscdevice pollux_batt_misc_device = {
	POLLUX_BATT_MINOR,
	DRV_NAME,
	&pollux_batt_fops,
};

int __init pollux_batt_init(void)
{   
	int err;
    bTimer = kzalloc(sizeof(struct batt_timer), GFP_KERNEL);
	if (unlikely(!bTimer))
		return -ENOMEM;
		
	INIT_WORK(&bTimer->work, batt_chk_time_work_handler);
	INIT_WORK(&bTimer->work_led, led_handler);
	init_timer(&bTimer->batt_chkTimer);
	init_timer(&bTimer->led_onTimer);
	
	bTimer->led_onTimer.function = led_onoff_timer;
	bTimer->led_onTimer.data = (unsigned long)bTimer;
	
	bTimer->batt_chkTimer.function = batt_level_chk_timer;
	bTimer->batt_chkTimer.data = (unsigned long)bTimer;
	bTimer->startTimer = 0;
	bTimer->offCnt = 0;
	bTimer->is_off = 0;
	bTimer->check_time = check_time_table[0];
	//mod_timer(&bTimer->batt_chkTimer, jiffies + CHECK_BATTRY_TIME);
	
	if (misc_register (&pollux_batt_misc_device)) {
		printk (KERN_WARNING "pwrbton: Couldn't register device 10, "
				"%d.\n", POLLUX_BATT_MINOR);
		kfree(bTimer);
		return -EBUSY;
	}

	err = sysfs_create_group(&pollux_batt_misc_device.this_device->kobj, &battary_attr_group);
	if(err) {
		printk("%s - create sysfs_create_group() error\n", __func__);
	}

#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE    
    printk("pollux_batt control device driver install\n");
#endif    
    return 0;	

}

void __exit pollux_batt_exit(void)
{
	flush_scheduled_work();
	kfree(bTimer);
	sysfs_remove_group(&pollux_batt_misc_device.this_device->kobj, &battary_attr_group);
	misc_deregister (&pollux_batt_misc_device);
	printk("pollux power off button check device driver uninstall\n");
    
}

module_init(pollux_batt_init);
module_exit(pollux_batt_exit);

MODULE_AUTHOR("godori working");
MODULE_DESCRIPTION("pollux battry driver");
MODULE_LICENSE("GPL"); 

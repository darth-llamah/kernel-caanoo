/*
 * pollux_ts.c
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
#include <linux/sound.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/input.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <asm/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-gpioalv.h>


#include "pollux-adc.h"
#include "pollux_ts.h"

int pendown;
unsigned int old_x,old_y;
unsigned long delta_t;

static struct task_struct *kidle_task;

struct input_dev *pollux_input;
static char *pollux_ts_name = "pollux ts";

wait_queue_head_t idle_wait;


//#define GDEBUG                  ^
#ifdef  GDEBUG
#    define gprintk(fmt, x... )  printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
#    define gprintk( x... )
#endif


////////////////////////////////////////////////////////////////////////////////////////
/*
 * POLLUX I2S REGISTER SETING & READ OPERATION
*/  	

static          DEFINE_MUTEX(alock);    

bool pollux_adc_init(U8 PrescalerValue)
{
	mutex_init(&alock);
	MES_ADC_Initialize();
	MES_ADC_SetBaseAddress(POLLUX_VA_ADC);
	MES_ADC_SetClockPClkMode(MES_PCLKMODE_ALWAYS);
	MES_ADC_SetPrescalerValue((U32)PrescalerValue );
	MES_ADC_SetPrescalerEnable(CTRUE);
	
	return true;
}

void pollux_adc_uninit(void)
{
    MES_ADC_SetPrescalerEnable(CFALSE);
	MES_ADC_SetClockPClkMode(MES_PCLKMODE_DYNAMIC);
}

bool adc_getvalue(U16 *ADCValue, enum ADCPort PortNumber, U32 Timeout)
{
	MES_ADC_SetInputChannel((U32)PortNumber);
	MES_ADC_Start();
	
	while(!MES_ADC_IsBusy() && Timeout--);	// for check between start and busy time
	while(MES_ADC_IsBusy());
	*ADCValue = (U16)MES_ADC_GetConvertedData();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
 * 
*/  	

inline void TouchMX(CBOOL onoff)
{
	pollux_gpio_setpin(TSMX_EN, (onoff ? TSMX_EN_AL : !TSMX_EN_AL));
}

inline void TouchPX(CBOOL onoff)
{
	pollux_gpio_setpin(TSPX_EN, (onoff ? TSPX_EN_AL : !TSPX_EN_AL));
}

inline void TouchMY(CBOOL onoff)
{
	pollux_gpio_setpin(TSMY_EN, (onoff ? TSMY_EN_AL : !TSMY_EN_AL));
}

inline void TouchPY(CBOOL onoff)
{
	pollux_gpio_setpin(TSPY_EN, (onoff ? TSPY_EN_AL : !TSPY_EN_AL));
}

void SetPendownDetectState(CBOOL state)
{
	TouchMX(CFALSE);
	TouchPX(CFALSE);
	TouchMY(state); 
	TouchPY(CFALSE);
    
    pollux_gpio_setpin(PENDOWN_CON, (state ? PENDOWN_CON_AL : !PENDOWN_CON_AL));

	if(state)
		mdelay(1);
}

void SetPendownDetectLevel(U32 IntMode)
{
	set_irq_type(PENDOWN_DET_IRQ, IntMode);
}

CBOOL GetPendownDetectLevel(void)
{
	return pollux_gpio_getpin(PENDOWN_DET);
}

CBOOL GetXYValue(int *x, int *y)
{
	U16 Value[3];
	TouchMX(CTRUE);
	TouchPX(CTRUE);
	TouchMY(CFALSE);
	TouchPY(CFALSE);
	//mdelay(1);// for stable delay
	while(!adc_getvalue(&Value[0],  ADC_TOUCH_XP, 0xff));
	while(!adc_getvalue(&Value[1],  ADC_TOUCH_XP, 0xff));
	while(!adc_getvalue(&Value[2],  ADC_TOUCH_XP, 0xff));
	if(TOUCH_ADC_CH_CHANGE)
		*y = (Value[1]+Value[2])/2;
		//*y = Value[0];
	else
		*x = (Value[1]+Value[2])/2;
		//*x = Value[0];

	TouchMX(CFALSE);
	TouchPX(CFALSE);
	TouchMY(CTRUE);
	TouchPY(CTRUE);
	//mdelay(1);// for stable delay
	while(!adc_getvalue(&Value[0],  ADC_TOUCH_YP, 0xff));
	while(!adc_getvalue(&Value[1],  ADC_TOUCH_YP, 0xff));
	while(!adc_getvalue(&Value[2],  ADC_TOUCH_YP, 0xff));
	if(TOUCH_ADC_CH_CHANGE)
		*x = (Value[1]+Value[2])/2;
		// *x = Value[0];
	else
		*y = (Value[1]+Value[2])/2;
		// *y = Value[0];

	if(TOUCH_REV_X_VALUE)
		*x = ((1<<10) - *x);		// adc is 10 bits
	if(TOUCH_REV_Y_VALUE)
		*y = ((1<<10) - *y);

	return CTRUE;
}



irqreturn_t pollux_pdowndetect_interrupt( int irq, void *dev_id )
{	
	int level;

	if( pendown == 0 ) 
	{
		level = GetPendownDetectLevel();
		if( level == 1 ) 
			goto done;
		pendown = 1;
		set_irq_type(PENDOWN_DET_IRQ, IRQT_RISING);
		wake_up_interruptible( &(idle_wait) );
		
	}
	else 
	{
		level = GetPendownDetectLevel(); 
		if( level == 0 ) 
			goto done;
		
		pendown = 0;
		
 		input_report_key(pollux_input, BTN_TOUCH, 0);
		input_report_abs(pollux_input, ABS_PRESSURE, 0);
		input_sync(pollux_input);
		
		set_irq_type(PENDOWN_DET_IRQ, IRQT_FALLING);
	}
	
done:	
	return IRQ_HANDLED;
}



#define YAGI_NO_DATA_TIMEOUT (HZ/50) 

wait_queue_head_t idle_wait = __WAIT_QUEUE_HEAD_INITIALIZER(idle_wait);

int pollux_ts_thread(void *kthread)
{
	int ret;
	unsigned int x,y;
	int check=0;

    
    //x = y = 0;
	do 
	{
		if( pendown == 0 ) 
		{
			gprintk("penup--------------> wait\n\n");
			interruptible_sleep_on(&idle_wait); 
	    }
		else 
		{
			gprintk("pendown--------------> read run\n");
        	ret = interruptible_sleep_on_timeout(&idle_wait, YAGI_NO_DATA_TIMEOUT); 
        	if( ret ) 
        	{
       			gprintk("Homing routine Fatal kernel error or EXIT to run\n");
			}
			else
			{
				if( pendown )
				{
					if( (GetPendownDetectLevel()) || (pollux_gpio_getpin(GPIO_HOLD_KEY)) )
					{
                        pendown = 0;
		
 		                input_report_key(pollux_input, BTN_TOUCH, 0);
		                input_report_abs(pollux_input, ABS_PRESSURE, 0);
		                input_sync(pollux_input);
		
		                set_irq_type(PENDOWN_DET_IRQ, IRQT_FALLING);
					}else{
					    disable_irq(PENDOWN_DET_IRQ);
					    SetPendownDetectState(CFALSE);
					    GetXYValue(&x, &y);
					    SetPendownDetectState(CTRUE);

						//mdelay(2);
					    if( (x < 0x10) || (y < 0x10) || GetPendownDetectLevel())
					    {
					        input_report_key(pollux_input, BTN_TOUCH, 0);
		                    input_report_abs(pollux_input, ABS_PRESSURE, 0);
		                    input_sync(pollux_input);
					    }
					    else
					    {
#if 1 // 2009.12.24
							if (jiffies - delta_t < 2) {
								x = (old_x+x)/2;
								y = (old_y+y)/2;							
							}
							
							old_x = x;
							old_y = y;
							
							delta_t = jiffies;
#endif
					        input_report_abs(pollux_input, ABS_X, x);
 					        input_report_abs(pollux_input, ABS_Y, y);
 					        input_report_key(pollux_input, BTN_TOUCH, 1);
	 				        input_report_abs(pollux_input, ABS_PRESSURE, 1);
 					        input_sync(pollux_input);
                        }
                        
					    enable_irq(PENDOWN_DET_IRQ);
				    }	    
				}
			}
		}
	} while (!kthread_should_stop());

	return 0;
}



static int __init pollux_ts_probe(struct platform_device *pdev)
{
	pollux_input = input_allocate_device();

	if (!pollux_input) {
		printk(KERN_ERR "Unable to allocate the input device !!\n");
		return -ENOMEM;
	}

	pollux_input = pollux_input;
	pollux_input->evbit[0] = BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	pollux_input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	
	input_set_abs_params(pollux_input, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(pollux_input, ABS_Y, 0, 0x3FF, 0, 0);
	input_set_abs_params(pollux_input, ABS_PRESSURE, 0, 1, 0, 0);

	pollux_input->name       = pollux_ts_name;
	pollux_input->id.bustype = BUS_RS232;
	pollux_input->id.vendor  = 0xDEAD;
	pollux_input->id.product = 0xBEEF;
	pollux_input->id.version = 0x0001;

	input_register_device(pollux_input);

	return 0;
}

static int pollux_ts_remove(struct platform_device *pdev)
{
	input_unregister_device(pollux_input);
	input_free_device(pollux_input);

	return 0;
}

#ifdef CONFIG_PM
static int pollux_ts_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int pollux_ts_resume(struct platform_device *pdev)
{
	return 0;
}

#else
#define pollux_ts_suspend NULL
#define pollux_ts_resume  NULL
#endif

static struct platform_driver pollux_ts_driver = {
       .driver         = {
	       .name   = "pollux-ts",
	       .owner  = THIS_MODULE,
       },
       .probe          = pollux_ts_probe,
       .remove         = pollux_ts_remove,
       .suspend        = pollux_ts_suspend,
       .resume         = pollux_ts_resume,

};

static struct platform_device pollux_ts_device = {
	.name	= "pollux-ts",
	.id	= 0,
};



int __init
pollux_touch_init(void)
{
	int res;


	res = platform_device_register(&pollux_ts_device);
	if(res)
	{
		printk("fail : platform device %s (%d) \n", pollux_ts_device.name, res);
		return res;
	}

	res =  platform_driver_register(&pollux_ts_driver);
	if(res)
	{
		printk("fail : platrom driver %s (%d) \n", pollux_ts_driver.driver.name, res);
		return res;
	}

	//pollux_adc_init(0x7f);
	pollux_adc_init(0x3f);
	
    pollux_set_gpio_func(PENDOWN_CON, POLLUX_GPIO_MODE_GPIO);
	pollux_gpio_set_inout(PENDOWN_CON, POLLUX_GPIO_OUTPUT_MODE);

	
	SetPendownDetectState(CTRUE);
	
	set_irq_type(PENDOWN_DET_IRQ, IRQT_FALLING);
	if (request_irq( PENDOWN_DET_IRQ , &pollux_pdowndetect_interrupt, SA_INTERRUPT, "GPIO", NULL ) != 0)
	{
        printk( "cannot get irq %d\n", PENDOWN_DET_IRQ );
        return -1;
    }       
	
	kidle_task = kthread_run(pollux_ts_thread, NULL, "kidle_timeout");
	if( !IS_ERR(kidle_task) ) // no error
	{
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE	
		printk("pollux touch  device driver install\n");
#endif		
		return 0;
	}

   	printk("error k thread run\n");
    
    return -1;	
}

void __exit
pollux_touch_exit(void)
{
	SetPendownDetectState(CFALSE);
    free_irq(PENDOWN_DET_IRQ, NULL);
    pollux_adc_uninit();
	kthread_stop(kidle_task);

	platform_driver_unregister(&pollux_ts_driver);
	platform_device_unregister(&pollux_ts_device);

    printk("pollux touch device driver uninstall\n");
}

module_init(pollux_touch_init);
module_exit(pollux_touch_exit);

MODULE_AUTHOR("godori working");
MODULE_DESCRIPTION("pollux touchscreen driver");
MODULE_LICENSE("GPL"); 

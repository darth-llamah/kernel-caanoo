/* 
 * drivers/char/mes_gpioTokeky.c 
 * 
 * Copyright (C) 2005,2006 Gamepark Holdings, Inc. (www.gp2x.com) 
 * Hyun <hyun3678@gp2x.com> 
 */ 
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/rtc.h> 
#include <linux/irq.h> 
#include <linux/delay.h> 
#include <linux/platform_device.h> 
#include <asm/arch/regs-gpio.h> 
 
#include <asm/arch/pollux.h> 
#include "pollux_keyif.h" 

//extern spinlock_t rtc_lock; 
 
 
#define POLLUX_KEY_MAJOR 		253 
#define POLLUX_KEY_NAME     	"GPIO" 
 
#define VK_START		8	
#define VK_SELECT		9
#define VK_FL			10
#define VK_FR			11
#define VK_FA			12
#define VK_FB			13
#define VK_FX			14
#define VK_FY			15
#define VK_TAT			18
#define VK_FUNC			20			/* extend select key */

#define GPIO_POWER_OFF			POLLUX_GPA18
#define GPIO_HOLD_KEY           POLLUX_GPA10 
#define USB_VBUS_DECT	        POLLUX_GPC15 
#define SDMMC_INSERT            POLLUX_GPA19 
 
#define BD_VER_LSB              POLLUX_GPC18 
#define BD_VER_MSB              POLLUX_GPC17 

#define HOST2_EN                POLLUX_GPC16

#define USB_INTERRUPT   1 

#ifdef  USB_INTERRUPT
#define USB_VBUS_DECT_IRQ	IRQ_GPIO_C(15)		
#define USB_VBUS_REMOVED  0 
#define USB_VBUS_INSERTED 1
struct usb_connect {
	int vbus_en; 
    int check; 
};

int vbus_start = 0; 
struct usb_connect *usbCon = NULL; 
 
static unsigned char uInfo[32];
static int is_info = 0; 
static int usb_wireless_lan_status = 0;
static unsigned int usb_memory_stick_active = 0;
static unsigned int usb_memory_stick_is_mount = 0;

static irqreturn_t usb_vbus_irq(int irq, void *dev_id )
{
    struct usb_connect *usb_con = (struct usb_connect *)dev_id;
	int usben_state;

	disable_irq(irq);
    
    usben_state = pollux_gpio_getpin(USB_VBUS_DECT); 
	
    if( usb_con->vbus_en == USB_VBUS_INSERTED && usben_state == 0) 
	{
		usb_con->check = 0; 
		usb_con->vbus_en = USB_VBUS_REMOVED;	
		set_irq_type(USB_VBUS_DECT_IRQ, IRQT_RISING);	/* Current GPIO value is low: wait for raising high */
    }
    else if( usb_con->vbus_en == USB_VBUS_REMOVED && usben_state == 1) 
	{
		usb_con->check = 1;  
		usb_con->vbus_en = USB_VBUS_INSERTED;
    	set_irq_type(USB_VBUS_DECT_IRQ, IRQT_FALLING);	/* Current GPIO value is hight: wait for raising low */
		
    }
    else
    {
        if( usben_state == 1 ) // high 상태이면
    		set_irq_type( USB_VBUS_DECT_IRQ, IRQT_FALLING); // falling으로 바꾼다.
    	else
    		set_irq_type( USB_VBUS_DECT_IRQ, IRQT_RISING);
    }

	enable_irq(irq);
	
    return IRQ_HANDLED;	
} 
#endif 
 
int POLLUXkey_open(struct inode *inode, struct file *filp) 
{ 
    return (0);          /* success */		  
} 
 
int POLLUXkey_release(struct inode *inode, struct file *filp) 
{ 
	return (0); 
} 
 
ssize_t POLLUXkey_read(struct file *filp, char *Putbuf, size_t length, loff_t *f_pos) 
{ 
	unsigned long keyValue;  
	unsigned long keyTemp = 0xEB00ff; 
	 
	if( !pollux_gpio_getpin(GPIO_HOLD_KEY) ){ 
	    /* JOSYSTICK SDL MAPING */ 
	    keyTemp|=( pollux_gpio_getpin(POLLUX_GPB11) << VK_START); //HOME(START)
	    keyTemp|=( pollux_gpio_getpin(POLLUX_GPC0) << VK_SELECT); //SELECT
	    keyTemp|=( pollux_gpio_getpin(POLLUX_GPC1) << VK_FUNC); //FUNC
	    keyTemp|=( pollux_gpio_getpin(POLLUX_GPC2) << VK_FL); //L-SHLDR
	    keyTemp|=( pollux_gpio_getpin(POLLUX_GPC3) << VK_FR); //R-SHLDR    
        keyTemp|=( pollux_gpio_getpin(POLLUX_GPC4) << VK_FA); //A
		keyTemp|=( pollux_gpio_getpin(POLLUX_GPC5) << VK_FB); //B
	    keyTemp|=( pollux_gpio_getpin(POLLUX_GPC6) << VK_FX); //X    
        keyTemp|=( pollux_gpio_getpin(POLLUX_GPC7) << VK_FY); //Y
    	keyTemp|=( pollux_gpio_getpin(POLLUX_GPA12) << VK_TAT); //ANALOG TAT
    }else{ /* key hold */ 
	     keyValue = 0; 
	     copy_to_user( Putbuf, &keyValue, 4); 
	     return length;	    
	}         
	
	keyValue = ( ~keyTemp) & 0xffff00;

	copy_to_user( Putbuf, &keyValue, 4); 
	return length;	
} 
 
void usb_wireless_lan_insert(void)
{	
	usb_wireless_lan_status = 1;
	//printk("%s, status : %d\n", __func__, usb_wireless_lan_status);
}

EXPORT_SYMBOL(usb_wireless_lan_insert);

void usb_wireless_lan_remove(void)
{
	usb_wireless_lan_status = 0;
	//printk("%s, status : %d\n", __func__, usb_wireless_lan_status);
}
EXPORT_SYMBOL(usb_wireless_lan_remove);

void usb_memory_stick_enable(int flag)
{	
    usb_memory_stick_active = flag;    	
}
EXPORT_SYMBOL(usb_memory_stick_enable);

int POLLUXkey_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,unsigned long arg) 
{ 
	int ret; 
     
    switch(cmd) 
	{ 
        case IOCTL_SET_POWER_OFF: 
            printk("GO GO GO Power off \n"); 
            pollux_gpio_setpin(HOST2_EN ,0); 
            msleep(200);
            
            pollux_set_gpio_func(POLLUX_GPA18, POLLUX_GPIO_MODE_GPIO);    
		    pollux_gpio_set_inout(POLLUX_GPA18, POLLUX_GPIO_OUTPUT_MODE); 
		    pollux_gpio_setpin(GPIO_POWER_OFF ,0); 
            return 0; 
         case IOCTL_GET_INFO_LSB: 
            { 
                unsigned int info; 
                info = GetInfo(1); 
                if( copy_to_user((unsigned char*)arg, &info, sizeof(unsigned int)) ) 
	                return -EFAULT; 
            } 
            return 0; 
        case IOCTL_GET_INFO_MSB: 
            { 
                unsigned int info; 
                info = GetInfo(0); 
                if( copy_to_user((unsigned char*)arg, &info, sizeof(unsigned int)) ) 
	                return -EFAULT; 
            } 
            return 0; 
         case IOCTL_GET_USB_CHECK: 
            { 
                int usb_in; 
#ifdef  USB_INTERRUPT                 
                if(!vbus_start) usb_in = 0; 
                else usb_in = usbCon->check; 
#else                 
                usb_in = pollux_gpio_getpin(USB_VBUS_DECT); 
#endif      
                if( copy_to_user((unsigned char*)arg, &usb_in, sizeof(int)) ) 
	                return -EFAULT; 
             
            } 
            return 0;  
        case IOCTL_SET_SOFT_DISCONNECT: 
            if(usbCon != NULL)
            {
                usbCon->vbus_en = USB_VBUS_REMOVED;
    	        usbCon->check = 0;  
    	        set_irq_type(USB_VBUS_DECT_IRQ, IRQT_RISING);	/* Current GPIO value is low: wait for raising high */
                printk("IOCTL_SET_SOFT_DISCONNECT:\n");
            }               
            return 0; 
         case IOCTL_GET_INSERT_SDMMC:    
            { 
                int sd_in; 
      
                sd_in = pollux_gpio_getpin(SDMMC_INSERT); 
                if( copy_to_user((unsigned char*)arg, &sd_in, sizeof(int)) ) 
	                return -EFAULT; 
            } 
            return 0; 
        case IOCTL_GET_BOARD_VERSION: 
            { 
                int bd_rev; 
                if( pollux_gpio_getpin(BD_VER_LSB) && (!pollux_gpio_getpin(BD_VER_MSB)) ) 
                    bd_rev = 1; 
                else if( (!pollux_gpio_getpin(BD_VER_LSB)) && pollux_gpio_getpin(BD_VER_MSB) ) 
                    bd_rev = 2; 
                else if( pollux_gpio_getpin(BD_VER_LSB) && pollux_gpio_getpin(BD_VER_MSB) ) 
                    bd_rev = 3; 
                else 
                    bd_rev = 0; 
                 
                if( copy_to_user((unsigned char*)arg, &bd_rev, sizeof(int)) ) 
	                return -EFAULT; 
            }        
            return 0; 
        case IOCTL_GET_HOLD_STATUS:    
            { 
                int hold_status; 
                hold_status = pollux_gpio_getpin(GPIO_HOLD_KEY); 
                if( copy_to_user((unsigned char*)arg, &hold_status, sizeof(int)) ) 
	                return -EFAULT; 
            } 
            return 0;    
        case IOCTL_GET_PWR_STATUS: 
            { 
                int pwr_status; 
                pwr_status = Get_Pwr_Status(); 
                if( copy_to_user((unsigned char*)arg, &pwr_status, sizeof(int)) ) 
	                return -EFAULT; 
            } 
            return 0;    
        case IOCTL_GET_ID_NUM:
        	{                  	
            	if(!is_info) return -EFAULT;
            	if( copy_to_user((unsigned char*)arg, uInfo, sizeof(unsigned char[32])) ) 
	        		return -EFAULT;             	
            }
            return 0; 
        case IOCTL_SET_ID_NUM:
        	{ 
          		unsigned char *buf = uInfo;
          		int i;
            	if( copy_from_user((void*)buf,(const void*)arg,sizeof(unsigned char[32])) )
	            	return -EFAULT;         		
        		is_info = 1;        		
        	}
            return 0;  
     	case IOCTL_GET_ID_STATUS:    
            { 
                int i;
                int status = is_info; 
                if( copy_to_user((unsigned char*)arg, &status, sizeof(int)) ) 
	                return -EFAULT; 
            } 
            return 0;    
		case IOCTL_GET_USB_WIRELESS_LAN:
			{
				int status = usb_wireless_lan_status;
				if( copy_to_user((unsigned char*)arg, &status, sizeof(int)) )
					return -EFAULT;
			}
			return 0;
        case IOCTL_GET_USB_MEMORY_STICK_MOUNT:    
            { 
                unsigned int status =  usb_memory_stick_is_mount;
                if( copy_to_user((unsigned char*)arg, &status, sizeof(unsigned int)) ) 
	                return -EFAULT; 
            } 
            return 0;
        case IOCTL_SET_USB_MEMORY_STICK_MOUNT:    
            { 
                unsigned int status;
                if(copy_from_user((void *)&status, (const void *)arg, sizeof(unsigned int)))
			        return -EFAULT;
			        
                usb_memory_stick_is_mount = status;
            }    
            return 0;    
        case IOCTL_GET_USB_MEMORY_STICK_ENABLE:
			{
				unsigned int status = usb_memory_stick_active;
				
				if( copy_to_user((unsigned char*)arg, &status, sizeof(unsigned int)) )
					return -EFAULT;
			}
			return 0;
        case IOCTL_GET_PRODUCT_VERSION: 
            {                 
                unsigned int val;
              	val = 0x4E3335;
				if( copy_to_user((unsigned char*)arg, &val, sizeof(unsigned int)) ) 
	                return -EFAULT; 
            } 
            return 0;
    }	                 
 
    return 0; 
} 
 
struct file_operations POLLUXkey_fops = { 
	open:       POLLUXkey_open, 
	read:       POLLUXkey_read, 
	ioctl:      POLLUXkey_ioctl, 
	release:    POLLUXkey_release, 
}; 
 
 
struct POLLUX_key_info {
	struct device	*dev;
	struct clk		*clk;
};

static struct class *POLLUXkey_class;

static int __init POLLUX_key_probe(struct platform_device *pdev)
{
	int	ret;
	 
	ret = register_chrdev(POLLUX_KEY_MAJOR,POLLUX_KEY_NAME,&POLLUXkey_fops);
	if(ret<0)
	{
		printk("regist fail\n");
		return ret;
	}

	POLLUXkey_class = class_create(THIS_MODULE, POLLUX_KEY_NAME);
	device_create(POLLUXkey_class, NULL, MKDEV(POLLUX_KEY_MAJOR, 0), POLLUX_KEY_NAME);
                
	return 0;
} 
 
/*
 *  Cleanup
 */
static int POLLUX_key_remove(struct platform_device *pdev)
{
	printk("POLLUX_gpio_to_key_remove\n");

	return 0;
} 
 
#ifdef CONFIG_PM

/* suspend and resume support for the lcd controller */
static int POLLUX_key_suspend(struct platform_device *dev, pm_message_t state)
{
	printk("POLLUX_gpio_to_key_suspend\n");

	return 0;
}

static int POLLUX_key_resume(struct platform_device *dev)
{
	printk("POLLUX_gpio_to_key_reume\n");

	return 0;
}

#else

#define POLLUX_key_suspend NULL
#define POLLUX_key_resume  NULL

#endif

static struct platform_driver POLLUX_key_driver = {
	.probe		= POLLUX_key_probe,
	.remove		= POLLUX_key_remove,
	.suspend	= POLLUX_key_suspend,
	.resume		= POLLUX_key_resume,
	.driver		= {
		.name	= POLLUX_KEY_NAME,
		.owner	= THIS_MODULE,
	},
};

static struct platform_device POLLUX_key_device = {
	.name	= POLLUX_KEY_NAME,
	.id	= 0,
}; 
 
 
int __devinit POLLUX_key_init(void)
{ 
	int ret; 
	 
	ret = platform_device_register(&POLLUX_key_device); 
	if(ret){
		printk("failed to add platform device %s (%d) \n", POLLUX_key_device.name, ret);
		return ret;
	}

	/* register platform driver, exec platform_driver probe */
	ret =  platform_driver_register(&POLLUX_key_driver);
	if(ret){
		printk("failed to add platrom driver %s (%d) \n", POLLUX_key_driver.driver.name, ret);
	} 
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE	 
	printk(KERN_NOTICE "POLLUX gpio to key Driver\n"); 
#endif 
	 
	 	 
#ifdef  USB_INTERRUPT     /* HYUN_DEBUG */         
    if(!vbus_start) 
    {               
        usbCon = kzalloc(sizeof(struct usb_connect), GFP_KERNEL); 
	         
	    if (unlikely(!usbCon)){ 
	        printk(KERN_WARNING "failed to kzalloc usb.\n" ); 
		    return 0; 
	    }	 
	     
	    vbus_start = 1; 
	    usbCon->vbus_en = pollux_gpio_getpin(USB_VBUS_DECT);  
	     
	    if( usbCon->vbus_en ){ 
	        set_irq_type(USB_VBUS_DECT_IRQ, IRQT_FALLING); 
	        usbCon->check = 1; 
	    }     
	    else{ 
	        set_irq_type( USB_VBUS_DECT_IRQ, IRQT_RISING); 
            usbCon->check = 0; 
        } 
        
        if(request_irq(USB_VBUS_DECT_IRQ, &usb_vbus_irq, 0, "GPIO", usbCon ))  
	    { 
		    printk(KERN_WARNING "failed to request vubs detect interrupt.\n" ); 
		    kfree(usbCon); 
            vbus_start = 0; 
        }    	 
    } 
 
#endif	 
	 	 
	return 0;	 
} 
	 
static void __exit POLLUX_key_exit() 
{
	unregister_chrdev( POLLUX_KEY_MAJOR, POLLUX_KEY_NAME ); 
	 
	platform_device_unregister(&POLLUX_key_device); 
	platform_driver_unregister(&POLLUX_key_driver); 

	printk(KERN_NOTICE "POLLUX gpio to key Driver release\n"); 
} 
module_init(POLLUX_key_init); 
module_exit(POLLUX_key_exit); 
 
MODULE_LICENSE("GPL");

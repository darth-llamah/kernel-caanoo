
/*
센서 드라이버를 application단에서 처리할것인지, 커널 드라이버단에서 처리할것인지 ??

1차적으로는 application단에서 처리하고, 인터럽트만 띄워주는 방식으로 진행한다.

*/

#undef ACCEL_KERNEL_DRIVER


#define ACCEL_VERSION	"0.0.1"


#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/sysctl.h>
#include <linux/wait.h>
#include <linux/bcd.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/platform_device.h> 

#include <asm/current.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
 
#include <asm/arch/regs-gpio.h> 
 
#include <asm/arch/pollux.h> 

#define ACCEL_DEV_MAJOR		252
#define ACCEL_DEV_NAME		"accel"
#define ACCEL_IRQ			IRQ_GPIO_B(14)

static struct fasync_struct *accel_async_queue;
static DECLARE_WAIT_QUEUE_HEAD(accel_wait);
static spinlock_t accel_lock;

#define ACCEL_IS_OPEN		0x01	/* means /dev/accel is in use */
static unsigned long accel_status = 0;	/* bitmapped status byte. */

static ssize_t accel_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static int accel_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static unsigned int accel_poll(struct file *file, poll_table *wait);

irqreturn_t accel_interrupt(int irq, void *dev_id)
{
	// lars remove spin_lock  (2010-08-04)
	//spin_lock (&accel_lock);
// TODO:

	//spin_unlock (&accel_lock);
	wake_up_interruptible(&accel_wait);	
	kill_fasync (&accel_async_queue, SIGIO, POLL_IN);

	//printk("accel_interrupt !!\n");
	return IRQ_HANDLED;
}

/*
 *	Now all the various file operations that we export.
 */
static ssize_t accel_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
#if 1
	return 0;
#else
	DECLARE_WAITQUEUE(wait, current);
	unsigned long irq_occured;
	unsigned char interrupt_status;
	ssize_t retval;

	add_wait_queue(&accel_wait, &wait);
	do {
		__set_current_state(TASK_INTERRUPTIBLE);
		spin_lock_irq (&accel_lock);
// TODO:
		spin_unlock_irq(&accel_lock);

// TODO:
//		if( .... ) break;

		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto out;
		}
		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}
		schedule();
	} while (1);

// TODO:
// Copy interrupt status information to user-area
//	copy_to_user(buf, .... , sizeof(unsigned long));
//	retval = sizeof( .... );
 out:
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&accel_wait, &wait);

	return retval;
#endif
}

static int accel_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
#if 1
	return ret;
#else
// TODO:
	switch(cmd)
	{
	case .... :
	{
	}
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
#endif
}

static int accel_open(struct inode *inode, struct file *filp)
{
	int ret=0;

	spin_lock_irq(&accel_lock);

	if(accel_status & ACCEL_IS_OPEN) {
		printk("accel driver is already open...\n");
		goto out_busy;
	}

	set_irq_type(ACCEL_IRQ, IRQT_RISING);
	ret = request_irq(ACCEL_IRQ, &accel_interrupt, SA_INTERRUPT, ACCEL_DEV_NAME, NULL);
	if (ret) {
		printk(KERN_ERR "-E- can't register IRQ %d for accelerometer, ret=%d\n", ACCEL_IRQ, ret);
		goto out_busy;
	}
	accel_status |= ACCEL_IS_OPEN;

	spin_unlock_irq(&accel_lock);
	return 0;

out_busy:
	spin_unlock_irq(&accel_lock);
	return -EBUSY;
};

static int accel_fasync(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &accel_async_queue);
}

static int accel_release(struct inode *inode, struct file *file)
{
	unsigned char tmp;

	free_irq(ACCEL_IRQ, NULL);

	if (file->f_flags & FASYNC) {
		accel_fasync (-1, file, 0);
	}
	spin_lock_irq(&accel_lock);
	accel_status &= ~ACCEL_IS_OPEN;
	spin_unlock_irq(&accel_lock);

	return 0;

}

static unsigned int accel_poll(struct file *file, poll_table *wait)
{
#if 0
	unsigned char l;

	poll_wait(file, &accel_wait, wait);

	spin_lock_irq(&accel_lock);
// TODO:
	l = .... ;
	spin_unlock_irq(&accel_lock);

	if(l!=0)
		return POLLIN | POLLRDNORM;
#endif
	return 0;
}

static const struct file_operations accel_fops = {
	owner:		THIS_MODULE,
	open:		accel_open,
	read:		accel_read,
	release:	accel_release,
	fasync:		accel_fasync,
	poll:		accel_poll,
	ioctl:		accel_ioctl,
};

static struct class *accel_class;

static int __init accel_probe(struct platform_device *pdev)
{
	int ret;

	ret = register_chrdev(ACCEL_DEV_MAJOR, ACCEL_DEV_NAME, &accel_fops);
	if(ret < 0) {
		printk("-E- can't register device for accel.\n");
		return ret;
	}

	accel_class = class_create(THIS_MODULE, ACCEL_DEV_NAME);
	device_create(accel_class, NULL, MKDEV(ACCEL_DEV_MAJOR, 0), ACCEL_DEV_NAME);

	return (0);
}

static int accel_remove(struct platform_device *pdev)
{
	return (0);
}

#ifdef CONFIG_PM
// TODO:
// implement suspend and resume function
static int accel_suspend(struct platform_device *pdev, pm_message_t state)
{
	return (0);
}
static int accel_resume(struct platform_device *pdev)
{
	return (0);
}

#else

#	define	accel_suspend	NULL
#	define accel_resume		NULL

#endif

static struct platform_driver accel_platform_driver = {
	.driver		= {
		.name	= ACCEL_DEV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= accel_probe,
	.remove		= accel_remove,
	.suspend	= accel_suspend,
	.resume		= accel_resume,
};

static struct platform_device accel_platform_device = {
	.name	= ACCEL_DEV_NAME,
	.id		= 0,
};

static int __init accel_init(void)
{
	int ret;

	ret = platform_device_register(&accel_platform_device);
	if(ret) {
		printk("-E- failed to add platform device %s (%d) \n", accel_platform_device.name, ret);
		return ret;
	}

	/* register platform driver. exec platform_driver probe */
	ret = platform_driver_register(&accel_platform_driver);
	if(ret) {
		printk("-E- failed to add platform driver %s (%d) \n", accel_platform_driver.driver.name, ret);
		return ret;
	}

	printk(KERN_NOTICE "-I- registered accelerometer device. \n");
	return (0);
}

static void __exit accel_exit(void)
{
	unregister_chrdev(ACCEL_DEV_MAJOR, ACCEL_DEV_NAME);

	platform_device_unregister(&accel_platform_device);
	platform_driver_unregister(&accel_platform_driver);

	printk(KERN_NOTICE "-I- accel: removed. \n");

}

module_init(accel_init);
module_exit(accel_exit);

MODULE_LICENSE("GPL");

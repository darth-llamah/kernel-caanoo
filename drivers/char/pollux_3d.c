/* pollux 3D accelerator driver 
 *
 * main.c -- Main driver functionality.
 *
 * Brian Cavagnolo <brian@cozybit.com>
 * Andrey Yurovsky <andrey@cozybit.com>
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#include "pollux_3d.h"


/* device private data */
struct ga3d_device ga3d = {
	.mem = NULL,
	.cdev = NULL,
	.major = GA3D_MAJOR,
};



int ga3d_read_proc(char *buf, char **start, off_t offset, int count,
		   int *eof, void *data)
{
	int len = 0;
	void *base = ga3d.mem;

	/* Warning: do not write more than one page of data. */
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_CPCONTROL", ioread32(base + GRP3D_CPCONTROL));
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_CMDQSTART", ioread32(base + GRP3D_CMDQSTART));
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_CMDQEND", ioread32(base + GRP3D_CMDQEND));
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_CMDQFRONT", ioread32(base + GRP3D_CMDQFRONT));
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_CMDQREAR", ioread32(base + GRP3D_CMDQREAR));
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_STATUS", ioread32(base + GRP3D_STATUS));
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_INT", ioread32(base + GRP3D_INT));
	len += sprintf(buf + len,"%s = 0x%08X\n",
		       "GRP3D_CONTROL", ioread32(base + GRP3D_CONTROL));

	*eof = 1; 
	return len; 
}

void ga3d_make_proc(void)
{
	ga3d.proc = create_proc_read_entry("driver/ga3d",
					   0,
					   NULL,
					   ga3d_read_proc,
					   NULL );
}

void ga3d_remove_proc(void)
{
	remove_proc_entry("driver/ga3d", NULL);
}


/*******************************
 * character device operations *
 *******************************/

static int ga3d_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static void ga3d_vma_open(struct vm_area_struct *vma)
{
	printk(KERN_DEBUG "ga3d: vma_open virt:%lX, phs %lX\n",
	       vma->vm_start, vma->vm_pgoff<<PAGE_SHIFT);
}

static void ga3d_vma_close(struct vm_area_struct *vma)
{
	printk(KERN_DEBUG "ga3d: vma_close\n");
}

struct vm_operations_struct ga3d_vm_ops = {
	.open  = ga3d_vma_open,
	.close = ga3d_vma_close,
};

static int ga3d_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret;
	
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_ops = &ga3d_vm_ops;
	vma->vm_flags |= VM_IO;
	
	ret = io_remap_pfn_range(vma,
				 vma->vm_start, 
				 0xc001a000>>PAGE_SHIFT,
				 vma->vm_end - vma->vm_start, 
				 vma->vm_page_prot);
	if(ret < 0) {
		printk(KERN_ALERT "ga3d: failed to mmap\n");
		return -EAGAIN;
	}
	
	ga3d_vma_open(vma);
	return 0;
}

struct file_operations ga3d_fops = {
	.owner = THIS_MODULE,
	.open  = ga3d_open,
	.mmap = ga3d_mmap,
};

static struct class *POLLUX3D_class;

/*********************
 *  module functions *
 *********************/

static int pollux_ga3d_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "ga3d: failed to get resource\n");
		return -ENXIO;
	}

	if(!request_mem_region(res->start, (res->end - res->start)+1, "pollux-ga3d")) {
		printk(KERN_ERR "ga3d: failed to map region.");
		return -EBUSY;
	}

#if 0
	ga3d.mem = ioremap_nocache(res->start, (res->end - res->start)+1);
#else
    ga3d.mem = (void __iomem *)(res->start);
#endif
	
	if(ga3d.mem == NULL) {
		printk(KERN_ERR "ga3d: failed to ioremap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

    ret = register_chrdev(ga3d.major, "ga3d", &ga3d_fops);
	if(ret < 0) {
		printk(KERN_ERR "ga3d: failed to get a device\n");
		goto fail_dev;
	}
	if(ga3d.major == 0) ga3d.major = ret;
    
    ga3d.cdev = cdev_alloc();
	ga3d.cdev->owner = THIS_MODULE;
	ga3d.cdev->ops = &ga3d_fops;
	ret = cdev_add(ga3d.cdev, 0, 1);
	if(ret < 0) {
		printk(KERN_ALERT "ga3d: failed to create character device\n");
		goto fail_add;
	}

#ifdef CONFIG_PROC_FS
	ga3d_make_proc();
#endif
	
	POLLUX3D_class = class_create(THIS_MODULE, "ga3d");
	device_create(POLLUX3D_class, NULL, MKDEV(ga3d.major, 0), "ga3d");


	return 0;

fail_add:
	unregister_chrdev(ga3d.major, "ga3d");
fail_dev:
fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);

	return ret;
}

static int pollux_ga3d_remove(struct platform_device *pdev)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

#ifdef CONFIG_PROC_FS
	ga3d_remove_proc();
#endif

	unregister_chrdev(ga3d.major, "ga3d");
	if(ga3d.cdev != NULL)
		cdev_del(ga3d.cdev);
	
	if(ga3d.mem != NULL)
		iounmap(ga3d.mem);

	release_mem_region(res->start, (res->end - res->start) + 1);

	return 0;
}

static struct platform_driver pollux_ga3d_driver = {
	.probe		= pollux_ga3d_probe,
	.remove		= pollux_ga3d_remove,
	.driver		= {
		.name	= "pollux-ga3d",
		.owner	= THIS_MODULE,
	},
};

static int __init ga3d_init(void)
{
	return platform_driver_register(&pollux_ga3d_driver);
}

static void __exit ga3d_cleanup(void)
{
	platform_driver_unregister(&pollux_ga3d_driver);
}

module_init(ga3d_init);
module_exit(ga3d_cleanup);
MODULE_AUTHOR("Brian Cavagnolo, Andrey Yurovsky");
MODULE_VERSION("1:2.0");

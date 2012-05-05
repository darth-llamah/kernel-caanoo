/*
 * $Id: mmcchar.c,v 1.76 2005/11/07 11:14:20 gleixner Exp $
 *
 * Character-device access to raw mmc devices.
 *
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/card.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/arch/regs-gpio.h>

static struct class *mmc_nd_class;
static struct mmc_host *mmc = NULL;
static struct mmc_card *pcard = NULL; 

static int in_use = 0;

#define ND_MAX_SIZE 	512 * 10
#define MMC_ND_CHAR_MAJOR	255
#define SDMMC_INSERT            POLLUX_GPA19

static loff_t mmc_nd_lseek (struct file *file, loff_t offset, int orig)
{
	loff_t new_offset = 0;
	
	switch (orig) {
	case SEEK_SET:
		new_offset = offset;
		break;
	case SEEK_CUR:
		new_offset = file->f_pos + offset;
		break;
	case SEEK_END:
		new_offset = ND_MAX_SIZE;
		break;
	default:
		return -EINVAL;
	}

	if (new_offset >= 0 && new_offset <= ND_MAX_SIZE )
		return file->f_pos = new_offset;

	return -EINVAL;
}

static int mmc_nd_open(struct inode *inode, struct file *file)
{
	int minor = iminor(inode);
	int devnum = minor >> 1;
	
		
	if(pollux_gpio_getpin(SDMMC_INSERT) )
		goto err;
	
	/* You can't open the RO devices RW */
	if ((file->f_mode & 2) && (minor & 1))
		goto err;
	
	if(mmc!=NULL) goto err;
	
	pcard = mmc_restore_card();
	mmc =  mmc_restore_host();
	if (IS_ERR(mmc))
		goto err;
	
	mmc_claim_host(mmc);
	
	return 0;
err:
	return -EACCES;
			
} /* mmc_open */

/*====================================================================*/

static int mmc_nd_close(struct inode *inode, struct file *file)
{
	/* Only sync if opened RW */
	mmc_release_host(mmc);
	mmc->buff_direct = 0;
	mmc = NULL;
	pcard = NULL;
	
	return 0;
} /* mmc_close */


static ssize_t mmc_nd_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct mmc_request	mrq;
	struct mmc_command	cmd;
	struct mmc_data		data;
	int i,j;

	if(pollux_gpio_getpin(SDMMC_INSERT) )
		return -EFAULT;
	
	if((*ppos + count > ND_MAX_SIZE) || (count == 0) || (count > 512)) 
		return -EFAULT;
	
	if( in_use ) 
		return -EFAULT; 
	
	mmc->kbuf = kmalloc(512, GFP_KERNEL);
	if (!mmc->kbuf)
		return -ENOMEM;
	
	mmc->buff_direct = 1;
	in_use = 1;
	
	memset(&mrq, 0, sizeof(struct mmc_request));
	mrq.cmd = &cmd;
	mrq.data = &data;
	
	cmd.arg = *ppos;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	data.blksz = 1 << 9;
	data.blocks = 1;
	
	if (mmc_card_blockaddr(pcard))
		cmd.arg >>= 9;
	
	mmc_set_data_timeout(&data, pcard, 0);
	
	mrq.stop = NULL;
	data.flags = MMC_DATA_READ;
	cmd.opcode = MMC_READ_SINGLE_BLOCK;
	
	data.sg = NULL;
	data.sg_len = 0;
	
	mmc_wait_for_req(mmc, &mrq);
	if (cmd.error) {
		printk(KERN_ERR "error %d sending read/write command\n",cmd.error);				
		kfree(mmc->kbuf);
		in_use = 0;
		mmc->buff_direct = 0;
		return -EFAULT;	
	}

	if (data.error) {
		printk(KERN_ERR "error %d transferring data\n",data.error);		
		kfree(mmc->kbuf);
		in_use = 0;
		mmc->buff_direct = 0;
		return -EFAULT;	
	}
	
	if (copy_to_user(buf, mmc->kbuf, 512)) {
		kfree(mmc->kbuf);
		in_use = 0;
		mmc->buff_direct = 0;
		return -EFAULT;	
	}

	mmc->buff_direct = 0;
	kfree(mmc->kbuf);
	in_use = 0;
	
	mdelay(50);
	return count;

} /* mmc_read */

static ssize_t mmc_nd_write(struct file *file, const char __user *buf, size_t count,loff_t *ppos)
{
	int err;
	struct mmc_request	mrq;
	struct mmc_command	cmd;
	struct mmc_data		data;
	
	if(pollux_gpio_getpin(SDMMC_INSERT) )
		return -EFAULT;
		
	if( (*ppos + count > ND_MAX_SIZE) || (count == 0) || (count > 512)) 
		return -EFAULT;
	
	if( in_use ) 
		return -EFAULT;
		
	mmc->kbuf=kmalloc(512, GFP_KERNEL);
	if (!mmc->kbuf)
		return -ENOMEM;
	
	err = copy_from_user(mmc->kbuf, buf, 512);
	if (err) {
		kfree(mmc->kbuf);
		return  -EFAULT;		
	}
	
	mmc->buff_direct = 1;
	in_use = 1;
	memset(&mrq, 0, sizeof(struct mmc_request));
	mrq.cmd = &cmd;
	mrq.data = &data;
	
	cmd.arg = *ppos;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	data.blksz = 1 << 9;
	data.blocks = 1;
	
	if (mmc_card_blockaddr(pcard))
		cmd.arg >>= 9;
		
	mmc_set_data_timeout(&data, pcard, 1);
	
	mrq.stop = NULL;
	data.flags = MMC_DATA_WRITE;
	cmd.opcode = MMC_WRITE_BLOCK;
	
	data.sg = NULL;
	data.sg_len = 0;
	
	mmc_wait_for_req(mmc, &mrq);
	if (cmd.error) {
		printk(KERN_ERR "error %d sending read/write command\n",cmd.error);				
		kfree(mmc->kbuf);
		in_use = 0;
		mmc->buff_direct = 0;
		return -EFAULT;	
	}

	if (data.error) {
		printk(KERN_ERR "error %d transferring data\n",data.error);		
		kfree(mmc->kbuf);
		in_use = 0;
		mmc->buff_direct = 0;
		return -EFAULT;	
	}
	
	do {
		int err;
		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = pcard->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
		err = mmc_wait_for_cmd(mmc, &cmd, 5);
		if (err) {
			printk(KERN_ERR "error %d requesting status\n",err);
			kfree(mmc->kbuf);
			in_use = 0;
			mmc->buff_direct = 0;
			return -EFAULT;	
		}
	} while (!(cmd.resp[0] & R1_READY_FOR_DATA));

	mmc->buff_direct = 0;
	kfree(mmc->kbuf);
	in_use = 0;
	
	mdelay(50);
	return count;
	
} /* mmc_write */

static const struct file_operations mmc_nd_fops = {
	.owner		= THIS_MODULE,
	.llseek		= mmc_nd_lseek,
	.read		= mmc_nd_read,
	.write		= mmc_nd_write,
	.open		= mmc_nd_open,
	.release	= mmc_nd_close,
};

static int __init init_mmc_nd_char(void)
{
	if (register_chrdev(MMC_ND_CHAR_MAJOR, "mmc_nd", &mmc_nd_fops)) {
		printk(KERN_NOTICE "Can't allocate major number %d for MMC/SD ND Character Device.\n",
		       MMC_ND_CHAR_MAJOR);
		return -EAGAIN;
	}

	mmc_nd_class = class_create(THIS_MODULE, "mmc_nd");
		
	if (IS_ERR(mmc_nd_class)) {
		printk(KERN_ERR "Error creating mmc nd class.\n");
		unregister_chrdev(MMC_ND_CHAR_MAJOR, "mmc_nd");
		return PTR_ERR(mmc_nd_class);
	}
	
	device_create(mmc_nd_class, NULL, MKDEV(MMC_ND_CHAR_MAJOR, 0), "mmc_nd");
	
	return 0;
}

static void __exit cleanup_mmc_nd_char(void)
{
	class_destroy(mmc_nd_class);
	unregister_chrdev(MMC_ND_CHAR_MAJOR, "mmc_nd");
}

module_init(init_mmc_nd_char);
module_exit(cleanup_mmc_nd_char);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Direct character-device access to mmc devices");

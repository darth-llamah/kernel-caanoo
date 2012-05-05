/*
 * Direct UBI block device access
 *
 * $Id: ubiblock.c,v 1.68 2005/11/07 11:14:20 gleixner Exp $
 *
 * (C) 2000-2003 Nicolas Pitre <nico@cam.org>
 * (C) 1999-2003 David Woodhouse <dwmw2@infradead.org>
 * (C) 2008 Yurong Tan <nancydreaming@gmail.com> :
 *        borrow mtdblock.c to work on top of UBI
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/hdreg.h>
#include <linux/mtd/ubi_blktrans.h>
#include <linux/mutex.h>
#include "ubi.h"

#define UNMAPPED 0

static struct ubiblk_dev {
	struct ubi_volume_desc *uv;
	int count;
	struct mutex cache_mutex;
	unsigned short vbw;           //virt block number of write cache
	unsigned short vbr;            //virt block number of read cache
	unsigned char *write_cache;
	unsigned char *read_cache;
	enum { STATE_UNUSED, STATE_USED } read_cache_state, write_cache_state;
} *ubiblks[UBI_MAX_VOLUMES];   

void ubiblk_setup_writecache(struct ubiblk_dev *ubiblk, int virt_block);
int ubiblk_flush_writecache(struct ubiblk_dev *ubiblk);

int ubiblk_flush_writecache(struct ubiblk_dev *ubiblk)
{
	if (STATE_UNUSED == ubiblk->write_cache_state)
		return 0;

	ubi_leb_write(ubiblk->uv, ubiblk->vbw, ubiblk->write_cache, 0, 
		      ubiblk->uv->vol->usable_leb_size, UBI_UNKNOWN);
	ubiblk->write_cache_state = STATE_UNUSED;
	return 0;
}

void ubiblk_setup_writecache(struct ubiblk_dev *ubiblk, int virt_block)
{
	ubiblk->vbw = virt_block;
	ubiblk->write_cache_state = STATE_USED;

	ubi_leb_read(ubiblk->uv, ubiblk->vbw, ubiblk->write_cache, 0, 
		     ubiblk->uv->vol->usable_leb_size, UBI_UNKNOWN);
}

static int do_cached_write (struct ubiblk_dev *ubiblk, unsigned long sector, 
			    int len, const char *buf)
{
	struct ubi_volume_desc *uv = ubiblk->uv;
	int ppb = uv->vol->ubi->leb_size / uv->vol->ubi->min_io_size;
	unsigned short sectors_per_page =  uv->vol->ubi->min_io_size >> 9;
	unsigned short page_shift =  ffs(uv->vol->ubi->min_io_size) - 1;
	unsigned short virt_block, page, page_offset; 	
	unsigned long virt_page; 
	
	virt_page = sector / sectors_per_page;
	page_offset = sector % sectors_per_page;
	virt_block = virt_page / ppb; 
	page = virt_page % ppb;

	if(ubi_is_mapped(uv, virt_block ) == UNMAPPED ){
		mutex_lock(&ubiblk->cache_mutex);
		ubiblk_flush_writecache(ubiblk);
		mutex_unlock(&ubiblk->cache_mutex);
	
		ubiblk_setup_writecache(ubiblk, virt_block);
          	ubi_leb_map(uv, virt_block, UBI_UNKNOWN);

	} else {
		if ( STATE_USED == ubiblk->write_cache_state ) {
			if ( ubiblk->vbw != virt_block) {
			// Commit before we start a new cache.
				mutex_lock(&ubiblk->cache_mutex);
				ubiblk_flush_writecache(ubiblk);
				mutex_unlock(&ubiblk->cache_mutex);

				ubiblk_setup_writecache(ubiblk, virt_block);
				ubi_leb_unmap(uv, virt_block);
			  	ubi_leb_map(uv, virt_block, UBI_UNKNOWN);
			} else {
				//dprintk("cache hit: 0x%x\n", virt_page);
			}
		} else {
//			printk("with existing mapping\n");
			ubiblk_setup_writecache(ubiblk, virt_block);
			ubi_leb_unmap(uv, virt_block);		
			ubi_leb_map(uv, virt_block, UBI_UNKNOWN);
		}                        
	}		
	memcpy(&ubiblk->write_cache[(page<<page_shift) +(page_offset<<9)],
	       buf,len);
	return 0;
}

static int do_cached_read (struct ubiblk_dev *ubiblk, unsigned long sector, 
			   int len, char *buf)
{
	struct ubi_volume_desc *uv = ubiblk->uv;
	int ppb = uv->vol->ubi->leb_size / uv->vol->ubi->min_io_size;
	unsigned short sectors_per_page =  uv->vol->ubi->min_io_size >> 9;
	unsigned short page_shift =  ffs(uv->vol->ubi->min_io_size) - 1;
	unsigned short virt_block, page, page_offset; 	
	unsigned long virt_page; 
		
	virt_page = sector / sectors_per_page;
	page_offset = sector % sectors_per_page;
	virt_block = virt_page / ppb; 
	page = virt_page % ppb;

	if(ubiblk->vbw == virt_block){		
		mutex_lock(&ubiblk->cache_mutex);
		ubiblk_flush_writecache(ubiblk);
		mutex_unlock(&ubiblk->cache_mutex);
	}

	if ( ubi_is_mapped( uv, virt_block) == UNMAPPED){
		// In a Flash Memory device, there might be a logical block that is
		// not allcated to a physical block due to the block not being used.
		// All data returned should be set to 0xFF when accessing this logical 
		// block.
		//	dprintk("address translate fail\n");
		memset(buf, 0xFF, 512);
	} else {

		if( ubiblk->vbr != virt_block ||ubiblk->read_cache_state == STATE_UNUSED ){
			ubiblk->vbr = virt_block;
			ubi_leb_read(uv, virt_block, ubiblk->read_cache, 0, uv->vol->usable_leb_size, 0);				
			ubiblk->read_cache_state = STATE_USED;
		}
		memcpy(buf, &ubiblk->read_cache[(page<<page_shift)+(page_offset<<9)], len);
	}
	return 0;
}

static int ubiblk_readsect(struct ubi_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct ubiblk_dev *ubiblk = ubiblks[dev->devnum];
	return do_cached_read(ubiblk, block, 512, buf);
}

static int ubiblk_writesect(struct ubi_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct ubiblk_dev *ubiblk = ubiblks[dev->devnum];
	return do_cached_write(ubiblk, block, 512, buf);
}

static int ubiblk_init_vol(int dev, struct ubi_volume_desc *uv)
{
	struct ubiblk_dev *ubiblk;
	int ret;
			
	ubiblk = kmalloc(sizeof(struct ubiblk_dev), GFP_KERNEL);
	if (!ubiblk)
		return -ENOMEM;

	memset(ubiblk, 0, sizeof(*ubiblk));

	ubiblk->count = 1;
	ubiblk->uv = uv;
	mutex_init (&ubiblk->cache_mutex);

	ubiblk->write_cache = vmalloc(ubiblk->uv->vol->usable_leb_size); 
	ubiblk->read_cache = vmalloc(ubiblk->uv->vol->usable_leb_size);
	
	if(!ubiblk->write_cache || 
		!ubiblk->read_cache )
		return -ENOMEM;

	ubiblk->write_cache_state = STATE_UNUSED;
	ubiblk->read_cache_state = STATE_UNUSED;

	ubiblks[dev] = ubiblk;
	DEBUG(MTD_DEBUG_LEVEL1, "ok\n");
	return 0;
}

static int ubiblk_open(struct ubi_blktrans_dev *ubd)
{
	int dev = ubd->devnum;
	int res = 0;

	DEBUG(MTD_DEBUG_LEVEL1,"ubiblock_open\n");

	if (ubiblks[dev]) {
		ubiblks[dev]->count++;
		printk("%s: increase use count\n",__FUNCTION__);
		return 0;
	}

	/* OK, it's not open. Create cache info for it */
	res = ubiblk_init_vol(dev, ubd->uv);
	return res;
}

static int ubiblk_release(struct ubi_blktrans_dev *ubd)
{
	int dev = ubd->devnum;
	struct ubiblk_dev *ubiblk = ubiblks[dev];
	struct ubi_device *ubi = ubiblk->uv->vol->ubi;
	
	mutex_lock(&ubiblk->cache_mutex);
	ubiblk_flush_writecache(ubiblk);
	mutex_unlock(&ubiblk->cache_mutex);

	if (!--ubiblk->count) {
		/* It was the last usage. Free the device */
		ubiblks[dev] = NULL;

		if (ubi->mtd->sync)
			ubi->mtd->sync(ubi->mtd);

		vfree(ubiblk->write_cache);
		vfree(ubiblk->read_cache);
		kfree(ubiblk);
	}
	return 0;
}

static int ubiblk_flush(struct ubi_blktrans_dev *dev)
{
	struct ubiblk_dev *ubiblk = ubiblks[dev->devnum];
	struct ubi_device *ubi = ubiblk->uv->vol->ubi;
	
	mutex_lock(&ubiblk->cache_mutex);
	ubiblk_flush_writecache(ubiblk);
	mutex_unlock(&ubiblk->cache_mutex);

	if (ubi->mtd->sync)
		ubi->mtd->sync(ubi->mtd);
	return 0;
}

static void ubiblk_add_vol_dev(struct ubi_blktrans_ops *tr, struct ubi_volume *vol)
{
	struct ubi_blktrans_dev *dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return;

	dev->devnum = vol->vol_id;
	dev->size = vol->used_bytes >> 9;
	dev->tr = tr;

	if (vol->bdev_mode == UBI_READONLY)
		dev->readonly = 1;

	vol->ubi->bdev_major = tr->major; 

	add_ubi_blktrans_dev(dev);
}

static void ubiblk_remove_vol_dev(struct ubi_blktrans_dev *dev)
{
	del_ubi_blktrans_dev(dev);
	kfree(dev);
}

static int ubiblk_getgeo(struct ubi_blktrans_dev *dev, struct hd_geometry *geo)
{
	memset(geo, 0, sizeof(*geo));
	geo->heads     = 4;
	geo->sectors   = 16;
	geo->cylinders = dev->size/(4*16); 
	return 0;
}

static struct ubi_blktrans_ops ubiblk_tr = {
	.name		         = "ubiblock",
	.major                   = 0,
	.part_bits	         = 0,
	.blksize 	         = 512,
	.open		         = ubiblk_open,
	.release	         = ubiblk_release,
	.readsect	         = ubiblk_readsect,
	.writesect	         = ubiblk_writesect,
	.getgeo                  = ubiblk_getgeo,
	.flush		         = ubiblk_flush,
	.add_vol	         = ubiblk_add_vol_dev,
	.remove_vol	         = ubiblk_remove_vol_dev,
	.owner		         = THIS_MODULE,
};

static int __init init_ubiblock(void)
{
	return register_ubi_blktrans(&ubiblk_tr);
}

static void __exit cleanup_ubiblock(void)
{
	deregister_ubi_blktrans(&ubiblk_tr);
}

module_init(init_ubiblock);
module_exit(cleanup_ubiblock);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Pitre <nico@cam.org> , Yurong Tan <nancydreaming@gmail.com>");
MODULE_DESCRIPTION("Caching read/erase/writeback block device emulation access to UBI volumes");

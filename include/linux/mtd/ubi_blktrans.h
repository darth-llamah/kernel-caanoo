/*
 * $Id: ubi_blktrans.h
 *
 * (C) 2003 David Woodhouse <dwmw2@infradead.org>
 * (C) 2008 Yurong Tan <nancydreaming@gmail.com> : borrow from MTD blktrans.h for UBI used
 * Interface to Linux block layer for UBI 'translation layers'.
 *
 */

#ifndef __UBI_TRANS_H__
#define __UBI_TRANS_H__

#include <linux/mutex.h>

struct hd_geometry;
struct ubi_volume_desc;
struct ubi_blktrans_ops;
struct file;
struct inode;

struct ubi_blktrans_dev {
	struct ubi_blktrans_ops *tr;
	struct list_head list;
	struct ubi_volume_desc *uv;
      	struct mutex lock;
	int devnum;
	unsigned long size;
	int readonly;
	void *blkcore_priv; /* gendisk in 2.5, devfs_handle in 2.4 */
};

struct blkcore_priv; /* Differs for 2.4 and 2.5 kernels; private */

struct ubi_blktrans_ops {
	char *name;
	int major;
	int part_bits;
	int blksize;
	int blkshift;

	/* Access functions */
	int (*readsect)(struct ubi_blktrans_dev *dev,
		    unsigned long block, char *buffer);
	int (*writesect)(struct ubi_blktrans_dev *dev,
		     unsigned long block, char *buffer);

	/* Block layer ioctls */
	int (*getgeo)(struct ubi_blktrans_dev *dev, struct hd_geometry *geo);
	int (*flush)(struct ubi_blktrans_dev *dev);

	/* Called with mtd_table_mutex held; no race with add/remove */
	int (*open)(struct ubi_blktrans_dev *dev);
	int (*release)(struct ubi_blktrans_dev *dev);

	/* Called on {de,}registration and on subsequent addition/removal
	   of devices, with mtd_table_mutex held. */
	void (*add_vol)(struct ubi_blktrans_ops *tr, struct ubi_volume *vol);
	void (*remove_vol)(struct ubi_blktrans_dev *dev);

	struct list_head devs;
	struct list_head list;
	struct module *owner;

	struct ubi_blkcore_priv *blkcore_priv;
};

extern int add_ubi_blktrans_dev(struct ubi_blktrans_dev *new);
extern int del_ubi_blktrans_dev(struct ubi_blktrans_dev *old);
extern int register_ubi_blktrans(struct ubi_blktrans_ops *tr);
extern int deregister_ubi_blktrans(struct ubi_blktrans_ops *tr);


#endif /* __UBI_TRANS_H__ */

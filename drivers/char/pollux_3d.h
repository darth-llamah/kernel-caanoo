/* pollux 3D Engine
 * 
 * ga3d.h -- Module and hardware-specific definitions.
 *
 * Andrey Yurovsky <andrey@cozybit.com> */

#ifndef POLLUX_3D_H
#define POLLUX_3D_H

#include <linux/module.h>
#include <linux/types.h>
#include <asm/io.h>

/* module-related definitions */

#define GA3D_MAJOR 	249	

struct ga3d_device {
	void __iomem *mem;
	//void *mem;
	struct cdev *cdev;
	dev_t dev;
	int major;
	struct proc_dir_entry *proc;
};

/* HAL */
/* Register offsets */
#define GRP3D_CPCONTROL		(0x0)
#define GRP3D_CMDQSTART		(0x4)
#define GRP3D_CMDQEND		(0x8)
#define GRP3D_CMDQFRONT		(0xC)
#define GRP3D_CMDQREAR		(0x10)
#define GRP3D_STATUS		(0x14)
#define GRP3D_INT		    (0x18)
#define GRP3D_CONTROL		(0x20)


#endif


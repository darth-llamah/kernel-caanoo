/*
 * drivers/mtd/nand/pollux.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This is a device driver for the NAND flash device found on the
 *   pollux NRK board which utilizes a Samsung 8 Mbyte part.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>

#define NAND_SIZE		128
#define MTD0_SIZE       8		// bootloader, kernel image	
#define MTD1_SIZE       88		// rootfs

static struct mtd_info *pollux_mtd = NULL;

#ifdef CONFIG_MTD_PARTITIONS

static const char *part_probes[] = { "cmdlinepart", NULL };

#ifdef CONFIG_POLLUX_FAST_BOOT_ENABLE
#define NUM_PARTITIONS	3
struct mtd_partition partition_info32[] = {
	[0] = {
		.name	= "d0",
		.size	= MTD0_SIZE*(SZ_1M),         
		.offset	= 0x0,			
		.mask_flags = MTD_WRITEABLE,        // read-only area
	},
    [1] = {
	    .name   = "d1",
		.size   = MTD1_SIZE*(SZ_1M),
		.offset = MTDPART_OFS_NXTBLK,   /* Align this partition with the erase size */
		.mask_flags = 0,
	},
	[2] = {
		.name   = "d2",
		.size   = MTDPART_SIZ_FULL,
		.offset = MTDPART_OFS_NXTBLK,   /* Align this partition with the erase size */
		.mask_flags = 0,
	},
};
#else
#define NUM_PARTITIONS	2
struct mtd_partition partition_info32[] = {
	[0] = {
		.name	= "d0",
		.size	= 8*(SZ_1M),         
		.offset	= 0x0,			
		.mask_flags = MTD_WRITEABLE,        // read-only area		
		},
	[1] = {
		.name	= "d1",
		.size	= MTDPART_SIZ_FULL,       	// 
		.offset	= MTDPART_OFS_APPEND,			
		.mask_flags = 0, 		
		},					
	};
#endif
#endif	/* CONFIG_MTD_PARTITIONS */

#ifdef CONFIG_POLLUX_SHADOW_ONE
#define NAND_PBR	0xAC000000
#else
#define NAND_PBR	0x2C000000
#endif   /* CONFIG_POLLUX_SHADOW_ONE */

#define MASK_CLE	0x10
#define MASK_ALE	0x18

static void pollux_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	void __iomem* addr = chip->IO_ADDR_W;

	if (cmd == NAND_CMD_NONE)
		return;

	if(ctrl & NAND_CLE)
		addr += MASK_CLE;
	else if(ctrl & NAND_ALE)
		addr += MASK_ALE;

	writeb(cmd, addr);
}

// Disable NAND write protect
// this code is temp. code
// by claude
static void disable_nand_wp(void)
{
	unsigned int *gpioav;

	gpioav = ioremap_nocache(0xC000F0C0, 0x10);
	printk("gpioav = %p\n", gpioav);
	
	printk("gpioav : %x%x\n",*gpioav, *(gpioav+0x04));
	*gpioav |= (1 << 6); //gpio av[6] bit set 1
	gpioav += 0x04;
	*gpioav |= (1 << 6); // gpio av[6] bit set output mode
	printk("gpioav : %x%x\n",*(gpioav-0x04), *gpioav);
	iounmap(gpioav);
}

/*
 * Main initialization routine
 */
static int __init pollux_nand_init(void)
{
	struct nand_chip *this;
	const char *part_type = 0;
	int mtd_parts_nb = 0;
	struct mtd_partition *mtd_parts = 0;
	void __iomem* nandaddr=0;


//	disable_nand_wp();
	nandaddr = ioremap(NAND_PBR,0x1000);
	if(!nandaddr)
	{
		printk("failed to ioremap nand flash\n");
		return -ENOMEM;
	}

	/* Allocate memory for MTD device structure and private data */
	pollux_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	if (!pollux_mtd) {
		printk("Unable to allocate NAND MTD device structure.\n");
		return -ENOMEM;
	}

	/* Get pointer to private data */
	this = (struct nand_chip *)(&pollux_mtd[1]);

	/* Initialize structures */
	memset(pollux_mtd, 0, sizeof(struct mtd_info));
	memset(this, 0, sizeof(struct nand_chip));

	/* Link the private data with the MTD structure */
	pollux_mtd->priv = this;
	pollux_mtd->owner = THIS_MODULE;

	/* insert callbacks */
	this->IO_ADDR_R = nandaddr;
	this->IO_ADDR_W = nandaddr;
	this->cmd_ctrl = pollux_nand_hwcontrol;
	this->dev_ready = NULL;
	//this->chip_delay = 20;
	this->chip_delay = 30;
	this->ecc.mode = NAND_ECC_SOFT;
	this->options = NAND_SAMSUNG_LP_OPTIONS| NAND_NO_AUTOINCR;

	printk("Searching for NAND flash...\n");
	/* Scan to find existence of the device */
	if (nand_scan(pollux_mtd, 1)) {
		kfree(pollux_mtd);
		return -ENXIO;
	}

#ifdef CONFIG_MTD_PARTITIONS
	pollux_mtd->name = "pollux-nand";
	mtd_parts_nb = parse_mtd_partitions(pollux_mtd, part_probes, &mtd_parts, 0);
	if (mtd_parts_nb > 0)
		part_type = "command line";
	else
		mtd_parts_nb = 0;
#endif
	
	if (mtd_parts_nb == 0) {
		mtd_parts = partition_info32;
		mtd_parts_nb = NUM_PARTITIONS;
		part_type = "static";
	}

	/* Register the partitions */
	printk(KERN_NOTICE "Using %s partition definition\n", part_type);
	add_mtd_partitions(pollux_mtd, mtd_parts, mtd_parts_nb);

	/* Return happy */
	return 0;
}

/*
 * Clean up routine
 */
static void __exit pollux_nand_cleanup(void)
{
	/* Unregister the device */
	del_mtd_device(pollux_mtd);

	/* Free the MTD device structure */
	kfree(pollux_mtd);
}

void print_oob(const char *header, struct mtd_info *mtd)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	printk("%s:\t", header);

	for(i=0; i<64; i++)
		printk("%02x ", chip->oob_poi[i]);

	printk("\n");
}
EXPORT_SYMBOL(print_oob);

module_init(pollux_nand_init);
module_exit(pollux_nand_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Claude <claude@mesdigital.com>");
MODULE_DESCRIPTION("NAND MTD map driver for pollux NRK board");

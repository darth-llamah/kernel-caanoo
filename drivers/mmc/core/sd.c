/*
 *  linux/drivers/mmc/sd.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  SD support Copyright (C) 2004 Ian Molton, All Rights Reserved.
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <asm/arch/pollux.h>

#include "core.h"
#include "sysfs.h"
#include "mmc_ops.h"
#include "sd_ops.h"

#include "core.h"

static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static void mmc_decode_cid(struct mmc_card *card)
{
	u32 *resp = card->raw_cid;
    u32 InfoTemp;
    
	memset(&card->cid, 0, sizeof(struct mmc_cid));

	/*
	 * SD doesn't currently have a version field so we will
	 * have to assume we can parse this.
	 */
	card->cid.manfid		= UNSTUFF_BITS(resp, 120, 8);
	card->cid.oemid			= UNSTUFF_BITS(resp, 104, 16);
	card->cid.prod_name[0]		= UNSTUFF_BITS(resp, 96, 8);
	card->cid.prod_name[1]		= UNSTUFF_BITS(resp, 88, 8);
	card->cid.prod_name[2]		= UNSTUFF_BITS(resp, 80, 8);
	card->cid.prod_name[3]		= UNSTUFF_BITS(resp, 72, 8);
	card->cid.prod_name[4]		= UNSTUFF_BITS(resp, 64, 8);
	card->cid.hwrev			= UNSTUFF_BITS(resp, 60, 4);
	card->cid.fwrev			= UNSTUFF_BITS(resp, 56, 4);
	card->cid.serial		= UNSTUFF_BITS(resp, 24, 32);
	card->cid.year			= UNSTUFF_BITS(resp, 12, 8);
	card->cid.month			= UNSTUFF_BITS(resp, 8, 4);

	card->cid.year += 2000; /* SD cards year offset */

#if 0
    InfoTemp = card->cid.manfid | (card->cid.oemid << 8) | 
                ( ((card->cid.fwrev & 0x0f) |  ( (card->cid.hwrev & 0x0f) <<4)) << 24 );

    SetInfo( InfoTemp, 1); 
    InfoTemp = card->cid.serial;
    SetInfo( InfoTemp, 0); 
#else
    InfoTemp = ((card->cid.prod_name[1] << 24) | (card->cid.prod_name[2] << 16) | (card->cid.prod_name[3] << 8) | 
                    (card->cid.prod_name[4])); 
    
    SetInfo( InfoTemp, 1); 
    InfoTemp = card->cid.prod_name[0];
    SetInfo( InfoTemp, 0); 
#endif
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static void mmc_decode_csd(struct mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m, csd_struct;
	u32 *resp = card->raw_csd;

	csd_struct = UNSTUFF_BITS(resp, 126, 2);

	switch (csd_struct) {
	case 0:
		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->capacity	  = (1 + m) << (e + 2);

		csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
		csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
		csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
		csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
		csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
		csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
		csd->write_partial = UNSTUFF_BITS(resp, 21, 1);
		break;
	case 1:
		/*
		 * This is a block-addressed SDHC card. Most
		 * interesting fields are unused and have fixed
		 * values. To avoid getting tripped by buggy cards,
		 * we assume those fixed values ourselves.
		 */
		mmc_card_set_blockaddr(card);

		csd->tacc_ns	 = 0; /* Unused */
		csd->tacc_clks	 = 0; /* Unused */

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m];
		csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

		m = UNSTUFF_BITS(resp, 48, 22);
		csd->capacity     = (1 + m) << 10;

		csd->read_blkbits = 9;
		csd->read_partial = 0;
		csd->write_misalign = 0;
		csd->read_misalign = 0;
		csd->r2w_factor = 4; /* Unused */
		csd->write_blkbits = 9;
		csd->write_partial = 0;
		break;
	default:
		printk("%s: unrecognised CSD structure version %d\n",
			mmc_hostname(card->host), csd_struct);
		mmc_card_set_bad(card);
		return;
	}
}

/*
 * Given a 64-bit response, decode to our card SCR structure.
 */
static void mmc_decode_scr(struct mmc_card *card)
{
	struct sd_scr *scr = &card->scr;
	unsigned int scr_struct;
	u32 resp[4];

	BUG_ON(!mmc_card_sd(card));

	resp[3] = card->raw_scr[1];
	resp[2] = card->raw_scr[0];

	scr_struct = UNSTUFF_BITS(resp, 60, 4);
	if (scr_struct != 0) {
		printk("%s: unrecognised SCR structure version %d\n",
			mmc_hostname(card->host), scr_struct);
		mmc_card_set_bad(card);
		return;
	}

	scr->sda_vsn = UNSTUFF_BITS(resp, 56, 4);
	scr->bus_widths = UNSTUFF_BITS(resp, 48, 4);
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
static int mmc_switch_hs(struct mmc_card *card)
{
	int err;
	u8 *status;

	if (!(card->host->caps & MMC_CAP_SD_HIGHSPEED))
		return MMC_ERR_NONE;

	err = MMC_ERR_FAILED;

	status = kmalloc(64, GFP_KERNEL);
	if (!status) {
		printk("%s: could not allocate a buffer for switch "
		       "capabilities.\n",
			mmc_hostname(card->host));
		return err;
	}

	err = mmc_sd_switch(card, 0, 0, 1, status);
	if (err != MMC_ERR_NONE) {
		/*
		 * Card not supporting high-speed will ignore the
		 * command.
		 */
		if (err == MMC_ERR_TIMEOUT)
			err = MMC_ERR_NONE;
		goto out;
	}

	if (status[13] & 0x02)
		card->sw_caps.hs_max_dtr = 50000000;

	err = mmc_sd_switch(card, 1, 0, 1, status);
	if (err != MMC_ERR_NONE)
		goto out;

	if ((status[16] & 0xF) != 1) {
		printk(KERN_WARNING "%s: Problem switching card "
			"into high-speed mode!\n",
			mmc_hostname(card->host));
	} else {
		mmc_card_set_highspeed(card);
		mmc_set_timing(card->host, MMC_TIMING_SD_HS);
	}

out:
	kfree(status);

	return err;
}

/*
 * Host is being removed. Free up the current card.
 */
static void mmc_sd_remove(struct mmc_host *host)
{
	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_remove_card(host->card);
	host->card = NULL;
}

/*
 * Card detection callback from host.
 */
static void mmc_sd_detect(struct mmc_host *host)
{
	int err;

	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_claim_host(host);

	/*
	 * Just check if our card has been removed.
	 */
	err = mmc_send_status(host->card, NULL);

	mmc_release_host(host);

	if (err != MMC_ERR_NONE) {
		mmc_remove_card(host->card);
		host->card = NULL;

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_release_host(host);
	}
}

static const struct mmc_bus_ops mmc_sd_ops = {
	.remove = mmc_sd_remove,
	.detect = mmc_sd_detect,
};

/*
 * Starting point for SD card init.
 */
int mmc_attach_sd(struct mmc_host *host, u32 ocr)
{
	struct mmc_card *card;
	int err;
	u32 cid[4];
	unsigned int max_dtr;

	BUG_ON(!host);
	BUG_ON(!host->claimed);

	mmc_attach_bus(host, &mmc_sd_ops);

	host->ocr = mmc_select_voltage(host, ocr);

	/*
	 * Can we support the voltage(s) of the card(s)?
	 */
	if (!host->ocr)
		goto err;

	/*
	 * Since we're changing the OCR value, we seem to
	 * need to tell some cards to go back to the idle
	 * state.  We wait 1ms to give cards time to
	 * respond.
	 */
	mmc_go_idle(host);

	/*
	 * If SD_SEND_IF_COND indicates an SD 2.0
	 * compliant card and we should set bit 30
	 * of the ocr to indicate that we can handle
	 * block-addressed SDHC cards.
	 */
	err = mmc_send_if_cond(host, host->ocr);
	if (err == MMC_ERR_NONE)
		ocr = host->ocr | (1 << 30);

	mmc_send_app_op_cond(host, ocr, NULL);

	/*
	 * Fetch CID from card.
	 */
	err = mmc_all_send_cid(host, cid);
	if (err != MMC_ERR_NONE)
		goto err;

	/*
	 * Allocate card structure.
	 */
	card = mmc_alloc_card(host);
	if (IS_ERR(card))
		goto err;

	card->type = MMC_TYPE_SD;
	memcpy(card->raw_cid, cid, sizeof(card->raw_cid));

	/*
	 * Get card RCA.
	 */
	err = mmc_send_relative_addr(host, &card->rca);
	if (err != MMC_ERR_NONE)
		goto free_card;

	mmc_set_bus_mode(host, MMC_BUSMODE_PUSHPULL);


	/*
	 * Fetch CSD from card.
	 */
	err = mmc_send_csd(card, card->raw_csd);
	if (err != MMC_ERR_NONE)
		goto free_card;

	mmc_decode_csd(card);
	mmc_decode_cid(card);

	/*
	 * Fetch SCR from card.
	 */
	err = mmc_select_card(card);
	if (err != MMC_ERR_NONE)
		goto free_card;

	err = mmc_app_send_scr(card, card->raw_scr);
	if (err != MMC_ERR_NONE)
		goto free_card;

	mmc_decode_scr(card);

	/*
	 * Check if card can be switched into high-speed mode.
	 */
	err = mmc_switch_hs(card);
	if (err != MMC_ERR_NONE)
		goto free_card;

	/*
	 * Compute bus speed.
	 */
	max_dtr = (unsigned int)-1;

	if (mmc_card_highspeed(card)) {
		if (max_dtr > card->sw_caps.hs_max_dtr)
			max_dtr = card->sw_caps.hs_max_dtr;
	} else if (max_dtr > card->csd.max_dtr) {
		max_dtr = card->csd.max_dtr;
	}

	mmc_set_clock(host, max_dtr);

	/*
	 * Switch to wider bus (if supported).
	 */
	if ((host->caps && MMC_CAP_4_BIT_DATA) &&
		(card->scr.bus_widths & SD_SCR_BUS_WIDTH_4)) {
		err = mmc_app_set_bus_width(card, MMC_BUS_WIDTH_4);
		if (err != MMC_ERR_NONE)
			goto free_card;

		mmc_set_bus_width(host, MMC_BUS_WIDTH_4);
	}

	/*
	 * Set to read-only mode according to the write-protection tab state
	 */
	if (!host->ops->get_ro) {
		printk(KERN_WARNING "%s: host does not "
			"support reading read-only "
			"switch. assuming write-enable.\n",
			mmc_hostname(host));
	} else {
		if (host->ops->get_ro(host)) {
			printk(KERN_DEBUG "Attached card is in read-only mode\n");
			mmc_card_set_readonly(card);
		}
	}

	host->card = card;

	mmc_release_host(host);

    
	err = mmc_register_card(card);
	if (err)
		goto reclaim_host;

	return 0;

reclaim_host:
	mmc_claim_host(host);
free_card:
	mmc_remove_card(card);
	host->card = NULL;
err:
	mmc_detach_bus(host);
	mmc_release_host(host);

	return 0;
}


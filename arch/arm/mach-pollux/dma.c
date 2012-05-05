/*
 * arch/arm/mach-mp2530/dma.c
 *
 * godori(Ko Hyun Chul), omega5                 							- project manager
 * nautilus_79(Lee Jang Ho) 												- main programmer
 * amphell(Bang Chang Hyeok)                        						- Hardware Engineer
 *
 * 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/mp2530/index.html
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
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sysdev.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/delay.h>
 
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/dma.h>

#include <asm/mach/dma.h>
#include <asm/arch/map.h>

static irqreturn_t mp2530_dma_irq_handler(int irq, void *dev_id);
static void mp2530_dma_process(struct mp2530_dma_channel_t *p);


int mp2530_dma_open_channel(int ch, int reqid, enum OPMODE op, dma_isr_t isr, dma_transfer_t tx)
{
	struct mp2530_dma_channel_t *pch;
	int ret = 0;
	
	if( (ch < 0) && (ch >= POLLUX_NUM_DMA) ) 
	{
		printk("open_dma_channel() error:  ch = %d\n", ch);
		return -1;
	}
	
	pch = &mp2530dma[ch];
	
	memset( (char *)pch, 0x00, sizeof(struct mp2530_dma_channel_t) );
	
	pch->dmabase = dma_channel_base[ch];
	pch->irq     = dma_channel_irq[ch];
	pch->dreg    = (struct dma_regset_t *)pch->dmabase;
	pch->ch      = ch;
	pch->reqid   = reqid;
	pch->op      = op;
	pch->isrfn   = isr;
	pch->txfn    = tx;
	pch->opened  = 1;  
	pch->active  = 0;  
	pch->qcnt    = 0;
	
	init_dmaq(pch);
	
	//ret = request_irq(pch->irq, mp2530_dma_irq_handler, SA_INTERRUPT, "DMA", (void *)pch); // old style before 2.6.24
	ret = request_irq(pch->irq, mp2530_dma_irq_handler, IRQF_DISABLED, "DMA", (void *)pch);
    if (ret != 0)
    {
        printk(KERN_ERR "DMA: cannot get irq %d\n", pch->irq);
        return ret;
    }
	return 0;
}


void mp2530_dma_close_channel(int ch)
{
	struct mp2530_dma_channel_t *pch;
	int ret;
	if( (ch < 0) || (ch >= POLLUX_NUM_DMA) )
	{
		printk("mp2530_dma_close_channel: input channel number error: %d\n", ch);
		return;
	}

	pch = &mp2530dma[ch];
	if( pch->opened == 0 ) 
	{
		printk(KERN_ERR "mp2530_dma_close_channel: Not opened dma channel try to close: %d\n", ch);
		return;
	}
	
	pch->opened	 = 0; 
	ret = mp2530_dma_flush_all(ch);
	if( ret < 0 )
	{
		printk(KERN_ERR "mp2530_dma_flush_all error\n");
	}
	
	free_irq(pch->irq, (void *)pch);
}

static irqreturn_t mp2530_dma_irq_handler(int irq, void *dev_id)
{
	struct mp2530_dma_channel_t *p = (struct mp2530_dma_channel_t *)dev_id;

	if( p->isrfn)
		p->isrfn(p->curp);
		
	p->active = 0;
	
	mp2530_dma_process(p);
	
    return IRQ_HANDLED;
}

static void mp2530_dma_process(struct mp2530_dma_channel_t *p)
{
	if( (p->active==0) && (p->opened==1) ) 
	{
		p->curp = get_dmaq(p);
		if(p->curp == NULL ) 
			return;
			
        if( p->txfn )
        	p->txfn( (void *)p );
		p->active = 1;
		p->qcnt--;
	}
}

int mp2530_dma_queue_buffer(int ch, void *data)
{
	struct mp2530_dma_channel_t *pch;
	unsigned long flags;
	int ret = 0;

	if( (ch < 0) || (ch >= POLLUX_NUM_DMA) )
	{
		printk("mmsp2_dma_queue_buffer: input channel number error: %d\n", ch);
		return -1;
	}
	
	pch = &mp2530dma[ch];
	if( pch->opened == 0 ) 
	{
		printk(KERN_ERR "mmsp2_dma_queue_buffer: Not opened dma channel try to close : %d\n", ch);
		return -1;
	}
	local_irq_save(flags);
	
	ret = put_dmaq(pch, data);
	if( ret < 0 )
	{
		printk(KERN_ERR "mp2530_dma_queue_buffer(): put dma q error\n");
		goto qerror;
	}
	pch->qcnt++;
	mp2530_dma_process(pch);

	local_irq_restore(flags);

	return 0;

qerror:	
	local_irq_restore(flags);
	return -1;
}

int mp2530_dma_flush_all(int ch)
{
	struct mp2530_dma_channel_t *pch;
	unsigned long flags;

	if( (ch < 0) || (ch >= POLLUX_NUM_DMA) )
	{
		printk("mp2530_dma_flush_all: input channel number error: %d\n", ch);
		return -1;
	}

	pch = &mp2530dma[ch];

	while(1)
	{
		if( mp2530_dma_check_running(pch) == 0) 
			break;
		else
		{
			mdelay(100); 
		}
	}
	
	
	local_irq_save(flags);
	pch->active = 0;
	pch->qcnt = 0;
	pch->curp = NULL;
	init_dmaq(pch);
	local_irq_restore(flags);
	
	return 0;
}


int mp2530_dma_change_txfn(int ch, dma_transfer_t tx)
{
	struct mp2530_dma_channel_t *pch;
	unsigned long flags;
	
	if( (ch < 0) && (ch >= POLLUX_NUM_DMA) ) 
	{
		printk("open_dma_channel() error:  ch = %d\n", ch);
		return -1;
	}
	
	pch = &mp2530dma[ch];

	if( (pch->active = 1) || (mp2530_dma_check_running(pch) == 1) ) 	
	
	local_irq_save(flags);
	pch->txfn    = tx;
	local_irq_restore(flags);
	
	return 0;
}



EXPORT_SYMBOL(mp2530_dma_open_channel);
EXPORT_SYMBOL(mp2530_dma_close_channel);
EXPORT_SYMBOL(mp2530_dma_queue_buffer);
EXPORT_SYMBOL(mp2530_dma_flush_all);
EXPORT_SYMBOL(mp2530_dma_change_txfn);


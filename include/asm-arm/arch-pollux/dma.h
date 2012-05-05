/*
 * include/asm-arm/arch-mp2530/dma.h
 *
 * Copyright. 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/mp2530/index.html
 * 
 * godori(Ko Hyun Chul), omega5                 							- project manager
 * gtgkjh(Kim Jung Han), choi5946(Choi Hyun Jin), nautilus_79(Lee Jang Ho) 	- main programmer
 * amphell(Bang Chang Hyeok)                        						- Hardware Engineer
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
#ifndef __ASM_ARM_ARCH_DMA_H
#define __ASM_ARM_ARCH_DMA_H

#include <asm/arch/regs-dma.h>

typedef void (*dma_isr_t)(void *data);	
typedef void (*dma_transfer_t)(void *data);	


#define DMA_QUEUE_SIZE  16 
struct mp2530_dma_channel_t
{
	struct  dma_regset_t *dreg;    // register map pointer
	int                   ch;      // dma channel number
	int                   irq;     // irq number
	int                   opened;  // open?
	int                   active;  // active? 
	unsigned long         dmabase; // base address setting
	int                   reqid;   // dma request id
	enum OPMODE           op;      // transfer mode, mem-to-mem, mem-to-io, io-to-mem
	void                 *dmaq[DMA_QUEUE_SIZE];
	int                   qfront;  
	int                   qrear;
	void                 *curp; // dmaq로서부터 가지고 온 녀석은 여기에 대입한다.
	int                   qcnt;
	
	/* driver handlers */
	dma_isr_t             isrfn;  /* buffer done callback */
	dma_transfer_t        txfn;   /* channel transfer callback */
};

extern struct mp2530_dma_channel_t mp2530dma[POLLUX_NUM_DMA];
extern unsigned long dma_channel_base[POLLUX_NUM_DMA];
extern unsigned long dma_channel_irq[POLLUX_NUM_DMA];

// mp2530-dma-proto.c
int   mp2530_dma_set_request_id(struct mp2530_dma_channel_t *p, int id);
int   mp2530_dma_get_request_id(struct mp2530_dma_channel_t *p);
int   mp2530_dma_check_running(struct mp2530_dma_channel_t *p);
void  mp2530_dma_transfer_mem_to_mem( struct mp2530_dma_channel_t *p, u32 srcaddr, u32 dstaddr, u32 txsize);
void  mp2530_dma_transfer_mem_to_io( struct mp2530_dma_channel_t *p, u32 srcaddr, u32 dstaddr, u32 dst_bitwidth, u32 txsize);
void  mp2530_dma_transfer_io_to_mem( struct mp2530_dma_channel_t *p, u32 srcaddr, u32 src_bitwidth, u32 dstaddr, u32 txsize);
void  mp2530_dma_stop( struct mp2530_dma_channel_t *p );
u32   mp2530_dma_get_max_transfer_size( void );
void  mp2530_dma_run( struct mp2530_dma_channel_t *p );
void  mp2530_dma_set_src_addr(struct mp2530_dma_channel_t *p, u32 saddr);    
u32   mp2530_dma_get_src_addr(struct mp2530_dma_channel_t *p);   
void  mp2530_dma_set_dst_addr(struct mp2530_dma_channel_t *p, u32 daddr);  
u32   mp2530_dma_get_dst_addr(struct mp2530_dma_channel_t *p); 
void  mp2530_dma_set_data_size(struct mp2530_dma_channel_t *p, u32 dwSize );
u32   mp2530_dma_get_data_size(struct mp2530_dma_channel_t *p );
void  mp2530_dma_set_attribute(struct mp2530_dma_channel_t *p, enum OPMODE opmode, u32 bitwidth, int src_increase, int dst_increase );
int   put_dmaq(struct mp2530_dma_channel_t *p, void *abuf);
void *get_dmaq(struct mp2530_dma_channel_t *p);
void  init_dmaq(struct mp2530_dma_channel_t *p);


// arch/arm/mach-mp2530/dma.c
int  mp2530_dma_open_channel(int ch, int reqid, enum OPMODE op, dma_isr_t isr, dma_transfer_t tx);
void mp2530_dma_close_channel(int ch);
int  mp2530_dma_queue_buffer(int ch, void *data);
int  mp2530_dma_flush_all(int ch);
int  mp2530_dma_change_txfn(int ch, dma_transfer_t tx); // transfer function change, read/write process of 1 dma channel(ex> SDHC read/write)

#endif // __ASM_ARM_REGS_DMA_H


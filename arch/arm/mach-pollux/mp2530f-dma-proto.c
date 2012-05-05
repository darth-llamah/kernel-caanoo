/*
 * arch/arm/mach-mp2530/dma.c
 *
 * Driver for DMA on the Magiceyes POLLUX
 *
 * Based on Magiceyes's WinCE BSP mes_dma03.cpp
 *
 * Copyright. 2003-2008 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/pollux/index.html
 * 
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho)	    - main programmer
 * amphell(Bang Chang Hyeok)    - Hardware Engineer
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
 *
 * 2008-03-20 prototype, only write through mode implement(by godori)
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

//#include <asm/mach/dma.h>
#include <asm/arch/map.h>

#include <asm/arch/dma.h>

 
//#define GDEBUG                  // ghcstop: my(godori) debug style....^^
#ifdef  GDEBUG
#    define dprintk(fmt, x... )  printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
#    define dprintk( x... )
#endif

struct mp2530_dma_channel_t mp2530dma[POLLUX_NUM_DMA];

unsigned long dma_channel_base[POLLUX_NUM_DMA] = 
{
	POLLUX_DMA_BASE(0),
	POLLUX_DMA_BASE(1),
	POLLUX_DMA_BASE(2),
	POLLUX_DMA_BASE(3),
	POLLUX_DMA_BASE(4),
	POLLUX_DMA_BASE(5),
	POLLUX_DMA_BASE(6),
	POLLUX_DMA_BASE(7)
};


unsigned long dma_channel_irq[POLLUX_NUM_DMA] = 
{
	IRQ_DMA_SUB(0),
	IRQ_DMA_SUB(1),
	IRQ_DMA_SUB(2),
	IRQ_DMA_SUB(3),
	IRQ_DMA_SUB(4),
	IRQ_DMA_SUB(5),
	IRQ_DMA_SUB(6),
	IRQ_DMA_SUB(7)
};
	




int mp2530_dma_set_request_id(struct mp2530_dma_channel_t *p, int id)
{
	if( (id < 0) && (id > 64) ) 
	{
		printk("set_dma_request_id() error:  id = %d\n", id);
		return -1;
	}
	p->dreg->DMAREQID	= (u16)id;
	
	return 1;
}

int mp2530_dma_get_request_id(struct mp2530_dma_channel_t *p)
{
	return (int)p->dreg->DMAREQID;
}


int mp2530_dma_check_running(struct mp2530_dma_channel_t *p)
{
	unsigned int busy = (1UL<<16);
	return (p->dreg->DMAMODE & busy) ? 1 : 0; // 1이면 busy
}




//------------------------------------------------------------------------------
//	MES_IDMAModule implementation
//------------------------------------------------------------------------------
/**
 *  @brief	    Transfer memory to memory.
 *  @param[in]  srcaddr      source buffer address.\n this value must be aligned by 64 bits(8 bytes).     
 *  @param[in]  dstaddr destination buffer address.\n this value must be aligned by 64 bits(8 bytes).
 *  @param[in]  txsize transfer size in bytes.                                                      
 *  @return	    None.
 */
void mp2530_dma_transfer_mem_to_mem( struct mp2530_dma_channel_t *p, u32 srcaddr, u32 dstaddr, u32 txsize)
{
	u32 regvalue = p->dreg->DMAMODE & POLLUX_DMA_INTMASK; // interrup mask bit 유지해야한다.
	
    MES_ASSERT ( 0 == (((u32)srcaddr) % 8) );
    MES_ASSERT ( 0 == (((u32)dstaddr) % 8) );
    MES_ASSERT ( (MAX_TRANSFER_SIZE >= txsize) && (0 != txsize) );
	MES_ASSERT( CNULL != p->dreg );
	MES_ASSERT( ! mp2530_dma_check_running(p) );

	p->dreg->DMASRCADDR = (u32)srcaddr;
	p->dreg->DMADSTADDR = (u32)dstaddr;
	p->dreg->DMALENGTH 	= (U16)(txsize-1);

	regvalue |= (u32)( 0
					| (0UL<< 0) // SRCIOSIZE 0:8bit 1:16bit 2:32bit
					| (0UL<< 2) // SRCIOMODE 0:memory to memory 1:i/o to memory
					| (0UL<< 3) // PACKMODE must be 0 (not supported)
					| (0UL<< 4) // SRCNOTINC 0:Normal, 1:not incremented
					| (1UL<< 5) // SRCNOTREQCHK 1:No Chk, 0:Chk Request, when SRCIOMODE=1
					| (0UL<< 8) // DSTIOSIZE 0:8bit 1:16bit 2:32bit
					| (0UL<<10) // DSTIOMODE 0:memory to memory 1:memory to i/o
					| (0UL<<12)	// DSTNOTINC 0:Normal, 1:not incremented
					| (1UL<<13)	// DSTNOTREQCHK	1:No Chk, 0:Chk Request, when DSTIOMODE=1
					| (1UL<<19) // RUN
					| (0UL<<20) // STOP, for pollux
					);
	//dprintk("1 p->ch = %d, 0x%08x -> 0x%08x, len = %u mode = 0x%08x\n", p->ch, p->dreg->DMASRCADDR, p->dreg->DMADSTADDR, p->dreg->DMALENGTH, regvalue);
					
	p->dreg->DMAMODE = regvalue;
	
	//dprintk("2 p->ch = %d, 0x%08x -> 0x%08x, len = %u mode = 0x%08x\n", p->ch, p->dreg->DMASRCADDR, p->dreg->DMADSTADDR, p->dreg->DMALENGTH, p->dreg->DMAMODE);	
}

//------------------------------------------------------------------------------
/**
 *  @brief	    Transfer memory to peripheral.
 *  @param[in]  srcaddr			        Source buffer address.                 
 *  @param[in]  dstaddr             	A baseaddress of peripheral device.    
 *  @param[in]  DestinationPeriID	    An index of peripheral device.         
 *  @param[in]  dst_bitwidth	    IO size, in bits, of peripheral device.
 *  @param[in]  txsize			Transfer size in bytes.                
 *  @return	    None.
 */
void mp2530_dma_transfer_mem_to_io( struct mp2530_dma_channel_t *p, u32 srcaddr, u32 dstaddr, u32 dst_bitwidth,	u32 txsize)
{
	u32 regvalue = p->dreg->DMAMODE & POLLUX_DMA_INTMASK; // interrup mask bit 유지해야한다.
	
#if 0	
    MES_ASSERT ( 0 == (((u32)srcaddr) % 4) );
    MES_ASSERT ( 8 == dst_bitwidth || 16 == dst_bitwidth || 32 == dst_bitwidth );
	MES_ASSERT ( 64 > p->reqid );    			  
    MES_ASSERT ( (MAX_TRANSFER_SIZE >= txsize) && (0 != txsize) );
	MES_ASSERT( CNULL != p->dreg );
	MES_ASSERT( ! mp2530_dma_check_running(p) );
#endif

	p->dreg->DMASRCADDR = (u32)srcaddr;
	p->dreg->DMADSTADDR = (u32)dstaddr;
	p->dreg->DMALENGTH  = (U16)(txsize-1);
	p->dreg->DMAREQID   = p->reqid; // open시 세팅

	//dprintk("1 p->ch = %d, 0x%08x -> 0x%08x, len = %u reqid= %d mode = 0x%08x\n", p->ch, p->dreg->DMASRCADDR, p->dreg->DMADSTADDR, p->dreg->DMALENGTH, p->dreg->DMAREQID, p->dreg->DMAMODE);

	regvalue |= (u32)( 0
					| (0UL<< 0) // SRCIOSIZE 0:8bit 1:16bit 2:32bit
					| (0UL<< 2) // SRCIOMODE 0:memory to memory 1:i/o to memory
					| (0UL<< 3) // PACKMODE must be 0 (not supported)
					| (0UL<< 4) // SRCNOTINC 0:Normal, 1:not incremented
					| (1UL<< 5) // SRCNOTREQCHK 1:No Chk, 0:Chk Request, when SRCIOMODE=1
					| ((dst_bitwidth>>4)<< 8) // DSTIOSIZE 0:8bit 1:16bit 2:32bit
					| (1UL<<10) // DSTIOMODE 0:memory to memory 1:memory to i/o
					| (1UL<<12)	// DSTNOTINC 0:Normal, 1:not incremented
					| (0UL<<13)	// DSTNOTREQCHK	1:No Chk, 0:Chk Request, when DSTIOMODE=1
					| (1UL<<19) // RUN
					| (0UL<<20) // STOP, for pollux
					);
	p->dreg->DMAMODE = regvalue;
	//dprintk("2 p->ch = %d, 0x%08x -> 0x%08x, len = %u reqid= %d mode = 0x%08x\n", p->ch, p->dreg->DMASRCADDR, p->dreg->DMADSTADDR, p->dreg->DMALENGTH, p->dreg->DMAREQID, regvalue);
}

//------------------------------------------------------------------------------
/**
 *  @brief	    Transfer peripheral to memory.
 *  @param[in]  srcaddr	A baseaddress of peripheral device.    
 *  @param[in]  SourcePeriID	    An index of peripheral device.         
 *  @param[in]  src_bitwidth		IO size, in bits, of peripheral device.
 *  @param[in]  dstaddr 		Destination buffer address.            
 *  @param[in]  txsize		Transfer size in bytes.                
 *  @return	    None.
 */
void mp2530_dma_transfer_io_to_mem( struct mp2530_dma_channel_t *p,	u32	srcaddr, u32 src_bitwidth, u32 dstaddr, u32 txsize)
{
	u32 regvalue = p->dreg->DMAMODE & POLLUX_DMA_INTMASK; // interrup mask bit 유지해야한다.
	
    MES_ASSERT ( 8 == src_bitwidth || 16 == src_bitwidth || 32 == src_bitwidth );
    MES_ASSERT ( 0 == (((u32)dstaddr) % 4) );
    MES_ASSERT ( 64 > p->reqid );    			  
    MES_ASSERT ( (MAX_TRANSFER_SIZE >= txsize) && (0 != txsize) );
	MES_ASSERT( CNULL != p->dreg );
	MES_ASSERT( ! mp2530_dma_check_running(p) );

	p->dreg->DMASRCADDR = (u32)srcaddr;
	p->dreg->DMADSTADDR = (u32)dstaddr;
	p->dreg->DMALENGTH  = (U16)(txsize-1);
	p->dreg->DMAREQID   = p->reqid; // open 시 setting

	regvalue |= (u32)( 0
					| ((src_bitwidth>>4)<< 0) // SRCIOSIZE 0:8bit 1:16bit 2:32bit
					| (1UL<< 2) // SRCIOMODE 0:memory to memory 1:i/o to memory
					| (0UL<< 3) // PACKMODE must be 0 (not supported)
					| (1UL<< 4) // SRCNOTINC 0:Normal, 1:not incremented
					| (0UL<< 5) // SRCNOTREQCHK 1:No Chk, 0:Chk Request, when SRCIOMODE=1
					| (0UL<< 8) // DSTIOSIZE 0:8bit 1:16bit 2:32bit
					| (0UL<<10) // DSTIOMODE 0:memory to memory 1:memory to i/o
					| (0UL<<12)	// DSTNOTINC 0:Normal, 1:not incremented
					| (1UL<<13)	// DSTNOTREQCHK	1:No Chk, 0:Chk Request, when DSTIOMODE=1
					| (1UL<<19) // RUN
					| (0UL<<20) // STOP, for pollux
					);
	p->dreg->DMAMODE = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief	Stop/Cancel DMA Transfer.
 *  @return	None.
 *	@remark DMA controller of POLLUXF does not support this function.\n
 *			It makes a system hang to call this function under MES_DEBUG build.
 */
void mp2530_dma_stop( struct mp2530_dma_channel_t *p )
{
	MES_ASSERT( CFALSE );
//	MES_ASSERT( CNULL != p->dreg );
//	const u32 INTPEND   = (1UL<<17);
//	const u32 RUN       = (1UL<<19);
//	register u32 regvalue;
//	regvalue = p->dreg->DMAMODE;
//	regvalue &= ~(INTPEND | RUN);
//	p->dreg->DMAMODE = regvalue;

	MES_ASSERT( CNULL != p->dreg );
	u32 INTPEND   = (1UL<<17);
	u32 RUN       = (1UL<<19);
	u32 STOP      = (1UL<<20);  // fixed for pollux
	u32 regvalue;
	regvalue = p->dreg->DMAMODE;
	regvalue &= ~(INTPEND | RUN);
	regvalue |= STOP;
	p->dreg->DMAMODE = regvalue;

}


//------------------------------------------------------------------------------
/**
 *  @brief	Get a maximum size for DMA transfer at once.
 *  @return	a maximum transfer size.
 */
u32 mp2530_dma_get_max_transfer_size( void )
{
	return (u32)MAX_TRANSFER_SIZE;
}

//------------------------------------------------------------------------------
//	Module customized operations
//------------------------------------------------------------------------------
/**
 *  @brief	Run DMA transfer.
 *  @return	None.
 *	@see	Stop, CheckRunning
 */
// ghcstop: pend를 여기서 clear해야하는가? 
void mp2530_dma_run( struct mp2530_dma_channel_t *p )
{
	u32 INTPEND   = (1UL<<17);
	u32 RUN       = (1UL<<19);
	u32 STOP      = (1UL<<20);  // fixed for pollux
	u32 regvalue;
	
	MES_ASSERT( CNULL != p->dreg );

	regvalue = p->dreg->DMAMODE;
	regvalue &= ~(INTPEND|STOP); // fixed for pollux
	regvalue |=  RUN;
	p->dreg->DMAMODE = regvalue;
}

//------------------------------------------------------------------------------
/**
 *  @brief	    Set a source address.
 *  @param[in]  dwAddr  A source address
 *  @return	    None.
 *	@remark	\n
 *			    - If the source is peripheral device, set it to base address of
 * 			      peripheral device. 
 *	    		- If the source is memory and destination is peripheral device, 
 *		    	  the source address must be aligned by IO size of peripheral device.
 * 		    	- If the source and the destination are memory, source address must 
 *			      be aligned by 64 bit(8 bytes).
 *	@see	    GetSourceAddress
 */
void mp2530_dma_set_src_addr(struct mp2530_dma_channel_t *p, u32 saddr)    
{
	MES_ASSERT( CNULL != p->dreg );
	p->dreg->DMASRCADDR = saddr;
}

//------------------------------------------------------------------------------
/**
 *  @brief	Get a source address.
 *  @return	a source address.
 *	@see	SetSourceAddress
 */
u32 mp2530_dma_get_src_addr(struct mp2530_dma_channel_t *p)   
{
	MES_ASSERT( CNULL != p->dreg );
	return p->dreg->DMASRCADDR;
}

//------------------------------------------------------------------------------
/**
 *  @brief	    Set a destination address.
 *  @param[in]  dwAddr   A destination address.
 *  @return	    None.
 *	@remark	\n
 *			    - If the destination is peripheral device, set it to base address of
 *			      peripheral device. 
 *		    	- If the source is peripheral device and destination is memory, 
 *			      the destination address must be aligned by IO size of peripheral 
 *	    		  device.
 *		    	- If the source and the destination are memory, destination address 
 *			      must be aligned by 64 bit(8 bytes).
 *	@see    	GetDestinationAddress
 */
void mp2530_dma_set_dst_addr(struct mp2530_dma_channel_t *p, u32 daddr)  
{
	MES_ASSERT( CNULL != p->dreg );
	p->dreg->DMADSTADDR = daddr;
}

//------------------------------------------------------------------------------
/**
 *  @brief	Get a destination address.
 *  @return	a destination address.
 *	@see	SetDestinationAddress
 */
u32 mp2530_dma_get_dst_addr(struct mp2530_dma_channel_t *p) 
{
	MES_ASSERT( CNULL != p->dreg );
	return p->dreg->DMADSTADDR;
}

//------------------------------------------------------------------------------
/**
 *  @brief	    Set a transfer size in bytes.
 *  @param[in]  dwSize A transfer size in bytes.
 *  @return	    None.
 *	@see	    GetDataSize
 */
void mp2530_dma_set_data_size(struct mp2530_dma_channel_t *p, u32 dwSize )
{
	MES_ASSERT( (MAX_TRANSFER_SIZE >= dwSize) && (0 != dwSize) );
	MES_ASSERT( CNULL != p->dreg );
	p->dreg->DMALENGTH = (U16)(dwSize - 1);
}

//------------------------------------------------------------------------------
/**
 *  @brief	Get a transfer size in bytes.
 *	@remark This function informs a remained data size to be transferred while 
 *			DMA transfer is running.
 *  @return	a transfer size in bytes.
 *	@see	SetDataSize
 */
u32 mp2530_dma_get_data_size(struct mp2530_dma_channel_t *p )
{
	MES_ASSERT( CNULL != p->dreg );
	return (u32)p->dreg->DMALENGTH;
}


//------------------------------------------------------------------------------
/**
 *  @brief	    Set a DMA transfer mode.
 *  @param[in]  opmode		A DMA transfer mode.                                         
 *  @param[in]  bitwidth	IO size, in bits, of peripheral device.\n                    
 *	                      	If opmode is TRANSFER_MEM_TO_MEM, this parameter is not used.
 *	@param[in] 	src_increase	Specifies whether to increase source address or not.\n
 *							This value is only used when source is memory and
 * 							default	is CTRUE.
 *	@param[in] 	dst_increase	Specifies whether to increase destination address or not.\n
 *							This value is only used when destination is memory and
 * 							default	is CTRUE.
 *  @return	    None.
 *	@remark	    You have not to call this function while DMA transfer is busy.
 */
void mp2530_dma_set_attribute(struct mp2530_dma_channel_t *p, enum OPMODE opmode, u32 bitwidth, int src_increase, int dst_increase )
{
	u32 regvalue = p->dreg->DMAMODE & POLLUX_DMA_INTMASK; // interrup mask bit 유지해야한다.
	
	MES_ASSERT ( 3 > opmode );
    MES_ASSERT ( (OPMODE_MEM_TO_MEM == opmode) || 8 == bitwidth || 16 == bitwidth || 32 == bitwidth );
	MES_ASSERT( CNULL != p->dreg );
	MES_ASSERT( !mp2530_dma_check_running(p) );

	switch( opmode )
	{
	case OPMODE_MEM_TO_MEM :
		regvalue |= (u32)( 0
						| (0UL<< 0) // SRCIOSIZE 0:8bit 1:16bit 2:32bit
						| (0UL<< 2) // SRCIOMODE 0:memory to memory 1:i/o to memory
						| (0UL<< 3) // PACKMODE must be 0 (not supported)
						| (0UL<< 4) // SRCNOTINC 0:Normal, 1:not incremented
						| (1UL<< 5) // SRCNOTREQCHK 1:No Chk, 0:Chk Request, when SRCIOMODE=1
						| (0UL<< 8) // DSTIOSIZE 0:8bit 1:16bit 2:32bit
						| (0UL<<10) // DSTIOMODE 0:memory to memory 1:memory to i/o
						| (0UL<<12)	// DSTNOTINC 0:Normal, 1:not incremented
						| (1UL<<13)	// DSTNOTREQCHK	1:No Chk, 0:Chk Request, when DSTIOMODE=1
						| (0UL<<19) // RUN
						);
		if( src_increase == 0 )	regvalue |= (1UL<< 4);	// SRCNOTINC
		if( dst_increase == 0 )	regvalue |= (1UL<<12);	// DSTNOTINC
		break;
	
	case OPMODE_MEM_TO_IO :
		regvalue |= (u32)( 0
						| (0UL<< 0) // SRCIOSIZE 0:8bit 1:16bit 2:32bit
						| (0UL<< 2) // SRCIOMODE 0:memory to memory 1:i/o to memory
						| (0UL<< 3) // PACKMODE must be 0 (not supported)
						| (0UL<< 4) // SRCNOTINC 0:Normal, 1:not incremented
						| (1UL<< 5) // SRCNOTREQCHK 1:No Chk, 0:Chk Request, when SRCIOMODE=1
						| ((bitwidth>>4)<< 8) // DSTIOSIZE 0:8bit 1:16bit 2:32bit
						| (1UL<<10) // DSTIOMODE 0:memory to memory 1:memory to i/o
						| (1UL<<12)	// DSTNOTINC 0:Normal, 1:not incremented
						| (0UL<<13)	// DSTNOTREQCHK	1:No Chk, 0:Chk Request, when DSTIOMODE=1
						| (0UL<<19) // RUN
						);
		if( src_increase == 0 )	regvalue |= (1UL<< 4);	// SRCNOTINC
		break;
		
	case OPMODE_IO_TO_MEM :
		regvalue |= (u32)( 0
						| ((bitwidth>>4)<< 0) // SRCIOSIZE 0:8bit 1:16bit 2:32bit
						| (1UL<< 2) // SRCIOMODE 0:memory to memory 1:i/o to memory
						| (0UL<< 3) // PACKMODE must be 0 (not supported)
						| (1UL<< 4) // SRCNOTINC 0:Normal, 1:not incremented
						| (0UL<< 5) // SRCNOTREQCHK 1:No Chk, 0:Chk Request, when SRCIOMODE=1
						| (0UL<< 8) // DSTIOSIZE 0:8bit 1:16bit 2:32bit
						| (0UL<<10) // DSTIOMODE 0:memory to memory 1:memory to i/o
						| (0UL<<12)	// DSTNOTINC 0:Normal, 1:not incremented
						| (1UL<<13)	// DSTNOTREQCHK	1:No Chk, 0:Chk Request, when DSTIOMODE=1
						| (0UL<<19) // RUN
						);
		if( dst_increase == 0 )	regvalue |= (1UL<<12);	// DSTNOTINC
		break;
	default:
		printk("mode_error\n");
		break;
	}		
	p->dreg->DMAMODE = regvalue;
}

static DEFINE_MUTEX(qlock);    // codec read/write routine re-enterance protect

// audio_buf_t 가 입력으로 들어간다.
int put_dmaq(struct mp2530_dma_channel_t *p, void *abuf)
{
//	dprintk("1\n");
	mutex_lock(&qlock);
//	dprintk("2\n");
	
    if( ((p->qrear + 1) % DMA_QUEUE_SIZE) == p->qfront )
    {
        dprintk("dma %d q overflow\n", p->ch);
        goto error;
    }
	dprintk("p 1 vfront(%d), vvrear(%d), p->ch = %d\n",  p->qfront, p->qrear, p->ch); 

    p->dmaq[p->qrear] = abuf;
    p->qrear = (p->qrear + 1) % DMA_QUEUE_SIZE;
    
    mutex_unlock(&qlock);
    
	dprintk("p 2 vfront(%d), vvrear(%d) %p, %p\n\n",  p->qfront, p->qrear, abuf, p->dmaq[p->qrear-1]); 
    return 1;
    
error: 
    mutex_unlock(&qlock);
    return -1;
}

void *get_dmaq(struct mp2530_dma_channel_t *p)
{
	void *ret;

	mutex_lock(&qlock);
	
	dprintk("g 1 vfront(%d), vvrear(%d), p->ch = %d\n",  p->qfront, p->qrear, p->ch); 
    if (p->qfront == p->qrear)
    {
        dprintk("dma %d read queue underflow\n", p->ch);
        ret = NULL;
        goto error;
    }
	ret = p->dmaq[p->qfront];

    p->qfront = (p->qfront + 1) % DMA_QUEUE_SIZE;
	dprintk("g 2 vfront(%d), vvrear(%d), %p\n\n",  p->qfront, p->qrear, p->dmaq[p->qfront-1]);             

error:
    mutex_unlock(&qlock);
	
    return ret;
    
    
}


void init_dmaq(struct mp2530_dma_channel_t *p)
{
	mutex_lock(&qlock);
	
    p->qfront = 0;
    p->qrear = 0;
    mutex_unlock(&qlock);
    
    dprintk("DMA %02d channel ring Q initialize\n", p->ch);
}



EXPORT_SYMBOL(mp2530_dma_transfer_mem_to_mem);
EXPORT_SYMBOL(mp2530_dma_transfer_mem_to_io);
EXPORT_SYMBOL(mp2530_dma_transfer_io_to_mem);

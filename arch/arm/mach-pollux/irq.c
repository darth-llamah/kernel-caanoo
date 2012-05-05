/* 
 *  linux/arch/arm/mach-pollux/irq.c
 *
 * Copyright. 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/pollux/index.html
 * 
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho)     - main programmer
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
 */ 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/sched.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/mach/irq.h>

#include <asm/arch/dma.h>
#include <asm/arch/regs-interrupt.h>
#include <asm/arch/regs-timer.h>
#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-gpio.h>

#if 0 // ghcstop debug
extern void printascii(const char *); // debug.S

static void
gdebug(const char *fmt, ...)
{
	va_list va;
	char buff[256];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	printascii(buff);
}
#else /* no debug */
	#if 1
		#define gdebug(x...) do {} while(0)
	#else
		#define gdebug(fmt, x... )  printk( "%s: " fmt, __FUNCTION__ , ## x)
	#endif
#endif



/* Main IRQ */
#define pollux_irqext_wake NULL
#define pollux_irq_wake NULL

inline void pollux_irq_ack(unsigned int irq)
{
	unsigned int ofs = 0;
	
	if( irq < MAIN_IRQ_LOW_END ) // low
	{
		ofs = (1 << irq);
		INTPENDL = ofs;
		
    }
	else						// high
	{
		ofs = (1 << ( irq - MAIN_IRQ_LOW_END ));
		INTPENDH = ofs;	 //	interrupt pending flag clear		
	}			
}

inline void pollux_irq_maskack(unsigned int irq)
{
	unsigned int ofs = 0;
	unsigned int pend;
	
	if( irq < MAIN_IRQ_LOW_END ) // low
	{
		ofs = (1 << irq);
		INTMASKL |= ofs;
		INTPENDL = ofs;
		
    }
	else						// high
	{				
		ofs = (1 << ( irq - MAIN_IRQ_LOW_END ));
		INTMASKH |= ofs;
		INTPENDH = ofs;	 //	interrupt pending flag clear		
		pend = INTPENDH;
	}		
}

inline void pollux_irq_mask(unsigned int irq)
{
	unsigned int ofs = 0;
	
	if( irq < MAIN_IRQ_LOW_END ) // low
	{
		ofs = (1 << irq);
		INTMASKL |= ofs;
		
	}
	else						// high
	{
		ofs = (1 << ( irq - MAIN_IRQ_LOW_END ));
		INTMASKH |= ofs;		
	}				
}


inline void pollux_irq_unmask(unsigned int irq)
{
    if( irq < MAIN_IRQ_LOW_END ) // low
	{
		
		INTMASKL &= ~( 1 << irq );		
	}
	else						// high
	{
		INTMASKH &= ~( 1 << ( irq - MAIN_IRQ_LOW_END ) );
	}			
}

	
static void pollux_uart0_irq_demux( unsigned int irq, struct irq_desc *desc )
{
	int i;
    unsigned short event;
    unsigned short mask, tmp;
    
    gdebug("uart0 = %d\n", irq);
    if(irq != IRQ_UART0) 
	{
		gdebug("IRQ is not IRQ_UART0: %d\n", irq);
        return;
    }

    event = __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT );
    gdebug( "pollux_uart0_irq_demux irq = %d, event = 0x%x \n", irq, event );

    mask = (event>>4)&0xf;
    tmp = (mask & (event&0xf) );
    
	if(event & (1<<1))  // rx부터..
    	i = 1;
    else if (event & (1<<0)) 
    	i = 0;
    else if (event & (1<<2)) 
    	i = 2;
	else if (event & (1<<3)) 
    	i = 3;        	
    else
    	return;	

	#if 1
	gdebug( "POLLUX_VA_UART0 UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT ) );		
	gdebug( "irq = %d, event = 0x%x \n", irq, event );
	gdebug( "i   = %d, IRQ_UART0_OFFSET = %d \n\n", i, IRQ_UART0_OFFSET );
	#endif		
	
	desc = irq_desc + IRQ_UART_SUB( IRQ_UART0_OFFSET + i);
    desc_handle_irq(IRQ_UART_SUB( IRQ_UART0_OFFSET + i ), desc );
	
	INTPENDL = 0x0400; // uart0 pending clear
}

static void pollux_uart1_irq_demux( unsigned int irq, struct irq_desc *desc )
{
	int i;
    unsigned short event;
    unsigned short mask, tmp;
    
    if(irq != IRQ_UART1) 
	{
		gdebug("IRQ is not IRQ_UART1: %d\n", irq);
        return;
    }

    event = __raw_readw( POLLUX_VA_UART1 + POLLUX_UINTSTAT );
    
    mask = (event>>4)&0xf;
    tmp = (mask & (event&0xf) );
    
	#if 0
    for(i=0; i< 4; i++) 
    {
    	if(event & (1<<i)) break;
    }
    if(i == 4) return;
	#else		
	if(event & (1<<1))  // rx부터..
    	i = 1;
    else if (event & (1<<0)) 
    	i = 0;
    else if (event & (1<<2)) 
    	i = 2;
	else if (event & (1<<3)) 
    	i = 3;        	
    else
    	return;	
    #endif	

	
	desc = irq_desc + IRQ_UART_SUB( IRQ_UART1_OFFSET + i);
    desc_handle_irq(IRQ_UART_SUB( IRQ_UART1_OFFSET + i ), desc );
	
	
	INTPENDH = 0x0004; // uart1 pending clear
}




static void pollux_uart2_irq_demux( unsigned int irq, struct irq_desc *desc )
{
	int i;
    unsigned short event;
    unsigned short mask, tmp;
    
    if(irq != IRQ_UART2) 
	{
		gdebug("IRQ is not IRQ_UART2: %d\n", irq);
        return;
    }

    event = __raw_readw( POLLUX_VA_UART2 + POLLUX_UINTSTAT );

    mask = (event>>4)&0xf;
    tmp = (mask & (event&0xf) );
    
	if(event & (1<<1))  // rx부터..
    	i = 1;
    else if (event & (1<<0)) 
    	i = 0;
    else if (event & (1<<2)) 
    	i = 2;
	else if (event & (1<<3)) 
    	i = 3;        	
    else
    	return;	
	
	desc = irq_desc + IRQ_UART_SUB( IRQ_UART2_OFFSET + i);
    desc_handle_irq(IRQ_UART_SUB( IRQ_UART2_OFFSET + i ), desc );
	
	
	INTPENDH = 0x0008; // uart2 pending clear
}



static void pollux_uart3_irq_demux( unsigned int irq, struct irq_desc *desc )
{
		int i;
        unsigned short event;
        unsigned short mask, tmp;
        
        if(irq != IRQ_UART3) 
		{
			gdebug("IRQ is not IRQ_UART3: %d\n", irq);
            return;
        }

        //event = *(volatile unsigned short *)io_p2v(0xc0001240);
        event = __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT );
        gdebug( "a pollux_uart3_irq_demux irq = %d, event = 0x%x \n\n", irq, event );

		// 이렇게 해도/안해도 증상은 같다.  결국은 밑의 event register터는 부분서부터 체킹해야한다.        
        mask = (event>>4)&0xf;
        tmp = (mask & (event&0xf) );
        
		#if 0
        for(i=0; i< 4; i++) 
        {
        	if(event & (1<<i)) break;
        }
        if(i == 4) return;
		#else		
		if(event & (1<<1))  // rx부터..
        	i = 1;
        else if (event & (1<<0)) 
        	i = 0;
        else if (event & (1<<2)) 
        	i = 2;
		else if (event & (1<<3)) 
        	i = 3;        	
        else
        	return;	
        #endif	

        /*
         * first, mask parent uart interrupt
         */
        //INTMASK |= (1<<IRQ_UART);             // parent interrupt mask
        //INTPENDH = 0x00000010;        	

#if 1
		gdebug( "pollux_uart3_irq_demux POLLUX_VA_UART3 UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );		
		gdebug( "pollux_uart3_irq_demux irq = %d, event = 0x%x \n", irq, event );
		gdebug( "b pollux_uart3_irq_demux i = %d, IRQ_UART3_OFFSET = %d \n\n", i, IRQ_UART3_OFFSET );
#endif		
		
		desc = irq_desc + IRQ_UART_SUB( IRQ_UART3_OFFSET + i);
        desc_handle_irq(IRQ_UART_SUB( IRQ_UART3_OFFSET + i ), desc );
		
		//gdebug( "IRQ_UART_SUB( IRQ_UART3_OFFSET + i ) %d \n", IRQ_UART_SUB( IRQ_UART3_OFFSET + i ) );
		
		//__raw_writew( event, POLLUX_VA_UART3 + POLLUX_UINTSTAT); 	//  sub interrupt register mask 

//		 INTMASKH &= ~(1<< (IRQ_UART3 - 32) );
				
	/*	event = __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON ); 		// sync pend clear
		event |= POLLUX_LCON_SYNCPENDCLR;
		__raw_writew( event, POLLUX_VA_UART3 + POLLUX_ULCON );*/
		
		INTPENDH = 0x0010; // uart3 pending clear
		//pendh = INTPENDH;
		
#if 0
	//gdebug( "pollux_irq_uart3_ack POLLUX_VA_UART3 + POLLUX_ULCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON ) );
	
	event = __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON );  // sync pend clear
	event |= POLLUX_LCON_SYNCPENDCLR;
	//gdebug( "pollux_irq_uart3_ack syncpend event 0x%x \n", event );
	__raw_writew( event, POLLUX_VA_UART3 + POLLUX_ULCON );
	
	//gdebug( "pollux_irq_uart3_ack POLLUX_VA_UART3 + POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
	//gdebug( "pollux_irq_uart3_ack POLLUX_VA_UART3 + POLLUX_ULCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON ) );
#endif	
		
		
#if 0
gdebug( "pollux_uart3_irq_demux INTMASKH INTPENDH = 0x%04x 0x%04x\n", INTMASKH ,INTPENDH );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_ULCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UCON ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UFCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UFCON ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UMCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UMCON ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UTRSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UTRSTAT ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UERSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UERSTAT ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UFSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UFSTAT ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UMSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UMSTAT ) );
//Gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UTXH 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UTXH ) );
//gdebug( "pollux_uart3_irq_demux uart3 POLLUX_URXH 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_URXH ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UBRDIV 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UBRDIV ) );
gdebug( "pollux_uart3_irq_demux uart3 POLLUX_UTOUT 0x%x \n\n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UTOUT ) );
//#else
//gdebug( "pollux_uart3_irq_demux uart3POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
//gdebug( "pollux_uart3_irq_demux uart3 POLLUX_URXH 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_URXH ) );
#endif

}

struct irq_chip pollux_irq_level_chip = 
{
	.name      = "pollux-level",
	.ack	   = pollux_irq_maskack,
	.mask	   = pollux_irq_mask,
	.unmask	   = pollux_irq_unmask,
	//.set_wake  = pollux_irq_wake
};


static struct irq_chip pollux_irq_chip = 
{
	.name      = "pollux",
	.ack	   = pollux_irq_ack,
	.mask	   = pollux_irq_mask,
	.unmask	   = pollux_irq_unmask,
	//.set_wake  = pollux_irq_wake
};

/* UART0 */
static void
pollux_irq_uart0_mask(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT );
	mask &= ~(1 << (irqno - ( MAIN_IRQ_END + IRQ_UART0_OFFSET) + 4 ) ); 
	__raw_writew( mask, POLLUX_VA_UART0 + POLLUX_UINTSTAT);
}

static void
pollux_irq_uart0_unmask(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART0_OFFSET) + 4  ) );
	__raw_writew( mask, POLLUX_VA_UART0 + POLLUX_UINTSTAT);
	
	gdebug( "pollux_irq_uart0_unmask POLLUX_VA_UART0 + POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT ) );	
}

static void
pollux_irq_uart0_ack(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART0_OFFSET) ) );  // 으악..
	__raw_writew( mask, POLLUX_VA_UART0 + POLLUX_UINTSTAT); 
	
	mask = __raw_readw( POLLUX_VA_UART0 + POLLUX_ULCON );  // sync pend clear
	mask |= POLLUX_LCON_SYNCPENDCLR;
	__raw_writew( mask, POLLUX_VA_UART0 + POLLUX_ULCON );
}

static struct irq_chip pollux_irq_uart0 = 
{
		.name       = "pollux-uart0",
        .mask       = pollux_irq_uart0_mask,
        .unmask     = pollux_irq_uart0_unmask,
        .ack        = pollux_irq_uart0_ack,
};

/* UART1 */
static void
pollux_irq_uart1_mask(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART1 + POLLUX_UINTSTAT );
	mask &= ~(1 << (irqno - ( MAIN_IRQ_END + IRQ_UART1_OFFSET) + 4 ) ); 
	__raw_writew( mask, POLLUX_VA_UART1 + POLLUX_UINTSTAT);
}

static void
pollux_irq_uart1_unmask(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART1 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART1_OFFSET) + 4  ) );
	__raw_writew( mask, POLLUX_VA_UART1 + POLLUX_UINTSTAT);
	
	gdebug( "pollux_irq_uart1_unmask POLLUX_VA_UART1 + POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART1 + POLLUX_UINTSTAT ) );	
}

static void
pollux_irq_uart1_ack(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART1 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART1_OFFSET) ) );  // 으악..
	__raw_writew( mask, POLLUX_VA_UART1 + POLLUX_UINTSTAT); 
	
	mask = __raw_readw( POLLUX_VA_UART1 + POLLUX_ULCON );  // sync pend clear
	mask |= POLLUX_LCON_SYNCPENDCLR;
	__raw_writew( mask, POLLUX_VA_UART1 + POLLUX_ULCON );
}

static struct irq_chip pollux_irq_uart1 = 
{
		.name       = "pollux-uart1",
        .mask       = pollux_irq_uart1_mask,
        .unmask     = pollux_irq_uart1_unmask,
        .ack        = pollux_irq_uart1_ack,
};




/* UART2 */
static void
pollux_irq_uart2_mask(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART2 + POLLUX_UINTSTAT );
	
	mask &= ~(1 << (irqno - ( MAIN_IRQ_END + IRQ_UART2_OFFSET) + 4 ) ); 
	__raw_writew( mask, POLLUX_VA_UART2 + POLLUX_UINTSTAT); 
}

static void
pollux_irq_uart2_unmask(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART2 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART2_OFFSET) + 4  ) );
	__raw_writew( mask, POLLUX_VA_UART2 + POLLUX_UINTSTAT); 
	
	gdebug( "\n\npollux_irq_uart2_unmask POLLUX_VA_UART2 + POLLUX_UINTSTAT 0x%x \n\n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UINTSTAT ) );
}

static void
pollux_irq_uart2_ack(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART2 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART2_OFFSET) ) );
	__raw_writew( mask, POLLUX_VA_UART2 + POLLUX_UINTSTAT); 
	
	mask = __raw_readw( POLLUX_VA_UART2 + POLLUX_ULCON );  
	mask |= POLLUX_LCON_SYNCPENDCLR;
	__raw_writew( mask, POLLUX_VA_UART2 + POLLUX_ULCON );

}

static struct irq_chip pollux_irq_uart2 = 
{
		.name       = "pollux-uart2",
        .mask       = pollux_irq_uart2_mask,
        .unmask     = pollux_irq_uart2_unmask,
        .ack        = pollux_irq_uart2_ack,
};




/* UART3 */
static void
pollux_irq_uart3_mask(unsigned int irqno)
{
	//gdebug( " irqno %d\n", irqno );
	unsigned short mask = __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT );
	
	mask &= ~(1 << (irqno - ( MAIN_IRQ_END + IRQ_UART3_OFFSET) + 4 ) ); // 55 - ( 42+12) + 4 
//	mask |= (0 << (irqno - ( MAIN_IRQ_END + IRQ_UART3_OFFSET) + 4 ) ); // 55 - ( 42+12) + 4 
	
	__raw_writew( mask, POLLUX_VA_UART3 + POLLUX_UINTSTAT); //  interrupt disable

	//gdebug( "pollux_irq_uart3_mask POLLUX_VA_UART3 + POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
}

static void
pollux_irq_uart3_unmask(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART3_OFFSET) + 4  ) );
	__raw_writew( mask, POLLUX_VA_UART3 + POLLUX_UINTSTAT); //  interrupt enable
	
	//gdebug( "pollux_irq_uart3_unmask POLLUX_VA_UART3 + POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
	gdebug( "\n\npollux_irq_uart3_unmask POLLUX_VA_UART3 + POLLUX_UINTSTAT 0x%x \n\n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
}

static void
pollux_irq_uart3_ack(unsigned int irqno)
{
	unsigned short mask = __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT );
	mask |= ( 1 << (irqno - ( MAIN_IRQ_END + IRQ_UART3_OFFSET) ) );  // 으악..
	__raw_writew( mask, POLLUX_VA_UART3 + POLLUX_UINTSTAT); //  pending clear


	//gdebug( "pollux_irq_uart3_ack POLLUX_VA_UART3 + POLLUX_ULCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON ) );
	
	mask = __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON );  // sync pend clear
	mask |= POLLUX_LCON_SYNCPENDCLR;
	//gdebug( "pollux_irq_uart3_ack syncpend mask 0x%x \n", mask );
	__raw_writew( mask, POLLUX_VA_UART3 + POLLUX_ULCON );

	
	//gdebug( "pollux_irq_uart3_ack POLLUX_VA_UART3 + POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
	//gdebug( "pollux_irq_uart3_ack POLLUX_VA_UART3 + POLLUX_ULCON 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON ) );

}

static struct irq_chip pollux_irq_uart3 = 
{
		.name       = "pollux-uart3",
        .mask       = pollux_irq_uart3_mask,
        .unmask     = pollux_irq_uart3_unmask,
        .ack        = pollux_irq_uart3_ack,
};





/* GPIO IRQ */
/*
 * Demux handler for GPIOA[31:0] ~ GPIOC[20:0] interrupts
 */
#define GPIO_MASK(i)	    (*(volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_INTENB    + 0x40*(i)))
#define GPIO_PEND(i)	    (*(volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_INTPEND   + 0x40*(i)))
#define GPIO_INOUT_MODE(i)  (*(volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_OUTENB    + 0x40*(i)))
#define GPIO_EVTMODE_LOW(i) (*(volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_DETMODE0  + 0x40*(i)))
#define GPIO_EVTMODE_HI(i)  (*(volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_DETMODE1  + 0x40*(i)))
#define GPIO_FUNC_LOW(i)    (*(volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_FUNC0     + 0x40*(i)))
#define GPIO_FUNC_HI(i)     (*(volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_FUNC1     + 0x40*(i)))

// 번지 debug용
#define PGPIO_INOUT_MODE(i)  ((volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_OUTENB    + 0x40*(i)))
#define PGPIO_EVTMODE_LOW(i) ((volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_DETMODE0  + 0x40*(i)))
#define PGPIO_EVTMODE_HI(i)  ((volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_DETMODE1  + 0x40*(i)))
#define PGPIO_FUNC_LOW(i)    ((volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_FUNC0     + 0x40*(i)))
#define PGPIO_FUNC_HI(i)     ((volatile unsigned int *)(POLLUX_VA_GPIO + POLLUX_GPIO_FUNC1     + 0x40*(i)))


static int
pollux_irq_gpio_set_type(unsigned int irq, unsigned int type)
{
	unsigned int   gpio_nr  = IRQ_TO_GPIO(irq);	   // 0 ~ (32*3)-1
	unsigned int   index    = gpio_nr >> 5;		   // 32 단위로 만든다.
	unsigned int   residual = (gpio_nr%32);        // 내부 gpio A의 몇번인지 찾아낸다.
	unsigned int   typeval  = 0;
	

	if( gpio_nr >= 96 ) // 32*3
	{
		gdebug("pollux_irq_gpio_set_type: input irq number error gpio_nr: %d\n", gpio_nr);
		return -1;
	}

	
	/* Set the external interrupt to pointed trigger type */
	switch (type)
	{
		case IRQT_NOEDGE:
			printk(KERN_WARNING "pollux_irq_gpio_set_type: No edge setting!\n");
			break;

		case IRQT_RISING:
			typeval = POLLUX_EXTINT_RISEEDGE;
			break;

		case IRQT_FALLING:
			typeval = POLLUX_EXTINT_FALLEDGE;
			break;

		case IRQT_LOW:
			typeval = POLLUX_EXTINT_LOWLEVEL;
			break;

		case IRQT_HIGH:
			typeval = POLLUX_EXTINT_HILEVEL;
			break;

		default:
			printk(KERN_ERR "pollux_irq_gpio_set_type: No such irq type %d", type);
			return -1;
	}
	
	gdebug("pollux_irq_gpio_set_type: irq %d, gpio_nr %d, residual %d, typeval %d\n", irq, gpio_nr, residual, typeval);
	
	
	// gpio mode로 세팅 후, input으로 바꾸고, event type을 설정한다.
	if( residual < 16 ) // low register setting
	{
		GPIO_FUNC_LOW(index)    &= ~( 3<<(residual*2) );       // 해당 위치의 2bit clear == gpio mode로 세팅
		GPIO_INOUT_MODE(index)  &= ~( 1<<residual );           // 해당 위치의 1bit clear == gpio input mode로 세팅
		
		GPIO_EVTMODE_LOW(index) &= ~( 3<<(residual*2) );       // 해당 위치의 2bit clear == detection mode clear
	    GPIO_EVTMODE_LOW(index) |=  ( typeval<<(residual*2) ); // 해당 위치에 event detect mode setting
	    
	    gdebug("func  low : %p, 0x%08x\n", PGPIO_FUNC_LOW(index), GPIO_FUNC_LOW(index) );
	    gdebug("inout mode: %p, 0x%08x\n", PGPIO_INOUT_MODE(index), GPIO_INOUT_MODE(index) );
	    gdebug("evt   low : %p, 0x%08x\n", PGPIO_EVTMODE_LOW(index), GPIO_EVTMODE_LOW(index) );
	}
	else
	{
	    residual -= 16;
		
		GPIO_FUNC_HI(index)    &= ~( 3<<(residual*2) );       // 해당 위치의 2bit clear == gpio mode로 세팅
		GPIO_INOUT_MODE(index)  &= ~( 1<<residual );           // 해당 위치의 1bit clear == gpio input mode로 세팅
		
		GPIO_EVTMODE_HI(index) &= ~( 3<<(residual*2) );       // 해당 위치의 2bit clear == detection mode clear
	    GPIO_EVTMODE_HI(index) |=  ( typeval<<(residual*2) ); // 해당 위치에 event detect mode setting
	    
	    gdebug("func  hi  : %p, 0x%08x\n", PGPIO_FUNC_HI(index), GPIO_FUNC_HI(index) );
	    gdebug("inout mode: %p, 0x%08x\n", PGPIO_INOUT_MODE(index), GPIO_INOUT_MODE(index) );
	    gdebug("evt   hi  : %p, 0x%08x\n", PGPIO_EVTMODE_HI(index), GPIO_EVTMODE_HI(index) );
	    
	}
	
	return 0;
}

// gpioc[26] key_chg로 사용, sw3이다. low active이고
static void pollux_gpio_irq_demux( unsigned int irq, struct irq_desc *desc )
{
	int i, j;
	unsigned int gpio_irq;
	//volatile unsigned int gpio_event;
	//unsigned int gpio_event, gpio_mask, tmp;
	unsigned int tmp;
	
    //unsigned int   pendl;
        
	//gdebug("gpio 1\n");	        
    if(irq != IRQ_GPIO) 
	{
        gdebug("IRQ is not IRQ_GPIO: %d\n", irq);
        return;
    }

	//gdebug("gpio 2\n");	

	for(i=0; i<3; i++)
	{	
		#if 0
		gpio_event = GPIO_PEND(i);  // Read GPIO sub Event
		gpio_mask  = GPIO_MASK(i);  // Read GPIO sub mask register
		tmp = gpio_event & gpio_mask; // mask가 안되어 있는 것들 중 뜬것만 찾아야 한다.
		gdebug("pend: 0x%08x, mask = 0x%08x\n", gpio_event, gpio_mask);
		#else
		tmp = GPIO_PEND(i) & GPIO_MASK(i); // mask가 안되어 있는 것들 중 뜬것만 찾아야 한다.
		#endif
		if(tmp == 0)         // if there is no event,
		{	
			continue;
		}

		for(j=0; j<32; j++) // GPIOx[31:0]
		{	
			if(tmp & (1<<j)) 
			{
				gpio_irq = GPIO_SUB_IRQ_BASE + (i << 5) + j;

				//gdebug("i = %d, j = %d, irq = 0x%02x, %d\n", i, j, gpio_irq, gpio_irq);
				
				desc = irq_desc + gpio_irq;
		        desc_handle_irq(gpio_irq, desc );
				
			}
		}
	}
	
	INTPENDL = 0x00002000; // GPIO pending clear
	//pendl = INTPENDL;

}




static void pollux_irq_gpio_ack(unsigned int irq)
{
	unsigned int   gpio_nr = IRQ_TO_GPIO(irq);	   // 0 ~ (32*3)-1
	unsigned int   mask = 1 << (gpio_nr & 0x1f);   // 0x1f로 mask
	unsigned int   index = gpio_nr >> 5;		   // 32 단위로 만든다.

	//GPIO_MASK(index) &= ~mask;				// mask
	GPIO_PEND(index) = mask;					// pending irq
    //gdebug( "gpio ack index: %d, mask = 0x%08x\n\n", index, mask);	
	
}

static void pollux_irq_gpio_mask(unsigned int irq)
{
	unsigned int   gpio_nr = IRQ_TO_GPIO(irq);	
	unsigned int   mask = 1 << (gpio_nr & 0x1f);
	unsigned int   index = gpio_nr >> 5;		

	GPIO_MASK(index) &= ~mask;				
	//gdebug( "gpio mask index: %d, mask = 0x%08x\n\n", index, mask);	
}

static void pollux_irq_gpio_unmask(unsigned int irq)
{
	unsigned int   gpio_nr = IRQ_TO_GPIO(irq);	
	unsigned int   mask = 1 << (gpio_nr & 0x1f);
	unsigned int   index = gpio_nr >> 5;		

	GPIO_MASK(index) |= mask;					
	//gdebug( "gpio unmask index: %d, mask = 0x%08x\n\n", index, mask);	
}

static struct irq_chip pollux_irq_gpio = 
{
		.name       = "pollux-gpio",
        .mask       = pollux_irq_gpio_mask,
        .unmask     = pollux_irq_gpio_unmask,
        .ack        = pollux_irq_gpio_ack,
		.set_type	= pollux_irq_gpio_set_type,        
};


/* DMA IRQ */
/*
 * Demux handler for DMA interrupts
 */
#define DMA_MODE(i)	    (*(volatile unsigned int *)(POLLUX_DMA_BASE(i) + POLLUX_DMA_MODE))
// debug용
#define PDMA_MODE(i)	((volatile unsigned int *)(POLLUX_DMA_BASE(i) + POLLUX_DMA_MODE))


static void pollux_dma_irq_demux( unsigned int irq, struct irq_desc *desc )
{
	int i;
	unsigned int dma_irq;
	unsigned int tmp, pend, mask;
	
    if(irq != IRQ_DMA) 
	{
        gdebug("IRQ is not IRQ_DMA: %d\n", irq);
        return;
    }

	gdebug("dma irq demux 1\n");
	for(i=0; i<16; i++)
	{	
		#if 1
		pend  = (DMA_MODE(i)&POLLUX_DMA_INTPEND);  // Read dma pend bit
		mask  = ((DMA_MODE(i)&POLLUX_DMA_INTMASK)>>1);  // Read dma enb  bit >> 1 => pend와 mask시켜보기 위해서
		#else
		pend  = (DMA_MODE(i)&POLLUX_DMA_INTPEND);  // Read dma pend bit
		mask  = ((DMA_MODE(i)&POLLUX_DMA_INTMASK));  // Read dma enb  bit >> 1 => pend와 mask시켜보기 위해서
		gdebug("%d, DMA_MODE(i) = 0x%08x, pend = 0x%08x, mask = 0x%08x\n", i, DMA_MODE(i), pend, mask);
		mask >>= 1;
		#endif
		tmp = pend & mask; // mask가 안되어 있는 것들 중 뜬것만 찾아야 한다.
		if(tmp == 0)         // if there is no event,
		{	
			continue;
		}

		dma_irq = DMA_SUB_IRQ_BASE + i;

		gdebug("run dma isr irq = %d, 0x%08x\n", dma_irq, DMA_MODE(i));
				
		desc = irq_desc + dma_irq;
        desc_handle_irq(dma_irq, desc );
	}
	
	INTPENDL = 0x00000008; // DMA pending clear
}




static void pollux_irq_dma_ack(unsigned int irq)
{
	unsigned int   dma_nr = IRQ_TO_DMA(irq);	
	unsigned int   mask = DMA_MODE(dma_nr);
	
	gdebug("dma ack dmamode = 0x%08x(%d)\n", DMA_MODE(dma_nr), dma_nr);		
	
	mask |= (POLLUX_DMA_INTPEND); // pend clear
	DMA_MODE(dma_nr) = mask;				
    gdebug("dma ack dmamode = 0x%08x\n\n", DMA_MODE(dma_nr));	
	
}

static void pollux_irq_dma_mask(unsigned int irq)
{
	unsigned int   dma_nr = IRQ_TO_DMA(irq);	
	unsigned int   mask = DMA_MODE(dma_nr);
	
	mask &= ~(POLLUX_DMA_INTMASK);
	DMA_MODE(dma_nr) = mask;				
	gdebug("dma msk dmamode = 0x%08x\n\n", DMA_MODE(dma_nr));	
}

static void pollux_irq_dma_unmask(unsigned int irq)
{
	unsigned int   dma_nr = IRQ_TO_DMA(irq);	
	unsigned int   mask = DMA_MODE(dma_nr);
	
	mask |= (POLLUX_DMA_INTMASK);
	DMA_MODE(dma_nr) = mask;				
	gdebug("dma umk dmamode = 0x%08x\n\n", DMA_MODE(dma_nr));	
}

static struct irq_chip pollux_irq_dma = 
{
		.name       = "pollux-dma",
        .mask       = pollux_irq_dma_mask,
        .unmask     = pollux_irq_dma_unmask,
        .ack        = pollux_irq_dma_ack,
};




// 070323: printk 살리면서 uart3의 interrupt를 기본적으로 disable 시켰다. 왜냐하면 이녀석을 살려 놓으면 
// console이 열리기 전에도(request_irq를 하기 전에도) interrupt가 먹어버리기 때문이다.
// serial source의 startup()가 먹기 전에는 interrupt가 enable되어서는 안된다.....startup은 serial을 open할때
// 호출된다.
void __init pollux_init_irq(void)
{
	int 		   irqno    = 0;
	/* 
	 * first, clear all interrupts pending... 
	 */
	gdebug("pollux_init_irq: clearing interrupt status flags\n");

    
	//mask = __raw_readw( POLLUX_VA_UART3 + POLLUX_UCON );   
	
	// printk 살리면서 이녀석도 clear
	//__raw_writew( 0x305, POLLUX_VA_UART3 + POLLUX_UCON); // 2410 patform device 형식. level interrupt, debugging시...
	gdebug( "POLLUX_VA_UART3 + POLLUX_UCON = 0x%04x\n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UCON ) );		
	gdebug( "INTMASKL INTMASKH = 0x%04x \t 0x%04x\n", INTMASKL, INTMASKH );
	gdebug( "INTMODEL INTMODEH = 0x%04x \t 0x%04x\n", INTMODEL, INTMODEH );	
	gdebug( "INTPENDL INTPENDH = 0x%04x \t 0x%04x\n", INTPENDL, INTPENDH );
	gdebug( "TMRCONTROL0  = 0x%04x \n", TMRCONTROL0 );		

	
	/* disable all interrupts */
	INTMASKL	=	0xffffffff;
	INTMASKH	=	0xFFFFFFFF;

	/* all interrupts is irq mode */
	INTMODEL = 0x00000000;
	INTMODEH = 0x00000000;
	
	/* disable rotation */
	PRIORDER = 0x0;		

	//__raw_writew( 0x001F, POLLUX_VA_UART0 + POLLUX_UTOUT);

	gdebug( "POLLUX_VA_UART3 + POLLUX_UINTSTAT  = 0x%04x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );		
	__raw_writew( 0x0F, POLLUX_VA_UART0 + POLLUX_UINTSTAT); // 2410 patform device 형식.
	//__raw_writew( 0xFF, POLLUX_VA_UART0 + POLLUX_UINTSTAT); //  interrupt enable, debugging시....만 살림.
	
	__raw_writew( 0x0F, POLLUX_VA_UART1 + POLLUX_UINTSTAT);
	
	//__raw_writew( 0xFF, POLLUX_VA_UART2 + POLLUX_UINTSTAT); //  interrupt enable, debugging시....만 살림.
	__raw_writew( 0x0F, POLLUX_VA_UART2 + POLLUX_UINTSTAT);
		
	__raw_writew( 0x0F, POLLUX_VA_UART3 + POLLUX_UINTSTAT); //  interrupt disable, 070323 printk 살리면서 interrupt disable
	//__raw_writew( 0xFF, POLLUX_VA_UART3 + POLLUX_UINTSTAT); //  interrupt enable, debugging시....만 살림.
	
	
	gdebug( "POLLUX_VA_UART3 + POLLUX_UINTSTAT  = 0x%04x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );		
	
	
	for(irqno=0; irqno<3; irqno++)	// gpio A ~ C, clear
	{
		GPIO_MASK(irqno) = 0;		    /* disable all GPIO interrupts */
		GPIO_PEND(irqno) = 0xffffffff;	/* clear all GPIO pending registers */
	}
	
	// DMA pend clear & all dma irq disable
	for(irqno=0; irqno<16; irqno++)
	{
		__raw_writel( POLLUX_DMA_INTPEND, (POLLUX_DMA_BASE(irqno) + POLLUX_DMA_MODE) );  // all pend clear, all irq disable
	}


	INTPENDL = 0xffffffff;
	INTPENDH = 0xFFFFFFFF; // all pend clear
	
	/* 
	 * Register the main interrupts 
	 */
	
    for (irqno = MAIN_IRQ_BASE; irqno < MAIN_IRQ_END; irqno++) 
	{
		//gdebug("registering irq %d (pollux irq)\n", irqno);
		/* set all the pollux internal irqs */
		switch (irqno) 
		{
			/* deal with the special IRQs (cascaded) */
			case IRQ_UART0:
			case IRQ_UART1:
			case IRQ_UART2:
			case IRQ_UART3:
			case IRQ_GPIO:
			case IRQ_DMA:
				set_irq_chip(irqno, &pollux_irq_level_chip);
				set_irq_handler(irqno, handle_level_irq );  // 2.6.20 chagned
				set_irq_flags(irqno, IRQF_VALID);
				break;
			case IRQ_TIMER0: 
			case IRQ_SDMMC0:
			case IRQ_SDMMC1:
			case IRQ_PDISPLAY:
			case IRQ_UHC:   
				set_irq_chip(irqno, &pollux_irq_level_chip);
				set_irq_handler(irqno, handle_level_irq );  
				set_irq_flags(irqno, IRQF_VALID);
				break;
			default:	
				set_irq_chip(irqno, &pollux_irq_chip);
				set_irq_handler(irqno, handle_edge_irq ); 
				set_irq_flags(irqno, IRQF_VALID);
		}
	}

	/* setup the cascade irq handlers */
	set_irq_chained_handler( IRQ_UART0, pollux_uart0_irq_demux);
	set_irq_chained_handler( IRQ_UART1, pollux_uart1_irq_demux);
	set_irq_chained_handler( IRQ_UART2, pollux_uart2_irq_demux);
	set_irq_chained_handler( IRQ_UART3, pollux_uart3_irq_demux);
	set_irq_chained_handler( IRQ_GPIO,  pollux_gpio_irq_demux);
	set_irq_chained_handler( IRQ_DMA,   pollux_dma_irq_demux);
	
	/* register the uart interrupts */
	// uart 0
	for ( irqno = ( UART_SUB_IRQ_BASE + IRQ_UART0_OFFSET ); irqno < ( UART_SUB_IRQ_BASE + IRQ_UART1_OFFSET ); irqno++) 
	{
		set_irq_chip(irqno, &pollux_irq_uart0);
		set_irq_handler(irqno, handle_level_irq );
		set_irq_flags(irqno, IRQF_VALID);
	}

	// uart 1
	for ( irqno = ( UART_SUB_IRQ_BASE + IRQ_UART1_OFFSET ); irqno < ( UART_SUB_IRQ_BASE + IRQ_UART2_OFFSET ); irqno++) 
	{
		set_irq_chip(irqno, &pollux_irq_uart1);
		set_irq_handler(irqno, handle_level_irq );
		set_irq_flags(irqno, IRQF_VALID);
	}

	// uart 2
	for ( irqno = ( UART_SUB_IRQ_BASE + IRQ_UART2_OFFSET ); irqno < ( UART_SUB_IRQ_BASE + IRQ_UART3_OFFSET ); irqno++) 
	{
		set_irq_chip(irqno, &pollux_irq_uart2);
		set_irq_handler(irqno, handle_level_irq );
		set_irq_flags(irqno, IRQF_VALID);
	}

	// uart 3
	for ( irqno = ( UART_SUB_IRQ_BASE + IRQ_UART3_OFFSET ); irqno < ( UART_SUB_IRQ_END ); irqno++) 
	{
		//gdebug("registering irq %d (pollux uart3 irq)\n", irqno);
		set_irq_chip(irqno, &pollux_irq_uart3);
		set_irq_handler(irqno, handle_level_irq );
		set_irq_flags(irqno, IRQF_VALID);
	}


	/* register the gpio interrupts */
	for (irqno = GPIO_SUB_IRQ_BASE; irqno < GPIO_SUB_IRQ_END; irqno++) 
	{
		//gdebug("registering irq %d (pollux gpio irq)\n", irqno);
		set_irq_chip(irqno, &pollux_irq_gpio );
		set_irq_handler(irqno, handle_level_irq);
		set_irq_flags(irqno, IRQF_VALID);
	}

	/* register the gpio interrupts */
	for (irqno = DMA_SUB_IRQ_BASE; irqno < DMA_SUB_IRQ_END; irqno++) 
	{
		//gdebug("registering irq %d (pollux dma irq)\n", irqno);
		set_irq_chip(irqno, &pollux_irq_dma );
		set_irq_handler(irqno, handle_level_irq);
		set_irq_flags(irqno, IRQF_VALID);
	}
}


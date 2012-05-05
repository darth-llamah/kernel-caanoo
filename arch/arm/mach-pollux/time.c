/* 
 * linux/arch/arm/mach-pollux/time.c
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
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h> 		 		// for setup_irq()
#include <linux/err.h>
#include <linux/clk.h>

#include <asm/system.h>
#include <asm/leds.h>
#include <asm/mach-types.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/map.h>

#include <asm/arch/regs-timer.h>
#include <asm/mach/time.h> 

#include <asm/arch/regs-clock-power.h>
#include <asm/arch/regs-interrupt.h>
#include <asm/arch/regs-serial.h>


#include "pollux.h"

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
#else
	#define gdebug(x...) do {} while(0)
#endif


#if 0
static inline unsigned long pollux_get_rtc_time(void)
{
    return RTCTCNT;
}

static int pollux_set_rtc(void)
{
    unsigned long current_time = xtime.tv_sec;

    if (RTCCTRL & RTC_ALE) {
        /* make sure not to forward the clock over an alarm */
        unsigned long alarm = ALARMT;
        if (current_time >= alarm && alarm >= RTCTCNT)
            return -ERESTARTSYS;
    }
    /* plus one because RTCTSET value is set to RTCTCNT after 1 sec */
    RTCTSET = current_time + 1;
    return 0;
}

#endif


/*
 * Returns microsecond  since last clock interrupt.  Note that interrupts
 * will have been disabled by do_gettimeoffset()
 * IRQs are disabled before entering here from do_gettimeofday()
 */
static unsigned long pollux_gettimeoffset (void)
{
#if 0 // old code	
	unsigned long ticks_to_match, elapsed, usec;
	unsigned long tick = 10000;

	/* Get ticks before next timer match */
	//ticks_to_match = TMATCH0 - TCOUNT0; 
	// ghcstop_caution: LDCNT = 1로 바꾼 후 count 값을 읽어야 한다.
	ticks_to_match = TMRMATCH0 - TMRCOUNT0; 

	/* We need elapsed ticks since last match */
	elapsed = LATCH - ticks_to_match;

	/* Now convert them to usec */
	usec = (unsigned long)(elapsed*tick)/LATCH;
#else // ldcnt applied code
	unsigned long ticks_to_match, elapsed, usec;
	unsigned long tick = 10000;
	u32 regvalue		 = 0;

	/* Get ticks before next timer match */
	// ghcstop_caution: LDCNT = 1로 바꾼 후 count 값을 읽어야 한다.
	regvalue = TMRCONTROL0;
	regvalue |= TMRCONTROL_READ_COUNTER;
	TMRCONTROL0 = regvalue;
	ticks_to_match = TMRMATCH0 - TMRCOUNT0; 

	/* We need elapsed ticks since last match */
	elapsed = LATCH - ticks_to_match;

	/* Now convert them to usec */
	usec = (unsigned long)(elapsed*tick)/LATCH;

	
#endif

	return usec;
}

#if 0 // timer interrupt debugging

/*
 * IRQ handler for the timer
 */ 
static unsigned long time = 0; 		// 커널 코드일때는 static으로 잡는게 좋겠지..메모리구성상.
static unsigned long tsec  = 0;
// 2.6.20 커널 수정 사항 인터럽트핸들러 인자 바뀜..
//static irqreturn_t pollux_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs) 
static irqreturn_t pollux_timer_interrupt( int irq, void *dev_id )
{	
	u32    regvalue = 0;	

//	gdebug("timer_interrupt, TMRCONTROL0 0x%08x, time %d \n", TMRCONTROL0, time ); // debugging 
	if(time == 100)
	{			
		//gdebug( " %d second\n", tsec ); 	
		//printk( " %d second\n", tsec ); 	
		//printk("INTMASK = 0x%08x\n", INTMASK);
		//printk("SRCPEND = 0x%08x\n", SRCPEND);
		//printk("INTPEND = 0x%08x\n", INTPEND);
		time=0;
		tsec++;
	}
	time++;

	write_seqlock(&xtime_lock);
	
	timer_tick();

	regvalue		=	TMRCONTROL0;
	regvalue 		&= ~TMRCONTROL_INTPEND;
 	regvalue 		|=  ( TMRCONTROL_INTPEND );
 	TMRCONTROL0		=  regvalue; 		

	write_sequnlock(&xtime_lock);
	
	return IRQ_HANDLED;
}
#else
/*
 * IRQ handler for the timer
 */ 
static irqreturn_t pollux_timer_interrupt( int irq, void *dev_id )
{	
	u32    regvalue = 0;	

	write_seqlock(&xtime_lock);
	
	timer_tick();

	regvalue		=	TMRCONTROL0;
	regvalue 		&= ~TMRCONTROL_INTPEND;
 	regvalue 		|=  ( TMRCONTROL_INTPEND );
 	TMRCONTROL0		=  regvalue; 		

	write_sequnlock(&xtime_lock);
	
	return IRQ_HANDLED;
}
#endif



/*
 * Set up timer interrupt, and return the current time in seconds.
 *
 * Currently we only use timer0, as it is 
 */
#if 0 // 384Mhz 10ms timer.....pll0 
static void pollux_timer_setup (void)
{
	u32 regvalue		 = 0;
	
	
	regvalue	=	TMRCLKGEN0;
	regvalue	&= ~( TMRCLKGEN_CLKSRCSEL << 1 );   
 	regvalue  |=  ( TMRCLKGEN_CLKSRC_PLL0    );	 				// clock source PLL0
 	TMRCLKGEN0	=  regvalue; 	
 	
 	regvalue	=	TMRCLKGEN0;
 	regvalue 	&= ~( TMRCLKGEN_CLKDIV         );               // clock division
	regvalue	|=  ( TMRCLKGEN_CLKDIV0 - 1 ) << ( TMRCLKGEN_CLKDIV_SHIFT );
	TMRCLKGEN0	=  regvalue;


	regvalue	=	TMRCLKENB0;
	regvalue 	&=  ~( TMRCLKENB_TCLKMODE);
//TMRCLKENB0 	|= (clkmode & 0x1) << 3; // clkmode 1
	regvalue 	|=   ( TMRCLKENB_TCLKMODE ); 					// set clock pclk mode
	TMRCLKENB0	=	 regvalue;
	
	regvalue	=	TMRCLKENB0;
	regvalue	 &= ~( TMRCLKENB_CLKGENENB ); 					// set clock divisor enable
	regvalue	 |=  ( TMRCLKENB_CLKGENENB );
	TMRCLKENB0	 =	 regvalue;
	
	regvalue	=	TMRCONTROL0;
	regvalue 	&= ~( TMRCONTROL_INTPEND | TMRCONTROL_SET_TCLOCK );
	regvalue 	|=  ( TMRCONTROL_TCLOCK_DIV1                     ); 	// source clock
	TMRCONTROL0  =	regvalue;
				
	TMRCOUNT0	=   (u32)0;
		
	regvalue 	=	(u32)( ( get_pll_clk(CLKSRC_PLL0) / (unsigned long)TMRCLKGEN_CLKDIV0 ) / (unsigned long)(100) );  // 10ms 
	TMRMATCH0	=   (u32)regvalue;
 	
 	regvalue	=	TMRCONTROL0;
 	regvalue	&= ~( TMRCONTROL_INTENB | TMRCONTROL_INTPEND );
 	regvalue 	|=  ( TMRCONTROL_INTENB );
	TMRCONTROL0 =   regvalue;											// Interrupt Enable


	regvalue	=	TMRCONTROL0;
	regvalue &= ~( TMRCONTROL_INTPEND );								// RUN timer 0
	regvalue |=  ( TMRCONTROL_RUN     );
	TMRCONTROL0 = regvalue;
	
	//gdebug( " TMRCLKGEN0 0x%08x\n", TMRCLKGEN0 );
	
}
#else
static void pollux_timer_setup (void)
{
	u32 regvalue		 = 0;
	
	
	regvalue	=	TMRCLKGEN0;
	regvalue	&= ~( TMRCLKGEN_CLKSRCSEL << 1 );   
 	regvalue  |=  ( TMRCLKGEN_CLKSRC_PLL1    );	 				// clock source PLL0
 	TMRCLKGEN0	=  regvalue; 	
 	
 	regvalue	=	TMRCLKGEN0;
 	regvalue 	&= ~( TMRCLKGEN_CLKDIV         );               // clock division
	regvalue	|=  ( TMRCLKGEN_CLKDIV0 - 1 ) << ( TMRCLKGEN_CLKDIV_SHIFT );
	TMRCLKGEN0	=  regvalue;


	regvalue	=	TMRCLKENB0;
	regvalue 	&=  ~( TMRCLKENB_TCLKMODE);
//TMRCLKENB0 	|= (clkmode & 0x1) << 3; // clkmode 1
	regvalue 	|=   ( TMRCLKENB_TCLKMODE ); 					// set clock pclk mode
	TMRCLKENB0	=	 regvalue;
	
	regvalue	=	TMRCLKENB0;
	regvalue	 &= ~( TMRCLKENB_CLKGENENB ); 					// set clock divisor enable
	regvalue	 |=  ( TMRCLKENB_CLKGENENB );
	TMRCLKENB0	 =	 regvalue;
	
	regvalue	=	TMRCONTROL0;
	regvalue 	&= ~( TMRCONTROL_INTPEND | TMRCONTROL_SET_TCLOCK );
	regvalue 	|=  ( TMRCONTROL_TCLOCK_DIV1                     ); 	// source clock
	TMRCONTROL0  =	regvalue;
				
	TMRCOUNT0	=   (u32)0;

	regvalue 	=	(u32)( ( get_pll_clk(CLKSRC_PLL1) / (unsigned long)TMRCLKGEN_CLKDIV0 ) / (unsigned long)(100) );  // 10ms 
	TMRMATCH0	=   (u32)regvalue;
 	
 	regvalue	=	TMRCONTROL0;
 	regvalue	&= ~( TMRCONTROL_INTENB | TMRCONTROL_INTPEND );
 	regvalue 	|=  ( TMRCONTROL_INTENB );
	TMRCONTROL0 =   regvalue;											// Interrupt Enable


	regvalue	=	TMRCONTROL0;
	regvalue &= ~( TMRCONTROL_INTPEND );								// RUN timer 0
	regvalue |=  ( TMRCONTROL_RUN     );
	TMRCONTROL0 = regvalue;
	
	//gdebug( " TMRCLKGEN0 0x%08x\n", TMRCLKGEN0 );
	
}
#endif

static struct irqaction pollux_timer_irq = {
	.name		= "POLLUX Timer Tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER, // 2.6.20 커널 수정.. 기존 flag랑 완전 바뀜..^^
	.handler	= pollux_timer_interrupt,
};

//static void __init pollux_timer_init (void)
static void pollux_timer_init (void)
{
	gdebug("pollux_timer_init\n"); // debugging 	
	//gdebug( "get_pll_clk %lu\n",get_pll_clk(CLKSRC_PLL0) );
	pollux_timer_setup();
	setup_irq( IRQ_TIMER0, &pollux_timer_irq );
	
}

struct sys_timer pollux_timer = {
	.init		= pollux_timer_init,
	.offset		= pollux_gettimeoffset,
	.resume		= pollux_timer_setup
};


#if 0
//static irqreturn_t mp2530_uart3_interrupt( int irq, void *dev_id )
irqreturn_t mp2530_uart0_interrupt( int irq, void *dev_id )
{	
	unsigned short mask;
	
	gdebug( "mp2530_uart3_interrupt rx \n" );
	
	// ghcstop 070322
	// fifo status register의 rx fifo count만큼 읽어줘야 한다.
	// 나중에 rx fifo trigger level을 변화시킬 것.
	
	// uart 0
	while( 	((mask = __raw_readw(POLLUX_VA_UART0 + POLLUX_UFSTAT)) & 0x0f) != 0 )
		gdebug( "mp2530_uart3_irq_demux uart3 POLLUX_URXH 0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_URXH ) );

	// uart 3
	//while( 	((mask = __raw_readw(POLLUX_VA_UART3 + POLLUX_UFSTAT)) & 0x0f) != 0 )
	//	gdebug( "mp2530_uart3_irq_demux uart3 POLLUX_URXH 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_URXH ) );


	// uart 2
	//while( 	((mask = __raw_readw(POLLUX_VA_UART2 + POLLUX_UFSTAT)) & 0x0f) != 0 )
		//gdebug( "mp2530_uart3_irq_demux uart2 POLLUX_URXH 0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_URXH ) );
	
	
	return IRQ_HANDLED;
}

//static irqreturn_t mp2530_uart3_interrupt( int irq, void *dev_id )
irqreturn_t mp2530_uart0_tx_interrupt( int irq, void *dev_id )
{	
	unsigned short mask;
	gdebug( "mp2530_uart3_interrupt TTTTT \n" );
	//gdebug( "POLLUX_UFSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UFSTAT ) );	
	mask = __raw_readw( POLLUX_VA_UART0 + POLLUX_ULCON );  // sync pend clear
	mask |= POLLUX_LCON_SYNCPENDCLR;
	//gdebug( "mp2530_irq_uart3_ack syncpend mask 0x%x \n", mask );
	__raw_writew( mask, POLLUX_VA_UART0 + POLLUX_ULCON );
	gdebug("tx.....pend clear\n");
	
	return IRQ_HANDLED;
}


//static irqreturn_t mp2530_uart3_interrupt( int irq, void *dev_id )
void mp2530_uart0_tx(void)
{
	int i;
	unsigned short mask;
		
	gdebug("tx.....start\n");		
#if 0 // 이거 안해주면 interrupt 발생 안한다. 아니다 발생한다...
	mask = __raw_readw( POLLUX_VA_UART0 + POLLUX_ULCON );  // sync pend clear
	mask |= POLLUX_LCON_SYNCPENDCLR;
	//gdebug( "mp2530_irq_uart3_ack syncpend mask 0x%x \n", mask );
	__raw_writew( mask, POLLUX_VA_UART0 + POLLUX_ULCON );
	gdebug("tx.....pend clear\n");
#endif	
	
	
		
	//for(i=0; i<28; i++ ) // irq 두번 발생한다.
	for(i=0; i<16; i++ ) // irq 두번 발생한다.
		__raw_writew( 'a', (POLLUX_VA_UART0 + POLLUX_UTXH) );
	
	gdebug("tx.....end\n");
}
#endif

/*
 * linux/drivers/serial/pollux.c
 *
 * Driver for onboard UARTs on the Magiceyes POLLUX
 *
 * Based on drivers/serial/s3c2410.c
 *
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho) 	- main programmer
 * amphell(Bang Chang Hyeok)    - Hardware Engineer
 *
 * 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/pollux/index.html
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


#if defined(CONFIG_SERIAL_POLLUX_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#    define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/irq.h>

#include <asm/hardware.h>
#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-clock-power.h>


/* structures */

struct pollux_uart_info
{
    char           *name;
    unsigned int    type;
    unsigned int    fifosize;
    unsigned long   rx_fifomask;
    unsigned long   rx_fifoshift;
    unsigned long   rx_fifofull;
    unsigned long   tx_fifomask;
    unsigned long   tx_fifoshift;
    unsigned long   tx_fifofull;
};

struct pollux_uart_port
{
    unsigned char   rx_claimed;
    unsigned char   tx_claimed;

    struct pollux_uart_info *info;
    struct uart_port port;
};

#if 0

extern void     printascii(const char *);

static void
gdebug(const char *fmt, ...)
{
    va_list         va;
    char            buff[256];

    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);

    printascii(buff);
}

#include <asm/arch/regs-interrupt.h>

#else /* no debug */
	#define gdebug(x...) do {} while(0)
	//#define gdebug(x...) printk(x)
#endif

//#define dbg(x...)	do {} while(0)
#define dbg(fmt, x...)	gdebug("%s: " fmt, __FUNCTION__ , ## x)
// debug gtgkjh

/* UART name and device definitions */

#define POLLUX_SERIAL_NAME	"ttySAC"
#define POLLUX_SERIAL_MAJOR	204
#define POLLUX_SERIAL_MINOR	64


/* conversion functions */

#define pollux_dev_to_port(__dev) (struct uart_port *)dev_get_drvdata(__dev)
#define pollux_dev_to_cfg(__dev) (struct pollux_uartcfg *)((__dev)->platform_data)

/* we can support 3 uarts, but not always use them */
#define NR_PORTS (POLLUX_UART_NR_PORTS)

/* port irq numbers */
#define TX_IRQ(port) ((port)->irq)
#define RX_IRQ(port) ((port)->irq + 1)

/* register access controls */

#define portaddr(port, reg) ((port)->membase + (reg))

#define rd_regb(port, reg) (__raw_readb(portaddr(port, reg)))
#define rd_regw(port, reg) (__raw_readw(portaddr(port, reg))) 
#define rd_regl(port, reg) (__raw_readl(portaddr(port, reg)))

#define wr_regb(port, reg, val) \
  do { __raw_writeb(val, portaddr(port, reg)); } while(0)

#define wr_regw(port, reg, val) \
  do { __raw_writew(val, portaddr(port, reg)); } while(0)

#define wr_regl(port, reg, val) \
  do { __raw_writel(val, portaddr(port, reg)); } while(0)

/* macros to change one thing to another */
#define tx_enabled(port) ((port)->unused[0])
#define rx_enabled(port) ((port)->unused[1])

/* flag to ignore all characters comming in */
#define RXSTAT_DUMMY_READ (0x10000000)



inline void sync_pend_clear(struct uart_port *port)
{
	unsigned short ulcon;
	
	ulcon = rd_regw(port, POLLUX_ULCON );  
	ulcon |= POLLUX_LCON_SYNCPENDCLR;
	wr_regw( port, POLLUX_ULCON, ulcon);
}	


static inline struct pollux_uart_port *
to_ourport(struct uart_port *port)
{
    return container_of(port, struct pollux_uart_port, port);
}

/* translate a port to the device name */

static inline const char *
pollux_serial_portname(struct uart_port *port)
{
    return to_platform_device(port->dev)->name;
}

static int
pollux_serial_txempty_nofifo(struct uart_port *port)
{
    return (rd_regw(port, POLLUX_UTRSTAT) & POLLUX_UTRSTAT_TXE);
}


static inline void dispregs(struct uart_port *port)
{
	printk( "port = %s\n", pollux_serial_portname(port) );
}

static void
pollux_serial_rx_enable(struct uart_port *port)
{
    unsigned long   flags;
    unsigned short    ucon,
                    ufcon;
    int             count = 10000;

    spin_lock_irqsave(&port->lock, flags);

    while (--count && !pollux_serial_txempty_nofifo(port))
        udelay(100);

    ufcon = rd_regw(port, POLLUX_UFCON);
    ufcon |= POLLUX_UFCON_RESETRX;
    wr_regw(port, POLLUX_UFCON, ufcon);

    ucon = rd_regw(port, POLLUX_UCON);
    ucon |= POLLUX_UCON_RXIRQMODE;
    wr_regw(port, POLLUX_UCON, ucon);
    
    dbg("ulcon = 0x%08x\n", ucon);

    rx_enabled(port) = 1;
    spin_unlock_irqrestore(&port->lock, flags);
}

static void
pollux_serial_rx_disable(struct uart_port *port)
{
    unsigned long   flags;
    unsigned short    ucon;

    spin_lock_irqsave(&port->lock, flags);

    ucon = rd_regw(port, POLLUX_UCON);
    ucon &= ~POLLUX_UCON_RXIRQMODE;
    wr_regw(port, POLLUX_UCON, ucon);

    rx_enabled(port) = 0;
    spin_unlock_irqrestore(&port->lock, flags);
}

static void
pollux_serial_stop_tx(struct uart_port *port)
{	
    if (tx_enabled(port))
    {	
        disable_irq(TX_IRQ(port));
        tx_enabled(port) = 0;
        if (port->flags & UPF_CONS_FLOW)
            pollux_serial_rx_enable(port);
    }
}

static void
pollux_serial_start_tx(struct uart_port *port)
{	
	//sync_pend_clear(port); // ghcstop add		
dbg("start tx......1: %d\n", tx_enabled(port) );	
	//unsigned short ulcon;
    if (!tx_enabled(port)) // tx_enabled(port) == 0
    {
        if (port->flags & UPF_CONS_FLOW)
            pollux_serial_rx_disable(port);

 	    enable_irq(TX_IRQ(port));
 	    
dbg("start tx......2\n");	 	    
#if 0 // ghcstop fix	
		ulcon = rd_regw(port, POLLUX_ULCON );  
		ulcon |= POLLUX_LCON_SYNCPENDCLR;
		wr_regw( port, POLLUX_ULCON, ulcon);
#else
		sync_pend_clear(port); // ghcstop add		
#endif		
dbg("start tx......3\n");	 	     	    
        tx_enabled(port) = 1;
    }
}


static void
pollux_serial_stop_rx(struct uart_port *port)
{
    if (rx_enabled(port))
    {
        disable_irq(RX_IRQ(port));
        rx_enabled(port) = 0;
    }
}

static void
pollux_serial_enable_ms(struct uart_port *port)
{
}

static inline struct pollux_uart_info *
pollux_port_to_info(struct uart_port *port)
{
    return to_ourport(port)->info;
}

static inline struct pollux_uartcfg *
pollux_port_to_cfg(struct uart_port *port)
{
    if (port->dev == NULL)
        return NULL;

    return (struct pollux_uartcfg *) port->dev->platform_data;
}

static int
pollux_serial_rx_fifocnt(struct pollux_uart_port *ourport, unsigned long ufstat)
{
    struct pollux_uart_info *info = ourport->info;

    if (ufstat & info->rx_fifofull)
        return info->fifosize;

    return (ufstat & info->rx_fifomask) >> info->rx_fifoshift;
}

static int
pollux_serial_resetport(struct uart_port *port, struct pollux_uartcfg *cfg)
{
    unsigned int  clkenb, clkgen;
	
    wr_regw(port, POLLUX_UCON, cfg->ucon);
    wr_regw(port, POLLUX_ULCON, cfg->ulcon);

	clkenb = rd_regl(port, POLLUX_UCLKENB);    
	clkenb &= ~(POLLUX_UCLKENB_CLKGENENB);
	clkenb |= POLLUX_UCLKENB_PCLKMODE;
	wr_regl(port, POLLUX_UCLKENB, clkenb);

	clkgen = ((UART_BASE_DIV-1)<<4) | POLLUX_UCLKGEN_CLKSRC_PLL1;
	wr_regl(port, POLLUX_UCLKGEN, clkgen);
	
	clkenb = rd_regl(port, POLLUX_UCLKENB);    
	clkenb |= (POLLUX_UCLKENB_CLKGENENB);
	wr_regl(port, POLLUX_UCLKENB, clkenb);

    wr_regw(port, POLLUX_UFCON, cfg->ufcon | POLLUX_UFCON_RESETBOTH);
    wr_regw(port, POLLUX_UFCON, cfg->ufcon);

    return 0;
}



/* ? - where has parity gone?? */
#define POLLUX_UERSTAT_PARITY (0x1000)

static          irqreturn_t
pollux_serial_rx_chars(int irq, void *dev_id)
{
    struct pollux_uart_port *ourport = dev_id;
    struct uart_port *port = &ourport->port;
    struct tty_struct *tty = port->info->tty;
    unsigned int    ch,
                    flag;
    unsigned short ufcon,
                    ufstat,
                    uerstat;                    
    int             max_count = 64;
	
dbg("##pollux_serial_rx_chars: 0\n");  
    while (max_count-- > 0)
    {
        ufcon = rd_regw(port, POLLUX_UFCON);
        ufstat = rd_regw(port, POLLUX_UFSTAT);

        if (pollux_serial_rx_fifocnt(ourport, ufstat) == 0)
            break;

        uerstat = rd_regw(port, POLLUX_UERSTAT);
        ch = rd_regb(port, POLLUX_URXH);

        if (port->flags & UPF_CONS_FLOW)
        {
            int             txe = pollux_serial_txempty_nofifo(port);

            if (rx_enabled(port))
            {
                if (!txe)
                {
                    rx_enabled(port) = 0;
                    continue;
                }
            }
            else
            {
                if (txe)
                {
                    ufcon |= POLLUX_UFCON_RESETRX;
                    wr_regw(port, POLLUX_UFCON, ufcon);
                    rx_enabled(port) = 1;
                    goto out;
                }
                continue;
            }
        }

dbg("##pollux_serial_rx_chars: 1\n");  
        /*
         * insert the character into the buffer 
         */
        flag = TTY_NORMAL;
        port->icount.rx++;

        if (unlikely(uerstat & POLLUX_UERSTAT_ANY))
        {
            /*
             * check for break 
             */
            if (uerstat & POLLUX_UERSTAT_BREAK)
            {
                port->icount.brk++;
                if (uart_handle_break(port))
                    goto ignore_char;
            }

            if (uerstat & POLLUX_UERSTAT_FRAME)
                port->icount.frame++;
            if (uerstat & POLLUX_UERSTAT_OVERRUN)
                port->icount.overrun++;

            uerstat &= port->read_status_mask;

            if (uerstat & POLLUX_UERSTAT_BREAK)
                flag = TTY_BREAK;
            else if (uerstat & POLLUX_UERSTAT_PARITY)
                flag = TTY_PARITY;
            else if (uerstat & (POLLUX_UERSTAT_FRAME | POLLUX_UERSTAT_OVERRUN))
                flag = TTY_FRAME;
        }

        if (uart_handle_sysrq_char(port, ch))
            goto ignore_char;

        uart_insert_char(port, uerstat, POLLUX_UERSTAT_OVERRUN, ch, flag);

      ignore_char:
        continue;
    }
    tty_flip_buffer_push(tty);

  out:
    //sync_pend_clear(port); // ghcstop add
    return IRQ_HANDLED;
}

static irqreturn_t
pollux_serial_tx_chars(int irq, void *id)
{
    struct pollux_uart_port *ourport = id;
    struct uart_port *port = &ourport->port;
    struct circ_buf *xmit = &port->info->xmit;
    int             count = 256;
    
    unsigned short ufstat;


#if 0
/*gdebug( " INTMASKL INTPENDL = 0x%04x 0x%04x\n", INTMASKL ,INTPENDL );
gdebug( " INTMASKH INTPENDH = 0x%04x 0x%04x\n", INTMASKH ,INTPENDH );
gdebug( " uart3 POLLUX_ULCON    0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_ULCON ) );
gdebug( " uart3 POLLUX_UCON     0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UCON ) );
gdebug( " uart3 POLLUX_UFCON    0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UFCON ) );
gdebug( " uart3 POLLUX_UMCON    0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UMCON ) );
gdebug( " uart3 POLLUX_UTRSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UTRSTAT ) );
gdebug( " uart3 POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UINTSTAT ) );
gdebug( " uart3 POLLUX_UERSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UERSTAT ) );
gdebug( " uart3 POLLUX_UFSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UFSTAT ) );
gdebug( " uart3 POLLUX_UMSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UMSTAT ) );
gdebug( " uart3 POLLUX_UBRDIV   0x%x \n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UBRDIV ) );
gdebug( " uart3 POLLUX_UTOUT    0x%x \n\n", __raw_readw( POLLUX_VA_UART3 + POLLUX_UTOUT ) );
*/

/*gdebug( " INTMASKL INTPENDL = 0x%04x 0x%04x\n", INTMASKL ,INTPENDL );
gdebug( " INTMASKH INTPENDH = 0x%04x 0x%04x\n", INTMASKH ,INTPENDH );
gdebug( " uart3 POLLUX_ULCON    0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_ULCON ) );
gdebug( " uart3 POLLUX_UCON     0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UCON ) );
gdebug( " uart3 POLLUX_UFCON    0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UFCON ) );
gdebug( " uart3 POLLUX_UMCON    0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UMCON ) );
gdebug( " uart3 POLLUX_UTRSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UTRSTAT ) );
gdebug( " uart3 POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UINTSTAT ) );
gdebug( " uart3 POLLUX_UERSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UERSTAT ) );
gdebug( " uart3 POLLUX_UFSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UFSTAT ) );
gdebug( " uart3 POLLUX_UMSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UMSTAT ) );
gdebug( " uart3 POLLUX_UBRDIV   0x%x \n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UBRDIV ) );
gdebug( " uart3 POLLUX_UTOUT    0x%x \n\n", __raw_readw( POLLUX_VA_UART2 + POLLUX_UTOUT ) );
*/
/*gdebug( " INTMASKL INTPENDL = 0x%04x 0x%04x\n", INTMASKL ,INTPENDL );
gdebug( " INTMASKH INTPENDH = 0x%04x 0x%04x\n", INTMASKH ,INTPENDH );
gdebug( " uart3 POLLUX_ULCON    0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_ULCON ) );
gdebug( " uart3 POLLUX_UCON     0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UCON ) );
gdebug( " uart3 POLLUX_UFCON    0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UFCON ) );
gdebug( " uart3 POLLUX_UMCON    0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UMCON ) );
gdebug( " uart3 POLLUX_UTRSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UTRSTAT ) );
gdebug( " uart3 POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT ) );
gdebug( " uart3 POLLUX_UERSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UERSTAT ) );
gdebug( " uart3 POLLUX_UFSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UFSTAT ) );
gdebug( " uart3 POLLUX_UMSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UMSTAT ) );
gdebug( " uart3 POLLUX_UBRDIV   0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UBRDIV ) );
gdebug( " uart3 POLLUX_UTOUT    0x%x \n\n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UTOUT ) );
*/

#endif

	
	//gdebug("0out..............\n");
		
    if (port->x_char)
    {
        wr_regb(port, POLLUX_UTXH, port->x_char);
        ufstat = rd_regw(port, POLLUX_UFSTAT);
        
        port->icount.tx++;
        port->x_char = 0;
        goto out;
    }

	//gdebug("1out..............\n");
    /*
     * if there isnt anything more to transmit, or the uart is now stopped, disable the uart and exit 
     */

    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
    {
        pollux_serial_stop_tx(port);
        goto out;
    }

	//gdebug("2out..............\n");

    /*
     * try and drain the buffer... 
     */

    while (!uart_circ_empty(xmit) && count-- > 0)
    {
        if (rd_regw(port, POLLUX_UFSTAT) & ourport->info->tx_fifofull)
            break;

        wr_regb(port, POLLUX_UTXH, xmit->buf[xmit->tail]);
        ufstat = rd_regw(port, POLLUX_UFSTAT);
        
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
    }

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

    if (uart_circ_empty(xmit))
        pollux_serial_stop_tx(port);
        
  out:
    //sync_pend_clear(port); // ghcstop add
  
    return IRQ_HANDLED;
}

static unsigned int
pollux_serial_tx_empty(struct uart_port *port)
{
    struct pollux_uart_info *info = pollux_port_to_info(port);
    unsigned short   ufstat = rd_regw(port, POLLUX_UFSTAT);
    unsigned short   ufcon = rd_regw(port, POLLUX_UFCON);
	
	dbg("1\n");
    if (ufcon & POLLUX_UFCON_FIFOMODE)
    {
        if ((ufstat & info->tx_fifomask) != 0 || (ufstat & info->tx_fifofull))
        {
        	dbg("2\n");
            return 0;
        }

		dbg("3\n");
        return 1;
    }

	dbg("4\n");
    return pollux_serial_txempty_nofifo(port);
}

/* no modem control lines */
static unsigned int
pollux_serial_get_mctrl(struct uart_port *port)
{
    unsigned int    umstat = rd_regb(port, POLLUX_UMSTAT);

    if (umstat & POLLUX_UMSTAT_CTS)
        return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
    else
        return TIOCM_CAR | TIOCM_DSR;
}

static void
pollux_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    /*
     * todo - possibly remove AFC and do manual CTS 
     */
}

static void
pollux_serial_break_ctl(struct uart_port *port, int break_state)
{
    unsigned long   flags;
    unsigned short    ucon;

    spin_lock_irqsave(&port->lock, flags);

    ucon = rd_regw(port, POLLUX_UCON);

    if (break_state)
        ucon |= POLLUX_UCON_SBREAK;
    else
        ucon &= ~POLLUX_UCON_SBREAK;

    wr_regw(port, POLLUX_UCON, ucon);

    spin_unlock_irqrestore(&port->lock, flags);
}

static void
pollux_serial_shutdown(struct uart_port *port)
{
    struct pollux_uart_port *ourport = to_ourport(port);

    if (ourport->tx_claimed)
    {
        free_irq(TX_IRQ(port), ourport);
        tx_enabled(port) = 0;
        ourport->tx_claimed = 0;
    }

    if (ourport->rx_claimed)
    {
        free_irq(RX_IRQ(port), ourport);
        ourport->rx_claimed = 0;
        rx_enabled(port) = 0;
    }
}


static int
pollux_serial_startup(struct uart_port *port)
{
    struct pollux_uart_port *ourport = to_ourport(port);
    int             ret;
	unsigned short ulcon;
    
    struct pollux_uartcfg *cfg;
    cfg = pollux_dev_to_cfg(port->dev);

    dbg("port=0x%08x (0x%08x)\n", port->mapbase, port->membase);

    rx_enabled(port) = 1;

	dbg("rx irq = %d\n", RX_IRQ(port));
    ret = request_irq(RX_IRQ(port), pollux_serial_rx_chars, 0, pollux_serial_portname(port), ourport);
    if (ret != 0)
    {
        printk("cannot get irq %d\n", RX_IRQ(port));
        return ret;
    }
    
    ourport->rx_claimed = 1;
	dbg("1\n");
#if 1
    tx_enabled(port) = 1;

    ret = request_irq(TX_IRQ(port), pollux_serial_tx_chars, 0, pollux_serial_portname(port), ourport);
    if (ret)
    {
        printk("cannot get irq %d\n", TX_IRQ(port));
        goto err;
    }
#endif    

	dbg("tx irq = %d\n", TX_IRQ(port));
    ourport->tx_claimed = 1;

	ulcon = rd_regw(port, POLLUX_ULCON );  
	ulcon |= POLLUX_LCON_SYNCPENDCLR;
	wr_regw( port, POLLUX_ULCON, ulcon);
    
	dbg("2\n");
    /*
     * the port reset code should have done the correct register setup for the port controls 
     */
    return ret;

  err:
	dbg("3 ========> error\n");  
    pollux_serial_shutdown(port);
    return ret;
}

/* power power management control */

static void
pollux_serial_pm(struct uart_port *port, unsigned int level, unsigned int old)
{
    switch (level)
    {
    case 3:
        break;

    case 0:
        break;
    default:
        printk(KERN_ERR "pollux_serial: unknown pm %d\n", level);
    }
}

/* baud rate calculation
 *
 * The UARTs on the POLLUX/S3C2440 can take their clocks from a number
 * of different sources, including the peripheral clock ("pclk") and an
 * external clock ("uclk"). The S3C2440 also adds the core clock ("fclk")
 * with a programmable extra divisor.
 *
 * The following code goes through the clock sources, and calculates the
 * baud clocks (and the resultant actual baud rates) and then tries to
 * pick the closest one and select that.
 *
*/

static unsigned int
pollux_calc_brdiv(struct uart_port *port, unsigned int baud)
{
    int             calc_brd;
    unsigned long pll1, uart_src_clk;
    
    pll1         = get_pll_clk(CLKSRC_PLL1);
    uart_src_clk = pll1/UART_BASE_DIV;
   
	calc_brd = (unsigned int)((uart_src_clk/(baud * 16)) -1);

    return calc_brd;
}

static void
pollux_serial_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
    struct pollux_uartcfg *cfg = pollux_port_to_cfg(port);
    unsigned long   flags;
    unsigned int    baud,
                    quot;
    unsigned short    ulcon;
    unsigned short    umcon;
    /*
     * We don't support modem control lines.
     */
    termios->c_cflag &= ~(HUPCL | CMSPAR);
    termios->c_cflag |= CLOCAL;

    /*
     * Ask the core to calculate the divisor for us.
     */
    baud = uart_get_baud_rate(port, termios, old, 0, 115200 * 8);

    if (baud == 38400 && (port->flags & UPF_SPD_MASK) == UPF_SPD_CUST)
        quot = port->custom_divisor;
    else
        quot = pollux_calc_brdiv(port, baud);

    switch (termios->c_cflag & CSIZE)
    {
    case CS5:
        dbg("config: 5bits/char\n");
        ulcon = POLLUX_LCON_CS5;
        break;
    case CS6:
        dbg("config: 6bits/char\n");
        ulcon = POLLUX_LCON_CS6;
        break;
    case CS7:
        dbg("config: 7bits/char\n");
        ulcon = POLLUX_LCON_CS7;
        break;
    case CS8:
    default:
        dbg("config: 8bits/char\n");
        ulcon = POLLUX_LCON_CS8;
        break;
    }

    /*
     * preserve original lcon IR settings 
     */
    ulcon |= (cfg->ulcon & POLLUX_LCON_IRM);

    if (termios->c_cflag & CSTOPB)
        ulcon |= POLLUX_LCON_STOPB;

    /* 
     * ghcstop_caution: add 080316: 2410/2440과 틀린 부분이 있기 때문에 이 부분을 제어해 주어야 한다. 
     * 그래서 기존의 값을 그냥 읽어서 or 해서 사용해야 한다. see regs-serial.h & manual's modem control register(bit: 5, 6, 7)
     */
	umcon = rd_regw(port, POLLUX_UMCON );  
    umcon |= (termios->c_cflag & CRTSCTS) ? POLLUX_UMCOM_AFC : 0;

    if (termios->c_cflag & PARENB)
    {
        if (termios->c_cflag & PARODD)
            ulcon |= POLLUX_LCON_PODD;
        else
            ulcon |= POLLUX_LCON_PEVEN;
    }
    else
    {
        ulcon |= POLLUX_LCON_PNONE;
    }
    spin_lock_irqsave(&port->lock, flags);

	gdebug( " uart3 POLLUX_UMCON    0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UMCON ) );
    dbg("setting ulcon to %08x, brddiv to %d: baud: %d\n", ulcon, quot, baud);

    wr_regw(port, POLLUX_ULCON, ulcon);
    wr_regw(port, POLLUX_UBRDIV, quot);
    wr_regw(port, POLLUX_UMCON, umcon);
    

	#if 0
	gdebug( " INTMASKL INTPENDL = 0x%04x 0x%04x\n", INTMASKL ,INTPENDL );
	gdebug( " INTMASKH INTPENDH = 0x%04x 0x%04x\n", INTMASKH ,INTPENDH );
	gdebug( " uart3 POLLUX_ULCON    0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_ULCON ) );
	gdebug( " uart3 POLLUX_UCON     0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UCON ) );
	gdebug( " uart3 POLLUX_UFCON    0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UFCON ) );
	gdebug( " uart3 POLLUX_UMCON    0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UMCON ) );
	gdebug( " uart3 POLLUX_UTRSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UTRSTAT ) );
	gdebug( " uart3 POLLUX_UINTSTAT 0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UINTSTAT ) );
	gdebug( " uart3 POLLUX_UERSTAT  0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UERSTAT ) );
	gdebug( " uart3 POLLUX_UFSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UFSTAT ) );
	gdebug( " uart3 POLLUX_UMSTAT   0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UMSTAT ) );
	gdebug( " uart3 POLLUX_UBRDIV   0x%x \n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UBRDIV ) );
	gdebug( " uart3 POLLUX_UTOUT    0x%x \n\n", __raw_readw( POLLUX_VA_UART0 + POLLUX_UTOUT ) );
	#endif


    /*
     * Update the per-port timeout.
     */
    uart_update_timeout(port, termios->c_cflag, baud);

    /*
     * Which character status flags are we interested in?
     */
    port->read_status_mask = POLLUX_UERSTAT_OVERRUN;
    if (termios->c_iflag & INPCK)
        port->read_status_mask |= POLLUX_UERSTAT_FRAME | POLLUX_UERSTAT_PARITY;
    /*
     * Which character status flags should we ignore?
     */
    port->ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR)
        port->ignore_status_mask |= POLLUX_UERSTAT_OVERRUN;
    if (termios->c_iflag & IGNBRK && termios->c_iflag & IGNPAR)
        port->ignore_status_mask |= POLLUX_UERSTAT_FRAME;

    /*
     * Ignore all characters if CREAD is not set.
     */
    if ((termios->c_cflag & CREAD) == 0)
        port->ignore_status_mask |= RXSTAT_DUMMY_READ;
    spin_unlock_irqrestore(&port->lock, flags);
}

static const char *
pollux_serial_type(struct uart_port *port)
{
    switch (port->type)
    {
    case PORT_POLLUX: 
    gdebug(".....port_pollux\n");
        return "POLLUX";
    default:
        return NULL;
    }
}

#define MAP_SIZE (0x100)

static void
pollux_serial_release_port(struct uart_port *port)
{
}

static int
pollux_serial_request_port(struct uart_port *port)
{
    return 0;
}

static void
pollux_serial_config_port(struct uart_port *port, int flags)
{
    struct pollux_uart_info *info = pollux_port_to_info(port);

    if (flags & UART_CONFIG_TYPE && pollux_serial_request_port(port) == 0)
        port->type = info->type;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int
pollux_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
    struct pollux_uart_info *info = pollux_port_to_info(port);

    if (ser->type != PORT_UNKNOWN && ser->type != info->type)
        return -EINVAL;

    return 0;
}


#ifdef CONFIG_SERIAL_POLLUX_CONSOLE

static struct console pollux_serial_console;

#    define POLLUX_SERIAL_CONSOLE &pollux_serial_console
#else
#    define POLLUX_SERIAL_CONSOLE NULL
#endif

static struct uart_ops pollux_serial_ops = {
    .pm = pollux_serial_pm,
    .tx_empty = pollux_serial_tx_empty,
    .get_mctrl = pollux_serial_get_mctrl,
    .set_mctrl = pollux_serial_set_mctrl,
    .stop_tx = pollux_serial_stop_tx,
    .start_tx = pollux_serial_start_tx,
    .stop_rx = pollux_serial_stop_rx,
    .enable_ms = pollux_serial_enable_ms,
    .break_ctl = pollux_serial_break_ctl,
    .startup = pollux_serial_startup,
    .shutdown = pollux_serial_shutdown,
    .set_termios = pollux_serial_set_termios,
    .type = pollux_serial_type,
    .release_port = pollux_serial_release_port,
    .request_port = pollux_serial_request_port,
    .config_port = pollux_serial_config_port,
    .verify_port = pollux_serial_verify_port,
};


static struct uart_driver pollux_uart_drv = {
    .owner = THIS_MODULE,
    .dev_name = "pollux_serial",
    //.dev_name = "mp2530_serial",
    .nr = NR_PORTS,
    .cons = POLLUX_SERIAL_CONSOLE,
    .driver_name = POLLUX_SERIAL_NAME,
    .major = POLLUX_SERIAL_MAJOR,
    .minor = POLLUX_SERIAL_MINOR,
};

static struct pollux_uart_port pollux_serial_ports[NR_PORTS] = {
    [0] = {
           .port = {
                    .lock = SPIN_LOCK_UNLOCKED,
                    .iotype = UPIO_MEM,
                    .irq = IRQ_TXD0,
                    .uartclk = 0,
                    .fifosize = 16,
                    .ops = &pollux_serial_ops,
                    .flags = UPF_BOOT_AUTOCONF,
                    .line = 0,
                    }
           },
    [1] = {
           .port = {
                    .lock = SPIN_LOCK_UNLOCKED,
                    .iotype = UPIO_MEM,
                    .irq = IRQ_TXD1,
                    .uartclk = 0,
                    .fifosize = 16,
                    .ops = &pollux_serial_ops,
                    .flags = UPF_BOOT_AUTOCONF,
                    .line = 1,
                    }
           },
    [2] = {
           .port = {
                    .lock = SPIN_LOCK_UNLOCKED,
                    .iotype = UPIO_MEM,
                    .irq = IRQ_TXD2,
                    .uartclk = 0,
                    .fifosize = 16,
                    .ops = &pollux_serial_ops,
                    .flags = UPF_BOOT_AUTOCONF,
                    .line = 2,
                    }
           },
    [3] = {
           .port = {
                    .lock = SPIN_LOCK_UNLOCKED,
                    .iotype = UPIO_MEM,
                    .irq = IRQ_TXD3,
                    .uartclk = 0,
                    .fifosize = 16,
                    .ops = &pollux_serial_ops,
                    .flags = UPF_BOOT_AUTOCONF,
                    .line = 3,
                    }
           }
           
};


/* pollux_serial_init_port
 *
 * initialise a single serial port from the platform device given
 */

static int
pollux_serial_init_port(struct pollux_uart_port *ourport, struct pollux_uart_info *info, struct platform_device *platdev)
{
    struct uart_port *port = &ourport->port;
    struct pollux_uartcfg *cfg;
    struct resource *res;

    dbg("port=%p, platdev=%p\n", port, platdev);

    if (platdev == NULL)
        return -ENODEV;

	dbg("0\n");
    cfg = pollux_dev_to_cfg(&platdev->dev);

    if (port->mapbase != 0)
        return 0;

	dbg("1\n");
    if (cfg->hwport >= NR_PORTS)
        return -EINVAL;

    /*
     * setup info for port 
     */
    port->dev = &platdev->dev;
    ourport->info = info;

    /*
     * copy the info in from provided structure 
     */
    ourport->port.fifosize = info->fifosize;


    port->uartclk = 1;

    if (cfg->uart_flags & UPF_CONS_FLOW)
    {
        port->flags |= UPF_CONS_FLOW;
    }

    /*
     * sort our the physical and virtual addresses for each UART 
     */
    dbg("2\n");
    res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
    if (res == NULL)
    {
        printk(KERN_ERR "failed to find memory resource for uart\n");
        return -EINVAL;
    }

    port->mapbase = res->start;
    port->membase = (unsigned char __iomem	*)res->start;
    port->irq = platform_get_irq(platdev, 0);
    if (port->irq < 0)
        port->irq = 0;

	dbg("3\n");

    /*
     * reset the fifos (and setup the uart) 
     */
    pollux_serial_resetport(port, cfg);
    return 0;
}

/* Device driver serial port probe */

static int      probe_index = 0;

static int
pollux_serial_probe_with_info(struct platform_device *dev, struct pollux_uart_info *info)
{
    struct pollux_uart_port *ourport;
    int             ret;
    
    dbg( " IRQTX0 %d\n", IRQ_TXD0 );

    dbg("(%p, %p) %d\n", dev, info, probe_index);

    ourport = &pollux_serial_ports[probe_index];
    probe_index++;

    dbg("initialising port %p..., irq %d \n", ourport, (&ourport->port)->irq  );

    ret = pollux_serial_init_port(ourport, info, dev);
    if (ret < 0)
        goto probe_err;

    dbg("adding port irq %d \n", (&ourport->port)->irq );
    //printk("uart_add_one_port() call pre\n");
    
    
    uart_add_one_port(&pollux_uart_drv, &ourport->port);
    platform_set_drvdata(dev, &ourport->port);

    return 0;

  probe_err:
    return ret;
}

static int
pollux_serial_remove(struct platform_device *dev)
{
    struct uart_port *port = pollux_dev_to_port(&dev->dev);

    if (port)
        uart_remove_one_port(&pollux_uart_drv, port);

    return 0;
}

/* UART power management code */

#ifdef CONFIG_PM

static int
pollux_serial_suspend(struct platform_device *dev, pm_message_t state)
{
    struct uart_port *port = pollux_dev_to_port(&dev->dev);

    if (port)
        uart_suspend_port(&pollux_uart_drv, port);

    return 0;
}

static int
pollux_serial_resume(struct platform_device *dev)
{
    struct uart_port *port = pollux_dev_to_port(&dev->dev);

    if (port)
    {
        pollux_serial_resetport(port, pollux_port_to_cfg(port));
        uart_resume_port(&pollux_uart_drv, port);
    }

    return 0;
}

#else
#    define pollux_serial_suspend NULL
#    define pollux_serial_resume  NULL
#endif

static int
pollux_serial_init(struct platform_driver *drv, struct pollux_uart_info *info)
{
    return platform_driver_register(drv);
}


/* now comes the code to initialise either the pollux or s3c2440 serial
 * port information
*/




static struct pollux_uart_info pollux_uart_inf = {
    .name = "Magiceye Inc. POLLUX UART",
    .type = PORT_POLLUX, // include/linux/serial_core.h
    .fifosize = 16,
    .rx_fifomask = POLLUX_UFSTAT_RXMASK,
    .rx_fifoshift = POLLUX_UFSTAT_RXSHIFT,
    .rx_fifofull = POLLUX_UFSTAT_RXFULL,
    .tx_fifofull = POLLUX_UFSTAT_TXFULL,
    .tx_fifomask = POLLUX_UFSTAT_TXMASK,
    .tx_fifoshift = POLLUX_UFSTAT_TXSHIFT,
};

/* device management */

static int
pollux_serial_probe(struct platform_device *dev)
{
	dbg("dev=%p\n", dev);
    return pollux_serial_probe_with_info(dev, &pollux_uart_inf);
}

static struct platform_driver pollux_serial_drv = {
    .probe = pollux_serial_probe,
    .remove = pollux_serial_remove,
    .suspend = pollux_serial_suspend,
    .resume = pollux_serial_resume,
    .driver = {
               .name = "pollux-uart",
               .owner = THIS_MODULE,
               },
};


#define pollux_uart_inf_at &pollux_uart_inf


/* module initialisation code */

static int __init
pollux_serial_modinit(void)
{
    int             ret;

    ret = uart_register_driver(&pollux_uart_drv);
    if (ret < 0)
    {
        return -1;
    }

    pollux_serial_init(&pollux_serial_drv, &pollux_uart_inf);

    return 0;
}

static void __exit
pollux_serial_modexit(void)
{
    platform_driver_unregister(&pollux_serial_drv);

    uart_unregister_driver(&pollux_uart_drv);
}


module_init(pollux_serial_modinit);
module_exit(pollux_serial_modexit);

/* Console code */

#ifdef CONFIG_SERIAL_POLLUX_CONSOLE

static struct uart_port *cons_uart;

static int
pollux_serial_console_txrdy(struct uart_port *port, unsigned int ufcon)
{
    struct pollux_uart_info *info = pollux_port_to_info(port);
    unsigned short   ufstat,
                    utrstat;

    if (ufcon & POLLUX_UFCON_FIFOMODE)
    {
        /*
         * fifo mode - check ammount of data in fifo registers... 
         */

        ufstat = rd_regw(port, POLLUX_UFSTAT);
        return (ufstat & info->tx_fifofull) ? 0 : 1;
    }

    /*
     * in non-fifo mode, we go and use the tx buffer empty 
     */

    utrstat = rd_regw(port, POLLUX_UTRSTAT);
    return (utrstat & POLLUX_UTRSTAT_TXE) ? 1 : 0;
}

static void
pollux_serial_console_putchar(struct uart_port *port, int ch)
{
    unsigned short    ufcon = rd_regw(cons_uart, POLLUX_UFCON);

    while (!pollux_serial_console_txrdy(port, ufcon))
        barrier();
    wr_regb(cons_uart, POLLUX_UTXH, ch);
}

static void
pollux_serial_console_write(struct console *co, const char *s, unsigned int count)
{
    uart_console_write(cons_uart, s, count, pollux_serial_console_putchar);
}

static void     __init
pollux_serial_get_options(struct uart_port *port, int *baud, int *parity, int *bits)
{
    unsigned short    ulcon;
    unsigned short    ucon;
    unsigned short    ubrdiv;
    unsigned long   uart_src_clk, pll1;

    ulcon = rd_regw(port, POLLUX_ULCON);
    ucon = rd_regw(port, POLLUX_UCON);
    ubrdiv = rd_regw(port, POLLUX_UBRDIV);

    if ((ucon & 0xf) != 0)
    {
        /*
         * consider the serial port configured if the tx/rx mode set 
         */

        switch (ulcon & POLLUX_LCON_CSMASK)
        {
        case POLLUX_LCON_CS5:
            *bits = 5;
            break;
        case POLLUX_LCON_CS6:
            *bits = 6;
            break;
        case POLLUX_LCON_CS7:
            *bits = 7;
            break;
        default:
        case POLLUX_LCON_CS8:
            *bits = 8;
            break;
        }

        switch (ulcon & POLLUX_LCON_PMASK)
        {
        case POLLUX_LCON_PEVEN:
            *parity = 'e';
            break;

        case POLLUX_LCON_PODD:
            *parity = 'o';
            break;

        case POLLUX_LCON_PNONE:
        default:
            *parity = 'n';
        }


	    pll1         = get_pll_clk(CLKSRC_PLL1);
	    uart_src_clk = pll1/UART_BASE_DIV; 

        *baud = uart_src_clk / (16 * (ubrdiv + 1));
    }

}

/* pollux_serial_init_ports
 *
 * initialise the serial ports from the machine provided initialisation
 * data.
*/

static int
pollux_serial_init_ports(struct pollux_uart_info *info)
{
    struct pollux_uart_port *ptr = pollux_serial_ports;
    struct platform_device **platdev_ptr;
    int             i;

    dbg(" initialising ports...\n");

    platdev_ptr = pollux_uart_devs;

    for (i = 0; i < NR_PORTS; i++, ptr++, platdev_ptr++)
    {
        pollux_serial_init_port(ptr, info, *platdev_ptr);
    }

    return 0;
}

static int      __init
pollux_serial_console_setup(struct console *co, char *options)
{
    struct uart_port *port;
    int             baud = 9600;
    int             bits = 8;
    int             parity = 'n';
    int             flow = 'n';

    dbg(" co=%p (%d), %s\n", co, co->index, options);

    /*
     * is this a valid port 
     */

    if (co->index == -1 || co->index >= NR_PORTS)
        co->index = 0;

    port = &pollux_serial_ports[co->index].port;

    /*
     * is the port configured? 
     */

    if (port->mapbase == 0x0)
    {
        co->index = 0;
        port = &pollux_serial_ports[co->index].port;
    }

    cons_uart = port;

    dbg(" port=%p (%d)\n", port, co->index);

    /*
     * Check whether an invalid uart number has been specified, and
     * if so, search for the first available port that does have
     * console support.
     */
    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);
    else
        pollux_serial_get_options(port, &baud, &parity, &bits);

    dbg(": baud %d\n", baud);

    return uart_set_options(port, co, baud, parity, bits, flow);
}

/* pollux_serial_initconsole
 *
 * initialise the console from one of the uart drivers
*/

static struct console pollux_serial_console = {
    .name = POLLUX_SERIAL_NAME,
    .device = uart_console_device,
    .flags = CON_PRINTBUFFER,
    .index = -1,
    .write = pollux_serial_console_write,
    .setup = pollux_serial_console_setup
};

static int
pollux_serial_initconsole(void)
{
    struct pollux_uart_info *info;
    struct platform_device *dev = pollux_uart_devs[0];

    dbg("#\n");

    /*
     * select driver based on the cpu 
     */

    if (dev == NULL)
    {
        printk(KERN_ERR "pollux: no devices for console init\n");
        return 0;
    }

	if (strcmp(dev->name, "pollux-uart") == 0)
    {
        info = pollux_uart_inf_at;
    }
    else
    {
        return 0;
    }

    if (info == NULL)
    {
        return 0;
    }

    pollux_serial_console.data = &pollux_uart_drv;
    pollux_serial_init_ports(info);

    register_console(&pollux_serial_console);
    return 0;
}

console_initcall(pollux_serial_initconsole);

#endif                          /* CONFIG_SERIAL_POLLUX_CONSOLE */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("godori <ghcstop@gmail.com> from http://www.aesop-embedded.org");
MODULE_DESCRIPTION("Magiceyes Inc. POLLUX Serial port driver");

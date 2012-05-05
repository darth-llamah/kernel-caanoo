/* linux/drivers/i2c/busses/i2c-pollux.c
 *
 * Copyright (C) 2007 MagicEyes Digital Co.
 *	Jonathan Lee <jonathan@mesdigital.com>
 *
 * POLLUX I2C Controller
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
*/

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/hardware.h>

#include <asm/mach-types.h>

#include <asm/io.h>
#include <asm/irq.h>
#include "mes_i2c.h"


#define POLLUX_I2C_NAME	"pollux-i2c"
#if 0
	#define dprintk(x...) printk(x)
#else
	#define dprintk(x...) do { } while (0)
#endif


// For AESOP
#define PHY_BASEADDR_I2C0                       0xC000E000

#define MES_I2C0	0
#define MES_I2C1	1

#define TXCLKSRC_PCLK16		16
#define TXCLKSRC_PCLK256	256

#define MAX_BUSY_COUNT 		(100000)

struct pollux_i2c {
	spinlock_t		lock;
	wait_queue_head_t	wait;

	struct i2c_msg		*msg;		/* pointer to message buffer */
	unsigned int		msg_num;	/* number of requested i2c message buffer */
	unsigned int		msg_idx;	/* (current) index of message buffer in the buffer array */
	unsigned int		msg_ptr;	/* (currnet) byte offset in i2c messsage buffer */

	U32					clock_source;
	U32					clock_scale;
	U32					scl_sda_diff;

	void __iomem 		*regs;
	struct device		*dev;
	struct resource		*irq;
	struct resource		*ioarea;
	struct i2c_adapter	adap;
};

#define POLLUX_I2C_SPINLOCK
#ifdef POLLUX_I2C_SPINLOCK
void i2c_spin_lock_irq(spinlock_t * lock)
{
	spin_lock_irq(lock);
}

void i2c_spin_unlock_irq(spinlock_t * lock)
{
	spin_unlock_irq(lock);
}

void i2c_spin_lock_init(spinlock_t * lock)
{
	spin_lock_init(lock);
}
#else /* POLLUX_I2C_SPINLOCK */
inline void i2c_spin_lock_irq(spinlock_t * lock) { }
inline void i2c_spin_unlock_irq(spinlock_t * lock) { }
inline void i2c_spin_lock_init(spinlock_t * lock) { }
#endif /* POLLUX_I2C_SPINLOCK */

/* s3c24xx_i2c_irq
 *
 * top level IRQ servicing routine
 */

static irqreturn_t pollux_i2c_irq(int irqno, void *dev_id)
{
	//struct pollux_i2c *i2c = dev_id;
	
	return IRQ_HANDLED;
}


struct  MES_I2C_RegisterSet
{
    volatile U32 ICCR;              ///< 0x00 : I2C Control Register
    volatile U32 ICSR;              ///< 0x04 : I2C Status Register
    volatile U32 IAR;               ///< 0x08 : I2C Address Register
    volatile U32 IDSR;              ///< 0x0C : I2C Data Register
	volatile U32 QCNT_MAX;	        ///< 0x10 : I2C Quarter Period Register
	const    U32 __Reserved0[4];	///< 0x14, 0x18, 0x1C, 0x20 : Reserved region        
    volatile U32 PEND;		        ///< 0x24 : I2C IRQ PEND Register
    const    U32 __Reserved1[0x36]; ///< 0x28 ~ 0xFC : Reserved region
    volatile U32 CLKENB;            ///< 0x100 : Clock Enable Register.
};

/* dump_i2c_registers 
 *
 * Show I2C controller registers
 */
static void dump_i2c_registers(struct pollux_i2c *i2c)
{
	struct  MES_I2C_RegisterSet *i2c_regs;
	
	i2c_regs = i2c->regs;

	dprintk("I2C_ICCR          0x%8.8x:0x%4.4x\n", (unsigned int)&(i2c_regs->ICCR), (unsigned int)i2c_regs->ICCR);
	dprintk("I2C_ICSR          0x%8.8x:0x%4.4x\n", (unsigned int)&(i2c_regs->ICSR), (unsigned int)i2c_regs->ICSR);
	dprintk("I2C_IAR           0x%8.8x:0x%4.4x\n", (unsigned int)&(i2c_regs->IAR), (unsigned int)i2c_regs->IAR);
	dprintk("I2C_IDSR          0x%8.8x:0x%4.4x\n", (unsigned int)&(i2c_regs->IDSR), (unsigned int)i2c_regs->IDSR);
	dprintk("I2C_QCNT_MAX      0x%8.8x:0x%4.4x\n", (unsigned int)&(i2c_regs->QCNT_MAX), (unsigned int)i2c_regs->QCNT_MAX);
	dprintk("I2C_IRQ_PEND      0x%8.8x:0x%4.4x\n", (unsigned int)&(i2c_regs->PEND), (unsigned int)i2c_regs->PEND);
	dprintk("I2C_IRQ_CLKENB    0x%8.8x:0x%4.4x\n", (unsigned int)&(i2c_regs->CLKENB), (unsigned int)i2c_regs->CLKENB);
}


/* pollux_i2c_setup
 *
 * Setup POLLUX I2C hw module
 */

static void pollux_i2c_setup(struct pollux_i2c *i2c)
{
	MES_I2C_OpenModule(MES_I2C0);
	MES_I2C_SetClockPrescaler(MES_I2C0, i2c->clock_source , i2c->clock_scale);
	MES_I2C_SetDataDelay(MES_I2C0, i2c->scl_sda_diff);
	MES_I2C_SetInterruptEnableAll(MES_I2C0, CFALSE);
	MES_I2C_ClearInterruptPendingAll(MES_I2C0);
	MES_I2C_ClearOperationHold(MES_I2C0);

	MES_I2C_SetExtendedIRQEnable (MES_I2C0, CFALSE);
	MES_I2C_SetAckGenerationEnable (MES_I2C0, CFALSE);
}


/* pollux_i2c_wait_ack
 *
 * Wait for the acknowledge signal form I2C device
 *
 * return 0 if acknowledge is received
 */
static int pollux_i2c_wait_ack(struct pollux_i2c *i2c, CBOOL expect_ack)
{
	U32 result = CFALSE;
	CBOOL ack = CTRUE;
	int cnt;

	for(cnt=0; cnt < 100000; cnt++)
	{
		result = MES_I2C_GetInterruptPending(MES_I2C0, 0);
		if(result)
			break;
		cnt++;	
	}
	ack = MES_I2C_IsACKReceived(MES_I2C0);

	if(!result) 
	{
		printk(" Error: Wait Ack Timeout %d, addr:%0x\n", cnt, i2c->msg->addr);
		return CFALSE;
	}
	
	if(ack == CFALSE && expect_ack == CTRUE)
	{
		printk(" Error: Get NACK Response %d, addr:%0x\n", cnt, i2c->msg->addr);
		return CFALSE;		
	}
	return CTRUE;
}

// tansform - |S|SLAVEADDRESS|A|DATA byte|...|A|DATA byte|NA|P|
static int pollux_i2c_read(struct pollux_i2c *i2c)
{
	int ret = 0;
	CBOOL last   = CTRUE;
	struct i2c_msg	*msg;
	U8 id = 0;
	int k;
	int busy_counter;

	// Activate following lines if needed
	MES_I2C_SetClockPClkMode(MES_I2C0, MES_PCLKMODE_ALWAYS); 	// CLKENB: PCLKMODE		
	udelay(10);

	// Set current message buffer according to the index information
	msg = (struct i2c_msg *)(i2c->msg + i2c->msg_idx);
	id = msg->addr << 1;	
	pollux_i2c_setup(i2c);

	dprintk("pollux_i2c_read(Addr:0x%x, DATA:0x%x, Len:%d)\n", msg->addr << 1, (unsigned int)msg->buf, msg->len);    	

	// BUSY check & STOP condition
	busy_counter = 0;
 	while(CTRUE == MES_I2C_IsBusy(MES_I2C0)) {
		if(MAX_BUSY_COUNT <= busy_counter) {
			printk("%s() - busy check count error, before send addr : 0x%02x\n", __func__, msg->addr << 1);
			break;
		}
		busy_counter++;
	}

	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_SLAVE_RX, MES_I2C_SIGNAL_STOP);
    MES_I2C_ClearOperationHold(MES_I2C0);
 	
	i2c_spin_lock_irq(&i2c->lock);
	// START condition & ID transmit
	MES_I2C_WriteByte(MES_I2C0, id);	
	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_MASTER_RX, MES_I2C_SIGNAL_START);

	// ACK check
 	if(!pollux_i2c_wait_ack(i2c, CTRUE)) {
		i2c_spin_unlock_irq(&i2c->lock);
		goto HWI2C_READ_FAIL;
	}

	for (k=0; k<msg->len; k++)
	{
		if(k+1 == msg->len)	last = CFALSE; 
		// DATA receive
		MES_I2C_SetAckGenerationEnable(MES_I2C0, last);			// ICCR: ACK_GEN-Not ACK, last Data		
 		MES_I2C_ClearInterruptPending(MES_I2C0, 0);				// IRQ PEND[0]: clear interrupt pend 	
 		MES_I2C_ClearOperationHold(MES_I2C0);					// IRQ PEND[1]: clear operatin hold
 		if(! pollux_i2c_wait_ack(i2c, CFALSE)) {
			i2c_spin_unlock_irq(&i2c->lock);
 			goto HWI2C_READ_FAIL;
 		}

		msg->buf[k] = MES_I2C_ReadByte(MES_I2C0);				// IDSR: READ DATA
	}	
	i2c_spin_unlock_irq(&i2c->lock);

	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_MASTER_RX, MES_I2C_SIGNAL_STOP);	// ISSR: ST_ENB, MASTER_TX, TX_ENB, ST_BUSY(STOP)
 	MES_I2C_ClearInterruptPending(MES_I2C0, 0);										// IRQ PEND[0]: clear interrupt pend
 	MES_I2C_ClearOperationHold(MES_I2C0);											// IRQ PEND[1]: clear operatin hold

	// BUSY check	 		 
	busy_counter = 0;
 	while(CTRUE == MES_I2C_IsBusy(MES_I2C0)) {
		if(MAX_BUSY_COUNT <= busy_counter) {
			printk("%s() - busy check count error, after send addr : 0x%02x\n", __func__, msg->addr << 1);
			break;
		}
		busy_counter++;
	}

	MES_I2C_SetClockPClkMode(MES_I2C0, MES_PCLKMODE_DYNAMIC); 	// CLKENB: Disable	
	MES_I2C_BusDisable(MES_I2C0);

	return ret;
	
HWI2C_READ_FAIL:

	// STOP	condition		
	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_MASTER_RX, MES_I2C_SIGNAL_STOP);	// ISSR: ST_ENB, MASTER_TX, TX_ENB, ST_BUSY(STOP)
 	MES_I2C_ClearInterruptPending(MES_I2C0, 0);										// IRQ PEND[0]: clear interrupt pend
 	MES_I2C_ClearOperationHold(MES_I2C0);											// IRQ PEND[1]: clear operatin hold
	
	// Following lines are already blocked in reference code
	// BUSY check	 		 
 	// while(CTRUE == MES_I2C_IsBusy());

	MES_I2C_SetClockPClkMode(MES_I2C0, MES_PCLKMODE_DYNAMIC); 	// CLKENB: Disable
	MES_I2C_BusDisable(MES_I2C0);

	ret = -1;		
	return ret;
}


// tansform - |S|SLAVEADDRESS|A|DATA byte|A|P|
static int pollux_i2c_write(struct pollux_i2c *i2c)
{
	int ret = 0;
	struct i2c_msg	*msg;
	U8 id = 0;
	int k;
	int busy_counter;

	// Activate following lines if needed
	MES_I2C_SetClockPClkMode(MES_I2C0, MES_PCLKMODE_ALWAYS); 	// CLKENB: PCLKMODE		
	udelay(10);

	// Set current message buffer according to the index information
	msg = (struct i2c_msg *)(i2c->msg + i2c->msg_idx);
	id = msg->addr << 1;	
	pollux_i2c_setup(i2c);
	
	dprintk("pollux_i2c_write(Addr:0x%x, DATA:0x%x, Len:%d)\n", msg->addr<<1, (unsigned int)msg->buf, msg->len);    	

	MES_I2C_SetAckGenerationEnable(MES_I2C0, CTRUE);

	// BUSY check & STOP condition
	busy_counter = 0;
 	while(CTRUE == MES_I2C_IsBusy(MES_I2C0)) {
		if(MAX_BUSY_COUNT <= busy_counter) {
			printk("%s() - busy check count error, before send addr : 0x%02x\n", __func__, msg->addr << 1);
			break;
		}
		busy_counter++;
	}
 	
	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_SLAVE_RX, MES_I2C_SIGNAL_STOP);	// ISSR: ST_ENB, MASTER_TX, TX_ENB, ST_BUSY(STOP)
    MES_I2C_ClearOperationHold (MES_I2C0);

	// For Test
	//dump_i2c_registers(i2c);
	
	i2c_spin_lock_irq(&i2c->lock);
	// START condition & ID transmit
	MES_I2C_WriteByte(MES_I2C0, id);													// IDSR: WRITE_ID address 	
	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_MASTER_TX, MES_I2C_SIGNAL_START);	// ISSR: ST_ENB, MASTER_TX, TX_ENB, ST_BUSY(STOP)
	
	// ACK check
	if(! pollux_i2c_wait_ack(i2c, CTRUE)) {
		i2c_spin_unlock_irq(&i2c->lock);
		goto HWI2C_WRITEB_FAIL;
	}

	for (k=0; k<msg->len; k++)
	{
 		// DATA transmit
	 	MES_I2C_WriteByte(MES_I2C0, msg->buf[k]);						// IDSR: Data address	 	
	 	MES_I2C_ClearInterruptPending(MES_I2C0, 0);					// IRQ PEND[0]: clear interrupt pend 		 	
	 	MES_I2C_ClearOperationHold(MES_I2C0);						// IRQ PEND[1]: clear operatin hold 
 	
		// ACK check
	 	if(! pollux_i2c_wait_ack(i2c, CTRUE)) {
			i2c_spin_unlock_irq(&i2c->lock);
 			goto HWI2C_WRITEB_FAIL;
 		}
 	}
 	
	i2c_spin_unlock_irq(&i2c->lock);
	// STOP	condition	
	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_MASTER_TX, MES_I2C_SIGNAL_STOP);	// ISSR: ST_ENB, MASTER_TX, TX_ENB, ST_BUSY(STOP)
 	MES_I2C_ClearInterruptPending(MES_I2C0, 0);										// IRQ PEND[0]: clear interrupt pend
 	MES_I2C_ClearOperationHold(MES_I2C0);											// IRQ PEND[1]: clear operatin hold
 	
	// BUSY check	 		 
	busy_counter = 0;
 	while(CTRUE == MES_I2C_IsBusy(MES_I2C0)) {
		if(MAX_BUSY_COUNT <= busy_counter) {
			printk("%s() - busy_count error, addr : 0x%02x\n", __func__, msg->addr << 1);
			break;
		}
		busy_counter++;
	}

	MES_I2C_BusDisable(MES_I2C0);
	// Activate following line if needed
	MES_I2C_SetClockPClkMode(MES_I2C0, MES_PCLKMODE_DYNAMIC); 	// CLKENB: Disable

	return ret;		
	
HWI2C_WRITEB_FAIL:
	
	// STOP	condition	
	MES_I2C_ControlMode(MES_I2C0, MES_I2C_TXRXMODE_MASTER_TX, MES_I2C_SIGNAL_STOP);	// ISSR: ST_ENB, MASTER_TX, TX_ENB, ST_BUSY(STOP)
 	MES_I2C_ClearInterruptPending(MES_I2C0, 0);										// IRQ PEND[0]: clear interrupt pend
 	MES_I2C_ClearOperationHold(MES_I2C0);											// IRQ PEND[1]: clear operatin hold
 	
	// Following lines are already blocked in reference code
	busy_counter = 0;
 	while(CTRUE == MES_I2C_IsBusy(MES_I2C0)) {
		if(MAX_BUSY_COUNT <= busy_counter) {
			printk("%s() - busy_count error, addr : 0x%02x\n", __func__, msg->addr << 1);
			break;
		}
		busy_counter++;
	}

	MES_I2C_BusDisable(MES_I2C0);
	// Activate following line if needed
	MES_I2C_SetClockPClkMode(MES_I2C0, MES_PCLKMODE_DYNAMIC); 	// CLKENB: Disable

	ret = -1;		
	return ret;
}


/* pollux_i2c_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
 */

static int pollux_i2c_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
	struct pollux_i2c *i2c = (struct pollux_i2c *)adap->algo_data;
	int i;
	int ret;
	struct i2c_msg *cur_msg;

	dprintk("pollux_i2c_xfer: processing %d messages:\n", num);

	i2c->msg = msgs;

	for (i = 0; i < num; i++) {
		cur_msg = msgs + i;
		dprintk(" #%d: %sing %d byte%s %s 0x%02x\n", i,
			cur_msg->flags & I2C_M_RD ? "read" : "writ",
			cur_msg->len, cur_msg->len > 1 ? "s" : "",
			cur_msg->flags & I2C_M_RD ? "from" : "to",	cur_msg->addr<<1);

		dprintk("Flags: %x\n", cur_msg->flags);

		i2c->msg_num = i;
		i2c->msg_idx = i;

		if (cur_msg->len && cur_msg->buf) {	/* sanity check */
			if (cur_msg->flags & I2C_M_RD)
				ret = pollux_i2c_read(i2c);
			else
				ret = pollux_i2c_write(i2c);

			if (ret)
				return ret;

			/* Wait until transfer is finished */
		}
		dev_dbg(&adap->dev, "transfer complete\n");
	}
	return i;
}


/* pollux_i2c_init
 *
 * initialise the controller, set the IO lines and frequency 
 */

static int pollux_i2c_init(struct pollux_i2c *i2c)
{
	/*
	 * Initialize I2C controller prototype 
	 */
	dprintk("pollux_i2c_init\n");
	MES_I2C_Initialize();	// This initializes all channels

	MES_I2C_SetBaseAddress(MES_I2C0, (U32)i2c->regs);

	i2c->clock_source = TXCLKSRC_PCLK256;   
	i2c->clock_scale = 1;                 //(( PCLK(62100000) / TXCLKSRC_PCLK256) / 2 ) = 121.28khz(8us)
	//i2c->clock_scale = 0;               //(( PCLK(62100000) / TXCLKSRC_PCLK256)) = 242.57khz(16us)
	//i2c->clock_scale = 16;              //(( PCLK(62100000) / TXCLKSRC_PCLK256) / 17) = 14.26khz
	i2c->scl_sda_diff = 4;                  

	MES_I2C_OpenModule(MES_I2C0);
	MES_I2C_SetClockPrescaler(MES_I2C0, i2c->clock_source , i2c->clock_scale);
	MES_I2C_SetDataDelay(MES_I2C0, i2c->scl_sda_diff);                            
	MES_I2C_SetInterruptEnableAll(MES_I2C0, CFALSE);
	MES_I2C_SetExtendedIRQEnable(MES_I2C0, CFALSE);
	MES_I2C_ClearInterruptPendingAll(MES_I2C0);

	dump_i2c_registers(i2c);
	
	return 0;
}


static void pollux_i2c_free(struct pollux_i2c *i2c)
{
}


/* declare our i2c functionality */
static u32 pollux_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}


/* i2c bus registration info */

static const struct i2c_algorithm pollux_i2c_algorithm = {
	.master_xfer		= pollux_i2c_xfer,
	.functionality		= pollux_i2c_func,
};

static struct pollux_i2c pollux_i2c = {
	.lock	= SPIN_LOCK_UNLOCKED,
	.wait	= __WAIT_QUEUE_HEAD_INITIALIZER(pollux_i2c.wait),
	.adap	= {
		.name			= POLLUX_I2C_NAME,
		.owner			= THIS_MODULE,
		.algo			= &pollux_i2c_algorithm,
		.retries		= 2,
		.class			= I2C_CLASS_HWMON,
	},
};


/* mp2350f_i2c_probe
 *
 * called by the bus driver when a suitable device is found
*/

static int pollux_i2c_probe(struct platform_device *pdev)
{
	struct pollux_i2c *i2c = &pollux_i2c;
	struct resource *res;
	int ret;

	/* find the clock and enable it */

	i2c->dev = &pdev->dev;

	/* map the registers */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "cannot find IO resource\n");
		ret = -ENOENT;
		goto out;
	}

	i2c->ioarea = request_mem_region(res->start, (res->end-res->start)+1,
					 pdev->name);

	if (i2c->ioarea == NULL) {
		dev_err(&pdev->dev, "cannot request IO\n");
		ret = -ENXIO;
		goto out;
	}

	i2c->regs = ioremap(res->start, (res->end-res->start)+1);

	if (i2c->regs == NULL) {
		dev_err(&pdev->dev, "cannot map IO\n");
		ret = -ENXIO;
		goto out;
	}

	dprintk(KERN_INFO "registers %p (%p, %p)\n", i2c->regs, i2c->ioarea, res);

	/* setup info block for the i2c core */

	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;

	/* initialize the i2c controller */

	ret = pollux_i2c_init(i2c);
	if (ret != 0)
		goto out;

	/* find the IRQ for this unit (note, this relies on the init call to
	 * ensure no current IRQs pending 
	 */

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "cannot find IRQ\n");
		ret = -ENOENT;
		goto out;
	}

	ret = request_irq(res->start, pollux_i2c_irq, IRQF_DISABLED,
			  pdev->name, i2c);

	if (ret != 0) {
		dev_err(&pdev->dev, "cannot claim IRQ\n");
		goto out;
	}

	i2c->irq = res;
	//dev_dbg(&pdev->dev, "irq resource %p (%ld)\n", res, res->start);

	ret = i2c_add_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add bus to i2c core\n");
		goto out;
	}

	platform_set_drvdata(pdev, i2c);
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
	dev_info(&pdev->dev, "%s: POLLUX I2C adapter\n", i2c->adap.dev.bus_id);
#endif
	i2c_spin_lock_init(&i2c->lock);
//	printk("GPIO A 0x%8.8x\n", *(volatile unsigned int *)(VIR_BASEADDR_GPIOA + 0x64));

 out:
	if (ret < 0)
		pollux_i2c_free(i2c);

	return ret;
}

/* pollux_i2c_remove
 *
 * called when device is removed from the bus
*/

static int pollux_i2c_remove(struct platform_device *pdev)
{
	struct pollux_i2c *i2c = platform_get_drvdata(pdev);
	
	if (i2c != NULL) {
		pollux_i2c_free(i2c);
		platform_set_drvdata(pdev, NULL);
	}

	return 0;
}


#ifdef CONFIG_PM
static int pollux_i2c_resume(struct platform_device *dev)
{
	struct pollux_i2c *i2c = platform_get_drvdata(dev);

	if (i2c != NULL)
		pollux_i2c_init(i2c);

	return 0;
}

#else
#define pollux_i2c_resume NULL
#endif


/* device driver for platform bus bits */

static struct platform_driver pollux_i2c_driver = {
	.probe		= pollux_i2c_probe,
	.remove		= pollux_i2c_remove,
	.resume		= pollux_i2c_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= POLLUX_I2C_NAME,
	},
};


static struct resource pollux_i2c_resource[] = {
    [0] = {
        .start = PHY_BASEADDR_I2C0,
        .end   = PHY_BASEADDR_I2C0 + 0x200,
        .flags = IORESOURCE_MEM,
    },  
    [1] = {
        .start = IRQ_IIC0,
        .end   = IRQ_IIC0,
        .flags = IORESOURCE_IRQ,
    } 
};


static struct platform_device pollux_i2c_device = {
	.name	= POLLUX_I2C_NAME,
	.id	= 0,	// -1: set 'platform_device->device.bus_id' with 'platform_device->name' &  strlcpy(,,BUS_ID_SIZE)
			//  0: set 'platform_device->device.bus_id' with 'platform_device->name' &  snprintf
	.num_resources = ARRAY_SIZE(pollux_i2c_resource),
	.resource = pollux_i2c_resource,
	
};


static int __init i2c_adap_pollux_init(void)
{
	int ret;

#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
	printk(KERN_INFO "MagicEyes POLLUX I2C Bus driver\n");
#endif

	/* register platform device */
	ret = platform_device_register(&pollux_i2c_device);
	if(ret){
		printk(KERN_WARNING "failed to add platform device %s (%d) \n", pollux_i2c_device.name, ret);
		return ret;
	}

	/* register platform driver */
	ret = platform_driver_register(&pollux_i2c_driver);
	if (ret){
		printk(KERN_WARNING "failed to add platrom driver %s (%d) \n", pollux_i2c_driver.driver.name, ret);
	}

	return ret;
}


static void __exit i2c_adap_pollux_exit(void)
{
	platform_driver_unregister(&pollux_i2c_driver);
}

module_init(i2c_adap_pollux_init);
module_exit(i2c_adap_pollux_exit);

MODULE_DESCRIPTION("POLLUX I2C Bus driver");
MODULE_AUTHOR("Jonathan, <jonathan@mesdigital.com>");
MODULE_LICENSE("GPL");

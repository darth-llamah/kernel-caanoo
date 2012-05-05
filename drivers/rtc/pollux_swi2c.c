/*
 *  MMSP2 Software I2C Driver using GPIO
 *  Copyright (C) 2004,2005 DIGNSYS Inc. (www.dignsys.com)
 *  Kane Ahn < hbahn@dignsys.com >
 *  hhsong < hhsong@dignsys.com >
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/arch/regs-gpio.h>
#include <linux/delay.h>

#define GPIO_SCL    POLLUX_GPA28
#define GPIO_SDA    POLLUX_GPA29

#define SCCB_SCL_INPUT	    pollux_gpio_set_inout(GPIO_SCL, POLLUX_GPIO_INPUT_MODE)
#define SCCB_SCL_OUTPUT	    pollux_gpio_set_inout(GPIO_SCL, POLLUX_GPIO_OUTPUT_MODE)
#define SCCB_SCL_LOW	    pollux_gpio_setpin(GPIO_SCL, 0)
#define SCCB_SCL_HIGH	    pollux_gpio_setpin(GPIO_SCL, 1)
#define SCCB_SCL_DATA	    pollux_gpio_getpin(GPIO_SCL)
                                
#define SCCB_SDA_INPUT	    pollux_gpio_set_inout(GPIO_SDA, POLLUX_GPIO_INPUT_MODE)
#define SCCB_SDA_OUTPUT	    pollux_gpio_set_inout(GPIO_SDA, POLLUX_GPIO_OUTPUT_MODE)
#define SCCB_SDA_LOW	    pollux_gpio_setpin(GPIO_SDA, 0)
#define SCCB_SDA_HIGH	    pollux_gpio_setpin(GPIO_SDA, 1)
#define SCCB_SDA_DATA	    pollux_gpio_getpin(GPIO_SDA)


int i2c_set;


int I2C_failed(void);
int I2C_wbyte( unsigned char id, unsigned char addr, unsigned char data );
int I2C_rbyte( unsigned char id, unsigned char addr);


int	I2C_wbyte( unsigned char id, unsigned char addr, unsigned char data ) 
{
	int i;

	SCCB_SDA_HIGH;
	SCCB_SCL_HIGH;
	SCCB_SCL_OUTPUT;
	SCCB_SDA_OUTPUT;
	mdelay(1);

	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;

	mdelay(1);
	
    // START
	SCCB_SDA_LOW;
	mdelay(1);	
	SCCB_SCL_LOW;
    
	// Write Slave ID
	for( i=7 ; i>=0 ; i-- )
	{
		if( id & (1<<i) )			SCCB_SDA_HIGH;
		else						SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	
	if( SCCB_SDA_DATA )		
	{
		printk("I2C_wbyte : No Ack at id(0x%02X) transfer\r\n", id );
		I2C_failed();
	}
	mdelay(1);	
	
	// Write Slave Address
	SCCB_SCL_LOW;
	SCCB_SDA_OUTPUT;
	for( i=7 ; i>=0 ; i-- )
	{
		if( addr & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_wbyte : No Ack at addr(0x%02X) transfer\r\n", addr );
		I2C_failed();
	}
	mdelay(1);	

	// Write Slave data
	SCCB_SCL_LOW;
	SCCB_SDA_OUTPUT;
	for( i=7 ; i>=0 ; i-- )
	{
		if( data & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_wbyte : No Ack at data(0x%02X) transfer\r\n", data );
		I2C_failed();
	}
	mdelay(1);	
	
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;
	
	SCCB_SCL_INPUT;
	SCCB_SDA_INPUT;
	
	return 1;
}


//------------------------------------------------------------------------------
int I2C_rbyte( unsigned char id, unsigned char addr)
{
	unsigned char temp;
	int i;
	
	SCCB_SDA_HIGH;
	SCCB_SCL_HIGH;
	SCCB_SCL_OUTPUT;
	SCCB_SDA_OUTPUT;
	mdelay(1);

	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;

	mdelay(1);
	
	// START
	SCCB_SDA_LOW;
	mdelay(1);	
	SCCB_SCL_LOW;
	
	// Write Slave ID
	for( i=7 ; i>=0 ; i-- )
	{
		if( id & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )		
	{
		printk("I2C_rbyte : No Ack at id(0x%02X) transfer\r\n", id );
		I2C_failed();
	}
	mdelay(1);	

	// Write Slave Address
	SCCB_SCL_LOW;
	SCCB_SDA_OUTPUT;
	for( i=7 ; i>=0 ; i-- )
	{
		if( addr & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_rbyte : No Ack at addr(0x%02X) transfer\r\n", addr );
		I2C_failed();
	}
	mdelay(1);	
	
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	SCCB_SDA_OUTPUT;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;

	mdelay(1);
	
	// START
	SCCB_SDA_LOW;
	mdelay(1);	
	SCCB_SCL_LOW;

	// Write Slave ID
	id |= 1;
	for( i=7 ; i>=0 ; i-- )
	{
		if( id & (1<<i) )	SCCB_SDA_HIGH;
		else				SCCB_SDA_LOW;
		mdelay(2);	
		SCCB_SCL_HIGH;
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_INPUT;
	mdelay(1);	
	SCCB_SCL_HIGH;
	if( SCCB_SDA_DATA )
	{
		printk("I2C_rbyte : No Ack at Repeated Start(0x%02X) transfer\r\n", id );
		I2C_failed();
	}
	mdelay(1);	
	
	// Read Data
	SCCB_SCL_LOW;
	temp = 0;
	for( i=7 ; i>=0 ; i-- )
	{
		mdelay(2);
		SCCB_SCL_HIGH;
		if( SCCB_SDA_DATA )	temp |= (1<<i);
		else				temp |= (0<<i);
		mdelay(2);
		SCCB_SCL_LOW;
	}
	SCCB_SDA_HIGH;		// No ACK
	SCCB_SDA_OUTPUT;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	mdelay(1);
	SCCB_SCL_HIGH;
	mdelay(1);
	SCCB_SDA_HIGH;
	
	SCCB_SCL_INPUT;
	SCCB_SDA_INPUT;
	
    return temp;
}

int I2C_failed(void)
{
	// STOP	
	SCCB_SCL_LOW;
	SCCB_SDA_LOW;
	SCCB_SDA_OUTPUT;
	mdelay(5);
	SCCB_SCL_HIGH;
	mdelay(5);
	SCCB_SDA_HIGH;
	
	SCCB_SCL_INPUT;
	SCCB_SDA_INPUT;		
	
	return 0;
}


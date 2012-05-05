 /*
  * linux/drivers/input/joystick/ad7993.c
  *
  * Copyright (C) 2007-2008 Avionic Design Development GmbH
  * Copyright (C) 2008-2009 Avionic Design GmbH
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  *
  * Written by Thierry Reding <thierry.reding@xxxxxxxxxxxxxxxxx>
  */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <asm/arch/regs-gpio.h>
#include <linux/sysfs.h>
//#include <linux/time.h>

#define GPIO_HOLD_KEY                   POLLUX_GPA10
#define DRIVER_NAME "pollux-analog"
#define DRIVER_VERSION  "1"

//#define DEBUG_ON
#ifdef DEBUG_ON
#define dprintk(x...) printk(x)
#else
#define dprintk(x...) //do nothing
#endif


#define USE_CPU_ADC
#ifdef USE_CPU_ADC
static int pollux_adc_value(int channel);
#define ADC_CHANNEL_X 6
#define ADC_CHANNEL_Y 5
#endif
static struct platform_device * ad7993_device;
static char pollux_analog_name[] = DRIVER_NAME;

#ifndef USE_CPU_ADC
/* Addresses to scan */
static unsigned short normal_i2c[] = { 0x21 , I2C_CLIENT_END };			// AD7993-0
//static unsigned short normal_i2c_2[] = { 0x23 , I2C_CLIENT_END };		// AD7993-1
/* I2C Magic */
I2C_CLIENT_INSMOD;

/* register address */
#define CONVERSION_RESULT_REGISTER	0x0		// Read
#define ALERT_STATUS_REGISTER		0x1		// Read/Write
#define CONFIGURATION_REGISTER		0x2		// Read/Write
#define CYCLE_TIMER_REGISTER		0x3		// Read/Write
#define DATA_LOW_REG_CH1			0x4		// Read/Write
#define DATA_HIGH_REG_CH1			0x5		// Read/Write
#define HYSTERESIS_REG_CH1			0x6		// Read/Write
#define DATA_LOW_REG_CH2			0x7		// Read/Write
#define DATA_HIGH_REG_CH2			0x8		// Read/Write
#define HYSTERESIS_REG_CH2			0x9		// Read/Write
#define DATA_LOW_REG_CH3			0xA		// Read/Write
#define DATA_HIGH_REG_CH3			0xB		// Read/Write
#define HYSTERESIS_REG_CH3			0xC		// Read/Write
#define DATA_LOW_REG_CH4			0xD		// Read/Write
#define DATA_HIGH_REG_CH4			0xE		// Read/Write
#define HYSTERESIS_REG_CH4			0xF		// Read/Write

/* basic commands */
#define CMD_NO_CHANNEL						0x00
#define CONVERT_ON_VIN_1					0x10    // N35 Y값 받기
#define CONVERT_ON_VIN_2					0x20
#define CONVERT_ON_VIN_1_VIN_2				0x30
#define CONVERT_ON_VIN_3					0x40	// N35 X값 받기
#define CONVERT_ON_VIN_1_VIN_3				0x50	// N35 (X, Y 값 동시에 받기)
#define CONVERT_ON_VIN_2_VIN_3				0x60
#define CONVERT_ON_VIN_1_VIN_2_VIN_3		0x70
#define CONVERT_ON_VIN_4					0x80
#define CONVERT_ON_VIN_1_VIN_4				0x90
#define CONVERT_ON_VIN_2_VIN_4				0xA0
#define CONVERT_ON_VIN_1_VIN_2_VIN_4		0xB0
#define CONVERT_ON_VIN_3_VIN_4				0xC0
#define CONVERT_ON_VIN_1_VIN_3_VIN_4		0xD0
#define CONVERT_ON_VIN_2_VIN_3_VIN_4		0xE0
#define CONVERT_ON_VIN_1_VIN_2_VIN_3_VIN_4	0xF0
#define FLTR_ENABLE							0x08	// 필터링 기능
#define ALERT_ENABLE						0x04	// ALERT/BUSY핀 연결시...
#define BUSY_ALERT							0x02    // ALERT/BUSY핀 연결시...
#define BUSY_ALERT_POLARITY					0x01    // ALERT/BUSY핀 연결시...
#endif

/* periodic polling delay and period */
#define JOYSTIC_SAMPLING_INTERVAL (20 * 1000000)  //20ms
#ifdef CONFIG_AD7993
  #define JOYSTIC_INIT_DELAY_S		(5)
  #define JOYSTIC_INIT_DELAY        ktime_set(JOYSTIC_INIT_DELAY_S, 0)
#else /* (CONFIG_AD7993_MODULE == 1) */ //Compile As a Module... 
  //#define JOYSTIC_INIT_DELAY_MS    (50 * 1000000)   
  #define JOYSTIC_INIT_DELAY_MS    (50 * 1000000)   
  #define JOYSTIC_INIT_DELAY        ktime_set(0, JOYSTIC_INIT_DELAY_MS)
#endif

//Key Definition.. 
#define N35_BTN_LEFT		0x124
#define N35_BTN_RIGHT		0x125
#define N35_BTN_A 			0x120
#define N35_BTN_B			0x122
#define N35_BTN_X			0x121
#define N35_BTN_Y			0x123
#define N35_BTN_I			0x128
//#define N35_BTN_I			0x12B
#define N35_BTN_II			0x129
#define N35_BTN_HOME		0x126
#define N35_BTN_ANALOG		0x12A
#define N35_BTN_HOLD		0x127



/**
 * struct ad7993_event - analog joystick event structure
 * @x:      X-coordinate of the event
 * @y:      Y-coordinate of the event
 */
struct ad7993_event {
	short x;
	short y;
};

/**
 * struct ad7993 - analog joystick controller context
 * @client: I2C client
 * @input:  analog joystick input device
 * @lock:   lock for resource protection
 * @timer:  timer for periodical polling
 * @work:   workqueue structure
 * @event:  current touchscreen eventA
 * @cal:    A
 * @pdata:  platform-specific information
 */
struct ad7993 {
#ifndef USE_CPU_ADC
	struct i2c_client *client;
#endif
	struct input_dev *input;
	spinlock_t lock;
	struct hrtimer timer;
	struct work_struct work;
	struct ad7993_event event;
	struct platform_device  *pdev;
	int timer_running;
};

int calibration_flag = 0;

#ifndef USE_CPU_ADC
static struct i2c_client *save_client;

/**
  * ad7993_read() - send a command and read the response
  * @client: I2C client
  * @command:    command to send
  */
static int ad7993_read(struct i2c_client *client, u8 command)
{
	u8 value[2] = { 0, 0 };
	int size = 2;
	int status;
	int result=0;

	status = i2c_master_send(client, &command, 1);
	if (status < 0)
		return status;

	status = i2c_master_recv(client, value, size);
	if (status < 0)
		return status;

	value[1] = (value[1] >> 2 ) | (value[0] & 0x03) << 6 ;
	value[0] = (value[0] & 0x0F ) >> 2;

	result = ( (value[0] << 8) | value[1] );

	return result;
}

#define DEFAULT_READ_NUM 1	
static int read_adc_value(struct i2c_client * client, u8 cmd)
{
	
	int i;
	int total_value = 0;
	int read_num = 0;
	int read_value = 0;

	for(i = 0; i < DEFAULT_READ_NUM; i++) {
		read_value = ad7993_read( client, cmd );
		if(read_value < 0) {
			i--;
		} else {
			// at least DEFAULT_READ_NUM-times, run below routine...
			read_num++;
			total_value += read_value;
		}
	}
	total_value /= read_num;

	return total_value;
}

/**
 * ad7993_read_x() - read X-coordinate
 * @client: I2C client
 * @cmd:    command
 */
static int ad7993_read_x(struct i2c_client *client, u8 cmd)
{
	return read_adc_value(client, CONVERT_ON_VIN_3 );
}

/**
 * ad7993_read_y() - read Y-coordinate
 * @client: I2C client
 * @cmd:    command
 */
static int ad7993_read_y(struct i2c_client *client, u8 cmd)
{
	return read_adc_value( client, CONVERT_ON_VIN_1 );
}

/**
 * ad7993_read_xy() - read X & Y-coordinate
 * @client: I2C client
 * @cmd:    command
 */
static int ad7993_read_xy(struct i2c_client *client, u8 cmd)
{
	return ad7993_read( client, CONVERT_ON_VIN_1_VIN_3 );
}

/**
 * ad7993_write() - send a command 
 * @client: I2C client
 * @command:    command to send
 */
static int ad7993_write(struct i2c_client *client, u8 command)
{
	int status;

	status = i2c_master_send(client, &command, 1);
	if (status < 0)
		return status;

	return 0;
}


static int ad7993_i2c_detect (struct i2c_adapter *adapter, int address, int kind);
static int ad7993_i2c_attach_adapter(struct i2c_adapter *adapter);
static int ad7993_i2c_detach_client(struct i2c_client *client);

static struct i2c_driver ad7993_driver = {
	.driver = {
		.name   = DRIVER_NAME,
	},
	.attach_adapter = ad7993_i2c_attach_adapter,
	.detach_client  = ad7993_i2c_detach_client,
};

static int ad7993_i2c_attach_adapter(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, ad7993_i2c_detect);
}

static int ad7993_i2c_detach_client(struct i2c_client *client)
{
	int err;

	if ((err = i2c_detach_client(client))) {
		dev_err(&client->dev, "Client deregistration failed, "
				"client not detached.\n");
		return err;
	}

	return 0;
}

static int ad7993_i2c_detect (struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client;
	int err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA
				| I2C_FUNC_I2C | I2C_FUNC_SMBUS_WORD_DATA)) {
			goto exit;
	}

	new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	new_client->addr = address;
	new_client->adapter = adapter;
	new_client->driver = &ad7993_driver;
	new_client->flags = 0;

	/* try a command, see if we get an ack; if we do, assume it's our device */
	if ((err = i2c_attach_client(new_client)))
		goto exit_free;

#ifdef CONFIG_AD7993
	printk(KERN_INFO "Analog joystick attached. ");
#else //CONFIG_AD7993_MODULE
	printk(KERN_INFO "Analog joystick module attached. ");
#endif

#ifdef USE_CPU_ADC
	printk(KERN_INFO "using internal adc\n");
#else 
	printk(KERN_INFO "using AD7993, Address: 0x%x\n", address);
#endif

	save_client = new_client;

	return 0;

exit_free:
	kfree(new_client);
exit:
	return err;
}

#endif

/**
 * ad7993_timer() - timer callback function
 * @timer:  timer that caused this function call
 */
static enum hrtimer_restart ad7993_timer(struct hrtimer *timer)
{
	struct ad7993 *ad = container_of(timer, struct ad7993, timer);
	if(ad->timer_running) {
		schedule_work(&ad->work);
		hrtimer_start(&ad->timer, ktime_set(0, JOYSTIC_SAMPLING_INTERVAL), HRTIMER_MODE_REL);
	}
	return HRTIMER_NORESTART;
}

//#define IGNORE_BIT_NUM 1
#define X_AXIS_CHANGE 0x01
#define Y_AXIS_CHANGE 0x02
int x_max = 861;
int x_min = 161;
int x_center = 511;
int y_max = 861;
int y_min = 161;
int y_center = 511;
int sensitivity = 1;
static int is_changed_position(short in_x , short in_y)
{
	int ret_val = 0;
	static short before_x = 0;
	static short before_y = 0;
	short x, y;
	
	if(calibration_flag) {
		return (X_AXIS_CHANGE | Y_AXIS_CHANGE); 
	}

	//x = in_x >> IGNORE_BIT_NUM;
	//y = in_y >> IGNORE_BIT_NUM;
	x = in_x;
	y = in_y;

	if(x != before_x) {
		ret_val |= X_AXIS_CHANGE;
		before_x = x;
	}
	if(y != before_y) {
		ret_val |= Y_AXIS_CHANGE;
		before_y = y;
	}

	return ret_val;
}

/*

	input_set_abs_params(ad->input, ABS_X, 161, 861, 1, 511);
	input_set_abs_params(ad->input, ABS_Y, 161, 861, 1, 511);

 */
#define OUTPUT_CENTER (0x80)
#define CENTER_ADC_OFFSET (100)
//#define CENTER_ADC_OFFSET (70)
#define OUTPUT_MINIMUM (0x00)
#define OUTPUT_MAXIMUM (0xFF)
int apply_calibration(int axis, int adc_value)
{
	int min, max, center_pos, fuzz;
	int mask;
	//int negative_flag = 0;
	int x_axis_max;
	int x_axis_in;
	int y_axis_max;
	int ret_val;
	int offset;
	
	if(calibration_flag) 
		return adc_value;

	fuzz = sensitivity;
	mask = (1 << fuzz) - 1;
	if(axis == ABS_X) {
		max = x_max;
		min = x_min;
		center_pos = x_center;
	} else if (axis == ABS_Y) {
		max = y_max;
		min = y_min;
		center_pos = y_center;
	} else {
		// wrong value... 
		dprintk("wrong axis input...!! \n");
		return OUTPUT_CENTER;
	}

	dprintk("min:%d, max:%d, center_pos:%d, fuzz:%d, adc_value:%d\n", min, max, center_pos, fuzz, adc_value);
	
	//check limit value
	if(adc_value < (min + 10 ) ) {
		dprintk("adc low\n");
		return OUTPUT_MINIMUM;
	}
	else if((max - 10) < adc_value) {
		dprintk("adc max\n");
		return OUTPUT_MAXIMUM;
	}else if ((adc_value <= (center_pos + CENTER_ADC_OFFSET)) && 
			((center_pos - CENTER_ADC_OFFSET) <= adc_value)) {	//check center position;
		return OUTPUT_CENTER;
	}

	//convert real-adc to logical coordination
	if(adc_value < center_pos) {
		x_axis_max = (center_pos - CENTER_ADC_OFFSET) - min;
		x_axis_in = adc_value - min;
		offset = OUTPUT_MINIMUM;
		y_axis_max = OUTPUT_CENTER - OUTPUT_MINIMUM;	
	} else {
		x_axis_max = max - (center_pos + CENTER_ADC_OFFSET) ;
		x_axis_in = adc_value - (center_pos + CENTER_ADC_OFFSET);
		offset = OUTPUT_CENTER;
		y_axis_max = OUTPUT_MAXIMUM - OUTPUT_CENTER;
	}

	ret_val = (x_axis_in * y_axis_max) / x_axis_max + offset;
	ret_val &= (~mask);

	dprintk("%s() ret_val: %d\n", __func__, ret_val);
	return ret_val;
}

#ifdef USE_CPU_ADC
#define READ_COUNT 3
int read_internal_adc(int channel) 
{
	int i;
	int min = 0;
	int max = 0;
	int sum = 0;
	int read_buff[READ_COUNT];

	for(i = 0; i < READ_COUNT; i++) {
		read_buff[i] = pollux_adc_value(channel);
		if(i == 0) {
			min = max = read_buff[0];
		} else {
			if(read_buff[i] < min) min = read_buff[i];
			else if(max < read_buff[i]) max = read_buff[i];
		}
		sum += read_buff[i];
	}

	sum -= (min + max);
	sum /= (READ_COUNT - 2);

	return sum;
}
#endif 

/**
 * ad7993_work() - work queue handler (initiated by the interrupt handler)
 * @work:   work queue to handle
 */
static void ad7993_work(struct work_struct *work)
{
	struct ad7993 *ad = container_of(work, struct ad7993, work);
	unsigned int read_x_val;
	unsigned int read_y_val;
	int hold_key_val;
#ifndef USE_CPU_ADC
	static int ad7993_setting = 0;
	int err;
#endif
	int axis_change_result = 0;
#if 0
#define TEST_INTERVAL 	(5 * HZ)
	int i;
	//u64 test_before_time;
	u64 test_after_time;
#endif 

#ifndef USE_CPU_ADC
	if(!ad7993_setting) {
		/* ad7993 Configuration Register Initialization */
		//select 1,3,4 channel, filter enable (ch1 : Y, ch3 : X, ch4 : dsp vol)
		//err = ad7993_write( ad->client, (CONVERT_ON_VIN_1_VIN_3_VIN_4 | FLTR_ENABLE) ); 
		err = ad7993_write( ad->client, (CONVERT_ON_VIN_1_VIN_3 | FLTR_ENABLE) ); 
		if (err < 0) {
			printk("failed to ad7993 write: %d\n",err);
			return;
		} else 	{
			ad7993_setting = 1;
			//return;
		}
#if 0 	/* Just performance test  */
		if(ad7993_setting) {
			//Test ADC sampling vs I2C sampling
			i = 0;
			test_after_time = get_jiffies_64() + TEST_INTERVAL;
			//while(i < 100000) {
			while(1) {
				ad7993_read_x(ad->client, 1);
				i++;
				if(test_after_time <= get_jiffies_64()) 
					break;
			}
			printk("\ni2c read number = %d\n", i);

			i = 0;
			test_after_time = get_jiffies_64() + TEST_INTERVAL;
			//while(i < 100000) {
			while(1) {
				pollux_adc_value(ADC_CHANNEL_X);
				i++;
				if(test_after_time <= get_jiffies_64()) 
					break;
			}
			printk("adc read number = %d\n", i);
			msleep(5000);
			return;
		}
#endif 
	}
#endif 

	hold_key_val = pollux_gpio_getpin(GPIO_HOLD_KEY);
	if(hold_key_val) {	//only report HOLD-key status...
		input_report_key(ad->input, N35_BTN_HOLD, hold_key_val);	// II
		return;	
	}
#ifdef USE_CPU_ADC
	read_x_val = pollux_adc_value(ADC_CHANNEL_X);
	read_y_val = pollux_adc_value(ADC_CHANNEL_Y);		
	//read_x_val = read_internal_adc(ADC_CHANNEL_X);
	//read_y_val = read_internal_adc(ADC_CHANNEL_Y);		
	dprintk("adc (%d, %d)\t", read_x_val, read_y_val);
#else
	read_x_val = ad7993_read_x(ad->client, 1);
	read_y_val = ad7993_read_y(ad->client, 1);
	dprintk("i2c (%d, %d)\n", read_x_val, read_y_val);
#endif /* USE_CPU_ADC */

	ad->event.x = apply_calibration(ABS_X, read_x_val);
	ad->event.y = apply_calibration(ABS_Y, read_y_val);
	axis_change_result = is_changed_position(ad->event.x, ad->event.y);

	if(axis_change_result) {
		//spin_lock_irqsave(&ad->lock, flags);
		if(axis_change_result & Y_AXIS_CHANGE) {
			dprintk("y report..%d\n", ad->event.y);
			input_report_abs(ad->input, ABS_Y, ad->event.y);
		}
		if(axis_change_result & X_AXIS_CHANGE) {
			dprintk("x report..%d\n", ad->event.x);
			input_report_abs(ad->input, ABS_X, ad->event.x);
		}
		input_sync(ad->input);
		//spin_unlock_irqrestore(&ad->lock, flags);
	}
		
	input_report_key(ad->input, N35_BTN_LEFT, pollux_gpio_getpin(POLLUX_GPC2) ? 0 : 1);
	input_report_key(ad->input, N35_BTN_RIGHT, pollux_gpio_getpin(POLLUX_GPC3) ? 0 : 1);
	input_report_key(ad->input, N35_BTN_A, pollux_gpio_getpin(POLLUX_GPC4) ? 0 : 1);
	input_report_key(ad->input, N35_BTN_B, pollux_gpio_getpin(POLLUX_GPC5) ? 0 : 1);
	input_report_key(ad->input, N35_BTN_X, pollux_gpio_getpin(POLLUX_GPC6) ? 0 : 1);
	input_report_key(ad->input, N35_BTN_Y, pollux_gpio_getpin(POLLUX_GPC7) ? 0 : 1);
	input_report_key(ad->input, N35_BTN_HOME, pollux_gpio_getpin(POLLUX_GPB11) ? 0 : 1);	//HOME
	input_report_key(ad->input, N35_BTN_ANALOG, pollux_gpio_getpin(POLLUX_GPA12) ? 0 : 1);  //TAT
	input_report_key(ad->input, N35_BTN_II, pollux_gpio_getpin(POLLUX_GPC0) ? 0 : 1);	// 
	input_report_key(ad->input, N35_BTN_I, pollux_gpio_getpin(POLLUX_GPC1) ? 0 : 1);	// 
	input_report_key(ad->input, N35_BTN_HOLD, hold_key_val);	// II
	
	//input_sync(ad->input);
}

static ssize_t calibration_store(struct device * dev, struct device_attribute * attr,
		                            const char * buffer, size_t count)
{
	sscanf(buffer, "%d %d %d %d %d %d %d 0", 
			&x_max, &x_min, &x_center, &y_max, &y_min, &y_center, &sensitivity);

	if(!x_max || !x_min || !x_center || !y_max || !y_min || !y_center || !sensitivity) {
		calibration_flag = 1;
	} else 
		calibration_flag = 0;

	return count;
}

static ssize_t calibration_show(struct device * dev, struct device_attribute * attr, 
		                            char * buffer)
{
	return sprintf(buffer, "%d %d %d %d %d %d %d 0", 
			x_max, x_min, x_center, y_max, y_min, y_center, sensitivity);
}

/* Attach the sysfs write method  */
DEVICE_ATTR(calibration, S_IWUSR | S_IRUGO, calibration_show, calibration_store);

/* Attribute Descriptor */
static struct attribute * ad7993_attrs[] = { 
	    &dev_attr_calibration.attr,
		    NULL
};

/* Attribute Group */
static struct attribute_group ad7993_attr_group = { 
	    .attrs = ad7993_attrs,
};



static int __init ad7993_probe(struct platform_device *pdev)
{
	struct ad7993 *ad;
	int err = 0;

	ad = kzalloc(sizeof(struct ad7993), GFP_KERNEL);
	if (unlikely(!ad))
		return -ENOMEM;
#ifndef USE_CPU_ADC
	ad->client = save_client;
#endif
	ad->input = input_allocate_device();

	if (!ad->input) {
		err = -ENOMEM;
		goto fail;
	}

	/* setup the input device */
	ad->input->name = pollux_analog_name;
#ifndef USE_CPU_ADC
	ad->input->id.bustype = BUS_I2C;
#else
	ad->input->id.bustype = BUS_HOST;
#endif
	ad->input->id.vendor  = 0xDEAD;
	ad->input->id.product = 0xBEEF;
	ad->input->id.version = 0x0001;

    /* Announce that the analog stick will generate
	   abs coordinates */
	set_bit(EV_ABS, ad->input->evbit);
	set_bit(ABS_X, ad->input->absbit);
	input_set_abs_params(ad->input, ABS_X, 0, 255, 0, 0);
	set_bit(ABS_Y, ad->input->absbit);
	input_set_abs_params(ad->input, ABS_Y, 0, 255, 0, 0);
	
	set_bit(EV_KEY, ad->input->evbit); 		
	set_bit(N35_BTN_LEFT, ad->input->keybit);		//left
	set_bit(N35_BTN_RIGHT, ad->input->keybit);		//right
	set_bit(N35_BTN_A, ad->input->keybit);		//a
	set_bit(N35_BTN_B, ad->input->keybit);		//b
	set_bit(N35_BTN_X, ad->input->keybit);		//x
	set_bit(N35_BTN_Y, ad->input->keybit);		//y
	set_bit(N35_BTN_I, ad->input->keybit);	//I	
	set_bit(N35_BTN_II, ad->input->keybit);	//II
	set_bit(N35_BTN_HOME, ad->input->keybit);	//HOME
	set_bit(N35_BTN_ANALOG, ad->input->keybit);		//TAT
	set_bit(N35_BTN_HOLD, ad->input->keybit);		//HOLD

	spin_lock_init(&ad->lock);
	hrtimer_init(&ad->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ad->timer.function = ad7993_timer;
	INIT_WORK(&ad->work, ad7993_work);

    err = input_register_device(ad->input);
	if (err) {
		printk("failed to register input device: %d\n",err);
		goto fail_irq;
	}
#ifndef USE_CPU_ADC
	i2c_set_clientdata(ad->client, ad);
#endif

	err = 0;
	ad->pdev = pdev;
	platform_set_drvdata(pdev, ad);
#ifndef USE_CPU_ADC
	err = sysfs_create_group(&save_client->dev.kobj, &ad7993_attr_group);
#else
	err = sysfs_create_group(&ad7993_device->dev.kobj, &ad7993_attr_group);
#endif
    if(err) {
		printk("%s - create sysfs group error\n", __func__);
	}
		 
	//Timer 스타트 시점 중요!!
	hrtimer_start(&ad->timer, JOYSTIC_INIT_DELAY, HRTIMER_MODE_REL); 
	ad->timer_running = 1;
	goto out;

fail_irq:
	if (ad) {
		input_free_device(ad->input);
	}
fail:
	kfree(ad);

out:
	return err;
}

static int ad7993_remove(struct platform_device *pdev)
{
	struct ad7993 *ad = platform_get_drvdata(pdev);

	ad->timer_running = 0;
	hrtimer_cancel(&ad->timer);
#ifndef USE_CPU_ADC
	sysfs_remove_group(&ad->client->dev.kobj, &ad7993_attr_group);
#else
	sysfs_remove_group(&ad7993_device->dev.kobj, &ad7993_attr_group);
#endif
	platform_set_drvdata(pdev, NULL);
	input_unregister_device(ad->input);
	input_free_device(ad->input);
#ifndef USE_CPU_ADC
	i2c_del_driver(&ad7993_driver);
#endif

	kfree(ad);
	return 0;
}

static struct platform_driver ad7993_platform_driver = {
	.driver         = {
		.name   = "pollux-analog",
		.owner  = THIS_MODULE,
	},
    .probe  = ad7993_probe,
	.remove = ad7993_remove,
};


static int __init ad7993_init(void)
{
	int res=0;
#ifndef USE_CPU_ADC
	i2c_add_driver(&ad7993_driver);
#endif
	res =  platform_driver_register(&ad7993_platform_driver);
	if(res){
		printk("fail : platrom driver %s (%d) \n", ad7993_platform_driver.driver.name, res);
		return res;
	}
	
	ad7993_device = platform_device_alloc("pollux-analog", 0);
	if(!ad7993_device) {
		printk("%s - platform_device_alloc() fail...\n", __func__);
		return -ENOMEM;
	}
	
	res = platform_device_add(ad7993_device);
	if(res) {
		printk("%s- platform_device_add() fail\n", __func__);
		platform_device_put(ad7993_device);
		platform_driver_unregister(&ad7993_platform_driver);
		return res;
	}

	return 0;
}

/**
 * ad7993_exit() - module cleanup
 */
static void __exit ad7993_exit(void)
{	
	platform_device_unregister(ad7993_device);
	platform_driver_unregister(&ad7993_platform_driver);
}

module_init(ad7993_init);
module_exit(ad7993_exit);

MODULE_AUTHOR("Thierry Reding <thierry.reding@xxxxxxxxxxxxxxxxx>");
MODULE_DESCRIPTION("Analog Device AD7993 I2C Analog Joystick driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);

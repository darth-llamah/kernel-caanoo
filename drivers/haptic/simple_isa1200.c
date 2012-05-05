#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-gpioalv.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
//#include <linux/workqueue.h>
#include <asm/arch/pollux.h>
#include <linux/isa1200.h>
#include <linux/simple_isa1200if.h>
#include <asm/arch/sys/sys.h>

#include "pollux_pwm.h"

//#define SIMPLE_ISA1200_DEBUG
#ifdef SIMPLE_ISA1200_DEBUG
  #define dprintk(x...) printk(x)
#else
  #define dprintk(x...) do {} while(0);
#endif

typedef enum {ISA1200A, ISA1200B} isa1200_index_t;
//typedef enum {CHIP_NOT_DETECTED, CHIP_DETECTED} chip_detect_flag_t;

typedef enum {
	LDO_LEVEL_24V,
	LDO_LEVEL_25V,
	LDO_LEVEL_26V,
	LDO_LEVEL_27V,
	LDO_LEVEL_28V,
	LDO_LEVEL_29V,
	LDO_LEVEL_30V,
	LDO_LEVEL_31V,
	LDO_LEVEL_32V,
	LDO_LEVEL_33V,
	LDO_LEVEL_34V,
	LDO_LEVEL_35V,
	LDO_LEVEL_36V,
} ldo_level_t;

//-----------------------------------------------------------------------------
// devfs, sysfs Related setting.. 
//-----------------------------------------------------------------------------
#define ISA1200_MAJOR	250
#define ISA1200_NAME	"isa1200"
#define POLLUX_HAPTIC_CLASS_NAME "haptic"
#define POLLUX_HAPTIC_NAME	ISA1200_NAME
#define GPIO_HOLD_KEY                   POLLUX_GPA10

//-----------------------------------------------------------------------------
// I2C Related setting.. 
//-----------------------------------------------------------------------------
#define POLLUX_GPIOALV3	3
#define ISA1200_GPIO_HEN	POLLUX_GPA9
#define ISA1200_GPIO_LEN	POLLUX_GPIOALV3

#define I2C_ISA1200A_NAME	"isa1200a"
#define I2C_ISA1200B_NAME	"isa1200b"

#define ISA1200_CHIP_NUM 2
#define ISA1200A_ADDR 0x90
#define ISA1200B_ADDR 0x92

#define I2C_ISA1200A_ADDR (ISA1200A_ADDR >> 1)
#define I2C_ISA1200B_ADDR (ISA1200B_ADDR >> 1)

int ldo_level_table[] = {0x09, 0x0a, 0x0b, 0x0c, 0x0d,0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
#define DEFAULT_LDO_LEVEL	LDO_LEVEL_33V

//-----------------------------------------------------------------------------
// PWM Related setting.. 
//-----------------------------------------------------------------------------
extern void    MES_PWM_SetPeriod( U32 channel, U32 period );
extern void    MES_PWM_SetDutyCycle( U32 channel, U32 duty );
extern void    MES_PWM_SetPreScale( U32 channel, U32 prescale );
extern void    MES_PWM_SetPolarity( U32 channel, CBOOL bypass );

enum _polarity { POLARITY_INVERT = 0, POLARITY_BYPASS };
#define POLLUX_ISA1200_PWM_CH 1
#define DEFAULT_POLARITY	POLARITY_BYPASS
#define PWM_PERIOD_VALUE	100
#define DUTY_MAX_VALUE		(PWM_PERIOD_VALUE - 1)
#define DEFAULT_DUTY_VALUE	(DUTY_MAX_VALUE / 2) 		// 50%

#define DEFAULT_VIB_LEVEL  VIB_LEVEL_MAX
static vibration_level_t vib_level = DEFAULT_VIB_LEVEL;
static int isa1200_attach(struct i2c_adapter * adap);
static int isa1200_detach(struct i2c_client * client);
void isa1200_pwm_init(void);
void timer_over(unsigned long data);
void register_timer(void);
void pattern_job_fflush(void);

/*
 * ISA1200 has two possible addresses: 0x90, 0x92
 */

static unsigned short normal_i2c[] = {I2C_ISA1200A_ADDR, I2C_ISA1200B_ADDR, I2C_CLIENT_END};
//static unsigned short normal_i2c[] = {I2C_ISA1200A_ADDR, I2C_CLIENT_END};

static struct i2c_driver isa1200_driver = {
	.driver = {
		.name = ISA1200_NAME,
	},
	.attach_adapter		= isa1200_attach,
	.detach_client 		= isa1200_detach,
};

I2C_CLIENT_INSMOD_1(isa1200);
//I2C_CLIENT_INSMOD;

//chip_detect_flag_t  chip_status[ISA1200_CHIP_NUM] = {CHIP_NOT_DETECTED, CHIP_NOT_DETECTED};
struct i2c_client * i2c_client_isa1200[ISA1200_CHIP_NUM];
static unsigned int isa1200_chip_disabled; 
//pattern_data_t * pattern_data;
static int individual_mode = 0;

struct vibration_timer {
   int busy_flag;
   int pattern_sequence;
   pattern_data_t pattern_data;
   struct timer_list timer;
   struct timer_list timer_motor_off;
   struct work_struct work;
};

struct vibration_timer vib_timer;


void set_isa1200_gpio_hen(int value)
{
	pollux_gpio_setpin(ISA1200_GPIO_HEN, value);	
}

void set_isa1200_gpio_len(int value)
{
	pollux_gpioalv_set_write_enable(1);
	pollux_gpioalv_set_pin(ISA1200_GPIO_LEN, value);
	pollux_gpioalv_set_write_enable(0);
}

void set_pwm_value(int value)
{
	MES_PWM_SetDutyCycle(POLLUX_ISA1200_PWM_CH, value); 
}

void set_pwm_status_normal(void)
{
	//MES_PWM_SetDutyCycle(POLLUX_ISA1200_PWM_CH, DEFAULT_DUTY_VALUE);
	set_pwm_value(DEFAULT_DUTY_VALUE);
}

/*
void set_pwm_status_low(void)
{
	MES_PWM_SetDutyCycle(POLLUX_ISA1200_PWM_CH, PWM_PERIOD_VALUE); 
}
*/

void isa1200_enable_sequence(void)
{
	set_pwm_status_normal();
	mdelay(1);
	set_isa1200_gpio_hen(1);
	set_isa1200_gpio_len(1);
}

void isa1200_disable_sequence(void)
{
	//set_pwm_status_low();
	//udelay(1);
	set_isa1200_gpio_hen(0);
	set_isa1200_gpio_len(0);
}

static void isa1200_chip_disabled_fflush(void) 
{
	isa1200_chip_disabled = 0;
}

static void mask_isa1200_chip_disabled(unsigned short i2c_addr)
{
	if(i2c_addr == I2C_ISA1200A_ADDR) 
		isa1200_chip_disabled |= 0x01;
	else if(i2c_addr == I2C_ISA1200B_ADDR)
		isa1200_chip_disabled |= (0x01 << 1);
}

static int check_isa1200_chip_disabled(unsigned short i2c_addr) 
{
	int ret_val = 0;
	int compare_value = 0;

	if(i2c_addr == I2C_ISA1200A_ADDR)
		compare_value = 0x01;
		//(isa1200_chip_disabled & 0x01) ? ret_val = 1 : ret_val = 0;
	else if(i2c_addr == I2C_ISA1200B_ADDR) 
		compare_value = 0x01 << 1;
		//(isa1200_chip_disabled & (0x01 << 1)) ? ret_val = 1 : ret_val = 0;
	else
		return ret_val;

	if(isa1200_chip_disabled & compare_value) 
		ret_val = 1;
	else 
		ret_val = 0;

	return ret_val;
}

static int isa1200_read_reg(struct i2c_client * client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if(ret < 0)
	   dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int isa1200_write_reg(struct i2c_client * client, int reg, u8 value)
{
	int ret;
	
	if(check_isa1200_chip_disabled(client->addr))
		return 0;

	ret = i2c_smbus_write_byte_data(client, reg, value);
	if(ret < 0) {
	    dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		mask_isa1200_chip_disabled(client->addr);
	}
	return ret;
}

int isa1200_i2c_reset(struct i2c_client * client)
{
	int ret = 0;
	int i2c_write_value; 
	
	mdelay(10);
	i2c_write_value = ISA1200_HSWRST;
	ret = isa1200_write_reg(client, ISA1200_HCTRL2, i2c_write_value);
	if(ret < 0)	{
	   	dprintk("%s isa1200_write_reg error - 1\n", __func__);
		ret = -1;
		goto exit_i2c_reset;
	}
	mdelay(10);

	i2c_write_value = 0x00;
	ret = isa1200_write_reg(client, ISA1200_HCTRL2, i2c_write_value);
	if(ret < 0)	{
	   	dprintk("%s isa1200_write_reg error - 2\n", __func__);
		ret = -1;
		goto exit_i2c_reset;
	}

	//return ret;

exit_i2c_reset:
	return ret;

}

static void isa1200_i2c_motor_drv_on(struct i2c_client * client)
{
	int i2c_write_value;
	i2c_write_value = ISA1200_HAPDREN | ISA1200_HAPDIGMOD_PWM_IN | ISA1200_PWMMOD_DIVIDER_256;
	if((isa1200_write_reg(client, ISA1200_HCTRL0, i2c_write_value)) < 0) {
		dprintk("%s isa1200_write_reg error - 4\n", __func__);
	}
}


static void isa1200_i2c_motor_drv_off(struct i2c_client * client)
	
{
	int i2c_write_value;
	i2c_write_value = 0;
	if((isa1200_write_reg(client, ISA1200_HCTRL0, i2c_write_value)) < 0) {
		dprintk("%s isa1200_write_reg error - 4\n", __func__);
	}
}

static int motor_drv_status_flag = 0;		//1: on, 0: off
static void motor_drv_on(void) 
{
	if(!motor_drv_status_flag) {
		isa1200_i2c_motor_drv_on(i2c_client_isa1200[ISA1200A]);
		isa1200_i2c_motor_drv_on(i2c_client_isa1200[ISA1200B]);
		motor_drv_status_flag = 1;
	}
}

static void motor_drv_off(void) 
{
	if(motor_drv_status_flag) {
		isa1200_i2c_motor_drv_off(i2c_client_isa1200[ISA1200A]);
		isa1200_i2c_motor_drv_off(i2c_client_isa1200[ISA1200B]);
		motor_drv_status_flag = 0;
	}
}

static int isa1200_i2c_setup(struct i2c_client * client)
{
   	int i2c_write_value;
	int ret = 0;	
	
	isa1200_i2c_reset(client);

	i2c_write_value = ISA1200_BIT6_ON | ISA1200_MOTTYP_LRA | 0X0b;		//AC-Motor
	//i2c_write_value = ISA1200_BIT6_ON | ISA1200_MOTTYP_ERM | 0X0b;			//DC-Motor
	ret = isa1200_write_reg(client, ISA1200_HCTRL1, i2c_write_value);
	if(ret < 0)	{
	   	dprintk("%s isa1200_write_reg error - 3\n", __func__);
		ret = -1;
		goto exit_setup;
	}
	
	i2c_write_value = ldo_level_table[DEFAULT_LDO_LEVEL];
	if((isa1200_write_reg(client, ISA1200_SCTRL0, i2c_write_value)) < 0) {
	   	dprintk("%s isa1200_write_reg error - 4\n", __func__);
	}

	isa1200_i2c_motor_drv_off(client);		//default mode: motor drive off

	return 0;

exit_setup:
	return ret;
}

#ifdef SIMPLE_ISA1200_DEBUG 
void display_isa1200_register(struct i2c_client * client)
{
   	int i;
	int read_result;
   	int read_reg_array[] = {0x30, 0x31, 0x32, 0x00};

	for(i = 0; i < ARRAY_SIZE(read_reg_array); i++)
	{
	   	read_result = isa1200_read_reg(client, read_reg_array[i]);
	   	dprintk("%02x reg read value = %02x\n", read_reg_array[i], read_result); 
	}
}
#else /* SIMPLE_ISA1200_DEBUG */
void inline display_isa1200_register(struct i2c_client * client) { }
#endif


/* this function called by i2c_probe() */
static int isa1200_probe(struct i2c_adapter * adap, int addr, int kind)
{
   	struct i2c_client * new_client;
	//struct isa1200_data * data;
	int err = 0;
	int chip_index;
	char * isa1200x_i2c_name;

	//dprintk("%s: addr = %x, kind = %d\n", __func__, addr, kind);
	if(!(i2c_check_functionality(adap, I2C_FUNC_I2C))) {
	   	dprintk("%s - functionality error...\n", __func__);
		return -ENODEV;
	}

	if(!(new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
		dprintk("%s - i2c_client kernel memory allocation failed...\n", __func__);
		err = -ENOMEM;
		goto exit;
	}

	set_isa1200_gpio_hen(1);
	new_client->addr = addr;
	new_client->adapter = adap;
	new_client->driver = &isa1200_driver;
	

	if(addr == I2C_ISA1200A_ADDR) {
	   	chip_index = ISA1200A;
	   	isa1200x_i2c_name = I2C_ISA1200A_NAME;
		i2c_client_isa1200[chip_index] = new_client;
	}
	else if(addr == I2C_ISA1200B_ADDR) {
	   	chip_index = ISA1200B;
	   	isa1200x_i2c_name = I2C_ISA1200B_NAME;
		i2c_client_isa1200[chip_index] = new_client;
	}
	else { 
		dprintk("%s - wrong address...\n",__func__);
	  	err = -1;
		goto exit_kfree;
	}
	
	strcpy(new_client->name, isa1200x_i2c_name);

	if((err = i2c_attach_client(new_client)))
	   goto exit_kfree;
	
	set_isa1200_gpio_hen(0);

	//chip_status[chip_index] = CHIP_DETECTED;
	printk("isa1200 addr : 0x%02x attached.\n", new_client->addr);

	return 0;

exit_kfree:
	//kfree(data);
	kfree(new_client);
exit:
	return err;
}


static int isa1200_attach(struct i2c_adapter * adap)
{
	return i2c_probe(adap, &addr_data, isa1200_probe);
}

static int isa1200_detach(struct i2c_client * client )
{
	int rc;
	printk("remove i2c-addr : 0x%x\n", client->addr);
	if((rc = i2c_detach_client(client)) == 0){
		kfree(client);
	}
	
	//kfree(client);

	return 0;
}

int pollux_haptic_open(struct inode * inode, struct file * filp)
{
	int i;
	struct i2c_client * client;
	//printk("%s()\n", __func__);
	isa1200_chip_disabled_fflush();
	isa1200_enable_sequence();
	
	for(i = 0; i < ISA1200_CHIP_NUM; i++) {
	   	client = i2c_client_isa1200[i];
		isa1200_i2c_setup(client);
	}

	init_timer(&(vib_timer.timer));
	vib_timer.timer.data = (unsigned long) &vib_timer;
	vib_timer.timer.function = timer_over;

	return 0;
}

int pollux_haptic_release(struct inode * inode, struct file * filp)
{
	dprintk("release..\n");
	del_timer(&(vib_timer.timer));
	pattern_job_fflush();
	//isa1200_disable_sequence();
	set_pwm_status_normal();

	if(!individual_mode) {
		motor_drv_off();
	}
	
	return 0;
}

ssize_t pollux_haptic_read(struct file * filp, char * buf, size_t length, loff_t * f_pos)
{
	return 0;
}


#if 0
void disp_pattern_data(pattern_data_t * pattern_data)
{
	int i;
	printk("\n\n--- %s : act_number : %d ---\n", __func__, pattern_data->act_number);
	for(i = 0; i < pattern_data->act_number; i++)
	{
		printk("\t[%d] timeline : %d, strength : %d\n", i, 
			  pattern_data->vib_act_array[i].time_line,
			  pattern_data->vib_act_array[i].vib_strength
		);
	}
}
#else
static inline void disp_pattern_data(pattern_data_t * pattern_data) { }
#endif

void convert_strength_to_duty(pattern_data_t * pattern_data)
{
	int i;
	int strength;		// -126 ~ 0 : 1~50%  0 ~ 126: 50~99%
	short duty;
	vibration_level_t current_vib_level = vib_level;
	int calibration_value = 100;

	if(current_vib_level == VIB_LEVEL_MAX) {
		/*  */
		//calibration_value = 100;	
	}else if(current_vib_level == VIB_LEVEL_NORMAL) {
		/* 66% calibration  */
		calibration_value = 66;
	}else if(current_vib_level == VIB_LEVEL_WEAK) {
		/*  33 % calibration */
		calibration_value = 33;
	}else {		// VIB_LEVEL_OFF
		calibration_value = 0;
	}

	for(i = 0; i < pattern_data->act_number; i++) {	
		strength = pattern_data->vib_act_array[i].vib_strength;
	   	//check value 
		if(strength < VIB_STRENGTH_MIN)
		   	strength = VIB_STRENGTH_MIN;
		else if(VIB_STRENGTH_MAX < strength)
		   	strength = VIB_STRENGTH_MAX;
		
		if(current_vib_level != VIB_LEVEL_MAX)
			strength = (strength * calibration_value) / 100;

		strength += VIB_STRENGTH_MAX;	// 0 ~ 252
		strength = (strength * PWM_PERIOD_VALUE) / (VIB_STRENGTH_MAX * 2) ;
		duty = strength;
		if(!duty)
		   	duty = 1;
		else if(duty == PWM_PERIOD_VALUE)
			duty = (PWM_PERIOD_VALUE - 1);
		
		pattern_data->vib_act_array[i].vib_strength = duty;
	}
}

void convert_timeline_to_single_time(pattern_data_t * pattern_data)
{
	int i;
	short single_time = 0;
	
	for(i = 0; i < pattern_data->act_number; i++) {
		single_time = pattern_data->vib_act_array[i + 1].time_line - pattern_data->vib_act_array[i].time_line;
		if(single_time < 0)
		   	single_time = 0;

		pattern_data->vib_act_array[i].time_line = single_time;
	}

}

void pattern_job_fflush(void)
{
	//del_timer(&(vib_timer.timer));
	vib_timer.busy_flag = 0;
}

int pollux_haptic_ioctl(struct inode * inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int size;
	job_status_t job_status;

	if(_IOC_TYPE(cmd) != HAPTIC_IOCTL_MAGIC) return -EINVAL;
	if(IOCTL_HAPTIC_MAXNR <= _IOC_NR(cmd)) return -EINVAL;

	size = _IOC_SIZE(cmd);

	if(size) {
		if(_IOC_DIR(cmd) & _IOC_READ) {
			if(!access_ok(VERIFY_WRITE, (void * ) arg, size)) return -EFAULT;
		}
		else if(_IOC_DIR(cmd) & _IOC_WRITE) {
		   	if(!access_ok(VERIFY_READ, (void *) arg, size)) return -EFAULT;
		}
	}

	switch(cmd)
	{
		case IOCTL_PLAY_PATTERN:
			dprintk("pattern input\n");
			if(pollux_gpio_getpin(GPIO_HOLD_KEY))
				break;
			
			if(vib_level == VIB_LEVEL_OFF) {
				break;
			}

			if(vib_timer.busy_flag == 0) {
				vib_timer.busy_flag = 1;
				
				if(!individual_mode) {
					motor_drv_on();
				}

				copy_from_user((void *)&vib_timer.pattern_data, (const void *) arg, size);
				//convert vib_strength to vib duty and time_line... 
				disp_pattern_data(&vib_timer.pattern_data);
				convert_strength_to_duty(&vib_timer.pattern_data);
				convert_timeline_to_single_time(&vib_timer.pattern_data);
				vib_timer.pattern_sequence = 0;
				register_timer();
			}
			else {
			   	// not finished vibration action... ignore command... 
				if(vib_timer.busy_flag) {
				   	dprintk("vib_timer.busy_flag = %d\n", vib_timer.busy_flag);
				}
			}
		   	break;

		case IOCTL_FFLUSH_JOB:
			pattern_job_fflush();
			set_pwm_status_normal();
			/*
			if(!individual_mode) {
				motor_drv_off();
			}*/

			break;

		case IOCTL_JOB_STATUS:
			job_status = JOB_STATUS_IDLE;
			//if((vib_timer.pattern_data != NULL) || (vib_timer.busy_flag)) {
			if(vib_timer.busy_flag) {
				job_status = JOB_STATUS_BUSY;
			}
			
			copy_to_user((void *) arg, &(job_status), size);
			break;

		case IOCTL_MOTOR_DRV_ENABLE:
			break;
		
	    case IOCTL_MOTOR_DRV_DISABLE:
			break;

		case IOCTL_INDIVIDUAL_MODE: 
			copy_from_user(&individual_mode, (void *) arg, size);
			if(individual_mode) //be sure haptic_test_flag value 0 or 1, nothing else!! 
				individual_mode = 1;
			break;

		case IOCTL_ISA1200A_ENABLE: 
			isa1200_i2c_motor_drv_on(i2c_client_isa1200[ISA1200A]);
			isa1200_i2c_motor_drv_off(i2c_client_isa1200[ISA1200B]);
			break;

		case IOCTL_ISA1200B_ENABLE: 
			isa1200_i2c_motor_drv_on(i2c_client_isa1200[ISA1200B]);
			isa1200_i2c_motor_drv_off(i2c_client_isa1200[ISA1200A]);
			break;
		
		case IOCTL_GET_VIB_LEVEL:
			copy_to_user((void *)arg, &vib_level, size);
			break;

		case IOCTL_SET_VIB_LEVEL:
			copy_from_user(&vib_level, (void *)arg, size);
			break;

		/* leave below functions for debugging hardware */
#if 0 
		case IOCTL_GPIO_LEN:
			copy_from_user((void *)&gpio_value, (const void *) arg, size);
			if(gpio_value)
				set_isa1200_gpio_len(1);
			else
			   	set_isa1200_gpio_len(0);
			break;
		
		case IOCTL_GPIO_HEN:
			copy_from_user((void *)&gpio_value, (const void *) arg, size);
			if(gpio_value)
				set_isa1200_gpio_hen(1);
			else
			   	set_isa1200_gpio_hen(0);
			break;
		
		case IOCTL_SET_PWM:
			copy_from_user((void *)&pwm_value, (const void *) arg, size);
			MES_PWM_SetDutyCycle(POLLUX_ISA1200_PWM_CH, pwm_value); 
			break;
#endif /* SIMPLE_ISA1200_DEBUG */
		default:	//undefined command input... 
			break;
	}
	return 0;
}

static void haptic_time_over_work(struct work_struct * work)
{
	//마지막으로 모터를 끄기 전에 한번 더 busy flag 확인 
	dprintk("%s - pattern end\n", __func__);
	if(vib_timer.busy_flag) {
		dprintk("%s - busy flag\n",__func__);
		return;
	}

	if(!individual_mode) {
		motor_drv_off();
	}
}

//#define MILLISECOND_BASE (HZ/1000)
static int pattern_end = 0;
void register_timer(void)
{
	pattern_end = 0;
	//init_timer(&(vib_timer.timer));
	//vib_timer.timer.expires = get_jiffies_64() 
	   //+ (vib_timer.pattern_data.vib_act_array[vib_timer.pattern_sequence].time_line * HZ / 1000);
	//vib_timer.timer.data = (unsigned long) &vib_timer;
	//vib_timer.timer.function = timer_over;
	//add_timer(&(vib_timer.timer));
	mod_timer(&vib_timer.timer, get_jiffies_64() \
	  + (vib_timer.pattern_data.vib_act_array[vib_timer.pattern_sequence].time_line * HZ / 1000));
	//set pwm duty
	//MES_PWM_SetDutyCycle(POLLUX_ISA1200_PWM_CH, vib_timer.pattern_data.vib_act_array[vib_timer.pattern_sequence].vib_strength); 
	set_pwm_value(vib_timer.pattern_data.vib_act_array[vib_timer.pattern_sequence].vib_strength); 
}

void timer_over(unsigned long data)
{	
	struct vibration_timer * ptimer;
	
	if(data) {
		ptimer = (struct vibration_timer *) data;

		if(pattern_end) {
			schedule_work(&ptimer->work);
			pattern_end = 0;
			return;
		}

		ptimer->pattern_sequence++;		//increase sequence number... 
		if((ptimer->pattern_data.vib_act_array[ptimer->pattern_sequence].time_line == 0) ||
			  (ptimer->pattern_data.act_number <= ptimer->pattern_sequence)) {
		   	//pattern play over...
			dprintk("%s(), pattern play over...\n", __func__);
			vib_timer.busy_flag = 0;
			set_pwm_status_normal();
			pattern_end = 1;
			mod_timer(&vib_timer.timer, get_jiffies_64() + HZ * 5);
		}
		else {
			// there are still some datas to play... 
			if(vib_timer.busy_flag) {
				pattern_end = 0;
				register_timer();
			}
			else {
				//pattern_job_fflush() 에 의해 pattern plat가 멈춤... 
				pattern_end = 1;
				mod_timer(&vib_timer.timer, get_jiffies_64() + HZ * 5);
			}
		}
	}


}

struct file_operations pollux_haptic_fops = {
	open:		pollux_haptic_open,
	read:		pollux_haptic_read,
	ioctl:		pollux_haptic_ioctl,
	release:	pollux_haptic_release,
};

static struct class * pollux_haptic_class;

static int __init pollux_haptic_probe(struct platform_device * pdev)
{
	int ret;
	//check isa1200 status by using i2c bus... 
	// pwm init... 
	isa1200_pwm_init();
	ret = i2c_add_driver(&isa1200_driver);
	if(ret)	{
		dprintk("%s i2c_add_driver() failed... \n", __func__);	
		return ret;
	}

	vib_timer.busy_flag = 0;
	INIT_WORK(&vib_timer.work, haptic_time_over_work);

	ret = register_chrdev(ISA1200_MAJOR, POLLUX_HAPTIC_NAME, &pollux_haptic_fops);
	if( ret < 0 ) {
		dprintk("%s register_chrdev() failed..\n", __func__);
		return ret;
	}

	pollux_haptic_class = class_create(THIS_MODULE, POLLUX_HAPTIC_CLASS_NAME);
	device_create(pollux_haptic_class, NULL, MKDEV(ISA1200_MAJOR, 0), POLLUX_HAPTIC_NAME);
	return 0;
}

static int pollux_haptic_remove(struct platform_device * pdev)
{
	dprintk("%s\n", __func__);
	device_destroy(pollux_haptic_class, MKDEV(ISA1200_MAJOR, 0));
	class_destroy(pollux_haptic_class);
   	unregister_chrdev(ISA1200_MAJOR, POLLUX_HAPTIC_NAME);
	i2c_del_driver(&isa1200_driver);
	return 0;
}	

#ifdef CONFIG_PM
static int pollux_haptic_suspend(struct platform_device * dev, pl_message_t state)
{
	dprintk("%s\n", __func__);
	return 0;
}

static int pollux_haptic_resume(struct platform_device * dev)
{
	dprintk("%s\n", __func__);
	return 0;
}
#else /* CONFIG_PM */

#define pollux_haptic_suspend NULL
#define pollux_haptic_resume NULL

#endif /* CONFIG_PM */

static struct platform_driver pollux_haptic_driver = {
	.probe		= pollux_haptic_probe,
	.remove		= pollux_haptic_remove,
	.suspend	= pollux_haptic_suspend,
	.resume		= pollux_haptic_resume,
	.driver		= {
		.name 	= POLLUX_HAPTIC_NAME,
		.owner	= THIS_MODULE,
	},
};

/*
static struct platform_device pollux_haptic_device = {
	.name		= POLLUX_HAPTIC_NAME,
	.id 		= 0,
};
*/

static struct platform_device * pollux_haptic_device;

//Caution - isa1200_pwm_init() must be same with function InitializePWM() in pollux_fb_ioctl.c file..
void isa1200_pwm_init(void)
{
	//MES_PWM_SetClockSource(0, SYSTEM_CLOCK_PWM_SELPLL);		//132.5 MHz
	//MES_PWM_SetClockDivisor(0, SYSTEM_CLOCK_PWM_DIV);		//35 -> 132.5 / 53 = 3.78MHz
	//MES_PWM_SetClockPClkMode(MES_PCLKMODE_ALWAYS);
	//MES_PWM_SetClockDivisorEnable(CTRUE);
	MES_PWM_SetPreScale(POLLUX_ISA1200_PWM_CH, 1);			// 3.78MHz /10 = 378khz
	MES_PWM_SetPolarity(POLLUX_ISA1200_PWM_CH, DEFAULT_POLARITY);
	MES_PWM_SetPeriod(POLLUX_ISA1200_PWM_CH, PWM_PERIOD_VALUE);			// 3780kHz / 100 = 37KHz...
	//set_pwm_status_normal();
}


static int __init isa1200_init(void)
{
   	int res;

	//regist platform_device..
	pollux_haptic_device = platform_device_alloc(POLLUX_HAPTIC_NAME, 0);
	if(!pollux_haptic_device) {
		dprintk("pollux device alloc failed..\n");
		return -1;
	}
	//res = platform_device_register(&pollux_haptic_device);
	res = platform_device_register(pollux_haptic_device);
	if(res) {
		dprintk("%s platform_device_register() failed..\n", POLLUX_HAPTIC_NAME);
		return res;
	}
	
	res = platform_driver_register(&pollux_haptic_driver);
	if(res) {
	   	dprintk("%s platform_driver_register() failed...\n", POLLUX_HAPTIC_NAME);
		return res;
	}

	return res;
}


static void __exit isa1200_exit(void)
{
	//if(vib_timer.busy_flag)
	   	//del_timer(&(vib_timer.timer));
	//motor_drv_off();
	//platform_device_unregister(&pollux_haptic_device);
	platform_device_unregister(pollux_haptic_device);
	platform_driver_unregister(&pollux_haptic_driver);

	return;
}

module_init(isa1200_init);
module_exit(isa1200_exit);

MODULE_AUTHOR("Soonmin, Hong <lars@gp2x.com>");
MODULE_DESCRIPTION("ISA1200 Simple Driver");
MODULE_LICENSE("GPL");


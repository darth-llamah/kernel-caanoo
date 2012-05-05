/*
 * sound/oss/pollux-cs42L51/pollux-audio.c
 *
 * Driver for I2S interface on the Magiceyes POLLUX
 *
 */
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/pm.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/string.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>

#include <asm/arch/dma.h>
#include <asm/arch/regs-gpioalv.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/aesop-pollux.h>
#include <asm/arch/pollux-adc.h>

#include "pollux-iis.h"
#include "pollux-cs42L51.h"

#ifdef  GDEBUG
#    define dprintk(fmt, x... )  printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
#    define dprintk( x... )
#endif


typedef struct  {
	unsigned short	volume;			/* master volume */
	unsigned char   volume_index;
	unsigned char	bass;
	unsigned char	treble;
	unsigned short	igain;
	unsigned short	micAmp;
	unsigned char recSel;
	int step_nums;		
	int vol_div100;
	int scale;
	int igain_scale;
	int micAmp_scale;
	int micAmp_step;
	unsigned char* vol_table;
	int	mod_cnt;
}cs42l51_mixer;


typedef struct {
	int size;						/* buffer size */
	char *start;					/* points to actual buffer */
	dma_addr_t dma_addr;			/* physical buffer address */
	struct semaphore sem;			/* down before touching the buffer */
	int master;						/* owner for buffer allocation, contain size when true */
	struct audio_stream_s *stream;	/* owning stream */
} audio_buf_t;

typedef struct audio_stream_s {
	char *name;				/* stream identifier */
	audio_buf_t *buffers;	/* pointer to audio buffer structures */
	audio_buf_t *buf;		/* current buffer used by read/write */
	u32 buf_idx;			/* index for the pointer above... */
	u32 fragsize;			/* fragment i.e. buffer size */
	u32 nbfrags;			/* nbr of fragments i.e. buffers */
	u32 channels;			/* audio channels 1:mono, 2:stereo */
	int bytecount;			/* nbr of processed bytes */
	int fragcount;			/* nbr of fragment transitions */
	u32 rate;				/* audio rate */
	int dma_ch;				/* DMA channel ID */
	wait_queue_head_t wq;   /* for poll */
	int mapped;				/* mmap()'ed buffers */
	int active;				/* actually in progress */
	int stopped;			/* might be active but stopped */
} audio_stream_t;


audio_stream_t output_stream;
audio_stream_t input_stream; 

struct audio_device {
	audio_stream_t *output;
	audio_stream_t *input;
	struct i2c_client *new_client;
	unsigned int hardware_addr;
	int chip_id;	
	int audio_dev_dsp;
	int audio_dev_mixer;
	int rd_ref;
	int wr_ref;
	int audio_mix_modcnt;
	struct timer_list volume_timer;
	struct timer_list headphone_timer;
	int ADC_reading;
	int last_reading;    
	struct workqueue_struct *codec_tasks;
	struct work_struct volume_work;         /* monitor volume  */
	struct work_struct headphone_work;      /* monitor headphone_mode */
	int isOpen;			                    /* monitor open/close state	*/
	int headphone_mode;                     /* whether we're on headphone or speaker */
	cs42l51_mixer mixer;
} audio_dev;


struct audio_device *dev = &audio_dev;

#define AOUT_DMA_CH 	4
#define AIN_DMA_CH 		5

#define AUDIO_NAME		"pollux-audio"
#define AUDIO_NBFRAGS_DEFAULT	8
#define AUDIO_FRAGSIZE_DEFAULT	32768

#define AUDIO_NAME_VERBOSE 		"CS42L51 audio driver"

#if 0
#define OEM_GPIO_AUDIO_RST_BITNUM  2    // gpioalive, DAC reset pin
#else
#define GPIOALV_AUDIO_RST_BITNUM  2     // gpioalive, DAC reset pin
#endif

#define	SND_SPK_ON_OFF				POLLUX_GPA9
#define HEADPHONE_DECT_PIN          POLLUX_GPA11
#define GPIO_HOLD_KEY               POLLUX_GPA10

#define VOLUME_CAPTURE_RANGE	3	            // Min Volume wheel change +/-
#define VOLUME_SAMPLING_J	HZ / 10	            // sample wheel every 100 ms
//#define VOLUME_SAMPLING_J	HZ / 5	            // sample wheel every 200 ms

#define HEADPHONE_SAMPLING_J	HZ / 10	        // sample headphone jack every 100 ms


#define NEXT_BUF(_s_,_b_) { \
	(_s_)->_b_##_idx++; \
	(_s_)->_b_##_idx %= (_s_)->nbfrags; \
	(_s_)->_b_ = (_s_)->buffers + (_s_)->_b_##_idx; }

#define AUDIO_ACTIVE(dev)	((dev)->rd_ref || (dev)->wr_ref)

static long audio_set_dsp_speed(long val, audio_stream_t * s);
static void pollux_set_volume(struct work_struct *work);
///////////////////////////////////////////////////////////////////////////////////////
/*
 * I2C OPERATION 
*/  	

static int cs42L51_write_byte(unsigned char reg, unsigned char value)
{
	unsigned char buf[2];
	int ret;

	buf[0] = reg;
	buf[1] = value;

	dprintk("cs42l51: writing 0x%02x to address 0x%02x\n", buf[1], buf[0]);
	if (2 != (ret = i2c_master_send(dev->new_client, buf, 2)) )
	{
		dprintk("i2c i/o error: rc == %d (should be 2)\n", ret);
		return -1;
	}
	
	return 0;
}

static int cs42L51_read_byte(unsigned char reg)
{
	int ret;
	unsigned char buf[1];
	
	buf[0] = reg;
	if (1 != (ret = i2c_master_send(dev->new_client, buf, 1)))
		printk("i2c i/o error: rc == %d (should be 1)\n", ret);

	msleep(10);

	if (1 != (ret = i2c_master_recv(dev->new_client, buf, 1)))
		printk("i2c i/o error: rc == %d (should be 1)\n", ret);

	dprintk("cs42l51: read 0x%02x = 0x%02x\n", reg, buf[0]);

	return (buf[0]);
}

static int cs42L51_write_array(unsigned char list[][2], int entries)
{
	int i;

	for(i = 0; i < entries; i++) 
		cs42L51_write_byte(list[i][0], list[i][1]);


#if 0	/* debug test */
	
	for(i = 0; i < entries; i++) 
	{
		printk("reg:0x0%x   value:0x%x  \n",cs42L51_settings[i][0], cs42L51_read_byte(list[i][0]) ); 
	}
#endif
	
	return 0;
}

/* Addresses to scan */
static unsigned short normal_i2c[] = { 0x94 >> 1, I2C_CLIENT_END };
/* I2C Magic */
I2C_CLIENT_INSMOD;

static int cs42l51_attach_adapter(struct i2c_adapter *adapter);
static int cs42l51_detect(struct i2c_adapter *adapter, int address, int kind);
static int cs42l51_detach_client(struct i2c_client *client);

static struct i2c_driver cs42l51_driver = {
	.driver = {
		.name	= "cs42l51",
	},
	.attach_adapter	= cs42l51_attach_adapter,
	.detach_client	= cs42l51_detach_client,
};

struct cs42l51_data {
	struct i2c_client client;
	int yyy;
};

static int cs42l51_attach_adapter(struct i2c_adapter *adapter)
{
	
	return i2c_probe(adapter, &addr_data, cs42l51_detect);
}

static int cs42l51_detect(struct i2c_adapter *adapter, int address, int kind)
{
	struct cs42l51_data *data;
	int err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		goto exit;

	if (!(data = kzalloc(sizeof(struct cs42l51_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	dev->new_client = &data->client;
	i2c_set_clientdata(dev->new_client, data);
	dev->new_client->addr = address;
	dev->new_client->adapter = adapter;
	dev->new_client->driver = &cs42l51_driver;
	dev->new_client->flags = 0;

	/* Tell the I2C layer a new client has arrived */
	if ((err = i2c_attach_client(dev->new_client)))
		goto exit_free;

	return 0;

exit_free:
	kfree(data);
exit:
	return err;
}

static int cs42l51_detach_client(struct i2c_client *client)
{
	int err;
	struct cs42l51_data *data = i2c_get_clientdata(client);

	if ((err = i2c_detach_client(client)))
		return err;

	kfree(data);
	return 0;
}

#if 0
static void pollux_i2c_enable(CBOOL en)
{
	MES_ALIVE_Initialize();
	MES_ALIVE_SetBaseAddress(0, POLLUX_VA_ALVGPIO);
	MES_ALIVE_OpenModule(0);

	MES_ALIVE_SetWriteEnable(0, CTRUE );
	if(en)
		MES_ALIVE_SetAliveGpio(0, OEM_GPIO_AUDIO_RST_BITNUM, CTRUE );
	else
		MES_ALIVE_SetAliveGpio(0, OEM_GPIO_AUDIO_RST_BITNUM, CFALSE );
	MES_ALIVE_SetWriteEnable(0, CFALSE );

	msleep(100);	
}
#else
static void pollux_i2c_enable(CBOOL en)
{
	pollux_gpioalv_set_write_enable(1);
	
	if(en)
		pollux_gpioalv_set_pin(GPIOALV_AUDIO_RST_BITNUM, 1);
	else
		pollux_gpioalv_set_pin(GPIOALV_AUDIO_RST_BITNUM, CFALSE );	
	pollux_gpioalv_set_write_enable(0);


	msleep(100);	
}
#endif

////////////////////////////////////////////////////////////////////////////////////////
/*
 * POLLUX I2S REGISTER SETING
*/  	

#define SYNC_PERIOD_32		32
#define SYNC_PERIOD_48		48
#define SYNC_PERIOD_64		64
#define BUFF_WIDTH_DATA16	16
static int init_polluxI2S( audio_stream_t * s)
{
	MES_AUDIO_Initialize();
	MES_AUDIO_SetBaseAddress(POLLUX_VA_SOUND); // ghcstop fix
	MES_AUDIO_OpenModule();
	MES_AUDIO_SetClockDivisorEnable(CFALSE);
	
	MES_AUDIO_SetClockSource(0, 1);  //PLL1
	MES_AUDIO_SetClockSource(1, 7);  //CLKGEN0 ouput clock
	audio_set_dsp_speed((unsigned long)SYNC_44100,s);
	MES_AUDIO_SetClockOutEnb(1, CTRUE);
	
	MES_AUDIO_SetClockPClkMode(MES_PCLKMODE_ALWAYS);
	MES_AUDIO_SetClockDivisorEnable(CTRUE);
	
	MES_AUDIO_SetI2SControllerReset(CFALSE);
	udelay(100);
	MES_AUDIO_SetI2SLinkOn();
	udelay(100);

	MES_AUDIO_SetI2SMasterMode(CTRUE);
	MES_AUDIO_SetSyncPeriod(SYNC_PERIOD_64);
#if 0	
	MES_AUDIO_SetPCMINDataWidth(BUFF_WIDTH_DATA16);
#endif	
	MES_AUDIO_SetPCMOUTDataWidth(BUFF_WIDTH_DATA16);
	
#if 0	
	MES_AUDIO_SetAudioBufferPCMINEnable(CTRUE);
#endif	
	MES_AUDIO_SetAudioBufferPCMOUTEnable(CTRUE);
	
#if 0	
	MES_AUDIO_SetI2SInputEnable(CTRUE);
#endif	
	
	MES_AUDIO_SetI2SOutputEnable(CTRUE);

#if 0	
	MES_AUDIO_SetInterruptEnable(0, CTRUE);	   //input 
#endif	
	MES_AUDIO_SetInterruptEnable(1, CTRUE);	   //output

	MES_AUDIO_SetI2SLinkOn();

#if 0
	MES_AUDIO_ClearInterruptPending(0);   // input 
#endif
	
	MES_AUDIO_ClearInterruptPending(1);   //output 

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
 * CS42L51 REGISTER SETING
*/  	

void cs42L51_set_volume(unsigned char left, unsigned char right)
{
	unsigned char indexTableL,indexTableR;
	unsigned char val_l,val_r;
    
    if (right > 100) right = 100;
	if (left > 100) left = 100;	
	
    indexTableL = left / dev->mixer.scale; 		
	indexTableR = right / dev->mixer.scale; 		

    val_l = dev->mixer.vol_table[indexTableL];
	val_r = dev->mixer.vol_table[indexTableR];
    
    cs42L51_write_byte(0x10, val_l);
	cs42L51_write_byte(0x11, val_r);
}


static int adc_GetVolume(unsigned short* adc)
{
    unsigned char Timeout = 0xff;
    
    MES_ADC_SetInputChannel(4);
	MES_ADC_Start();
	
	while(!MES_ADC_IsBusy() && Timeout--);	// for check between start and busy time
	while(MES_ADC_IsBusy());
	*adc = (U16)MES_ADC_GetConvertedData();
    
    return 0;
}


static void pollux_set_volume(struct work_struct *work)
{
	unsigned short reading, diff, org_reading;
	unsigned char volume_reg_val;
	int index, now_vol;
	
/*	if(dev->isOpen && !pollux_gpio_getpin(GPIO_HOLD_KEY) ) */
	if(dev->isOpen)
	{
	    /* Range ( 0x30 ~ 0x3d0 ) */
        adc_GetVolume(&reading);
	    org_reading = reading;
	
	    if( dev->ADC_reading > reading) diff =  dev->ADC_reading - reading;
        else if( reading > dev->ADC_reading) diff = reading - dev->ADC_reading;
        else diff = 0;
	
        if( reading < 45 ) reading = 0;
        else reading -= 45;      
        
        index =  reading / dev->mixer.scale;
        if(index > 0x12 ) index = 0x12;  
        	
        if( (dev->mixer.volume_index != index) && diff > 40 )
        {          
#if 0
            if( index == 0 ){   
	            pollux_gpio_setpin(SND_SPK_ON_OFF ,0);
	        }    
	        else if ( (dev->mixer.volume_index == 0) && (dev->headphone_mode) ) {
                pollux_gpio_setpin(SND_SPK_ON_OFF ,1);
            }                        
            
            dev->mixer.vol_table = dev->headphone_mode ? spk_table : hp_table;

            volume_reg_val = dev->mixer.vol_table[index];
            dev->mixer.volume = index *  dev->mixer.vol_div100;
	        dev->mixer.volume_index = index;
	        dev->ADC_reading = org_reading;
#else
			dev->mixer.vol_table = dev->headphone_mode ? spk_table : hp_table;
			
			if(index > dev->mixer.volume_index ) {
				dev->mixer.volume_index++;
				dev->ADC_reading += 50;
			}
			else {
				dev->mixer.volume_index--;
				dev->ADC_reading -= 50;
			}
			
			if( dev->mixer.volume_index == 0 )   
	            pollux_gpio_setpin(SND_SPK_ON_OFF ,0);
	       	else if ( (dev->mixer.volume_index != 0) && (dev->headphone_mode) )
                pollux_gpio_setpin(SND_SPK_ON_OFF ,1);
            
			volume_reg_val = dev->mixer.vol_table[dev->mixer.volume_index];
			dev->mixer.volume = dev->mixer.volume_index *  dev->mixer.vol_div100;
#endif	 
	        
	        cs42L51_write_byte(0x10, volume_reg_val);
	        cs42L51_write_byte(0x11, volume_reg_val);
            
//          	printk(" Volume Index : %d  diff : %d  volume:0x%x dev->ADC_reading:0x%x  \n", index, diff, volume_reg_val, dev->ADC_reading); 
            
        }
    }
}

static void pollux_set_headphones(struct work_struct *work)
{
	int status;
    unsigned char volume_reg_val;
    
    if(dev->isOpen){
        status = pollux_gpio_getpin(HEADPHONE_DECT_PIN);
        if(status != dev->headphone_mode) { /* change in status */
		    
		    dev->mixer.vol_table = status ? spk_table : hp_table;
		    if( (status == 0) )                                   //phone mode
	            pollux_gpio_setpin(SND_SPK_ON_OFF ,0);
	        else if ( status && dev->mixer.volume_index )         //spk mode
                pollux_gpio_setpin(SND_SPK_ON_OFF ,1);
		    
		    volume_reg_val = dev->mixer.vol_table[dev->mixer.volume_index];
		    dev->headphone_mode = status;
	        
	        cs42L51_write_byte(0x10, volume_reg_val);
	        cs42L51_write_byte(0x11, volume_reg_val);
            
	     //   printk(" change to %s  volume_reg_val:0x%x \n", dev->headphone_mode ? "spk" : "ear", volume_reg_val );
	    }
    }
}

void cs42L51_set_igain(unsigned char left, unsigned char right)
{
	printk(" input _gain \n");
	/*
	int val_l, val_r;
	if (right > 100) right = 100;
	if (left > 100) left = 100;	
			
	val_l = left / dev->mixer.igain_scale ;
	val_r = right / dev->mixer.igain_scale ;
	
	cs42L51_write_byte(CS42L51_PGAA, (unsigned char)val_l);
	cs42L51_write_byte(CS42L51_PGAB, (unsigned char)val_r);
    */
}					

static void volume_monitor_task(unsigned long data)
{
	struct audio_device *a_dev = (struct audio_device *)data;

	queue_work(a_dev->codec_tasks, &a_dev->volume_work);
	a_dev->volume_timer.expires += VOLUME_SAMPLING_J;
	a_dev->volume_timer.function = volume_monitor_task;
	a_dev->volume_timer.data = data;
	add_timer(&a_dev->volume_timer);
}

static void headphone_monitor_task(unsigned long data)
{
	struct audio_device *a_dev = (struct audio_device *)data;

	queue_work(a_dev->codec_tasks, &a_dev->headphone_work);
	a_dev->headphone_timer.expires += HEADPHONE_SAMPLING_J;
	a_dev->headphone_timer.function = headphone_monitor_task;
	a_dev->headphone_timer.data = data;
	add_timer(&a_dev->headphone_timer);
}


static int cs42L51_init(void)
{
	int ret, index;
	unsigned char vol;
	unsigned short reading;
			
	// set mixer table number selection
	dev->mixer.step_nums = 20;//10;  
    
    /* input value is 10 bit ADC, from 0 to 0x3FF */
    dev->mixer.scale = 1000 / dev->mixer.step_nums;
	dev->mixer.vol_div100 = 100 / dev->mixer.step_nums;
	
	dev->mixer.igain_scale = 5;  //20
#if 0	
	dev->mixer.micAmp_scale = 10;  //16
	dev->mixer.micAmp_step = 16 / dev->mixer.micAmp_scale;
#endif

    dev->headphone_mode = pollux_gpio_getpin(HEADPHONE_DECT_PIN);
    dev->mixer.vol_table = dev->headphone_mode ? spk_table : hp_table;
 
	/* initial volume setting */
    adc_GetVolume(&reading);
	dev->ADC_reading = reading;
		
	if( reading < 45 ) reading = 0;
    else reading -= 45;      
    
    index =  reading / dev->mixer.scale;
    if(index > 0x12 ) index = 0x12;

    vol = dev->mixer.vol_table[index];
    dev->mixer.volume = index *  dev->mixer.vol_div100;
    dev->mixer.volume_index = index;
    
    cs42L51_settings[14][1] = vol;
    cs42L51_settings[15][1] = vol;
	
	ret = cs42L51_write_array(cs42L51_settings,sizeof(cs42L51_settings)/2);
	return(ret);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
 * MIXER INTERFACE 
*/  	
static void pollux_set_mixer(unsigned int val, unsigned char cmd)
{
	unsigned char left,right;
	
	right = ((val >> 8)  & 0xff);
	left = (val  & 0xff);
			
	switch(cmd)
	{
		case CS42L51_CMD_VOLUME:			
			//cs42L51_set_volume(right,left);
			break;
		case CS42L51_CMD_BASS:  		/* -10.5db ~ 12db  ( 0 to 150 => 0 to 15) */
			{
				int setval;
				unsigned char reg;
				
				if (left > 150) left = 150;
			
				if(left == 0)  setval = 0x0f;
				else setval = 15 - (left / 10); 
			
				reg = cs42L51_read_byte(CS42L51_TONE_CTRL);
				reg = (reg & 0xf0) | (unsigned char) setval;
			
				cs42L51_write_byte(CS42L51_TONE_CTRL, reg);
			}
			break;
		case CS42L51_CMD_TREBLE:		/* -10.5db ~ 12db (0 to 150)  */		
			{
				int setval;
				unsigned char reg;
				
				if (left > 150) left = 150;
			
				if(left == 0)  setval = 0x0f;
				else setval = 15 - (left / 10); 
			
				setval <<= 4;
			
				reg = cs42L51_read_byte(CS42L51_TONE_CTRL);
				reg = (reg & 0x0f) | (unsigned char)setval;
			
				cs42L51_write_byte(CS42L51_TONE_CTRL, reg);
			}
			break;
		case CS42L51_CMD_IGAIN:
			cs42L51_set_igain(left, right);
			break;
		case CS42L51_CMD_MICAMP:					
			break;
		case CS42L51_CMD_RECSRC:
			{
				
#if 0				
				unsigned char reg;	
				
				reg = cs42L51_read_byte(CS42L51_INPUT_SELA);
				reg = (reg & 0xE0) | left;
				reg = cs42L51_read_byte(CS42L51_INPUT_SELB);
				reg = (reg & 0xE0) | left;
			
				if( (left == REC_SELECT_MIC )|| (left == REC_SELECT_MIC_LINE1 ) || 
						(left ==REC_SELECT_MIC_LINE1_LINE2) )
				{
					reg = cs42L51_read_byte(CS42L51_MICAMP_CTRLA);			
					reg &=  0xBF;
					reg |= REC_MIC_SELECT(right)? 0x40 : 0;  
					reg = cs42L51_write_byte(CS42L51_MICAMP_CTRLA, reg);			
				
					reg = cs42L51_read_byte(CS42L51_MICAMP_CTRLB);			
					reg &=  0xBF;
					reg |= REC_MIC_SELECT(right)? 0x40 : 0;  
					reg = cs42L51_write_byte(CS42L51_MICAMP_CTRLB, reg);
				}
#endif			
			
			}
			break;			
	}
}	


int 
pollux_mixer_ioctl(struct inode *inode, struct file *file, 
                                unsigned int cmd, unsigned long arg)
{
	int val, nr = _IOC_NR(cmd), ret = 0;

    if (cmd == SOUND_MIXER_INFO) {
		struct mixer_info mi;

		strncpy(mi.id, "CS42L51", sizeof(mi.id));
		strncpy(mi.name, "pollux CS42L51", sizeof(mi.name));
		mi.modify_counter = dev->audio_mix_modcnt;
		return copy_to_user((int *)arg, &mi, sizeof(mi));
	}

	if (_IOC_DIR(cmd) ==  (_IOC_WRITE |_IOC_READ) ) 
	{
	    if(get_user(val, (int *)arg))
	        return EFAULT;
	        
	    switch (nr) 
		{
	        case SOUND_MIXER_PCM:
	            pollux_set_mixer( val, CS42L51_CMD_VOLUME );
	            dev->mixer.volume = val;
	            return 0;
	    }     
	}
	
    if (_IOC_DIR(cmd) & _IOC_WRITE) 
	{
		ret = get_user(val, (int *)arg);
		if (ret)
			goto out;

		switch (nr) 
		{
			case SOUND_MIXER_VOLUME:
				pollux_set_mixer( val, CS42L51_CMD_VOLUME );
				dev->mixer.volume = val;
				break;
			case SOUND_MIXER_BASS:
				pollux_set_mixer( val, CS42L51_CMD_BASS );
				dev->mixer.bass = val & 0xff;
				break;
			case SOUND_MIXER_TREBLE:
				pollux_set_mixer( val, CS42L51_CMD_TREBLE );
				dev->mixer.treble = val & 0xff;
				break;	
			case SOUND_MIXER_LINE:
				pollux_set_mixer( val, CS42L51_CMD_IGAIN );
				dev->mixer.igain = val;
				break;
			case SOUND_MIXER_MIC:
				pollux_set_mixer( val, CS42L51_CMD_MICAMP );
				dev->mixer.igain = val;
				break;
			case SOUND_MIXER_RECSRC:
				pollux_set_mixer( (val&0x0f), CS42L51_CMD_VOLUME );
				dev->mixer.recSel = (val&0x0f);
				break;
			default:
				ret = -EINVAL;
		}
	}
	
													
	if (ret == 0 && _IOC_DIR(cmd) & _IOC_READ) 
	{
		int nr = _IOC_NR(cmd);
		ret = 0;

		switch (nr) 
		{
			case SOUND_MIXER_VOLUME:     val = dev->mixer.volume;	break;
			case SOUND_MIXER_BASS:       val = dev->mixer.bass;		break;
			case SOUND_MIXER_TREBLE:     val = dev->mixer.treble;	break;
			case SOUND_MIXER_MIC:        val = dev->mixer.micAmp;	break;
			case SOUND_MIXER_LINE:       val = dev->mixer.igain;	break;
			case SOUND_MIXER_RECSRC:     val = dev->mixer.recSel;	break;
#if 0		
			case SOUND_MIXER_RECMASK:    val = REC_MASK;			break;
			case SOUND_MIXER_DEVMASK:    val = DEV_MASK;			break;
#endif	
			case SOUND_MIXER_CAPS:       val = 0;					break;
			case SOUND_MIXER_STEREODEVS: val = 0;					break;
			default:	val = 0;		ret = -EINVAL;				break;
		}

		if (ret == 0)
			ret = put_user(val, (int *)arg);
	}


out:
	return ret;
}

static struct file_operations pollux_mixer_fops = {
    .ioctl = pollux_mixer_ioctl,
    .llseek = no_llseek,
    .owner = THIS_MODULE,
};


////////////////////////////////////////////////////////////////////////////////////////

/*
 * AUDIO INTERFACE 
*/  	

void aout_transmit(void *data)
{
	struct mp2530_dma_channel_t *p = (struct mp2530_dma_channel_t *)data;
	audio_buf_t *dp = (audio_buf_t *)(p->curp);

	mp2530_dma_transfer_mem_to_io( p, dp->dma_addr, POLLUX_PA_SOUND, 16, dp->size);
}

void ain_transmit(void *data)
{
	struct mp2530_dma_channel_t *p = (struct mp2530_dma_channel_t *)data;
	audio_buf_t *dp = (audio_buf_t *)(p->curp);
	
	mp2530_dma_transfer_io_to_mem(p, POLLUX_PA_SOUND, 16, dp->dma_addr, 32768);
}

/*
 * This function frees all buffers
 */

static void
audio_clear_buf(audio_stream_t * s)
{
    /*
     * ensure DMA won't run anymore 
     */
    s->active = 0;
    s->stopped = 0;
    mp2530_dma_flush_all(s->dma_ch);

    if (s->buffers)
    {
        kfree(s->buffers);
        s->buffers = NULL;
    }

    s->buf_idx = 0;
    s->buf = NULL;
}

/*
 * This function allocates the buffer structure array and buffer data space
 * according to the current number of fragments and fragment size.
 */
static int
audio_setup_buf(audio_stream_t * s)
{
    int             frag;
    
    if (s->buffers)
        return -EBUSY;

    s->buffers = kmalloc(sizeof(audio_buf_t) * s->nbfrags, GFP_KERNEL);
    if (!s->buffers)
        goto err;
    memzero(s->buffers, sizeof(audio_buf_t) * s->nbfrags);

    for (frag = 0; frag < s->nbfrags; frag++)
    {
        audio_buf_t    *b = &s->buffers[frag];

        b->start = (char *) (POLLUX_SOUND_VIO_BASE + frag * s->fragsize);
        b->dma_addr = (POLLUX_SOUND_PIO_BASE + frag * s->fragsize);
        b->stream = s;
        sema_init(&b->sem, 1);
    }

    s->buf_idx = 0;
    s->buf = &s->buffers[0];
    s->bytecount = 0;
    s->fragcount = 0;

    return 0;

  err:
    printk(AUDIO_NAME ": unable to allocate audio memory\n ");
    audio_clear_buf(s);
    return -ENOMEM;
}

/*
 * This function yanks all buffers from the DMA code's control and
 * resets them ready to be used again.
 */

static void
audio_reset_buf(audio_stream_t * s)
{
    int frag;

    s->active = 0;
    s->stopped = 0;
    mp2530_dma_flush_all(s->dma_ch);
    if (s->buffers)
    {
        for (frag = 0; frag < s->nbfrags; frag++)
        {
            audio_buf_t    *b = &s->buffers[frag];

            b->size = 0;
            sema_init(&b->sem, 1);
        }
    }
    s->bytecount = 0;
    s->fragcount = 0;
}

/*
 * DMA callback functions
 */
static void
audio_dmaout_done_callback(void *buf_id)
{
    audio_buf_t    *b = (audio_buf_t *) buf_id;
    audio_stream_t *s = b->stream;

	/*
     * Accounting 
     */
    s->bytecount += (b->size);
    s->fragcount++;
   	b->size = 0;
    /*
     * Recycle buffer 
     */
    up(&b->sem);

    /*
     * And any process polling on write. 
     */
    wake_up(&s->wq);
}

static void
audio_dmain_done_callback(void *buf_id)
{
    audio_buf_t    *b = (audio_buf_t *) buf_id;
    audio_stream_t *s = b->stream;
	
	/*
     * Accounting 
     */
    s->bytecount += (b->size);
    s->fragcount++;

    /*
     * Recycle buffer 
     */
    up(&b->sem);

    /*
     * And any process polling on write. 
     */
    wake_up(&s->wq);
}

static int
audio_sync(struct file *file)
{
    audio_stream_t *s = dev->output;
    audio_buf_t    *b;

    if (!(file->f_mode & FMODE_WRITE) || !s->buffers /* || s->mapped */ )
        return 0;

    /*
     * Send current buffer if it contains data.  Be sure to send
     * a full sample count.
     */
    b = s->buf;
    if (b->size &= ~3)
    {
        down(&b->sem);
        mp2530_dma_queue_buffer(s->dma_ch, (void *) b);
        b->size = 0;
        NEXT_BUF(s, buf);
    }

    /*
     * Let's wait for the last buffer we sent i.e. the one before the
     * current buf_idx.  When we acquire the semaphore, this means either:
     * - DMA on the buffer completed or
     * - the buffer was already free thus nothing else to sync.
     */
    b = s->buffers + ((s->nbfrags + s->buf_idx - 1) % s->nbfrags);
    if (down_interruptible(&b->sem))
        return -EINTR;
    up(&b->sem);
    return 0;
}

static inline int
copy_from_user_mono_stereo(char *to, const char *from, int count)
{
    u_int          *dst = (u_int *) to;
    const char     *end = from + count;



    if( !access_ok(VERIFY_READ, from, count) )
        return -EFAULT;

    if ((int) from & 0x2)
    {
        u_int           v;

        __get_user(v, (const u_short *) from);
        from += 2;
        *dst++ = v | (v << 16);
    }

    while (from < end - 2)
    {
        u_int           v,
                        x,
                        y;

        __get_user(v, (const u_int *) from);
        from += 4;
        x = v << 16;
        x |= x >> 16;
        y = v >> 16;
        y |= y << 16;
        *dst++ = x;
        *dst++ = y;
    }

    if (from < end)
    {
        u_int           v;

        __get_user(v, (const u_short *) from);
        *dst = v | (v << 16);
    }

    return 0;
}

int
pollux_audio_write(struct file *file, const char *buffer, size_t count, loff_t * ppos)
{
    const char     *buffer0 = buffer;
    audio_stream_t *s = dev->output;
    int chunksize, ret = 0;
	
	switch (file->f_flags & O_ACCMODE)
    {
    	case O_WRONLY:
    	case O_RDWR:
        	break;
    	default:
        	return -EPERM;
    }
    
    if (!s->buffers && audio_setup_buf(s))
    {
        return -ENOMEM;
    }


    while (count > 0)
    {
        audio_buf_t    *b = s->buf;

        /*
         * Wait for a buffer to become free 
         */
        if (file->f_flags & O_NONBLOCK)
        {
            ret = -EAGAIN;
            if (down_trylock(&b->sem))
            	break;
        	
        }
        else
        {
            ret = -ERESTARTSYS;
            if (down_interruptible(&b->sem))
           		break;
        }


        /*
         * Feed the current buffer 
         */
        if (s->channels == 2)
        {
            chunksize = s->fragsize - b->size;

            if (chunksize > count)
                chunksize = count;
                
            if (copy_from_user(b->start + b->size, buffer, chunksize))
            {
                up(&b->sem);
                return -EFAULT;
            }

            b->size += chunksize;
        }
        else
        {
            chunksize = (s->fragsize - b->size) >> 1;

            if (chunksize > count)
                chunksize = count;

            if (copy_from_user_mono_stereo(b->start + b->size,
                                           buffer, chunksize))
            {
                up(&b->sem);
                return -EFAULT;
            }

            b->size += chunksize * 2;
        }

        buffer += chunksize;
        count -= chunksize;
		
		if (b->size < s->fragsize)
        {
            up(&b->sem);
            break;
        }

        /*
         * Send current buffer to dma 
         */

        s->active = 1;
        mp2530_dma_queue_buffer(s->dma_ch, (void *)b );
        NEXT_BUF(s, buf);
    	
    }

    if ((buffer - buffer0))
        ret = buffer - buffer0;

	
	return ret;
}

static void
audio_prime_dma(audio_stream_t * s)
{
    int i;

    s->active = 1;
    for (i = 0; i < s->nbfrags; i++)
    {
        audio_buf_t    *b = s->buf;

        down(&b->sem);
        mp2530_dma_queue_buffer(s->dma_ch, (void *)b );
        NEXT_BUF(s, buf);
    }
}

int
pollux_audio_read(struct file *file, char *buffer, size_t count, loff_t * ppos)
{
    const char     *buffer0 = buffer;
    audio_stream_t *s = dev->input;
    int chunksize, ret = 0;

#if 0
    if (ppos != &file->f_pos)
        return -ESPIPE;
#endif        

    /*
     * pre dma 
     */
    if (!s->active)
    {
        if (!s->buffers && audio_setup_buf(s))
            return -ENOMEM;

        audio_prime_dma(s);
    }
	
    while (count > 0)
    {
        audio_buf_t    *b = s->buf;

        /*
         * Wait for a buffer to become full 
         */
        if (file->f_flags & O_NONBLOCK)
        {
            ret = -EAGAIN;
            if (down_trylock(&b->sem))
                break;
        }
        else
        {
            ret = -ERESTARTSYS;
            if (down_interruptible(&b->sem))
            {
//              if (signal_pending(current))
//                  return -ERESTARTSYS;
                break;
            }
        }

        /*
         * Grab data from the current buffer 
         */
        b->size=32768;
        chunksize = b->size;
        if (chunksize > count)
            chunksize = count;
        dprintk("read %d from %d\n", chunksize, s->buf_idx);
        
        if (copy_to_user(buffer, b->start + s->fragsize - b->size, chunksize))
        {
            up(&b->sem);
            return -EFAULT;
        }
        
        b->size -= chunksize;
        buffer += chunksize;
        count -= chunksize;
        if (b->size > 0)
        {
            up(&b->sem);
            break;
        }

        /*
         * Make current buffer available for DMA again 
         */
        mp2530_dma_queue_buffer(s->dma_ch, (void *)b );
        
        NEXT_BUF(s, buf);
    }

    if ((buffer - buffer0))
        ret = buffer - buffer0;

    return ret;
}


unsigned int
pollux_audio_poll(struct file *file, struct poll_table_struct *wait)
{
    audio_stream_t *is = dev->input;
    audio_stream_t *os = dev->output;
    unsigned int    mask = 0;
    int             i;

    if (file->f_mode & FMODE_READ)
    {
        /*
         * Start audio input if not already active 
         */
        if (!is->active)
        {
            if (!is->buffers && audio_setup_buf(is))
                return -ENOMEM;

            audio_prime_dma(is);
        }
        poll_wait(file, &is->wq, wait);
    }

    if (file->f_mode & FMODE_WRITE)
    {
        if (!os->buffers && audio_setup_buf(os))
            return -ENOMEM;
        poll_wait(file, &os->wq, wait);
    }

    if (file->f_mode & FMODE_READ)
    {
        for (i = 0; i < is->nbfrags; i++)
        {
            if (atomic_read(&is->buffers[i].sem.count) > 0)
            {
                mask |= POLLIN | POLLRDNORM;
                break;
            }
        }
//      }
    }
    if (file->f_mode & FMODE_WRITE)
    {
        for (i = 0; i < os->nbfrags; i++)
        {
            if (atomic_read(&os->buffers[i].sem.count) > 0)
            {
                mask |= POLLOUT | POLLWRNORM;
                break;
            }
        }
//      }
    }

    return mask;
}

static int
audio_set_fragments(audio_stream_t * s, int val)
{
    if (s->active)
        return -EBUSY;
    if (s->buffers)
        audio_clear_buf(s);
    s->nbfrags = (val >> 16) & 0x7FFF;

    val &= 0xffff;
    if (val < 4)
        val = 4;
    if (val > 15)
        val = 15;
    s->fragsize = 1 << val;
    if (s->nbfrags < 2)
        s->nbfrags = 2;
    if (s->nbfrags * s->fragsize > 256 * 1024)
        s->nbfrags = 256 * 1024 / s->fragsize;
    if (audio_setup_buf(s))
        return -ENOMEM;
    return val | (s->nbfrags << 16);
}

static long audio_set_dsp_speed(long val, audio_stream_t * s)
{
	unsigned int mclk_div,bitclk_div;
	unsigned long mclk;
	
	if( s->rate == val ) return val;
	
	switch (val) 
	{
		case SYNC_22050:
			mclk_div 	= 26;
			bitclk_div 	= 4;
			break;
		case SYNC_32000:
			mclk_div 	= 18;
			bitclk_div 	= 4;
			break;
		case SYNC_44100:
			mclk_div    = 13;
			bitclk_div  = 4;
			break;
		case SYNC_48000:
			mclk_div 	= 12;
			bitclk_div 	= 4;
			break;
		case SYNC_64000: 
			mclk_div 	= 9;
			bitclk_div  = 4;
			break;
		default:
			return -1;
	}
	
	MES_AUDIO_SetClockDivisor(0 ,mclk_div);
	MES_AUDIO_SetClockDivisor(1 ,bitclk_div);
	
	s->rate = val;
	return val;
}


int
pollux_audio_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
   	audio_stream_t *os = dev->output;
    audio_stream_t *is = dev->input;
    long val;
	int ret;

	
    /*
     * dispatch based on command 
     */
    switch (cmd)
    {
    case SNDCTL_DSP_GETFMTS:
        ret = put_user(AFMT_S16_LE, (int *) arg);
		break;
	
	case SNDCTL_DSP_SETFMT:
		if(get_user(val, (long *) arg) != 0)
		return -EFAULT;
	
		ret = put_user(AFMT_S16_LE, (int *) arg); 		/* we left the format the same */
		break;
	
    case SNDCTL_DSP_STEREO:
        if (get_user(val, (int *) arg))
            return -EINVAL;
        os->channels = (val) ? 2 : 1;
        is->channels = (val) ? 2 : 1;
        return 0;
	
	case SNDCTL_DSP_CHANNELS:
		if(get_user(val, (long *)arg) != 0)
			return -EFAULT;
		/* TODO: set # channels */
        os->channels = (val) ? 2 : 1;
        is->channels = (val) ? 2 : 1;
		ret = put_user(2, (int *) arg); 			/* we only allow 2 channels for now */
		break;
	
	case SNDCTL_DSP_SPEED:
		if(get_user(val, (long *)arg ) != 0)
			return -EFAULT;
		/* TODO: set sample rate = val */
		if((file->f_mode & FMODE_READ))
 			val = audio_set_dsp_speed( val, is);
 		else 
			val = audio_set_dsp_speed( val, os);
		
		if (val < 0) 
				return -EINVAL;
		
		put_user(val, (long *) arg);
		break;
		
    case SNDCTL_DSP_GETBLKSIZE:
        if (file->f_mode & FMODE_WRITE)
            return put_user(os->fragsize, (long *) arg);
        else
            return put_user(is->fragsize, (int *) arg);

    case SNDCTL_DSP_SETFRAGMENT:
        if (get_user(val, (long *) arg))
            return -EFAULT;
        if (file->f_mode & FMODE_READ)
        {
            int ret = audio_set_fragments(is, val);

            if (ret < 0)
                return ret;
            ret = put_user(ret, (int *) arg);
            if (ret)
                return ret;
        }
        if (file->f_mode & FMODE_WRITE)
        {
            int ret = audio_set_fragments(os, val);
			printk("audio_set_fragments \n");
            if (ret < 0)
                return ret;
            ret = put_user(ret, (int *) arg);
            if (ret)
                return ret;
        }
        return 0;

    case SNDCTL_DSP_SYNC:
        return audio_sync(file);

    case SNDCTL_DSP_GETOSPACE:
        {
            audio_buf_info *inf = (audio_buf_info *) arg;
            int err = !access_ok(VERIFY_WRITE, inf, sizeof(*inf));
            int i;
            int frags = 0, bytes = 0;

            if (!(file->f_mode & FMODE_WRITE))
                return -EINVAL;

            if (err)
                return err;

            if (!os->buffers && audio_setup_buf(os))
                return -ENOMEM;
                
            for (i = 0; i < os->nbfrags; i++)
            {
                if (atomic_read(&os->buffers[i].sem.count) > 0)
                {
                    if (os->buffers[i].size == 0)
                        frags++;
                    bytes += os->fragsize - os->buffers[i].size;
                }
            }
			
            put_user(frags, &inf->fragments);
            put_user(os->nbfrags, &inf->fragstotal);
            put_user(os->fragsize, &inf->fragsize);
            put_user(bytes, &inf->bytes);
            break;
        }

    case SNDCTL_DSP_GETISPACE:
        {
            audio_buf_info *inf = (audio_buf_info *) arg;
            int err = !access_ok(VERIFY_WRITE, inf,sizeof(*inf));

            int             i;
            int             frags = 0,bytes = 0;

            if (!(file->f_mode & FMODE_READ))
                return -EINVAL;

            if (err)
                return err;

            if (!is->buffers && audio_setup_buf(is))
                return -ENOMEM;
            for (i = 0; i < is->nbfrags; i++)
            {
                if (atomic_read(&is->buffers[i].sem.count) > 0)
                {
                    if (is->buffers[i].size == is->fragsize)
                        frags++;
                    bytes += is->buffers[i].size;
                }
            }
            put_user(frags, &inf->fragments);
            put_user(is->nbfrags, &inf->fragstotal);
            put_user(is->fragsize, &inf->fragsize);
            put_user(bytes, &inf->bytes);
            break;
        }

    case SNDCTL_DSP_RESET:
        if (file->f_mode & FMODE_READ)
        {
            audio_reset_buf(is);
        }
        if (file->f_mode & FMODE_WRITE)
        {
            audio_reset_buf(os);
        }
        return 0;

    case SNDCTL_DSP_NONBLOCK:
        file->f_flags |= O_NONBLOCK;
        return 0;

    case SNDCTL_DSP_POST:
    case SNDCTL_DSP_SUBDIVIDE:
    case SNDCTL_DSP_GETCAPS:
    case SNDCTL_DSP_GETTRIGGER:
    case SNDCTL_DSP_SETTRIGGER:
    case SNDCTL_DSP_GETIPTR:
    case SNDCTL_DSP_GETOPTR:
    case SNDCTL_DSP_MAPINBUF:
    case SNDCTL_DSP_MAPOUTBUF:
    case SNDCTL_DSP_SETSYNCRO:
    case SNDCTL_DSP_SETDUPLEX:
        return -ENOSYS;
    default:
        return pollux_mixer_ioctl(inode, file, cmd, arg);
    }

    return 0;
}

static int 
pollux_audio_open(struct inode *inode, struct file *file)
{
	audio_stream_t *is = dev->input;
    audio_stream_t *os = dev->output;
    int err,ret;       
	
	if (dev->isOpen) return -EBUSY;			// have only one device
	dev->isOpen++;
	
    err = -ENODEV;
    if ((file->f_mode & FMODE_WRITE) && !os) goto out;
    if ((file->f_mode & FMODE_READ) && !is) goto out;
    err = -EBUSY;
    if ((file->f_mode & FMODE_WRITE) && dev->wr_ref) goto out;
    if ((file->f_mode & FMODE_READ) && dev->rd_ref) goto out;
    	
	if (file->f_mode & FMODE_WRITE)
    {
    	os->dma_ch = AOUT_DMA_CH;
		ret = mp2530_dma_open_channel(os->dma_ch, 24, OPMODE_MEM_TO_IO, audio_dmaout_done_callback, aout_transmit); 
		if( ret < 0 ){
			printk("ouput dma open error\n");
			return ret;
		}
    }
    
    if (file->f_mode & FMODE_READ)
    {
    	is->dma_ch = AIN_DMA_CH;
		ret = mp2530_dma_open_channel(is->dma_ch, 26, OPMODE_IO_TO_MEM, audio_dmain_done_callback, ain_transmit); 
		if( ret < 0 ){
			printk("input dma open error\n");
			return ret;
		}	
	}

    if ((file->f_mode & FMODE_WRITE))
    {
        dev->wr_ref = 1;
        audio_clear_buf(os);
        os->fragsize = AUDIO_FRAGSIZE_DEFAULT;
        os->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		init_waitqueue_head(&os->wq);    
	}
    
    if ((file->f_mode & FMODE_READ))
    {
        dev->rd_ref = 1;
        audio_clear_buf(is);
        is->fragsize = AUDIO_FRAGSIZE_DEFAULT;
        is->nbfrags = AUDIO_NBFRAGS_DEFAULT;
		init_waitqueue_head(&is->wq);

        cs42L51_write_byte(CS42L51_PWR_CTRL, STEREO_DAC_OFF_MICA_PGAA_ON);  //02
		cs42L51_write_byte(CS42L51_SPDPWR_CTRL, MICA_ON_MICBIAS_OFF);  //03	
		cs42L51_write_byte(CS42L51_IF_CTRL, 0x0D);  //04	
		cs42L51_write_byte(0x09, 0x60);  
		cs42L51_write_byte(CS42L51_PCMADC_CHMIXER, 0x88);  //18	

		MES_AUDIO_SetPCMINDataWidth(BUFF_WIDTH_DATA16);
		MES_AUDIO_SetAudioBufferPCMINEnable(CTRUE);
		MES_AUDIO_SetI2SInputEnable(CTRUE);	
		MES_AUDIO_SetInterruptEnable(0, CTRUE);	   
		MES_AUDIO_ClearInterruptPending(0);   // input 
	
	}
    
    cs42L51_write_byte(0x08, 0x00);
    
    if( dev->headphone_mode && dev->mixer.volume_index )
        pollux_gpio_setpin(SND_SPK_ON_OFF ,1);
        
	err = 0;
out:
    return err;
}

int
pollux_audio_release(struct inode *inode, struct file *file)
{
    audio_stream_t *is = dev->input;
    audio_stream_t *os = dev->output;
    
    cs42L51_write_byte(0x08, 0x03);
    
    if (file->f_mode & FMODE_READ)
    {
        audio_clear_buf(is);
        mp2530_dma_close_channel(is->dma_ch);
        dev->rd_ref = 0;
        cs42L51_write_array(cs42L51_settings,sizeof(cs42L51_settings)/2);
    	MES_AUDIO_SetAudioBufferPCMINEnable(CFALSE);
    	MES_AUDIO_SetI2SInputEnable(CFALSE);	
    }

    if (file->f_mode & FMODE_WRITE)
    {      
        audio_sync(file);
        audio_clear_buf(os);
        mp2530_dma_close_channel(os->dma_ch);
        dev->wr_ref = 0;
	}
	
	pollux_gpio_setpin(SND_SPK_ON_OFF ,0);
	
	dev->isOpen--;	
	return 0;
}

loff_t pollux_audio_llseek(struct file *file, loff_t offset, int origin)
{
    return -ESPIPE;
}

static struct file_operations pollux_audio_fops = {
    .open = pollux_audio_open,
    .release = pollux_audio_release,
    .write = pollux_audio_write,
    .read = pollux_audio_read,
    .poll = pollux_audio_poll,
    .ioctl = pollux_audio_ioctl,
    .llseek = pollux_audio_llseek,
    .owner = THIS_MODULE,
};

//////////////////////////////////////////////////////////////////////////////////////////////

static int __init pollux_audio_init(void)
{
	int ret,i;
	
	pollux_i2c_enable(CTRUE);
	i2c_add_driver(&cs42l51_driver);
	
	if(dev->new_client == 0)
		printk(KERN_ALERT "audio: failed to connect to i2c bus\n");
	else 
	{ /* read chip ID from codec */
		dev->chip_id = cs42L51_read_byte(CS42L51_CHIP_ID);
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE		
		printk("CS42L51_CHIP ID:0x%x \n",dev->chip_id);
#endif		
		
		if(dev->chip_id != 0xd9 )
		{
	        for(i = 0 ; i < 3 ; i++)
	        {
	            pollux_i2c_enable(CFALSE);
	            i2c_del_driver(&cs42l51_driver);
	            
	            pollux_i2c_enable(CTRUE);
	            i2c_add_driver(&cs42l51_driver);
	            dev->chip_id = cs42L51_read_byte(CS42L51_CHIP_ID);
	            if(dev->chip_id == 0xd9 ) goto snd_init_ok;
	        }        
	        
	        printk(KERN_ERR "audio: failed to read chip ID\n");
            goto err_snd_init;    
        }

snd_init_ok:
        
		ret = cs42L51_init();
		if(ret < 0)
		    printk(KERN_ERR "audio: codec init failed\n");
		else
		{
		    dev->input = &input_stream;
			dev->output = &output_stream;
			dev->input->name = "i2s audio in";
			dev->output->name = "i2s audio out";
			dev->isOpen = 0;    // device not opened
			init_polluxI2S(dev->output);
			
			/* set up work queue for using the codec */
		    dev->codec_tasks = create_singlethread_workqueue("codec tasks");
			/* volume control */
		    INIT_WORK(&dev->volume_work, pollux_set_volume);
		    /* jack_dect control */
		    INIT_WORK(&dev->headphone_work, pollux_set_headphones);
		    
		    /* set up periodic sampling */
		    setup_timer(&dev->volume_timer, volume_monitor_task, 
				(unsigned long)dev);
			setup_timer(&dev->headphone_timer, headphone_monitor_task,
				(unsigned long)dev);
			
			dev->volume_timer.expires = get_jiffies_64() + 
						VOLUME_SAMPLING_J;
		    dev->headphone_timer.expires = get_jiffies_64() +
						HEADPHONE_SAMPLING_J;
			add_timer(&dev->volume_timer); 
		    add_timer(&dev->headphone_timer);
		    	
			dev->audio_dev_dsp = register_sound_dsp(&pollux_audio_fops, -1);
    		dev->audio_dev_mixer = register_sound_mixer(&pollux_mixer_fops, -1);       

#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE				
				printk(AUDIO_NAME_VERBOSE " initialized\n");
#endif				
				return 0;	
		}
	}


err_snd_init:
	
	i2c_del_driver(&cs42l51_driver);
	pollux_i2c_enable(CFALSE);
	
	return -1;
}

static void __exit pollux_audio_exit(void)
{
	pollux_i2c_enable(CFALSE);
	i2c_del_driver(&cs42l51_driver);
	unregister_sound_dsp(dev->audio_dev_dsp);
	unregister_sound_mixer(dev->audio_dev_mixer);		
	destroy_workqueue(dev->codec_tasks);
	
	printk(AUDIO_NAME_VERBOSE " unloaded\n");
}

module_init(pollux_audio_init);
module_exit(pollux_audio_exit);

MODULE_DESCRIPTION("audio interface for the pollux");
MODULE_LICENSE("GPL");

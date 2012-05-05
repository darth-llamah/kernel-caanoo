/*
 * sound/oss/pollux-wm1800/pollux-audio.c
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
#include <asm/arch/regs-clock-power.h>

#include "pollux-iis.h"
#include "pollux-wm1800.h"

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
	int flag_line_mono;
	int step_nums;		
	int vol_div100;
	int scale;
	int	mod_cnt;
}wm_1800_mixer;


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
	u32 cpu_pll;
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
	int audio_dev_dsp;
	int audio_dev_mixer;
	int rd_ref;
	int wr_ref;
	int audio_mix_modcnt;
	struct timer_list volume_timer;
	int ADC_reading;
	int last_reading;    
	struct workqueue_struct *codec_tasks;
	struct work_struct volume_work;         /* monitor volume  */
	int isOpen;			                    /* monitor open/close state	*/
	wm_1800_mixer mixer;
} audio_dev;


struct audio_device *dev = &audio_dev;

#define AOUT_DMA_CH 	4
#define AIN_DMA_CH 		5

#define AUDIO_NAME		"pollux-audio"
#define AUDIO_NBFRAGS_DEFAULT	8
#define AUDIO_FRAGSIZE_DEFAULT	32768

#define AUDIO_NAME_VERBOSE 		"WM1800 audio driver"

#define VOLUME_CAPTURE_RANGE	3	            // Min Volume wheel change +/-
//#define VOLUME_SAMPLING_J	HZ / 10	            // sample wheel every 100 ms
#define VOLUME_SAMPLING_J	HZ / 6	            
//#define VOLUME_SAMPLING_J	HZ / 5	            // sample wheel every 200 ms
//#define VOLUME_SAMPLING_J	HZ / 2

#define VOLUME_STEP_NUM 20
#define VOLUME_ADC_MINIMUM 70  //대부분 45 ~ 55 사이 값
#define VOLUME_ADC_MAXIMUM 950 //대부분 965 ~ 975 사이 값 
#define VOLUME_STEP_MAX 0x12 
//#define VOLUME_STEP_MAX VOLUME_STEP_NUM 



#define GPIO_EAR_MUTE           POLLUX_GPA11

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

static int wm1800_write_byte(unsigned char reg, unsigned int value)
{
	unsigned char buf[2];
	int ret;
	
	/* data is
	 *   D15..D9 WM1800 register offset
	 *   D8...D0 register data
	 */
	
	buf[0] = (reg << 1) | ((value >> 8) & 0x0001);
	buf[1] = value & 0x00ff;

	if (2 != (ret = i2c_master_send(dev->new_client, buf, 2)) )
	{
		dprintk("i2c i/o error: rc == %d (should be 2)\n", ret);
		return -1;
	}
	
	return 0;
}


/* Addresses to scan */
static unsigned short normal_i2c[] = { 0x1A, I2C_CLIENT_END };
/* I2C Magic */
I2C_CLIENT_INSMOD;

static int wm1800_attach_adapter(struct i2c_adapter *adapter);
static int wm1800_detect(struct i2c_adapter *adapter, int address, int kind);
static int wm1800_detach_client(struct i2c_client *client);

static struct i2c_driver wm1800_driver = {
	.driver = {
		.name	= "wm1800",
	},
	.attach_adapter	= wm1800_attach_adapter,
	.detach_client	= wm1800_detach_client,
};

struct wm1800_data {
	struct i2c_client client;
	int yyy;
};

static int wm1800_attach_adapter(struct i2c_adapter *adapter)
{
	
	return i2c_probe(adapter, &addr_data, wm1800_detect);
}

static int wm1800_detect(struct i2c_adapter *adapter, int address, int kind)
{
	struct wm1800_data *data;
	int err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		goto exit;

	if (!(data = kzalloc(sizeof(struct wm1800_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	dev->new_client = &data->client;
	i2c_set_clientdata(dev->new_client, data);
	dev->new_client->addr = address;
	dev->new_client->adapter = adapter;
	dev->new_client->driver = &wm1800_driver;
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

static int wm1800_detach_client(struct i2c_client *client)
{
	int err;
	struct wm1800_data *data = i2c_get_clientdata(client);

	if ((err = i2c_detach_client(client)))
		return err;

	kfree(data);
	return 0;
}


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
	MES_AUDIO_SetBaseAddress(POLLUX_VA_SOUND); //
	MES_AUDIO_OpenModule();
	MES_AUDIO_SetClockDivisorEnable(CFALSE);
	
	MES_AUDIO_SetClockSource(0, 0);  //PLL0
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
	MES_AUDIO_SetPCMOUTDataWidth(BUFF_WIDTH_DATA16);	
	MES_AUDIO_SetAudioBufferPCMOUTEnable(CTRUE);	
	MES_AUDIO_SetI2SOutputEnable(CTRUE);
	MES_AUDIO_SetInterruptEnable(1, CTRUE);	   
	MES_AUDIO_SetI2SLinkOn();
	MES_AUDIO_ClearInterruptPending(1);    

	return 0;
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

static int get_volume_step(int adc_read, int scale) 
{
	int ret_val = 0;
	int adc_value = adc_read;
	
	if( adc_value < VOLUME_ADC_MINIMUM )  adc_value= 0;
    else adc_value -= VOLUME_ADC_MINIMUM;      
        
    ret_val = adc_value / dev->mixer.scale;
    if(ret_val > VOLUME_STEP_MAX) ret_val = VOLUME_STEP_MAX;  

	return ret_val;
}


////////////////////////////////////////////////////////////////////////////////////////
/*
 * WM1800 REGISTER SETING
*/  	

static void wm1800_set_volume(unsigned char volume_index)
{
	unsigned char val;
	
	val = volume_Htable[dev->mixer.volume_index];
	wm1800_write_byte(WM1800_LOUT1, val | 0x80);
	wm1800_write_byte(WM1800_ROUT1, val | 0x80);  		
	
	//wm1800_write_byte(WM1800_LOUT1, val | 0x180);
	wm1800_write_byte(WM1800_ROUT1, val | 0x180);
  
	val = volume_Stable[dev->mixer.volume_index];
	wm1800_write_byte(WM1800_LOUT2, val | 0x80);		
	wm1800_write_byte(WM1800_ROUT2, val | 0x180);  	
}

static void wm1800_set_igain(unsigned char left, unsigned char right)
{
	int index_l, index_r;
/*
	printk("wm1800_set_igain \n");

	if (right > 100) right = 100;
	if (left > 100) left = 100;	
	
	index_l = left / 10;
	index_r = right / 10; 

	left = involume_table[index_l];
	right = involume_table[index_r];

	wm1800_write_byte(WM1800_LADC, left | 0x100);
	wm1800_write_byte(WM1800_RADC, right | 0x100);	
*/
}

static void wm1800_set_3d(unsigned char depth, unsigned char enable, u32 rate )
{
	int index = depth / 10;
	int depth_val;
	
	if(!enable){
		wm1800_write_byte(WM1800_3D, 0);	
	}else{	
		depth_val =  ( depth3d_table[index] << 1 );
		
		if(rate < SYNC_32000) 	
			wm1800_write_byte(WM1800_3D, depth_val | 0x01 | 0x60);
		else	
			wm1800_write_byte(WM1800_3D, depth_val | 0x01);
	}
}

static int wm1800_set_power(int event)
{
	switch(event){
	case WM1800_PWR_DOWN :
	    
	    MES_AUDIO_SetI2SOutputEnable(CFALSE);
	    MES_AUDIO_SetInterruptEnable(1, CFALSE);	
	    MES_AUDIO_SetClockOutEnb(1, CFALSE);
	    MES_AUDIO_SetClockDivisorEnable(CFALSE);

	    wm1800_write_byte(WM1800_LDAC, 0x100);
		wm1800_write_byte(WM1800_RDAC, 0x100);	    
        wm1800_write_byte(WM1800_LOUT2, 0x100);
		wm1800_write_byte(WM1800_ROUT2, 0x100); 
		wm1800_write_byte(WM1800_LOUT1, 0x100);
		wm1800_write_byte(WM1800_ROUT1, 0x100);  				

		msleep(50);

		wm1800_write_byte(WM1800_DACCTL1, 0x08);
		wm1800_write_byte(WM1800_APOP1, 0x94);
		wm1800_write_byte(WM1800_POWER2, 0x1e0);
		msleep(100);
		wm1800_write_byte(WM1800_POWER1, 0x00);
		msleep(300);

		wm1800_write_byte(WM1800_POWER2, 0x00);
		wm1800_write_byte(WM1800_RESET, 0x00);
		msleep(400);
	
		break;
	case WM1800_PWR_SPK_STANDDBY :
		break;
	case WM1800_PWR_HP_STANDBY: 	
		break;
	case WM1800_PWR_HP_ONLY :
		break;
	case WM1800_PWR_SPK_ONLY :
		break;
	case WM1800_PWR_SKP_HP :
		break;
	}
	return 0;
}

static int wm1800_input_enable(unsigned int select_line, unsigned int flag ,int mic_bias)
{
	
	wm1800_write_byte(WM1800_POWER1, 0x0C0 | flag | mic_bias | 0x08);				
	wm1800_write_byte(WM1800_POWER3, 0x00C | flag);	

	 /* ALC ENABLE */
	
	wm1800_write_byte(WM1800_ALC1, 0x14b);
	wm1800_write_byte(WM1800_ALC2, 0x40);
	wm1800_write_byte(WM1800_ALC3, 0x32);
	wm1800_write_byte(WM1800_NOISEG, 0x81);

	switch(flag){
	case IN_STEREO:
		wm1800_write_byte(WM1800_LINPATH, PGA_BOOST_GAIN20 | select_line);
		wm1800_write_byte(WM1800_RINPATH, PGA_BOOST_GAIN20 | select_line);
			
		wm1800_write_byte(WM1800_LINVOL, BASE_LINE_VOLUME | 0x100);
		wm1800_write_byte(WM1800_RINVOL, BASE_LINE_VOLUME  | 0x100);
		wm1800_write_byte(WM1800_LADC, BASE_ADC_VOLUME | 0x100);
		wm1800_write_byte(WM1800_RADC, BASE_ADC_VOLUME | 0x100);		
		break;	
	case IN_MONO_LEFT:
		//wm1800_write_byte(WM1800_LINPATH, MIC_BOOST_MIXER | PGA_BOOST_GAIN20 | select_line | 0x100 );
		wm1800_write_byte(WM1800_LINPATH,  MIC_BOOST_MIXER | PGA_BOOST_GAIN13 | select_line | 0x100 );
		wm1800_write_byte(WM1800_RINPATH, 0);
		
		wm1800_write_byte(WM1800_LINVOL, BASE_LINE_VOLUME | 0x100);
		wm1800_write_byte(WM1800_LADC, BASE_ADC_VOLUME | 0x100);
		break;
	case IN_MONO_RIGHT:
		wm1800_write_byte(WM1800_LINPATH, 0);
		wm1800_write_byte(WM1800_RINPATH, MIC_BOOST_MIXER | PGA_BOOST_GAIN20 | select_line);
		
		wm1800_write_byte(WM1800_RINVOL, BASE_LINE_VOLUME | 0x100);
		wm1800_write_byte(WM1800_RADC, BASE_ADC_VOLUME | 0x100);
		break;
	}
	
	return 0;
}

static void wm1800_input_disable(void)
{
	wm1800_write_byte(WM1800_POWER1, 0x0C0 );				
	wm1800_write_byte(WM1800_POWER3, 0x00C );	
	wm1800_write_byte(WM1800_LINPATH, 0);
	wm1800_write_byte(WM1800_RINPATH, 0);

	wm1800_write_byte(WM1800_ALC1,0);
	wm1800_write_byte(WM1800_ALC2, 0);
	wm1800_write_byte(WM1800_ALC3, 0);
	wm1800_write_byte(WM1800_NOISEG, 0);	

}

static int wm1800_init(void)
{	
	unsigned char val;

	wm1800_write_byte(WM1800_APOP1, 0x094);
	wm1800_write_byte(WM1800_APOP2, 0x40);
	mdelay(400);
	
	wm1800_write_byte(WM1800_POWER2, 0xf0); 	
	wm1800_write_byte(WM1800_APOP2, 0x00);
	wm1800_write_byte(WM1800_POWER1, 0x80);
	mdelay(100);
	
	wm1800_write_byte(WM1800_LINPATH, 0x000); 	/* Disable LMN1,LMP2 */
	wm1800_write_byte(WM1800_RINPATH, 0x000);   /* Disable path to RBMIX */
	wm1800_write_byte(WM1800_INBMIX1, 0x000);   /* Disable LIN to LBMIX  */ 	
	wm1800_write_byte(WM1800_INBMIX2, 0x000);  	/* Disable RIN to RBMIX */
  	wm1800_write_byte(WM1800_BYPASS1, 0x050);	/* Disable LB2LO */	
	wm1800_write_byte(WM1800_BYPASS2, 0x050);	/* Disable RB2RO */

	wm1800_write_byte(WM1800_POWER1, 0xC0);
	//wm1800_write_byte(WM1800_APOP1, 0x01);    
	wm1800_write_byte(WM1800_POWER2, 0x1F8);	/* Power Enable DAC_LR,SPKLR,HPLR */

	/* Headphone Jack Detect */
	wm1800_write_byte(WM1800_ADDCTL2, 0x60);    /* Enable headphone switch && HPDETECT high = speaker */
	/* Temperature Sensor Enable */ 
	//wm1800_write_byte(WM1800_ADDCTL4, 0x00A);   	/* JD2(LINPUT3) used for jack detect input  */ 	
	wm1800_write_byte(WM1800_ADDCTL4, 0x00B);   	/* JD2(LINPUT3) used for jack detect input  */ 	
    //wm1800_write_byte(WM1800_ADDCTL4, 0x009);   	/* JD2(LINPUT3) used for jack detect input  */ 	
    
	//wm1800_write_byte(WM1800_ADDCTL1, 0x1C3);   /* Jack detect debounce = slow clock enabled */
	wm1800_write_byte(WM1800_ADDCTL1, 0x1C7);     /* Jack detect debounce = slow clock enabled */
    //wm1800_write_byte(WM1800_ADDCTL1, 0xC5);      /* Jack detect debounce = slow clock enabled */
    
	wm1800_write_byte(WM1800_POWER3, 0x0c);		/* Enable VMID=50K ,Vref && Master clock enable */
	wm1800_write_byte(WM1800_CLASSD2, 0x084);   
	wm1800_write_byte(WM1800_CLASSD1, 0xc0);	/* Left and Right Class D Speakers Enabled */ 	
	wm1800_write_byte(WM1800_LOUTMIX, 0x100);
	wm1800_write_byte(WM1800_ROUTMIX, 0x100);	

	/* here code volume set */
	wm1800_write_byte(WM1800_LDAC,  BASE_DAC_VOLUME | 0x100 );
	wm1800_write_byte(WM1800_RDAC,  BASE_DAC_VOLUME | 0x100 );	

	val = volume_Htable[dev->mixer.volume_index];
	wm1800_write_byte(WM1800_LOUT1, val | 0x180);
	wm1800_write_byte(WM1800_ROUT1, val | 0x180);  		
	
	val = volume_Stable[dev->mixer.volume_index];
	wm1800_write_byte(WM1800_LOUT2, val | 0x180);		
	wm1800_write_byte(WM1800_ROUT2, val | 0x180);  	
	
	wm1800_write_byte(WM1800_IFACE1, 0x02);			/* I2S, 16bit Slave Mode */
	wm1800_write_byte(WM1800_DACCTL2, 0x02);
	//wm1800_write_byte(WM1800_DACCTL1, 0x00); 		/* Unmute DAC digital soft mute */
	wm1800_write_byte(WM1800_DACCTL1, 0x06); 		/* Unmute DAC digital soft mute */
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
 * Auto volume && Auto spk 
 
*/ 

static void pollux_set_volume(struct work_struct *work)
{
	//unsigned short reading, diff, org_reading;
	unsigned short reading, diff;
	int index;
	const int diff_threshold = dev->mixer.scale / 2;

	if(!dev->isOpen)
		return;

    /* Range ( 0x30 ~ 0x3d0 ) */
    adc_GetVolume(&reading);
	dprintk("%s reading : %d, index: %d\n", __func__, reading, dev->mixer.volume_index);

    if( dev->ADC_reading > reading) diff =  dev->ADC_reading - reading;
    else if( reading > dev->ADC_reading) diff = reading - dev->ADC_reading;
    else diff = 0;

	if(diff < diff_threshold)
		return;

	index = get_volume_step(reading, dev->mixer.scale);
    
	if(dev->mixer.volume_index != index) {          
		dprintk("volume index change : %d ->%d \n", dev->mixer.volume_index, index);
		/* Fade In/Out */
		if(index > dev->mixer.volume_index ) {
			dev->mixer.volume_index++;
			dev->ADC_reading += dev->mixer.scale;
		}
		else {
			dev->mixer.volume_index--;
			dev->ADC_reading -= dev->mixer.scale;
		}

		wm1800_set_volume( dev->mixer.volume_index);
		dev->mixer.volume = dev->mixer.volume_index *  dev->mixer.vol_div100;
		
        //printk("dev->mixer.volume_index : 0x%x  adc:%d  \n ", dev->mixer.volume_index, dev->ADC_reading);            
    } else {
		dev->ADC_reading = reading; //ADC read value가 volume index 경계에서 흔들릴 때 대비
	}

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


////////////////////////////////////////////////////////////////////////////////////////
/*
 * MIXER INTERFACE 
*/  	

static void pollux_set_mixer(unsigned int val, unsigned char cmd)
{
	unsigned char left,right;
	
	right = ((val >> 8)  & 0xff);
	left = (val  & 0xff);
			
	switch(cmd){
	case WM1800_CMD_VOLUME:	    /* Auto Set volume */		
		break;
	case WM1800_CMD_BASS:  		/* WM1800 Not support */
		break;
	case WM1800_CMD_TREBLE:		/* WM1800 Not Support  */		
		break;
	case WM1800_CMD_IGAIN:
		wm1800_set_igain(left, right);		
		break;
	case WM1800_CMD_MICAMP:					
		wm1800_set_igain(left, right);						
		break;
	case WM1800_CMD_RECSRC:
		break;			
	case WM1800_CMD_3D:
		{
			audio_stream_t *os = dev->output;
			wm1800_set_3d(left, right, os->rate);
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

		strncpy(mi.id, "WM1800", sizeof(mi.id));
		strncpy(mi.name, "pollux WM1800", sizeof(mi.name));
		mi.modify_counter = dev->audio_mix_modcnt;
		return copy_to_user((int *)arg, &mi, sizeof(mi));
	}

	if (cmd == SOUND_MIXER_3DSE){
		ret = get_user(val, (int *)arg);
		if (ret) return -EFAULT;
		pollux_set_mixer(val, WM1800_CMD_3D );
	}

	if (cmd == SOUND_MIXER_PWDOWN){
		wm1800_set_power(WM1800_PWR_DOWN);
		return 0;
	}

    if (_IOC_DIR(cmd) & _IOC_WRITE) 
	{
		ret = get_user(val, (int *)arg);
		if (ret)
			goto out;

		switch (nr) 
		{
			
			case SOUND_MIXER_VOLUME:
				break;
			case SOUND_MIXER_PCM:
				break;
			case SOUND_MIXER_BASS:
				break;
			case SOUND_MIXER_TREBLE:
				break;	
			case SOUND_MIXER_LINE:
				pollux_set_mixer(val, WM1800_CMD_IGAIN );
				dev->mixer.igain = val;
				break;
			case SOUND_MIXER_MIC:
				pollux_set_mixer(val, WM1800_CMD_MICAMP );
				dev->mixer.micAmp = val;
				break;
			case SOUND_MIXER_RECSRC:
				pollux_set_mixer(val, WM1800_CMD_RECSRC);
				dev->mixer.recSel = val;
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


static void pollux_mixer_init(void)
{
	//int ret, index;
	int index;
	unsigned short reading;
			
	// set mixer table number selection
	//dev->mixer.step_nums = 20;  
	dev->mixer.step_nums = VOLUME_STEP_NUM;  
    
    /* input value is 10 bit ADC, from 0 to 0x3FF */
    //dev->mixer.scale = 1000 / dev->mixer.step_nums;
    dev->mixer.scale = VOLUME_ADC_MAXIMUM / dev->mixer.step_nums;
	dev->mixer.vol_div100 = 100 / dev->mixer.step_nums;
    		
	/* initial volume setting */
    adc_GetVolume(&reading);
	dev->ADC_reading = reading;
	
	index = get_volume_step(reading, dev->mixer.scale);

    dev->mixer.volume = index *  dev->mixer.vol_div100;
    dev->mixer.volume_index = index;
}

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
#if 0    
    if (b->size &= ~3)
#else    
	if ((b->size &= ~3) && (b->size != 32768)  )
#endif
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
	
	switch (file->f_flags & O_ACCMODE){
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
        //printk("read %d from %d\n", chunksize, s->buf_idx);
        
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
	unsigned int mclk_div, bitclk_div;
	int m, b, d=1000000, pll = get_pll_clk(CLKSRC_PLL0);
		
	if( (s->rate == val) && (s->cpu_pll == pll) ) return val;
	
    if(val <= 8000 || val > 82000)
        return -1;
    
    for(m=32; m<=64; m++)
        for(b=2; b<=64; b++)
            if(abs(pll/m/b/64-val) <= d)
            {
                d = abs(pll/m/b/64-val);
                mclk_div = m;
                bitclk_div = b;
            }
    
    /* rate = pll1 / mclk_div / bitclk_div / 64 */                 
	MES_AUDIO_SetClockDivisor(0 ,mclk_div);
	MES_AUDIO_SetClockDivisor(1 ,bitclk_div);
	
	s->rate = val;
	s->cpu_pll = pll;
	
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
	
	pollux_gpio_setpin(GPIO_EAR_MUTE, 1);
	
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
		
		printk(" sound input init ...\n");
		MES_AUDIO_SetPCMINDataWidth(BUFF_WIDTH_DATA16);
		MES_AUDIO_SetAudioBufferPCMINEnable(CTRUE);
		MES_AUDIO_SetI2SInputEnable(CTRUE);	
		MES_AUDIO_SetInterruptEnable(0, CTRUE);	   
		MES_AUDIO_ClearInterruptPending(0);   // input 
		
		wm1800_input_enable(LINE1, IN_MONO_LEFT, MIC_BIAS_ON);		
		dev->mixer.recSel = (LINE1 >> 4);
		dev->mixer.flag_line_mono = 1;
	}
    
	err = 0;
out:
    return err;
}

int
pollux_audio_release(struct inode *inode, struct file *file)
{
    audio_stream_t *is = dev->input;
    audio_stream_t *os = dev->output;
    
    pollux_gpio_setpin(GPIO_EAR_MUTE, 0);
        
    if (file->f_mode & FMODE_READ)
    {
        audio_clear_buf(is); 
		printk(" sound input release \n");
		wm1800_input_disable();
		mp2530_dma_close_channel(is->dma_ch);
        dev->rd_ref = 0;
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

	i2c_add_driver(&wm1800_driver);
		
	if(dev->new_client == 0)
		printk(KERN_ALERT "audio: failed to connect to i2c bus\n");
	else 
	{ 	        	
		for ( i=0 ; i < 10 ; i++)
		{
			ret = wm1800_write_byte(WM1800_RESET, 0);	
			if(ret >= 0){
				pollux_mixer_init();	
				dev->input = &input_stream;
				dev->output = &output_stream;
				dev->input->name = "i2s audio in";
				dev->output->name = "i2s audio out";
				dev->isOpen = 0;    // device not opened
				init_polluxI2S(dev->output);
				ret = wm1800_init();	
				
				/* set up work queue for using the codec */
				dev->codec_tasks = create_singlethread_workqueue("codec tasks");
				/* volume control */
				INIT_WORK(&dev->volume_work, pollux_set_volume);
				/* set up periodic sampling */
				setup_timer(&dev->volume_timer, volume_monitor_task, 
					(unsigned long)dev);
				dev->volume_timer.expires = get_jiffies_64() + 
							VOLUME_SAMPLING_J;
								add_timer(&dev->volume_timer); 
									
				dev->audio_dev_dsp = register_sound_dsp(&pollux_audio_fops, -1);
				dev->audio_dev_mixer = register_sound_mixer(&pollux_mixer_fops, -1);  	
//#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE				
				printk(AUDIO_NAME_VERBOSE " initialized\n");
//#endif				
				return 0;
			
			}					
		}

		printk(KERN_ERR "audio: codec init failed\n");
	}

	i2c_del_driver(&wm1800_driver);	
	return -1;
}

static void __exit pollux_audio_exit(void)
{
	i2c_del_driver(&wm1800_driver);
	unregister_sound_dsp(dev->audio_dev_dsp);
	unregister_sound_mixer(dev->audio_dev_mixer);		
	destroy_workqueue(dev->codec_tasks);
	
	printk(AUDIO_NAME_VERBOSE " unloaded\n");
}

module_init(pollux_audio_init);
module_exit(pollux_audio_exit);

MODULE_DESCRIPTION("audio interface for the pollux");
MODULE_LICENSE("GPL");

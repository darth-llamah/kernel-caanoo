/*
 * sound/oss/pollux-cs42L52/pollux-audio.c
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

#include <asm/arch/aesop-pollux.h>

#include "pollux-iis.h"
#include "pollux-cs42L52.h"

//#define GDEBUG // my(godori) debug style....^^
#ifdef  GDEBUG
#    define dprintk(fmt, x... )  printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
#    define dprintk( x... )
#endif


typedef struct  {
	unsigned short	volume;			/* master volume */
	unsigned short	SVolume;		/* spkear volume */
	unsigned short	HVolume;    	/* HEADPHONE  volume */
	unsigned char	bass;
	unsigned char	treble;
	unsigned short	igain;
	unsigned short	micAmp;
	unsigned char recSel;
	int step_nums;		
	int scale;
	int igain_scale;
	int igain_step;
	int igain_zero_offset;
	int micAmp_scale;
	int micAmp_step;
	unsigned char* volTable;
	int	mod_cnt;
}cs42l52_mixer;


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
	
	cs42l52_mixer mixer;
} audio_dev;

struct audio_device *dev = &audio_dev;

#define AOUT_DMA_CH 	4
#define AIN_DMA_CH 		5

#define AUDIO_NAME		"pollux-audio"
#define AUDIO_NBFRAGS_DEFAULT	8
#define AUDIO_FRAGSIZE_DEFAULT	32768

#define AUDIO_NAME_VERBOSE 		"CS42L52 audio driver"


#define HEADPHONE_POLARITY		0
#define HEADPHONE_PIN			POLLUX_GPC7 // earphone pin

#if 0
#define OEM_GPIO_AUDIO_RST_BITNUM  2    // gpioalive, DAC reset pin
#else
#define GPIOALV_AUDIO_RST_BITNUM  2     // gpioalive, DAC reset pin
#endif

#define NEXT_BUF(_s_,_b_) { \
	(_s_)->_b_##_idx++; \
	(_s_)->_b_##_idx %= (_s_)->nbfrags; \
	(_s_)->_b_ = (_s_)->buffers + (_s_)->_b_##_idx; }

#define AUDIO_ACTIVE(dev)	((dev)->rd_ref || (dev)->wr_ref)

static long audio_set_dsp_speed(long val, audio_stream_t * s);

///////////////////////////////////////////////////////////////////////////////////////
/*
 * I2C OPERATION 
*/  	

static int cs42L52_write_byte(unsigned char reg, unsigned char value)
{
	unsigned char buf[2];
	int ret;

	buf[0] = reg;
	buf[1] = value;

	dprintk("cs42l52: writing 0x%02x to address 0x%02x\n", buf[1], buf[0]);
	if (2 != (ret = i2c_master_send(dev->new_client, buf, 2)) )
	{
		dprintk("i2c i/o error: rc == %d (should be 2)\n", ret);
		return -1;
	}
	
	return 0;
}

static int cs42L52_read_byte(unsigned char reg)
{
	int ret;
	unsigned char buf[1];
	
	buf[0] = reg;
	if (1 != (ret = i2c_master_send(dev->new_client, buf, 1)))
		printk("i2c i/o error: rc == %d (should be 1)\n", ret);

	msleep(10);

	if (1 != (ret = i2c_master_recv(dev->new_client, buf, 1)))
		printk("i2c i/o error: rc == %d (should be 1)\n", ret);

	dprintk("cs42l52: read 0x%02x = 0x%02x\n", reg, buf[0]);

	return (buf[0]);
}

static int cs42L52_write_array(unsigned char list[][2], int entries)
{
	int i;
	for(i = 0; i < entries; i++) 
		cs42L52_write_byte(list[i][0], list[i][1]);
	return 0;
}

/* Addresses to scan */
static unsigned short normal_i2c[] = { 0x94 >> 1, I2C_CLIENT_END };
/* I2C Magic */
I2C_CLIENT_INSMOD;

static int cs42l52_attach_adapter(struct i2c_adapter *adapter);
static int cs42l52_detect(struct i2c_adapter *adapter, int address, int kind);
static int cs42l52_detach_client(struct i2c_client *client);

static struct i2c_driver cs42l52_driver = {
	.driver = {
		.name	= "cs42l52",
	},
	.attach_adapter	= cs42l52_attach_adapter,
	.detach_client	= cs42l52_detach_client,
};

struct cs42l52_data {
	struct i2c_client client;
	int yyy;
};

static int cs42l52_attach_adapter(struct i2c_adapter *adapter)
{
	int adap_id = i2c_adapter_id(adapter);
	
	//if(adap_id != 0) return -1;
	//if(adap_id != 1) return -1;
	
	return i2c_probe(adapter, &addr_data, cs42l52_detect);
}

static int cs42l52_detect(struct i2c_adapter *adapter, int address, int kind)
{
	struct cs42l52_data *data;
	int err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		goto exit;

	if (!(data = kzalloc(sizeof(struct cs42l52_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

    dev->new_client = &data->client;
	i2c_set_clientdata(dev->new_client, data);
	dev->new_client->addr = address;
	dev->new_client->adapter = adapter;
	dev->new_client->driver = &cs42l52_driver;
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

static int cs42l52_detach_client(struct i2c_client *client)
{
	int err;
	struct cs42l52_data *data = i2c_get_clientdata(client);

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
	
	MES_AUDIO_SetPCMINDataWidth(BUFF_WIDTH_DATA16);
	MES_AUDIO_SetPCMOUTDataWidth(BUFF_WIDTH_DATA16);
	
	MES_AUDIO_SetAudioBufferPCMINEnable(CTRUE);
	MES_AUDIO_SetAudioBufferPCMOUTEnable(CTRUE);
	
	MES_AUDIO_SetI2SInputEnable(CTRUE);
	MES_AUDIO_SetI2SOutputEnable(CTRUE);
	
	MES_AUDIO_SetInterruptEnable(0, CTRUE);	
	MES_AUDIO_SetInterruptEnable(1, CTRUE);	

	MES_AUDIO_SetI2SLinkOn();

	MES_AUDIO_ClearInterruptPending(0);
	MES_AUDIO_ClearInterruptPending(1);


	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
 * CS42L52 REGISTER SETING
*/  	

void cs42L52_set_mute_headphone(void)
{
	unsigned char reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	if (!(reg & 0xC0)) {		// if headphones unmuted, then mute
		reg = reg | 0xC0;		// enable mute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

void cs42L52_set_unmute_headphone(unsigned char vol)
{
	unsigned char reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	cs42L52_write_byte(CS42L52_HEADPHONE_A, vol);
	cs42L52_write_byte(CS42L52_HEADPHONE_B, vol);

	if (reg & 0xC0) {		// if headphones muted, then unmute
		reg = reg & 0x3F;	// unmute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

void cs42L52_set_mute_speaker(void)
{
	unsigned char reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	if (!(reg & 0x30)) {		// if speakers unmuted, then mute
		reg = reg | 0x30;	// enable mute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

void cs42L52_set_speaker(unsigned char vol)
{
	unsigned char reg = cs42L52_read_byte(CS42L52_PLAYBACK_CONTROL_2);

	cs42L52_write_byte(CS42L52_SPEAKER_A, vol);
	cs42L52_write_byte(CS42L52_SPEAKER_B, vol);

	if (reg & 0x30) {		// if speakers muted, then unmute
		reg = reg & 0xCF;	// unmute
		cs42L52_write_byte(CS42L52_PLAYBACK_CONTROL_2, reg);
	}
}

void cs42L52_set_volume(unsigned char left, unsigned char right)
{
	unsigned char indexTableL,indexTableR;
	unsigned char val_l,val_r;
	int i;
	int entries = sizeof(cs42L52_settings)/2;
	
	
	
	if (right > 100) right = 100;
	if (left > 100) left = 100;	
	
	if(right == 0 && left == 0)
	    cs42L52_write_byte(0xf, 0xf0);
	else
	{
	    cs42L52_write_byte(0xf, 0);
	
	    indexTableL = left / dev->mixer.scale; 		
	    indexTableR = right / dev->mixer.scale; 		

	    val_l = dev->mixer.volTable[indexTableL];
	    val_r = dev->mixer.volTable[indexTableR];
	
	    cs42L52_write_byte(CS42L52_MASTER_A, val_l);
	    cs42L52_write_byte(CS42L52_MASTER_B, val_r);
    
    }	    
}

void cs42L52_set_igain(unsigned char left, unsigned char right)
{
	int val_l, val_r;
	if (right > 100) right = 100;
	if (left > 100) left = 100;	
			
	if( left == 0) 
		val_l = 260 - ( (dev->mixer.igain_zero_offset-1) * dev->mixer.igain_step ); //-96db
	else{		/* 0 db ~ 24db */
		if( dev->mixer.igain_zero_offset <= (left / dev->mixer.igain_scale) ){
			val_l = (left / dev->mixer.igain_scale) - dev->mixer.igain_zero_offset;
			val_l *= dev->mixer.igain_step;
		}else{	/* 0 db ~ (-96db + dev->mixer.igain_step)  */
			val_l = dev->mixer.igain_zero_offset - (left / dev->mixer.igain_scale) ;
			val_l = 260 - (val_l * dev->mixer.igain_step);
		}
	}	
			
	if( right == 0) 
		val_r = 260 - ( (dev->mixer.igain_zero_offset-1) * dev->mixer.igain_step ); 
	else{
		if( dev->mixer.igain_zero_offset <= (right / dev->mixer.igain_scale) ){
			val_r = (right / dev->mixer.igain_scale) - dev->mixer.igain_zero_offset;
			val_r *= dev->mixer.igain_step;
		}else{  
			val_r = dev->mixer.igain_zero_offset - (right / dev->mixer.igain_scale) ;
			val_r = 260 - (val_r * dev->mixer.igain_step);
		}
	} 
				
	cs42L52_write_byte(CS42L52_ADCA, (unsigned char)val_l);
	cs42L52_write_byte(CS42L52_ADCB, (unsigned char)val_r);
}					

static int cs42L52_init(void)
{
	int ret;
	
	ret = cs42L52_write_array(cs42L52_settings,sizeof(cs42L52_settings)/2);
	if (ret) return(ret);

	ret = cs42L52_write_byte(CS42L52_SPKCTL, 0x50);
	//ret = cs42L52_write_byte(CS42L52_SPKCTL, SPK_PCNL_SPK_OF_HP_ON);

	if (ret < 0)
		return(ret);

	// set mixer selection
	dev->mixer.step_nums = 10;  //20
	dev->mixer.scale = 100 / dev->mixer.step_nums;

	if(dev->mixer.step_nums == 10 )
		dev->mixer.volTable = masterVolumeTable10;
	else if(dev->mixer.step_nums == 20)
		dev->mixer.volTable = masterVolumeTable20;
		
	dev->mixer.igain_scale = 10;  //20
	dev->mixer.igain_step = 120  / dev->mixer.igain_scale; 		/* 24 db + 96db = 120 */
	dev->mixer.igain_zero_offset = dev->mixer.igain_scale - (24 / dev->mixer.igain_step);
	
	

	dev->mixer.micAmp_scale = 10;  //16
	dev->mixer.micAmp_step = 16 / dev->mixer.micAmp_scale;

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
		case CS42L52_CMD_VOLUME:			
			cs42L52_set_volume(right,left);
			break;
		case CS42L52_CMD_BASS:  		/* -10.5db ~ 12db  ( 0 to 150 => 0 to 15) */
			{
				int setval;
				unsigned char reg;
				
				if (left > 150) left = 150;
			
				if(left == 0)  setval = 0x0f;
				else setval = 15 - (left / 10); 
			
				reg = cs42L52_read_byte(CS42L52_TONE_CTRL);
				reg = (reg & 0xf0) | (unsigned char) setval;
			
				cs42L52_write_byte(CS42L52_TONE_CTRL, reg);
			}
			break;
		case CS42L52_CMD_TREBLE:		/* -10.5db ~ 12db (0 to 150)  */		
			{
				int setval;
				unsigned char reg;
				
				if (left > 150) left = 150;
			
				if(left == 0)  setval = 0x0f;
				else setval = 15 - (left / 10); 
			
				setval <<= 4;
			
				reg = cs42L52_read_byte(CS42L52_TONE_CTRL);
				reg = (reg & 0x0f) | (unsigned char)setval;
			
				cs42L52_write_byte(CS42L52_TONE_CTRL, reg);
			}
			break;
		case CS42L52_CMD_IGAIN:
			cs42L52_set_igain(left, right);
			break;
		case CS42L52_CMD_MICAMP:					
			{
				int val_l, val_r;
				unsigned char reg;
				
				if (right > 100) right = 100;
				if (left > 100) left = 100;	
			
				if(left == 0) val_l = 0;  //16db
				else val_l = (left / dev->mixer.micAmp_scale) * dev->mixer.micAmp_step;
			
				if(right == 0) val_r = 0;  
				else val_r = (right / dev->mixer.micAmp_scale) * dev->mixer.micAmp_step;
			
				reg = cs42L52_read_byte(CS42L52_MICAMP_CTRLA);
				reg = (reg & 0xE0) | (unsigned char) (val_l & 0x1f);
				cs42L52_write_byte(CS42L52_MICAMP_CTRLA, reg);
			
				reg = cs42L52_read_byte(CS42L52_MICAMP_CTRLB);
				reg = (reg & 0xE0) | (unsigned char) (val_r & 0x1f);
				cs42L52_write_byte(CS42L52_MICAMP_CTRLB, reg);
			}
			break;
		case CS42L52_CMD_RECSRC:
			{
				unsigned char reg;	
				
				reg = cs42L52_read_byte(CS42L52_INPUT_SELA);
				reg = (reg & 0xE0) | left;
				reg = cs42L52_read_byte(CS42L52_INPUT_SELB);
				reg = (reg & 0xE0) | left;
			
				if( (left == REC_SELECT_MIC )|| (left == REC_SELECT_MIC_LINE1 ) || 
						(left ==REC_SELECT_MIC_LINE1_LINE2) )
				{
					reg = cs42L52_read_byte(CS42L52_MICAMP_CTRLA);			
					reg &=  0xBF;
					reg |= REC_MIC_SELECT(right)? 0x40 : 0;  
					reg = cs42L52_write_byte(CS42L52_MICAMP_CTRLA, reg);			
				
					reg = cs42L52_read_byte(CS42L52_MICAMP_CTRLB);			
					reg &=  0xBF;
					reg |= REC_MIC_SELECT(right)? 0x40 : 0;  
					reg = cs42L52_write_byte(CS42L52_MICAMP_CTRLB, reg);
				}
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

		strncpy(mi.id, "CS42L52", sizeof(mi.id));
		strncpy(mi.name, "pollux CS42L52", sizeof(mi.name));
		mi.modify_counter = dev->audio_mix_modcnt;
		return copy_to_user((int *)arg, &mi, sizeof(mi));
	}

	if (_IOC_DIR(cmd) & _IOC_WRITE) 
	{
		ret = get_user(val, (int *)arg);
		if (ret)
			goto out;

		switch (nr) 
		{
			case SOUND_MIXER_VOLUME:
				pollux_set_mixer( val, CS42L52_CMD_VOLUME );
				dev->mixer.volume = val;
				break;
			case SOUND_MIXER_BASS:
				pollux_set_mixer( val, CS42L52_CMD_BASS );
				dev->mixer.bass = val & 0xff;
				break;
			case SOUND_MIXER_TREBLE:
				pollux_set_mixer( val, CS42L52_CMD_TREBLE );
				dev->mixer.treble = val & 0xff;
				break;	
			case SOUND_MIXER_LINE:
				pollux_set_mixer( val, CS42L52_CMD_IGAIN );
				dev->mixer.igain = val;
				break;
			case SOUND_MIXER_MIC:
				pollux_set_mixer( val, CS42L52_CMD_MICAMP );
				dev->mixer.igain = val;
				break;
			case SOUND_MIXER_RECSRC:
				pollux_set_mixer( (val&0x0f), CS42L52_CMD_VOLUME );
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

//dprintk("s->channels = %d\n", s->channels);    
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
//dprintk("1\n"); mdelay(10);

        /*
         * Feed the current buffer 
         */
        if (s->channels == 2)
        {
            chunksize = s->fragsize - b->size;

            if (chunksize > count)
                chunksize = count;
                
//dprintk("write %d to %d\n", chunksize, s->buf_idx);                
//dprintk("2\n"); mdelay(10);                
            if (copy_from_user(b->start + b->size, buffer, chunksize))
            {
                up(&b->sem);
                return -EFAULT;
            }
//dprintk("3\n"); mdelay(10);                            
            b->size += chunksize;
        }
        else
        {
            chunksize = (s->fragsize - b->size) >> 1;

            if (chunksize > count)
                chunksize = count;
//dprintk("write %d to %d\n", chunksize, s->buf_idx);                            
//dprintk("4\n"); mdelay(10);                                            
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
//printk("5\n"); mdelay(10);                                                    
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
    
    if (file->f_mode & FMODE_READ)
    {
        audio_clear_buf(is);
        mp2530_dma_close_channel(is->dma_ch);
        dev->rd_ref = 0;
    }

    if (file->f_mode & FMODE_WRITE)
    {
        audio_sync(file);
        audio_clear_buf(os);
        mp2530_dma_close_channel(os->dma_ch);
        dev->wr_ref = 0;
	}
	
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
	int ret;
	
	pollux_i2c_enable(CTRUE);
	i2c_add_driver(&cs42l52_driver);
	
	
	printk("POLLUX_SOUND_VIO_BASE = 0x%x\n", POLLUX_SOUND_VIO_BASE );
	printk("POLLUX_SOUND_PIO_BASE = 0x%x\n", POLLUX_SOUND_PIO_BASE );
	
	#if 0
	{
		int i;
		unsigned char *g = (unsigned char *)POLLUX_SOUND_VIO_BASE;
		
		for( i=0; i<0x200000; i++ )
		{
			g[i] = 0;
		}
	}	
	#endif
	
	if(dev->new_client == 0)
		printk(KERN_ALERT "audio: failed to connect to i2c bus\n");
	else { /* read chip ID from codec */
		dev->chip_id = cs42L52_read_byte(CS42L52_CHIP_ID);
		printk("CS42L52_CHIP ID:0x%x \n",dev->chip_id);
		if(dev->chip_id < 0)
			printk(KERN_ERR "audio: failed to read chip ID\n");
		else 
		{
			ret = cs42L52_init();
			if(ret < 0)
				printk(KERN_ERR "audio: codec init failed\n");
			else{
				dev->input = &input_stream;
				dev->output = &output_stream;
				dev->input->name = "i2s audio in";
				dev->output->name = "i2s audio out";
				init_polluxI2S(dev->output);
				dev->audio_dev_dsp = register_sound_dsp(&pollux_audio_fops, -1);
    			dev->audio_dev_mixer = register_sound_mixer(&pollux_mixer_fops, -1);
				
				printk(AUDIO_NAME_VERBOSE " initialized\n");
				
				return 0;	
			}
		}
	}
	
	pollux_i2c_enable(CFALSE);
	i2c_del_driver(&cs42l52_driver);
	return -1;
}

static void __exit pollux_audio_exit(void)
{
	pollux_i2c_enable(CFALSE);
	i2c_del_driver(&cs42l52_driver);
	unregister_sound_dsp(dev->audio_dev_dsp);
	unregister_sound_mixer(dev->audio_dev_mixer);		
	printk(AUDIO_NAME_VERBOSE " unloaded\n");
}

module_init(pollux_audio_init);
module_exit(pollux_audio_exit);

MODULE_DESCRIPTION("audio interface for the pollux");
MODULE_LICENSE("GPL");

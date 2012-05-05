#ifndef CS42L52_H
#define CS42L52_H

#define CS42L52_ADDR    	0x94

/* partial register list */
#define CS42L52_CHIP_ID				0x01
#define CS42L52_PLAYBACK_CONTROL_2 	0x0F


#define CS42L52_INPUT_SELA		0x08
#define CS42L52_INPUT_SELB		0x09
#define CS42L52_MICAMP_CTRLA	0x10
#define CS42L52_MICAMP_CTRLB	0x11
#define CS42L52_ADCA		0x16
#define CS42L52_ADCB		0x17
#define CS42L52_TONE_CTRL	0x1F
#define CS42L52_MASTER_A	0x20
#define CS42L52_MASTER_B	0x21
#define CS42L52_HEADPHONE_A	0x22
#define CS42L52_HEADPHONE_B	0x23
#define CS42L52_SPEAKER_A	0x24
#define CS42L52_SPEAKER_B	0x25


#define CS42L52_ID_NUM				0xE0
#define CS42L52_ID_NUM_MASK			0xF1


u8 cs42L52_settings[][2] = {
	/*REG, VALUE*/
	{0x02, 0x00},// Power Control 1
	{0x03, 0x00},// Power Control 2
// Power Control 3 -- See cs42L52_reg4_settings table
	{0x05, 0xA0},// Clocking Control
	{0x06, 0x27},// Interface Control 1, Slave, I2S
	{0x07, 0x00},// Interface Control 2
	{0x08, 0x81},// Input A Select ADCA and PGAA
	{0x09, 0x81},// Input B Select ADCB and PGAB
	{0x0A, 0xA5},// Analog & HPF Control
	{0x0B, 0x00},// ADC HPF Corner Frequency
	{0x0C, 0x00},// Misc ADC Control
	{0x0D, 0x10},// Playback Control 1
	{0x0E, 0x02},// Passthru Analog, adjust volume at zero-crossings
	{0x0F, 0x0},// Playback Control 2, mute headphone and speaker
	{0x10, 0x00},// MIC A
	{0x11, 0x00},// MIC B
	{0x12, 0x00},// ALC, PGA A
	{0x13, 0x00},// ALC, PGA B
	{0x14, 0x00},// Passthru A Volume
	{0x15, 0x00},// Passthru B Volume
	{0x16, 0x00},// ADC A Volume
	{0x17, 0x00},// ADC B Volume
	{0x18, 0x80},// ADC Mixer Channel A Mute
	{0x19, 0x80},// ADC Mixer Channel B Mute
	{0x1A, 0x00},// PCMA Mixer Volume
	{0x1B, 0x00},// PCMB Mixer Volume
	{0x1C, 0x00},// Beep Frequency
	{0x1D, 0x00},// Beep On Time
	{0x1E, 0x3F},// Beep & Tone Configuration
	{0x1F, 0xFF},// Tone Control
	{0x20, 0x06},// Master Volume Control MSTA
	{0x21, 0x06},// Master Volume Control MSTB
	{0x22, 0x00},// Headphone Volume Control HPA
	{0x23, 0x00},// Headphone Volume Control HPB
#if 0	
	{0x24, 0xCB},// Speaker Volume Control SPKA
	{0x25, 0xCB},// Speaker Volume Control SPKB
#else
	{0x24, 0x0},// Speaker Volume Control SPKA
	{0x25, 0x0},// Speaker Volume Control SPKB
#endif	
	{0x26, 0x50},// ADC & PCM Channel Mixer
	{0x27, 0x04},// Limiter Control 1
	{0x28, 0x8A},// Limiter Control 2
	{0x29, 0xC3},// Limiter Attack Rate
	{0x2A, 0x00},// ALC Enable & Attack Rate
	{0x2B, 0x00},// ALC Release Rate
	{0x2C, 0x00},// ALC Threshold
	{0x2D, 0x00},// Noise Gate Control
	{0x2F, 0x00},// Battery Compensation
	{0x32, 0x00},// Temperature monitor Control
	{0x33, 0x00},// Thermal Foldback
	{0x34, 0x5F},// Charge Pump Frequency
	{0x02, 0x9E},// Power Control 1
};

/*
 *  setup headphone jack control value in Cirrus Logic register 4,
 *  Power Control 3 register, SPK/HP_SW mutes speaker or headphone
 *
 *  Index into this table using board revision.
 *
 *  Earlier board versions have id code 0 on GPIO_B[31:27];
 *  set index 0 of the array for these boards at compile time.
 *
 *  Typical register 4 settings:
 *      0xAA -- Speaker and Headphones always on
 *      0x05 -- Pin 6 low:  Speaker on,  Headphones off
 *              Pin 6 high: Speaker off, Headphones on
 *      0x50 -- Pin 6 low:  Speaker off, Headphones on
 *              Pin 6 high: Speaker on,  Headphones off
 *      0xFF -- Speaker and Headphones always off
 */


#define 	SPK_HP_ON					0xAA
#define		SPK_PCNL_SPK_ON_HP_OFF 		0x05
#define		SPK_PCNL_SPK_OF_HP_ON 		0x05
#define		HP_PCNL_SPK_ON_HP_OFF 		0x50
#define		HP_PCNL_SPK_OF_HP_ON 		0x50
#define 	SPK_HP_OFF					0xFF

#define 	CS42L52_SPKCTL	0x04

/*
 *  Initial settings for Cirrus Logic ADC & PCM Channel Mixer Register 26
 *
 *  Mix together Left and Right channels when using mono speaker, disable
 *  mixing when listening with headphones.
 *
 *  Index into this table using board revision and GPIO jack level.  On older
 *  development boards jack level is 0 for headphones and 1 for speaker.
 *
 *  Earlier board versions have id code 0 on GPIO_B[31:27];
 *  set index 0 of the array for these boards at compile time.
 *
 *  Typical register 26 settings:
 *      0x00 -- Mixer off
 *      0x50 -- Mix Left and Right channels
 */

#define MIXER_OFF			0x00
#define MIXER_ON			0x50

#define cs42L52_reg26_settings_MAX_VERSION ((sizeof(cs42L52_reg26_settings)/2) - 1)
#define CS42L52_MIXER		0x26

/* audio clock */
	/* 384fs */
#define MCLK_22050		8467200

	/* 256fs */
#define MCLK_32000		8192000
#define	MCLK_44100		11289600
#define MCLK_48000		12288000
#define MCLK_64000		16384000

#define SYNC_22050		22050
#define SYNC_32000		32000
#define SYNC_44100		44100
#define SYNC_48000		48000
#define SYNC_64000		64000


/* MIXER USER COMMAND */

#define CS42L52_CMD_VOLUME				0

unsigned char headphoneVolumeTable10[10] = {
  	128,129,130,131, 133,134,135,136, 137,138
};  

unsigned char headphoneVolumeTable20[20] = {
  	128,129,130,131, 133,134,135,136, 137,138,
	128,129,130,131, 133,134,135,136, 137,138	
};  


unsigned char speakerVolumeTable10[10] = {
	128,129,130,131, 133,134,135,136, 137,138
};

unsigned char speakerVolumeTable20[20] = {
	128,129,130,131, 133,134,135,136, 137,138,
  	128,129,130,131, 133,134,135,136, 137,138
};


unsigned char masterVolumeTable10[10] = {
	0xA0,0xc0,0xD0,0xE8, 0xfC,0x0,0x6,0xC, 0x12,0x18
};

unsigned char masterVolumeTable20[20] = {
	128,129,130,131, 133,134,135,136, 137,138,
  	128,129,130,131, 133,134,135,136, 137,138
};


#define CS42L52_CMD_BASS				1
#define CS42L52_CMD_TREBLE				2
#define CS42L52_CMD_MICAMP				3
#define CS42L52_CMD_IGAIN				4
#define CS42L52_CMD_RECSRC				5
	
	/* record select left value */
	#define REC_SELECT_NO_INPUT				0x0
	#define REC_SELECT_LINE1				0x01
	#define REC_SELECT_LINE2				0x02
	#define REC_SELECT_LINE3				0x04
	#define REC_SELECT_LINE4				0x08
	#define REC_SELECT_MIC					0x10
	#define REC_SELECT_MIC_LINE1			0x11
	#define REC_SELECT_MIC_LINE1_LINE2		0x13
	
	/* record select left value */
	#define REC_MIC_SELECT(x)	((x) & (1 << 0) )    //0=mic1 1=mic2
	#define PATH_THRU_SELECT(x)	((x) & 0xC0 )		 //00 = path disabled 01=A 10=b 11=ALL

#endif

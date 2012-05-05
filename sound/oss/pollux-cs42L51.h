#ifndef CS42L51_H
#define CS42L51_H

#define CS42L51_ADDR    	0x94

/* partial register list */
#define CS42L51_CHIP_ID				0x01
#define CS42L51_PWR_CTRL			0x02
#define CS42L51_SPDPWR_CTRL			0x03
#define CS42L51_IF_CTRL				0x04

#define CS42L51_MIC_CTRL			0x05

#define CS42L51_INPUT_SEL			0x07

#define CS42L51_DAC_OUPUT		0x08
#define CS42L51_DAC_CNTL		0x09
#define CS42L51_PGAA			0x0A
#define CS42L51_PGAB			0x0B

#define CS42L51_PLAYBACK_CONTROL_2 	0x0F
#define CS42L51_PCMMIXVOL_A         0x10
#define CS42L51_PCMMIXVOL_B         0x11

#define CS42L51_VOLUME_A		0x16
#define CS42L51_VOLUME_B		0x17
#define CS42L51_PCMADC_CHMIXER	0x18


#define CS42L51_TONE_CTRL		0x1F

/* register value */
//#define STEREO_DAC_OFF_MICA_PGAA_ON	    0x60			
#define STEREO_DAC_OFF_MICA_PGAA_ON	    0x74			
#define STEREO_DAC_ON_MIC_PGA_OFF	    0x1E

#define MICA_ON_MICBIAS_ON		0xA8
#define MICA_ON_MICBIAS_OFF		0xAA
#define MIC_MICBIAS_OFF	        0xAE	

#define AINA_MUX_MIC_PREAMP		0x30
#define AINA_MUX_MIC			0x20



u8 cs42L51_settings[][2] = {
	/*REG, VALUE*/
	{0x02, 0x01},// Power Control 1
	{0x03, 0xAE},// Power Control 2 & Speed Control
	{0x04, 0x0E},// Slave,I2S
	{0x05, 0x21},// Mic Control
	{0x06, 0xA0},// ADC Control 2
	{0x07, 0x20},// MUXA (MIC1IN ->PGAA)
	{0x08, 0x00},// DAC Output Control 
	{0x09, 0x42},// DAC Control (ADC Serial Port to DAC & Soft lamp)	
	{0x0A, 0x0E},// ALCA & PGAA
    {0x0B, 0x18},// ALCB & PGAB
	{0x0C, 0x00},// ADCA Atten
	{0x0D, 0x00},// ADCB Atten
	{0x0E, 0x18},// ADCMIXA
	{0x0F, 0x18},// ADCMIXB
	{0x10, 0x00},// PCMMIXA
	{0x11, 0x00},// PCMMIXB
	{0x12, 0x00},// BEEP FREQ
	{0x13, 0x00},// BEEP OFF
	{0x14, 0x1F},// BEEP Control & Tone Config
	{0x15, 0xFF},// Tone Control
    {0x16, 0x00},// ADC A Volume
	{0x17, 0x00},// ADC B Volume
	{0x18, 0x88},// PCM & ADC Channel Mixer
	{0x19, 0x04},// Limiter Control 1
	{0x1A, 0x8A},// Limiter Control 2
	{0x1B, 0xC3},// Limiter Control 3
	{0x1C, 0x00},// ALC Enable & Attack Rate
	{0x1D, 0x00},// ALC Release Rate
	{0x1E, 0x00},// ALC Threshold
	{0x1F, 0x00},// Noise Gate Control
	{0x21, 0x5F},// Charge Pump Frequency
	{0x02, 0x1E},// Power Control 1
};

#define CS42L51_ID_NUM			0xE8

#define MIXER_OFF				0x00
#define MIXER_ON				0x50

#define CS42L51_MIXER			0x26

/* audio sync */
#define SYNC_22050		22050
#define SYNC_32000		32000
#define SYNC_44100		44100
#define SYNC_48000		48000
#define SYNC_64000		64000
 
#define OFFSET_DB       2        

/* MIXER USER DAC VOLUME TABLE  */
unsigned char spk_table[19] = {
#if 0 /* normal */
	0x80, 0x50, 0x55, 0x5A, 0x60, 0x65, 0x6A, 0x70, 0x75, 0x7A,
    0x00, 0x03, 0x06, 0x09, 0x0C, 0x0F, 0x12, 0x15, 0x18, 
#endif
#if 0  /* org */
    0x80, 0x60, 0x65, 0x6A, 0x70, 0x7A, 0x0, 0x02, 0x04, 0x06,
	0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x18
#endif
#if 1  
    0x80, 0x70, 0x73, 0x76, 0x79, 0x7A, 0x0, 0x02, 0x04, 0x06,
	0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x18
#endif
};

unsigned char hp_table[19] = {
	0x80, 0x30, 0x38, 0x40, 0x45, 0x4A, 0x50, 0x55, 0x5A, 0x60, 
	0x65, 0x6A, 0x70, 0x75, 0x7A, 0x00, 0x02, 0x05, 0x08
};


/* MIXER USER COMMAND */
#define CS42L51_CMD_VOLUME				0

#define CS42L51_CMD_BASS				1
#define CS42L51_CMD_TREBLE				2
#define CS42L51_CMD_MICAMP				3
#define CS42L51_CMD_IGAIN				4
#define CS42L51_CMD_RECSRC				5
	
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

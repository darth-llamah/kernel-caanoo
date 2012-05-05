#ifndef _WM1800_H
#define _WM1800_H


/* WM1800 register space */
#define WM1800_LINVOL		0x0
#define WM1800_RINVOL		0x1
#define WM1800_LOUT1		0x2
#define WM1800_ROUT1		0x3
#define WM1800_CLOCK1		0x4
#define WM1800_DACCTL1		0x5
#define WM1800_DACCTL2		0x6
#define WM1800_IFACE1		0x7
#define WM1800_CLOCK2		0x8
#define WM1800_IFACE2		0x9
#define WM1800_LDAC			0xa
#define WM1800_RDAC			0xb

#define WM1800_RESET		0xf
#define WM1800_3D			0x10
#define WM1800_ALC1			0x11
#define WM1800_ALC2			0x12
#define WM1800_ALC3			0x13
#define WM1800_NOISEG		0x14
#define WM1800_LADC			0x15
#define WM1800_RADC			0x16
#define WM1800_ADDCTL1		0x17
#define WM1800_ADDCTL2		0x18
#define WM1800_POWER1		0x19
#define WM1800_POWER2		0x1a
#define WM1800_ADDCTL3		0x1b
#define WM1800_APOP1		0x1c
#define WM1800_APOP2		0x1d
#define WM1800_LINPATH		0x20
#define WM1800_RINPATH		0x21
#define WM1800_LOUTMIX		0x22

#define WM1800_ROUTMIX		0x25
#define WM1800_MONOMIX1		0x26
#define WM1800_MONOMIX2		0x27
#define WM1800_LOUT2		0x28
#define WM1800_ROUT2		0x29
#define WM1800_MONO			0x2a
#define WM1800_INBMIX1		0x2b
#define WM1800_INBMIX2		0x2c
#define WM1800_BYPASS1		0x2d
#define WM1800_BYPASS2		0x2e
#define WM1800_POWER3		0x2f
#define WM1800_ADDCTL4		0x30
#define WM1800_CLASSD1		0x31

#define WM1800_CLASSD2		0x33
#define WM1800_PLL1			0x34
#define WM1800_PLL2			0x35
#define WM1800_PLL3			0x36
#define WM1800_PLL4			0x37

/*  WM1800 POWER EVENT */
#define WM1800_PWR_DOWN				0
#define WM1800_PWR_SPK_STANDDBY		1
#define WM1800_PWR_HP_STANDBY		2
#define WM1800_PWR_HP_ONLY			3
#define WM1800_PWR_SPK_ONLY			4
#define WM1800_PWR_SKP_HP			5


/* audio sync */
#define SYNC_8000		8000
#define SYNC_11025		11025
#define SYNC_22050		22050
#define SYNC_32000		32000
#define SYNC_44100		44100
#define SYNC_48000		48000
#define SYNC_64000		64000
#define SYNC_88200		88200


/* MIXER USER COMMAND */
#define WM1800_CMD_VOLUME				0
#define WM1800_CMD_BASS					1
#define WM1800_CMD_TREBLE				2
#define WM1800_CMD_MICAMP				3
#define WM1800_CMD_IGAIN				4
#define WM1800_CMD_RECSRC				5
#define WM1800_CMD_3D					7	
	/* boot base volumes */
	#define BASE_DAC_VOLUME				0xFA
	

	#define BASE_ADC_VOLUME				0xF0	    
	#define BASE_LINE_VOLUME			0x30	
		
/* Enable line input */
#define NOT_USE_LINE	0
#define LINE1			0x100
#define LINE2			0x40
#define LINE3			0x80

#define IN_STEREO		0x30
#define IN_MONO_LEFT	0x20
#define IN_MONO_RIGHT	0x10
			
#define PGA_BOOST_GAIN0		0x00
#define PGA_BOOST_GAIN13	0x10
#define PGA_BOOST_GAIN20	0x20
#define PGA_BOOST_GAIN29	0x30			
	
#define MIC_BIAS_ON			0x02
#define MIC_BIAS_OFF		0x00

#define MIC_BOOST_MIXER		0x08  //0x0


/* MIXER USER DAC VOLUME TABLE  */
#if 1
/*
unsigned char volume_Htable[19] = {
    0x00, 0x45, 0x4a, 0x50, 0x55, 0x59, 0x5c, 0x60, 0x64, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70
};
*/
unsigned char volume_Htable[19]= {
    0x00, 0x45, 0x4a, 0x50, 0x55, 0x5a, 0x60, 0x64, 0x68, 0x6a,
        0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74
};
#else
unsigned char volume_Htable[19] = {
    0x00, 0x45, 0x4a, 0x50, 0x55, 0x58, 0x5c, 0x60, 0x64, 0x68,
	0x6c, 0x70, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78
};
#endif

#if 0
unsigned char volume_Stable[19] = {
    0x00, 0x62, 0x64, 0x68, 0x6a, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79
};
#else
unsigned char volume_Stable[19] = {
    0x00, 0x62, 0x64, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
	0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77
};
#endif

unsigned char involume_table[11] = {
	0x00, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0
};

unsigned char depth3d_table[11] = {
	0x0, 0x03, 0x05, 0x07, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0xf
};

#endif

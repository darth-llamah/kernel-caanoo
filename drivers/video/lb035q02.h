#ifndef _LB035Q02_H
#define _LB035Q02_H

#include <asm/arch/regs-gpio.h>


typedef struct _lcd_seting {
	unsigned int reg;
	unsigned int* pVal;
}lcd_seting;

#define MAX_REG					24
#define LCDOFF_REG_NUMBER		2
#define DEVICEID_LGPHILIPS		0x70			

unsigned short valueREG[][2] = {
	/*
			     {0x03, 0x0117},
			     {0xff, 3},
			     {0x0D, 0x0030},
 			     {0xff, 3}, 
			     {0x0E, 0x2800},
 			     {0xff, 3}, 
			     {0x1E, 0x00C1},
 			     {0xff, 30},   // 30ms

			     {0x01, 0x6300}, 
			     {0x02, 0x200},
			     {0x04, 0x04C7},
 			     {0x05, 0xFFC0},
			     {0x0A, 0x4008},
			     {0x0B, 0x0000},
			     {0x0F, 0x0000},

			     {0x16, 0x9F80},
			     {0x17, 0x0A0F},
 			     {0xff, 30}
	 */
                 {0x01, 0x6300}, 
                 {0x02, 0x0200},
                 {0x03, 0x041C}, //{0x03, 0x0117},^M
                 {0xff, 3},
                 {0x04, 0x04C7},
                 {0x05, 0xFC70}, //{0x05, 0xFFC0},^M
                 {0x06, 0xE806},
                 {0x0A, 0x4008},
                 {0x0B, 0x0000},
                 {0x0D, 0x0024}, //{0x0D, 0x0030},^M
                 {0xff, 3},
                 {0x0E, 0x29C0}, //{0x0E, 0x2800},^M
                 {0xff, 3},
                 {0x0F, 0x0000},
                 //porch
                 {0x16, 0x9F80},
                 {0x17, 0x0A0F}, //{0x17, 0x2212}, //0a0f^M
				 {0xff, 30},
                 {0x1E, 0x00DD}, //{0x1E, 0x00DD}, //{0x1E, 0x00c1}, //c1^M
                 {0xff, 30},   // 30ms^M
                 //gamma collection
                 {0x30, 0x0700},
                 {0x31, 0x0207},
                 {0x32, 0x0000},
                 {0x33, 0x0105},
                 {0x34, 0x0007},
                 {0x35, 0x0101},
                 {0x36, 0x0707},
                 {0x37, 0x0504},
                 {0x3A, 0x140F},
                 {0x3B, 0x0509},
                 {0xff, 30}
};

unsigned short valueREG_lcdOff[][2] = {
				{0x05, 0x00},
 			    {0x01, 0x00}};

#define CS_LOW	( pollux_gpio_setpin(POLLUX_GPB10 ,0))
#define CS_HIGH	( pollux_gpio_setpin(POLLUX_GPB10 ,1))
#define SCL_LOW ( pollux_gpio_setpin(POLLUX_GPB9 ,0))
#define SCL_HIGH ( pollux_gpio_setpin(POLLUX_GPB9 ,1))
#define SDA_LOW ( pollux_gpio_setpin(POLLUX_GPB8 ,0))
#define SDA_HIGH ( pollux_gpio_setpin(POLLUX_GPB8 ,1))

#define BD_VER_LSB              POLLUX_GPC18
#define BD_VER_MSB             	POLLUX_GPC17

#define SEL_LG_LCD				0
#define SEL_TCL_LCD				1

void lcdSetWrite(unsigned char id, unsigned short reg, unsigned short val);
void lcd_spi_write(unsigned char addr, unsigned char data);

#endif

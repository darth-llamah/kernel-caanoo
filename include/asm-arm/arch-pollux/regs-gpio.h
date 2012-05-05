/*
 * linux/include/asm-arm/arch-pollux/regs-gpio.h
 *
 * Copyright. 2003-2007 AESOP-Embedded project
 *	           http://www.aesop-embedded.org/pollux/index.html
 * 
 * godori(Ko Hyun Chul), omega5 - project manager
 * nautilus_79(Lee Jang Ho)     - main programmer
 * amphell(Bang Chang Hyeok)    - Hardware Engineer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */ 
#ifndef __ASM_ARCH_REGS_GPIO_H
#define __ASM_ARCH_REGS_GPIO_H


#define POLLUX_GPIONO(bank,offset) ((bank) + (offset))

#define POLLUX_GPIO_BANKA   (32*0)
#define POLLUX_GPIO_BANKB   (32*1)
#define POLLUX_GPIO_BANKC   (32*2)

// POLLUX_VA_GPIO 대신 baseptr로 잠시 대체, (((pin) & ~31) == 0x20 단위로 나온다. 해서 0x40단위이므로 <<1을 해준다.
#define POLLUX_GPIO_BASE(pin)   ((((pin) & ~31) << 1) + POLLUX_VA_GPIO)  // gpio a, b, c의 base address를 찾아옴.
#define POLLUX_GPIO_OFFSET(pin) ((pin) & 31)                      // gpio number에 따른 bit offset, 0~31이 나온다. 핀에 따라서 <<1을 할 것인지 아닌지는 알아서 코드에서 결정해야함.


// register base 계산....
#define POLLUX_GPIOREG(x) ((x) + POLLUX_VA_GPIO) // not used


#define POLLUX_GPIO_OUT	     (0x00)
#define POLLUX_GPIO_OUTENB   (0x04) 
#define POLLUX_GPIO_DETMODE0 (0x08) // low 16개
#define POLLUX_GPIO_DETMODE1 (0x0C) // high 16개
#define POLLUX_GPIO_INTENB   (0x10) 
#define POLLUX_GPIO_INTPEND  (0x14) 
#define POLLUX_GPIO_PAD      (0x18) 
#define POLLUX_GPIO_PUENB    (0x1C) 
#define POLLUX_GPIO_FUNC0    (0x20) 
#define POLLUX_GPIO_FUNC1    (0x24) 


enum POLLUX_GPIO_MODE
{
	POLLUX_GPIO_MODE_GPIO = 0,
	POLLUX_GPIO_MODE_ALT1,
	POLLUX_GPIO_MODE_ALT2
};

enum POLLUX_GPIO_INT_DET_STATUS
{
	POLLUX_EXTINT_LOWLEVEL = 0,
	POLLUX_EXTINT_HILEVEL,
	POLLUX_EXTINT_FALLEDGE,
	POLLUX_EXTINT_RISEEDGE
};

enum 
{
	POLLUX_GPIO_PU_DISABLE = 0,
	POLLUX_GPIO_PU_ENABLE
};

enum 
{
	POLLUX_GPIO_INPUT_MODE = 0,
	POLLUX_GPIO_OUTPUT_MODE
};

enum 
{
	POLLUX_GPIO_LOW_LEVEL = 0,
	POLLUX_GPIO_HIGH_LEVEL
};
	


#define POLLUX_GPA0          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 0 )
#define POLLUX_GPA1          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 1 )
#define POLLUX_GPA2          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 2 )
#define POLLUX_GPA3          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 3 )
#define POLLUX_GPA4          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 4 )
#define POLLUX_GPA5          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 5 )
#define POLLUX_GPA6          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 6 )
#define POLLUX_GPA7          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 7 )
#define POLLUX_GPA8          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 8 )
#define POLLUX_GPA9          POLLUX_GPIONO(POLLUX_GPIO_BANKA, 9 )
#define POLLUX_GPA10         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 10)
#define POLLUX_GPA11         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 11)
#define POLLUX_GPA12         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 12)
#define POLLUX_GPA13         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 13)
#define POLLUX_GPA14         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 14)
#define POLLUX_GPA15         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 15)
#define POLLUX_GPA16         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 16)
#define POLLUX_GPA17         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 17)
#define POLLUX_GPA18         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 18)
#define POLLUX_GPA19         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 19)
#define POLLUX_GPA20         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 20)
#define POLLUX_GPA21         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 21)
#define POLLUX_GPA22         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 22)
#define POLLUX_GPA23         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 23)
#define POLLUX_GPA24         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 24)
#define POLLUX_GPA25         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 25)
#define POLLUX_GPA26         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 26)
#define POLLUX_GPA27         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 27)
#define POLLUX_GPA28         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 28)
#define POLLUX_GPA29         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 29)
#define POLLUX_GPA30         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 30)
#define POLLUX_GPA31         POLLUX_GPIONO(POLLUX_GPIO_BANKA, 31)


#define POLLUX_GPB0          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 0 )
#define POLLUX_GPB1          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 1 )
#define POLLUX_GPB2          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 2 )
#define POLLUX_GPB3          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 3 )
#define POLLUX_GPB4          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 4 )
#define POLLUX_GPB5          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 5 )
#define POLLUX_GPB6          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 6 )
#define POLLUX_GPB7          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 7 )
#define POLLUX_GPB8          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 8 )
#define POLLUX_GPB9          POLLUX_GPIONO(POLLUX_GPIO_BANKB, 9 )
#define POLLUX_GPB10         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 10)
#define POLLUX_GPB11         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 11)
#define POLLUX_GPB12         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 12)
#define POLLUX_GPB13         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 13)
#define POLLUX_GPB14         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 14)
#define POLLUX_GPB15         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 15)
#define POLLUX_GPB16         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 16)
#define POLLUX_GPB17         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 17)
#define POLLUX_GPB18         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 18)
#define POLLUX_GPB19         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 19)
#define POLLUX_GPB20         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 20)
#define POLLUX_GPB21         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 21)
#define POLLUX_GPB22         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 22)
#define POLLUX_GPB23         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 23)
#define POLLUX_GPB24         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 24)
#define POLLUX_GPB25         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 25)
#define POLLUX_GPB26         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 26)
#define POLLUX_GPB27         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 27)
#define POLLUX_GPB28         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 28)
#define POLLUX_GPB29         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 29)
#define POLLUX_GPB30         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 30)
#define POLLUX_GPB31         POLLUX_GPIONO(POLLUX_GPIO_BANKB, 31)

// pollux gpioc[0:20]
#define POLLUX_GPC0          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 0 )
#define POLLUX_GPC1          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 1 )
#define POLLUX_GPC2          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 2 )
#define POLLUX_GPC3          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 3 )
#define POLLUX_GPC4          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 4 )
#define POLLUX_GPC5          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 5 )
#define POLLUX_GPC6          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 6 )
#define POLLUX_GPC7          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 7 )
#define POLLUX_GPC8          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 8 )
#define POLLUX_GPC9          POLLUX_GPIONO(POLLUX_GPIO_BANKC, 9 )
#define POLLUX_GPC10         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 10)
#define POLLUX_GPC11         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 11)
#define POLLUX_GPC12         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 12)
#define POLLUX_GPC13         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 13)
#define POLLUX_GPC14         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 14)
#define POLLUX_GPC15         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 15)
#define POLLUX_GPC16         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 16)
#define POLLUX_GPC17         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 17)
#define POLLUX_GPC18         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 18)
#define POLLUX_GPC19         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 19)
#define POLLUX_GPC20         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 20)

/* 
#define POLLUX_GPC21         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 21)
#define POLLUX_GPC22         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 22)
#define POLLUX_GPC23         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 23)
#define POLLUX_GPC24         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 24)
#define POLLUX_GPC25         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 25)
#define POLLUX_GPC26         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 26)
#define POLLUX_GPC27         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 27)
#define POLLUX_GPC28         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 28)
#define POLLUX_GPC29         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 29)
#define POLLUX_GPC30         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 30)
#define POLLUX_GPC31         POLLUX_GPIONO(POLLUX_GPIO_BANKC, 31)*/



void          pollux_set_gpio_func(unsigned int pin, enum POLLUX_GPIO_MODE function);
unsigned long pollux_get_gpio_func(unsigned int pin);
void          pollux_gpio_pullup(unsigned int pin, unsigned int to);
void          pollux_gpio_set_inout(unsigned int pin, unsigned int to);
void          pollux_gpio_setpin(unsigned int pin, unsigned int to);
unsigned int  pollux_gpio_getpin(unsigned int pin);



#endif /* __ASM_ARM_REGS_GPIO_H */


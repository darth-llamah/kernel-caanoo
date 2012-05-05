/*
 * linux/include/asm-arm/arch-pollux/regs-rtc.h
 *
 * Copyright. 2003-2008 AESOP-Embedded project
 *	           http://www.aesop-embedded.org
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
#ifndef __ASM_ARM_REGS_RTC_H
#define __ASM_ARM_REGS_RTC_H

#include <asm/hardware.h>

#define RTCCNTWRITE __REG32(0xC000F080)	// 0x00 : RTC counter register (Write only)
#define RTCCNTREAD  __REG32(0xC000F084)	// 0x04 : RTC counter register (Read only)
#define RTCALARM    __REG32(0xC000F088)	// 0x08 : RTC alarm register
#define RTCCONTROL  __REG32(0xC000F08C)	// 0x0C : RTC control register
#define RTCINTENB   __REG32(0xC000F090)	// 0x10 : RTC interrupt enable register
#define RTCINTPEND  __REG32(0xC000F094)	// 0x14 : RTC interrupt pending register


int  pollux_rtc_checkbusy(void);
int  pollux_rtc_set_alarm_counter( u32 alarm );
u32  pollux_rtc_get_alarm_counter(void);
int  pollux_rtc_check_busy_alarm ( void );
int  pollux_rtc_set_counter(u32 val);
u32  pollux_rtc_get_counter(void);
int  pollux_rtc_check_busy_rtc ( void );
void pollux_rtc_set_write_enable(int enable);



#endif // __ASM_ARM_REGS_TIMER_H


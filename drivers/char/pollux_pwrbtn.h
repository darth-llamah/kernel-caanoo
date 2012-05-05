#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pm.h>

#ifndef _POLLUX_PWRBTN_H_
#define _POLLUX_PORBTN_H_

struct power_switch {
	struct timer_list	pwr_chkTimer;
	int power_switch_off; 
	int timer_done;
	struct work_struct	work;
	struct platform_device	*pdev;

};

struct power_button_platform_info {
	irqreturn_t		(*irq_handler)(int irq, void *data);
	unsigned int		irq_flags;
	unsigned int		bit;
	const char		*name;
	
};

#define PWRBTN_MINOR 158

#define POWER_SWITCH_DECT_IRQ	IRQ_GPIO_B(6)		
#define POWER_SWITCH_DECT	    POLLUX_GPB6
#define GPIO_POWER_OFF			POLLUX_GPA18
#define GPIO_LCD_AVDD			POLLUX_GPB14

#define GPIO_LOW_LEVEL		0
#define GPIO_HIGH_LEVEL		1

#endif

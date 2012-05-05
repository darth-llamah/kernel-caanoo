#ifndef _POLLUX_TS_H_
#define _POLLUX_TS_H_

#define TSPY_EN              			POLLUX_GPC12
#define TSPY_EN_AL           			POLLUX_GPIO_LOW_LEVEL

#define TSPX_EN              			POLLUX_GPC14
#define TSPX_EN_AL           			POLLUX_GPIO_LOW_LEVEL

#define TSMY_EN              			POLLUX_GPC11
#define TSMY_EN_AL           			POLLUX_GPIO_HIGH_LEVEL

#define TSMX_EN              			POLLUX_GPC13
#define TSMX_EN_AL           			POLLUX_GPIO_HIGH_LEVEL

#define PENDOWN_CON  					POLLUX_GPC19
#define PENDOWN_CON_AL       			POLLUX_GPIO_HIGH_LEVEL

#define PENDOWN_DET          			POLLUX_GPA17

#define PENDOWN_DET_ACTIVE   			IRQT_LOW
#define PENDOWN_DET_INACTIVE 			IRQT_HIGH
#define PENDOWN_DET_IRQ                	IRQ_GPIO_A(17)
#define PENDOWN_DET_AL       			POLLUX_GPIO_LOW_LEVEL

#define GPIO_HOLD_KEY                   POLLUX_GPA10



/********** TOUCH **********/
#define TOUCH_PERIOD_UNIT_MS                              					10
#define TOUCH_REV_X_VALUE                                 					CFALSE
#define TOUCH_REV_Y_VALUE                                 					CTRUE
#define TOUCH_ADC_CH_CHANGE                               					CFALSE


/*************** ADC LIST ***************/
#define ADC_TOUCH_XP                                      			1
#define ADC_TOUCH_YP                                      			0


// mx = on: high, off: low
// my = on: high, off: low
// px = on: low, off: high
// py = on: low, off: high


enum ADCPort
{
	ADCP0,
	ADCP1,
	ADCP2,
	ADCP3,
	ADCP4,
	ADCP5,
	ADCP6,
	ADCP7
};


#endif                         

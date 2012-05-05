#ifndef __OEM_SYSTEM_MAIN_H__
#define __OEM_SYSTEM_MAIN_H__

#ifndef __ASSEMBLY__

#include <asm/arch/wincetype.h>
/********** SYSTEM **********/

//// TIMMING ////
#define SYSTEM_TIMMING_MCUA_MSD                           					0
#define SYSTEM_TIMMING_MCUA_DS                            					0
#define SYSTEM_TIMMING_MCUA_DLYCK                         					0
#define SYSTEM_TIMMING_MCUA_tMRD                          					2
#define SYSTEM_TIMMING_MCUA_tRP                           					1
#define SYSTEM_TIMMING_MCUA_tRCD                          					1
#define SYSTEM_TIMMING_MCUA_tRC                           					6
#define SYSTEM_TIMMING_MCUA_tRAS                          					4
#define SYSTEM_TIMMING_MCUA_tWR                           					1
#define SYSTEM_TIMMING_MCUA_REF                           					0x200

//------------------------------------------------------------------------------
// MCU-A
//------------------------------------------------------------------------------
enum {
	CFG_SYS_BANKA_DIC			= 0,		// Output Drive Impedance   : 0=Normal      , 1=Weak
	CFG_SYS_BANKA_USEDLL		= CTRUE,	// DLL control              : CFALSE=Disable, CTRUE=Enable
	CFG_SYS_BANKA_ACTPWDNENB	= CFALSE,	// Active Power-down Mode   : CFALSE=Disable, CTRUE=Enable
	CFG_SYS_BANKA_ACTPWDNPERD	= 255,		// Active Power-down Period : 0 ~ 255  , unit = BCLK
	CFG_SYS_BANKA_REFPRED		= 256,		// Refresh Period           : 0 ~ 65535, unit = BCLK * 2
	CFG_SYS_BANKA_CASLAT		= 1,		// CAS latency              : 0=1.0, 1=2.0, 2=2.5, 3=3.0 clocks
	CFG_SYS_BANKA_READLAT		= 1,		// Read latency             : 0=1.0, 1=2.0, 2=2.5, 3=3.0 clocks
	CFG_SYS_BANKA_TMRD			= 3,		// Mode Register Set Cycle  : 1 ~ 16 clocks
	CFG_SYS_BANKA_TRP			= 5, 		// Row Prechage Time        : 1 ~ 16 clocks
	CFG_SYS_BANKA_TRCD			= 5, 		// RAS to CAS delay         : 1 ~ 16 clocks
	CFG_SYS_BANKA_TRC			= 11, 		// Row Cycle Time           : 1 ~ 16 clocks
	CFG_SYS_BANKA_TRAS			= 8, 		// Row Active Time          : 1 ~ 16 clocks
	CFG_SYS_BANKA_TWR			= 3, 		// Write Recovery Time      : 1 ~ 16 clocks
	CFG_SYS_BANKA_CLKDLY		= 2, 		// delay of DRAM clock      : 0=0, 1=0.5, 2=1, 3=1.5, 4=2, 5=2.5, 6=3, 7=3.5 (ns)
	CFG_SYS_BANKA_DQS0INDLY		= 2, 		// delay of DQS0 Input      : 0=0, 1=0.5, 2=1, 3=1.5, 4=2, 5=2.5, 6=3, 7=3.5 (ns)
	CFG_SYS_BANKA_DQS0OUTDLY	= 2, 		// delay of DQS0 Output     : 0=0, 1=0.5, 2=1, 3=1.5, 4=2, 5=2.5, 6=3, 7=3.5 (ns)
	CFG_SYS_BANKA_DQS1INDLY		= 2, 		// delay of DQS1 Input      : 0=0, 1=0.5, 2=1, 3=1.5, 4=2, 5=2.5, 6=3, 7=3.5 (ns)
	CFG_SYS_BANKA_DQS1OUTDLY	= 2, 		// delay of DQS1 Output     : 0=0, 1=0.5, 2=1, 3=1.5, 4=2, 5=2.5, 6=3, 7=3.5 (ns)
};

//------------------------------------------------------------------------------
// Static Bus #0 ~ #9, NAND, IDE configuration
//------------------------------------------------------------------------------
//	_BW	  : Staic Bus width for Static #0 ~ #9            : 8 or 16
//
//	_TACS : adress setup time before chip select          : 0 ~ 3
//	_TCOS : chip select setup time before nOE is asserted : 0 ~ 3
//	_TACC : access cycle                                  : 1 ~ 16
//	_TSACC: burst access cycle for Static #0 ~ #9 & IDE   : 1 ~ 16
//	_TOCH : chip select hold time after nOE not asserted  : 0 ~ 3
//	_TCAH : address hold time after nCS is not asserted   : 0 ~ 3
//
//	_WAITMODE : wait enable control for Static #0 ~ #9 & IDE : 1=disable, 2=Active High, 3=Active Low
//	_WBURST	  : burst write mode for Static #0 ~ #9          : 0=disable, 1=4byte, 2=8byte, 3=16byte 
//	_RBURST   : burst  read mode for Static #0 ~ #9          : 0=disable, 1=4byte, 2=8byte, 3=16byte 
//
//------------------------------------------------------------------------------
#define CFG_SYS_STATICBUS_CONFIG( _name_, bw, tACS, tCOS, tACC, tSACC, tCOH, tCAH, wm, rb, wb )	\
	enum {											\
		CFG_SYS_ ## _name_ ## _BW		= bw,		\
		CFG_SYS_ ## _name_ ## _TACS		= tACS,		\
		CFG_SYS_ ## _name_ ## _TCOS		= tCOS,		\
		CFG_SYS_ ## _name_ ## _TACC		= tACC,		\
		CFG_SYS_ ## _name_ ## _TSACC	= tSACC,	\
		CFG_SYS_ ## _name_ ## _TCOH		= tCOH,		\
		CFG_SYS_ ## _name_ ## _TCAH		= tCAH,		\
		CFG_SYS_ ## _name_ ## _WAITMODE	= wm, 		\
		CFG_SYS_ ## _name_ ## _RBURST	= rb, 		\
		CFG_SYS_ ## _name_ ## _WBURST	= wb		\
	};

//                      ( _name_ , bw, tACS tCOS tACC tSACC tOCH tCAH, wm, rb, wb )
CFG_SYS_STATICBUS_CONFIG( STATIC0, 16,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC1, 16,    1,   2,  5,    1,    2,   2,  0,  0,  0 )  /* DM9000A timing */
CFG_SYS_STATICBUS_CONFIG( STATIC2, 16,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC3, 16,    3,   3,  16,   16,   3,   3,  3,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC4,  8,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC5,  8,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC6,  8,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC7,  8,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC8,  8,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG( STATIC9,  8,    3,   3,  16,   16,   3,   3,  1,  0,  0 )
CFG_SYS_STATICBUS_CONFIG(    NAND,  8,    3,   3,   5,    4,   3,   3,  1,  0,  0 )		// tOCH, tCAH must be greter than 0 for NAND


//// FREQUENCY ////
#define SYSTEM_FREQUENCY_PLL0_P                           					9
#define SYSTEM_FREQUENCY_PLL0_M                           					256
#define SYSTEM_FREQUENCY_PLL0_S                           					1

#define SYSTEM_FREQUENCY_PLL1_P                           					6
#define SYSTEM_FREQUENCY_PLL1_M                           					59
#define SYSTEM_FREQUENCY_PLL1_S                           					1


/********** TOUCH **********/
#define TOUCH_PERIOD_UNIT_MS                              					10
#define TOUCH_REV_X_VALUE                                 					CFALSE
#define TOUCH_REV_Y_VALUE                                 					CTRUE
#define TOUCH_ADC_CH_CHANGE                               					CFALSE

/********** DISPLAY **********/
#define DISPLAY_PRI_INITIAL_LOGO_ON                      					CTRUE
#define DISPLAY_SEC_INITIAL_LOGO_ON                      					CFALSE
#define DISPLAY_PRI_DISPLAY_ENABLE                       					CTRUE
#define DISPLAY_SEC_DISPLAY_ENABLE                     						CFALSE
#define DISPLAY_MAIN_DISPLAY_SCREEN                        					DISP_PRI_MAIN_SCREEN /* DISP_ALL_MAIN_SCREEN */
#define DISPLAY_HARDWARE_CURSOR_ON                       					CTRUE

#define DISPLAY_MLC_RGB_FORMAT                            					MLC_RGBFMT_R8G8B8
#define DISPLAY_MLC_RGB3D_FORMAT                            				MLC_RGBFMT_R5G6B5
#define DISPLAY_MLC_BYTE_PER_PIXEL                        					3


#define DISPLAY_MLC_VIDEO_LAYER_PRIORITY                       				MLC_VID_PRIORITY_THIRD
#define DISPLAY_MLC_BACKGROUND_COLOR                        				0xFF0000

//// PRIMARY DPC ////
#define DISPLAY_PRI_MAX_X_RESOLUTION                  						320
#define DISPLAY_PRI_MAX_Y_RESOLUTION                  						240
    
#define DISPLAY_LCD_PRI_STN_TYPE            								CFALSE

#define DISPLAY_DPC_PRI_TFT_HSYNC_SWIDTH                      				30
#define DISPLAY_DPC_PRI_TFT_HSYNC_FRONT_PORCH                 				20
#define DISPLAY_DPC_PRI_TFT_HSYNC_BACK_PORCH                  				38
#define DISPLAY_DPC_PRI_TFT_HSYNC_ACTIVEHIGH                  				CFALSE
#define DISPLAY_DPC_PRI_TFT_VSYNC_SWIDTH                      				3
#define DISPLAY_DPC_PRI_TFT_VSYNC_FRONT_PORCH                 				5
#define DISPLAY_DPC_PRI_TFT_VSYNC_BACK_PORCH                  				15
#define DISPLAY_DPC_PRI_TFT_VSYNC_ACTIVEHIGH  	            				CFALSE 
#define DISPLAY_DPC_PRI_TFT_OUTORDER                       					DPC_ORDER_CbYCrY
#define DISPLAY_DPC_PRI_TFT_SCAN_INTERLACE                       			CFALSE
#define DISPLAY_DPC_PRI_TFT_POLFIELD_INVERT                    				CFALSE	
	
#define DISPLAY_DPC_PRI_VCLK_SOURCE                       					DPC_VCLK_SOURCE_PLL1

#define DISPLAY_DPC_PRI_VCLK_DIV                          					28
#define DISPLAY_DPC_PRI_VCLK_DIV_TCL                          				23
#define DISPLAY_DPC_PRI_VCLK2_SOURCE                      					DPC_VCLK_SOURCE_VCLK2
#define DISPLAY_DPC_PRI_VCLK2_DIV                         					1
#define DISPLAY_DPC_PRI_PAD_VCLK                          					DPC_PADVCLK_VCLK
#define DISPLAY_DPC_PRI_OUTPUT_FORMAT                     				    DPC_FORMAT_RGB888

#define DISPLAY_DPC_PRI_CLOCK_INVERT                        			    CTRUE	
#define DISPLAY_DPC_PRI_SYNC_DELAY                        					7		// STN: 8

//// SECONDARY DPC ////
#define DISPLAY_SEC_SCALE_UP_ENABLE                							CTRUE
#define DISPLAY_SEC_MAX_X_RESOLUTION                  						720
#define DISPLAY_SEC_MAX_Y_RESOLUTION                  						480
#define DISPLAY_DPC_SEC_HSYNC_SWIDTH                      					33
#define DISPLAY_DPC_SEC_HSYNC_FRONT_PORCH                 					24
#define DISPLAY_DPC_SEC_HSYNC_BACK_PORCH                  					81

#define DISPLAY_DPC_SEC_HSYNC_SWIDTH1                      					42
#define DISPLAY_DPC_SEC_HSYNC_FRONT_PORCH1                 					82
#define DISPLAY_DPC_SEC_HSYNC_BACK_PORCH1                  					20

#define DISPLAY_DPC_SEC_HSYNC_ACTIVEHIGH                  					CFALSE
#define DISPLAY_DPC_SEC_VSYNC_SWIDTH                      					3
#define DISPLAY_DPC_SEC_VSYNC_FRONT_PORCH                 					3
#define DISPLAY_DPC_SEC_VSYNC_BACK_PORCH                  					16

#define DISPLAY_DPC_SEC_VSYNC_SWIDTH1                      					2
#define DISPLAY_DPC_SEC_VSYNC_FRONT_PORCH1                 					21
#define DISPLAY_DPC_SEC_VSYNC_BACK_PORCH1                  					3
#define DISPLAY_DPC_SEC_VSYNC_ACTIVEHIGH                  					CFALSE
#define DISPLAY_DPC_SEC_VCLK_SOURCE                       					DPC_VCLK_SOURCE_XTI
#define DISPLAY_DPC_SEC_VCLK_DIV                          					1
#define DISPLAY_DPC_SEC_VCLK2_SOURCE                      					DPC_VCLK_SOURCE_VCLK2
#define DISPLAY_DPC_SEC_VCLK2_DIV                         					2
#define DISPLAY_DPC_SEC_PAD_VCLK                          					DPC_PADVCLK_VCLK2
#define DISPLAY_DPC_SEC_OUTPUT_FORMAT                     					DPC_FORMAT_CCIR601B
#define DISPLAY_DPC_SEC_SWAP_RGB                          					CFALSE
#define DISPLAY_DPC_SEC_OUTORDER                          					DPC_ORDER_CbYCrY
#define DISPLAY_DPC_SEC_SCAN_INTERLACE                       				CTRUE
#define DISPLAY_DPC_SEC_POLFIELD_INVERT                    					CFALSE
#define DISPLAY_DPC_SEC_CLOCK_INVERT                        				CFALSE	
#define DISPLAY_DPC_SEC_SYNC_DELAY                        					12

#define DISPLAY_DPC_SEC_ENCODER_ON                         					CTRUE
#define DISPLAY_DPC_SEC_ENCODER_FORMAT                    					DPC_VBS_NTSC_M
#define DISPLAY_DPC_SEC_ENCODER_PEDESTAL                  					CTRUE				// NEW rohan
#define	DISPLAY_DPC_SEC_ENCODER_Y_BANDWIDTH									DPC_BANDWIDTH_LOW	// NEW rohan
#define	DISPLAY_DPC_SEC_ENCODER_C_BANDWIDTH									DPC_BANDWIDTH_LOW	// NEW rohan

//// LCD_MISC ////
#define DISPLAY_LCD_PRI_MISC_CTRL_ON                						CTRUE
#define DISPLAY_LCD_PRI_BRIGHTNESS_ON		           						CTRUE
#define DISPLAY_LCD_PRI_BIRGHTNESS_ACTIVE            						0
#define DISPLAY_LCD_PRI_DEFAULT_BIGHTNESS            						255
#define DISPLAY_LCD_SEC_MISC_CTRL_ON                 						CFALSE
#define DISPLAY_LCD_SEC_BRIGHTNESS           								CFALSE
#define DISPLAY_LCD_SEC_BIRGHTNESS_ACTIVE            						1
#define DISPLAY_LCD_SEC_DEFAULT_BIGHTNESS            						255

/********** IDE_PIO **********/
#define IDE_PIO_USESTATIC                                 					CTRUE
#define IDE_PIO_nICE                                      					0

/********** BUTTONS **********/
#define BUTTONS_MOMENTARY_ON                              					CFALSE
#define BUTTONS_PWROFF_UNIT                               					40
#define BUTTONS_STOP_UNIT                                 					1
#define BUTTONS_SUSPEND_MODE                              					CFALSE
#define BUTTONS_PWRMONITOR_ADC                            					CTRUE
#define BUTTONS_GPIOBUTTON                                					CFALSE
#define BUTTONS_BUTTON_MATRIX                             					CFALSE
#define BUTTONS_MATRIX_NUM_ROW                            					0
#define BUTTONS_MATRIX_NUM_COL                            					0
#define BUTTONS_DIRECT_NUM                                					13

/********** USB_HOST **********/
#define USB_HOST_USE_USB_HOST                             					CTRUE
#define USB_HOST_USE_USB_HOST_POWERCTRL                   					CFALSE

/********** USB_DEVICE **********/
#define USB_DEVICE_USE_CONNECT_CTRL                       					CTRUE


/********** NAND **********/
#define NAND_LARGE_BLOCK                                  					CTRUE
#define NAND_NUM_OF_NAND                                  					1
#define NAND_SIZE_UNIT_MB                                 					256
#define NAND_USE_WRITEPROTECT                             					CTRUE

/********** SD_CARD **********/

/********** DUAL_CPU **********/

/********** VIP **********/
#define VIP_USE_I2C_CH                                    					0

/********** VPP **********/

/********** DMB **********/

/********** IR_REMO **********/

/********** SERIAL **********/
#define SERIAL_ENABLE_UART0                               					CTRUE
#define SERIAL_ENABLE_UART1                               					CTRUE
#define SERIAL_UART1_AFC                                  					DISABLE
#define SERIAL_ENABLE_UART2                               					CTRUE
#define SERIAL_ENABLE_UART3                               					CTRUE


/*************** PWM LIST ***************/
#define PWM_DISPLAY_LCD_PRI_BRIGHTNESS                    			0

/*************** ADC LIST ***************/
#define ADC_TOUCH_XP                                      			0
#define ADC_TOUCH_YP                                      			1
#define ADC_BUTTONS_POWER                                 			2

/*************** TIMER LIST ***************/
#define TIMER_SYSTEM_TIMER                                			0
#define TIMER_SYSTEM_SUB_TIMER                            			1
#define TIMER_TOUCH_TIMER                                 			2

/*************** SPI LIST ***************/
#define SPI_DMB_SPI                                       			0
#define SPI_DMB_SPI1                                      			1

/*************** LAYER LIST ***************/
#define LAYER_DISPLAY_CURSOR_RGB                          			0
#define LAYER_DISPLAY_SCREEN_RGB                          			1
#define LAYER_DISPLAY_3DIMAGE_RGB                         			2
#define LAYER_DISPLAY_SCREEN_VID                          			3

#endif

#endif //__OEM_SYSTEM_MAIN_H__

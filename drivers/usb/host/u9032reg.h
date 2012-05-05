/***************************************************************************
* file name	: u9032reg.h
*
* UBISYS TECHNOLOGY CO., LTD.
*
* author	: dahlsse
*
* date		: DEC 13, 2006
*
* desc		: UBI9032 register definition
*
* history	:
*	- DEC 13, 2006	the first version			by dahlsse
***************************************************************************/
#ifndef _U9032REG_H_
#define _U9032REG_H_


unsigned int g_uiUBI9032BaseAddress;

//!!! BE SURE TO DEFINE THIS ENDIAN CORRECTLY !!!
//!!!         O N L Y    H E R E              !!!
#define UBI9032_C_REG_ADD16(addr)	((volatile unsigned short *)(g_uiUBI9032BaseAddress + (addr)))
#define UBI9032_REG_ADDR16(addr)	((volatile unsigned short *)(g_uiUBI9032BaseAddress + (addr)))


//==========================================================================================================
// Kevin's Note on UBi9032 Register Structure
//==========================================================================================================
//
// in case of little-endian (use same H/W connection & set bus-swapping):
//
// * before bus-swapping:
//   all 16bit access is reversed (because data line is reversed).
//   ex) MainIntSts_DevIntSts.B[0] : 8bit  DevIntSts
//       MainIntSts_DevIntSts.B[1] : 8bit  MainIntSts
//
// * after bus-swapping:
//   - non-numeric type registers -> no byte position change
//                                   so if it's accessed 16bit wide then it's reversed.
//   - numeric type registers -> byte swapped : 16bit read/write remains same value.
//   ex) MainIntSts_DevIntSts.B[0] : 8bit  MainIntSts
//       MainIntSts_DevIntSts.B[1] : 8bit  DevIntSts
//       RamRdAdr                  : 16bit numeric value
//
//==========================================================================================================

/*==========================================================================================================/
/ Register 8bit Access Define																				/
/==========================================================================================================*/

#define rcMainIntStat			(*(UBI9032_C_REG_ADD16(0x00)))	/* Main Interrupt Status						*/
#define rcDeviceIntStat 		(*(UBI9032_C_REG_ADD16(0x00)))	/* USB Device Interrupt Status					*/
#define rcHostIntStat			(*(UBI9032_C_REG_ADD16(0x02)))	/* USB Host Interrupt Status					*/
#define rcCPU_IntStat			(*(UBI9032_C_REG_ADD16(0x02)))	/* CPU Interrupt Status 						*/
#define rcFIFO_IntStat			(*(UBI9032_C_REG_ADD16(0x04)))	/* FIFO Interrupt Status						*/
#define rcMainIntEnb			(*(UBI9032_C_REG_ADD16(0x08)))	/* Main Interrupt Enable						*/
#define rcDeviceIntEnb			(*(UBI9032_C_REG_ADD16(0x08)))	/* USB Device Interrupt Enable					*/
#define rcHostIntEnb			(*(UBI9032_C_REG_ADD16(0x0A)))	/* USB Host Interrupt Enable					*/
#define rcCPU_IntEnb			(*(UBI9032_C_REG_ADD16(0x0A)))	/* CPU Interrupt Enable 						*/
#define rcFIFO_IntEnb			(*(UBI9032_C_REG_ADD16(0x0C)))	/* FIFO Interrupt Enable						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcRevisionNum			(*(UBI9032_C_REG_ADD16(0x10)))	/* Revision Number								*/
#define rcChipReset 			(*(UBI9032_C_REG_ADD16(0x10)))	/* Chip Reset									*/
#define rcPM_Control_0			(*(UBI9032_C_REG_ADD16(0x12)))	/* Power Management Control 					*/
#define rcPM_Control			(*(UBI9032_C_REG_ADD16(0x12)))	/* Power Management Control 					*/
#define rcWakeupTim_H			(*(UBI9032_C_REG_ADD16(0x14)))	/* Wake Up Timer High							*/
#define rcWakeupTim_L			(*(UBI9032_C_REG_ADD16(0x14)))	/* Wake Up Timer Low							*/
#define rcH_USB_Control 		(*(UBI9032_C_REG_ADD16(0x16)))	/* Host USB Control 							*/
#define rcH_XcvrControl 		(*(UBI9032_C_REG_ADD16(0x16)))	/* Host Xcvr Control							*/
#define rcD_USB_Status			(*(UBI9032_C_REG_ADD16(0x18)))	/* Device USB Status							*/
#define rcH_USB_Status			(*(UBI9032_C_REG_ADD16(0x18)))	/* Host USB Status								*/
#define rcHostDeviceSel 		(*(UBI9032_C_REG_ADD16(0x1E)))	/* Host Device Select							*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcFIFO_Rd	 			(*(UBI9032_C_REG_ADD16(0x20)))	/* FIFO Read 0									*/
#define rcFIFO_Rd_H 			(*(UBI9032_C_REG_ADD16(0x20)))	/* FIFO Read 0									*/
#define rcFIFO_Rd_L 			(*(UBI9032_C_REG_ADD16(0x20)))	/* FIFO Read 1									*/
#define rcFIFO_Wr	 			(*(UBI9032_C_REG_ADD16(0x22)))	/* FIFO Write 0 								*/
#define rcFIFO_Wr_H 			(*(UBI9032_C_REG_ADD16(0x22)))	/* FIFO Write 0 								*/
#define rcFIFO_Wr_L 			(*(UBI9032_C_REG_ADD16(0x22)))	/* FIFO Write 1 								*/
#define rcFIFO_RdRemain_H		(*(UBI9032_C_REG_ADD16(0x24)))	/* FIFO Read Remain High						*/
#define rcFIFO_RdRemain_L		(*(UBI9032_C_REG_ADD16(0x24)))	/* FIFO Read Remain Low 						*/
#define rcFIFO_WrRemain_H		(*(UBI9032_C_REG_ADD16(0x26)))	/* FIFO Write Remain High						*/
#define rcFIFO_WrRemain_L		(*(UBI9032_C_REG_ADD16(0x26)))	/* FIFO Write Remain Low						*/
#define rcFIFO_ByteRd			(*(UBI9032_C_REG_ADD16(0x28)))	/* FIFO Byte Read								*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcRAM_RdAdrs_H			(*(UBI9032_C_REG_ADD16(0x30)))	/* RAM Read Address High						*/
#define rcRAM_RdAdrs_L			(*(UBI9032_C_REG_ADD16(0x30)))	/* RAM Read Address Low 						*/
#define rcRAM_RdControl 		(*(UBI9032_C_REG_ADD16(0x32)))	/* RAM Read Control 							*/
#define rcRAM_RdCount			(*(UBI9032_C_REG_ADD16(0x34)))	/* RAM Read Counter 							*/
#define rcRAM_WrAdrs_H			(*(UBI9032_C_REG_ADD16(0x38)))	/* RAM Write Address High						*/
#define rcRAM_WrAdrs_L			(*(UBI9032_C_REG_ADD16(0x38)))	/* RAM Write Address Low						*/
#define rcRAM_WrDoor_H			(*(UBI9032_C_REG_ADD16(0x3A)))	/* RAM Write Door Low							*/
#define rcRAM_WrDoor_L			(*(UBI9032_C_REG_ADD16(0x3A)))	/* RAM Write Door High							*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcRAM_Rd_00 			(*(UBI9032_C_REG_ADD16(0x40)))	/* RAM Read 00									*/
#define rcRAM_Rd_01 			(*(UBI9032_C_REG_ADD16(0x40)))	/* RAM Read 01									*/
#define rcRAM_Rd_02 			(*(UBI9032_C_REG_ADD16(0x42)))	/* RAM Read 02									*/
#define rcRAM_Rd_03 			(*(UBI9032_C_REG_ADD16(0x42)))	/* RAM Read 03									*/
#define rcRAM_Rd_04 			(*(UBI9032_C_REG_ADD16(0x44)))	/* RAM Read 04									*/
#define rcRAM_Rd_05 			(*(UBI9032_C_REG_ADD16(0x44)))	/* RAM Read 05									*/
#define rcRAM_Rd_06 			(*(UBI9032_C_REG_ADD16(0x46)))	/* RAM Read 06									*/
#define rcRAM_Rd_07 			(*(UBI9032_C_REG_ADD16(0x46)))	/* RAM Read 07									*/
#define rcRAM_Rd_08 			(*(UBI9032_C_REG_ADD16(0x48)))	/* RAM Read 08									*/
#define rcRAM_Rd_09 			(*(UBI9032_C_REG_ADD16(0x48)))	/* RAM Read 09									*/
#define rcRAM_Rd_0A 			(*(UBI9032_C_REG_ADD16(0x4A)))	/* RAM Read 0A									*/
#define rcRAM_Rd_0B 			(*(UBI9032_C_REG_ADD16(0x4A)))	/* RAM Read 0B									*/
#define rcRAM_Rd_0C 			(*(UBI9032_C_REG_ADD16(0x4C)))	/* RAM Read 0C									*/
#define rcRAM_Rd_0D 			(*(UBI9032_C_REG_ADD16(0x4C)))	/* RAM Read 0D									*/
#define rcRAM_Rd_0E 			(*(UBI9032_C_REG_ADD16(0x4E)))	/* RAM Read 0E									*/
#define rcRAM_Rd_0F 			(*(UBI9032_C_REG_ADD16(0x4E)))	/* RAM Read 0F									*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcRAM_Rd_10 			(*(UBI9032_C_REG_ADD16(0x50)))	/* RAM Read 10									*/
#define rcRAM_Rd_11 			(*(UBI9032_C_REG_ADD16(0x50)))	/* RAM Read 11									*/
#define rcRAM_Rd_12 			(*(UBI9032_C_REG_ADD16(0x52)))	/* RAM Read 12									*/
#define rcRAM_Rd_13 			(*(UBI9032_C_REG_ADD16(0x52)))	/* RAM Read 13									*/
#define rcRAM_Rd_14 			(*(UBI9032_C_REG_ADD16(0x54)))	/* RAM Read 14									*/
#define rcRAM_Rd_15 			(*(UBI9032_C_REG_ADD16(0x54)))	/* RAM Read 15									*/
#define rcRAM_Rd_16 			(*(UBI9032_C_REG_ADD16(0x56)))	/* RAM Read 16									*/
#define rcRAM_Rd_17 			(*(UBI9032_C_REG_ADD16(0x56)))	/* RAM Read 17									*/
#define rcRAM_Rd_18 			(*(UBI9032_C_REG_ADD16(0x58)))	/* RAM Read 18									*/
#define rcRAM_Rd_19 			(*(UBI9032_C_REG_ADD16(0x58)))	/* RAM Read 19									*/
#define rcRAM_Rd_1A 			(*(UBI9032_C_REG_ADD16(0x5A)))	/* RAM Read 1A									*/
#define rcRAM_Rd_1B 			(*(UBI9032_C_REG_ADD16(0x5A)))	/* RAM Read 1B									*/
#define rcRAM_Rd_1C 			(*(UBI9032_C_REG_ADD16(0x5C)))	/* RAM Read 1C									*/
#define rcRAM_Rd_1D 			(*(UBI9032_C_REG_ADD16(0x5C)))	/* RAM Read 1D									*/
#define rcRAM_Rd_1E 			(*(UBI9032_C_REG_ADD16(0x5E)))	/* RAM Read 1E									*/
#define rcRAM_Rd_1F 			(*(UBI9032_C_REG_ADD16(0x5E)))	/* RAM Read 1F									*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcDMA_Config			(*(UBI9032_C_REG_ADD16(0x60)))	/* DMA Configuration							*/
#define rcDMA_Control			(*(UBI9032_C_REG_ADD16(0x62)))	/* DMA Control									*/
#define rcDMA_Remain_H			(*(UBI9032_C_REG_ADD16(0x64)))	/* DMA FIFO Remain High 						*/
#define rcDMA_Remain_L			(*(UBI9032_C_REG_ADD16(0x64)))	/* DMA FIFO Remain Low							*/
#define rcDMA_Count_HH			(*(UBI9032_C_REG_ADD16(0x68)))	/* DMA Transfer Byte Counter High/High			*/
#define rcDMA_Count_HL			(*(UBI9032_C_REG_ADD16(0x68)))	/* DMA Transfer Byte Counter High/Low			*/
#define rcDMA_Count_LH			(*(UBI9032_C_REG_ADD16(0x6A)))	/* DMA Transfer Byte Counter Low/High			*/
#define rcDMA_Count_LL			(*(UBI9032_C_REG_ADD16(0x6A)))	/* DMA Transfer Byte Counter Low/Low			*/
#define rcDMA_RdData_0			(*(UBI9032_C_REG_ADD16(0x6C)))	/* DMA Read Data Low							*/
#define rcDMA_RdData_1			(*(UBI9032_C_REG_ADD16(0x6C)))	/* DMA Read Data High							*/
#define rcDMA_WrData_0			(*(UBI9032_C_REG_ADD16(0x6E)))	/* DMA Write Data Low							*/
#define rcDMA_WrData_1			(*(UBI9032_C_REG_ADD16(0x6E)))	/* DMA Write Data High							*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcReserved70			(*(UBI9032_C_REG_ADD16(0x70)))	/* Mode Protection								*/
#define rcModeProtect			(*(UBI9032_C_REG_ADD16(0x70)))	/* Mode Protection								*/
#define rcReserved72			(*(UBI9032_C_REG_ADD16(0x72)))	/* Mode Protection								*/
#define rcClkSelect 			(*(UBI9032_C_REG_ADD16(0x72)))	/* Clock Select 								*/
#define rcReserved74			(*(UBI9032_C_REG_ADD16(0x74)))	/* Mode Protection								*/
#define rcChipConfig			(*(UBI9032_C_REG_ADD16(0x74)))	/* CPU Configuration							*/
#define rcReserved76			(*(UBI9032_C_REG_ADD16(0x76)))	/* Mode Protection								*/
#define rcDummyRead 			(*(UBI9032_C_REG_ADD16(0x76)))	/* Dummy Read for CPU_Endian					*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcAREA0StartAdrs_H		(*(UBI9032_C_REG_ADD16(0x80)))	/* AREA0 Start Address High 					*/
#define rcAREA0StartAdrs_L		(*(UBI9032_C_REG_ADD16(0x80)))	/* AREA0 Start Address Low						*/
#define rcAREA0EndAdrs_H		(*(UBI9032_C_REG_ADD16(0x82)))	/* AREA0 End Address High						*/
#define rcAREA0EndAdrs_L		(*(UBI9032_C_REG_ADD16(0x82)))	/* AREA0 End Address Low						*/
#define rcAREA1StartAdrs_H		(*(UBI9032_C_REG_ADD16(0x84)))	/* AREA1 Start Address High 					*/
#define rcAREA1StartAdrs_L		(*(UBI9032_C_REG_ADD16(0x84)))	/* AREA1 Start Address Low						*/
#define rcAREA1EndAdrs_H		(*(UBI9032_C_REG_ADD16(0x86)))	/* AREA1 End Address High						*/
#define rcAREA1EndAdrs_L		(*(UBI9032_C_REG_ADD16(0x86)))	/* AREA1 End Address Low						*/
#define rcAREA2StartAdrs_H		(*(UBI9032_C_REG_ADD16(0x88)))	/* AREA2 Start Address High 					*/
#define rcAREA2StartAdrs_L		(*(UBI9032_C_REG_ADD16(0x88)))	/* AREA2 Start Address Low						*/
#define rcAREA2EndAdrs_H		(*(UBI9032_C_REG_ADD16(0x8A)))	/* AREA2 End Address High						*/
#define rcAREA2EndAdrs_L		(*(UBI9032_C_REG_ADD16(0x8A)))	/* AREA2 End Address Low						*/
#define rcAREA3StartAdrs_H		(*(UBI9032_C_REG_ADD16(0x8C)))	/* AREA3 Start Address High 					*/
#define rcAREA3StartAdrs_L		(*(UBI9032_C_REG_ADD16(0x8C)))	/* AREA3 Start Address Low						*/
#define rcAREA3EndAdrs_H		(*(UBI9032_C_REG_ADD16(0x8E)))	/* AREA3 End Address High						*/
#define rcAREA3EndAdrs_L		(*(UBI9032_C_REG_ADD16(0x8E)))	/* AREA3 End Address Low						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcAREA4StartAdrs_H		(*(UBI9032_C_REG_ADD16(0x90)))	/* AREA4 Start Address High 					*/
#define rcAREA4StartAdrs_L		(*(UBI9032_C_REG_ADD16(0x90)))	/* AREA4 Start Address Low						*/
#define rcAREA4EndAdrs			(*(UBI9032_C_REG_ADD16(0x92)))	/* AREA4 End Address High						*/
#define rcAREA4EndAdrs_H		(*(UBI9032_C_REG_ADD16(0x92)))	/* AREA4 End Address High						*/
#define rcAREA4EndAdrs_L		(*(UBI9032_C_REG_ADD16(0x92)))	/* AREA4 End Address Low						*/
#define rcAREA5StartAdrs_H		(*(UBI9032_C_REG_ADD16(0x94)))	/* AREA5 Start Address High 					*/
#define rcAREA5StartAdrs_L		(*(UBI9032_C_REG_ADD16(0x94)))	/* AREA5 Start Address Low						*/
#define rcAREA5EndAdrs_H		(*(UBI9032_C_REG_ADD16(0x96)))	/* AREA5 End Address High						*/
#define rcAREA5EndAdrs_L		(*(UBI9032_C_REG_ADD16(0x96)))	/* AREA5 End Address Low						*/
#define rcAREAnFIFO_Clr 		(*(UBI9032_C_REG_ADD16(0x9E)))	/* AREAn FIFO Clear 							*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcAREA0Join_0			(*(UBI9032_C_REG_ADD16(0xA0)))	/* AREA0 Join 0 								*/
#define rcAREA0Join_1			(*(UBI9032_C_REG_ADD16(0xA0)))	/* AREA0 Join 1 								*/
#define rcAREA1Join				(*(UBI9032_C_REG_ADD16(0xA2)))	/* AREA1 Join 0 								*/
#define rcAREA1Join_0			(*(UBI9032_C_REG_ADD16(0xA2)))	/* AREA1 Join 0 								*/
#define rcAREA1Join_1			(*(UBI9032_C_REG_ADD16(0xA2)))	/* AREA1 Join 1 								*/
#define rcAREA2Join				(*(UBI9032_C_REG_ADD16(0xA4)))	/* AREA2 Join 0 								*/
#define rcAREA2Join_0			(*(UBI9032_C_REG_ADD16(0xA4)))	/* AREA2 Join 0 								*/
#define rcAREA2Join_1			(*(UBI9032_C_REG_ADD16(0xA4)))	/* AREA2 Join 1 								*/
#define rcAREA3Join				(*(UBI9032_C_REG_ADD16(0xA6)))	/* AREA3 Join 0 								*/
#define rcAREA3Join_0			(*(UBI9032_C_REG_ADD16(0xA6)))	/* AREA3 Join 0 								*/
#define rcAREA3Join_1			(*(UBI9032_C_REG_ADD16(0xA6)))	/* AREA3 Join 1 								*/
#define rcAREA4Join				(*(UBI9032_C_REG_ADD16(0xA8)))	/* AREA4 Join 0 								*/
#define rcAREA4Join_0			(*(UBI9032_C_REG_ADD16(0xA8)))	/* AREA4 Join 0 								*/
#define rcAREA4Join_1			(*(UBI9032_C_REG_ADD16(0xA8)))	/* AREA4 Join 1 								*/
#define rcAREA5Join_0			(*(UBI9032_C_REG_ADD16(0xAA)))	/* AREA5 Join 0 								*/
#define rcAREA5Join_1			(*(UBI9032_C_REG_ADD16(0xAA)))	/* AREA5 Join 1 								*/
#define rcClrAREAnJoin_0		(*(UBI9032_C_REG_ADD16(0xAE)))	/* Clear AREA Join 0							*/
#define rcClrAREAnJoin_1		(*(UBI9032_C_REG_ADD16(0xAE)))	/* Clear AREA Join 1							*/

/*-------------------------------------------------------------------------------------------------*/
/*-------------------/
/ Device Registers /
/-------------------*/

#define rcD_SIE_IntStat 		(*(UBI9032_C_REG_ADD16(0xB0)))	/* Device SIE Interrupt Status					*/
#define rcD_BulkIntStat 		(*(UBI9032_C_REG_ADD16(0xB2)))	/* Device Bulk Interrupt Status 				*/
#define rcD_EPrIntStat			(*(UBI9032_C_REG_ADD16(0xB4)))	/* Device EPr Interrupt Status					*/
#define rcD_EP0IntStat			(*(UBI9032_C_REG_ADD16(0xB4)))	/* Device EP0 Interrupt Status					*/
#define rcD_EPaIntStat			(*(UBI9032_C_REG_ADD16(0xB6)))	/* Device EPa Interrupt Status					*/
#define rcD_EPbIntStat			(*(UBI9032_C_REG_ADD16(0xB6)))	/* Device EPb Interrupt Status					*/
#define rcD_EPcIntStat			(*(UBI9032_C_REG_ADD16(0xB8)))	/* Device EPc Interrupt Status					*/
#define rcD_EPdIntStat			(*(UBI9032_C_REG_ADD16(0xB8)))	/* Device EPd Interrupt Status					*/
#define rcD_EPeIntStat			(*(UBI9032_C_REG_ADD16(0xBA)))	/* Device EPe Interrupt Status					*/
#define rcD_AlarmIN_IntStat_H	(*(UBI9032_C_REG_ADD16(0xBC)))	/* Device Alarm IN Interrupt Status High		*/
#define rcD_AlarmIN_IntStat_L	(*(UBI9032_C_REG_ADD16(0xBC)))	/* Device Alarm IN Interrupt Status Low 		*/
#define rcD_AlarmOUT_IntStat_H	(*(UBI9032_C_REG_ADD16(0xBE)))	/* Device Alarm OUT Interrupt Status High		*/
#define rcD_AlarmOUT_IntStat_L	(*(UBI9032_C_REG_ADD16(0xBE)))	/* Device Alarm OUT Interrupt Status Low		*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcD_SIE_IntEnb			(*(UBI9032_C_REG_ADD16(0xC0)))	/* Device SIE Interrupt Enable					*/
#define rcD_BulkIntEnb			(*(UBI9032_C_REG_ADD16(0xC2)))	/* Device Bulk Interrupt Enable 				*/
#define rcD_EPrIntEnb			(*(UBI9032_C_REG_ADD16(0xC4)))	/* Device EPr Interrupt Enable					*/
#define rcD_EP0IntEnb			(*(UBI9032_C_REG_ADD16(0xC4)))	/* Device EP0 Interrupt Enable					*/
#define rcD_EPaIntEnb			(*(UBI9032_C_REG_ADD16(0xC6)))	/* Device EPa Interrupt Enable					*/
#define rcD_EPbIntEnb			(*(UBI9032_C_REG_ADD16(0xC6)))	/* Device EPb Interrupt Enable					*/
#define rcD_EPcIntEnb			(*(UBI9032_C_REG_ADD16(0xC8)))	/* Device EPc Interrupt Enable					*/
#define rcD_EPdIntEnb			(*(UBI9032_C_REG_ADD16(0xC8)))	/* Device EPd Interrupt Enable					*/
#define rcD_EPeIntEnb			(*(UBI9032_C_REG_ADD16(0xCA)))	/* Device EPe Interrupt Enable					*/
#define rcD_AlarmIN_IntEnb_H	(*(UBI9032_C_REG_ADD16(0xCC)))	/* Device Alarm IN Interrupt Enable High		*/
#define rcD_AlarmIN_IntEnb_L	(*(UBI9032_C_REG_ADD16(0xCC)))	/* Device Alarm IN Interrupt Enable Low 		*/
#define rcD_AlarmOUT_IntEnb_H	(*(UBI9032_C_REG_ADD16(0xCE)))	/* Device Alarm OUT Interrupt Enable High		*/
#define rcD_AlarmOUT_IntEnb_L	(*(UBI9032_C_REG_ADD16(0xCE)))	/* Device Alarm OUT Interrupt Enable Low		*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcD_NegoControl 		(*(UBI9032_C_REG_ADD16(0xD0)))	/* Device Negotiation Control					*/
#define rcD_XcvrControl 		(*(UBI9032_C_REG_ADD16(0xD2)))	/* Device Xcvr Control							*/
#define rcD_USB_Test			(*(UBI9032_C_REG_ADD16(0xD4)))	/* Device USB Test								*/
#define rcD_EPnControl			(*(UBI9032_C_REG_ADD16(0xD6)))	/* Device EPn Control							*/
#define rcD_BulkOnlyControl 	(*(UBI9032_C_REG_ADD16(0xD8)))	/* Device BulkOnly Control						*/
#define rcD_BulkOnlyConfig		(*(UBI9032_C_REG_ADD16(0xD8)))	/* Device BulkOnly Config						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcD_EP0SETUP_0			(*(UBI9032_C_REG_ADD16(0xE0)))	/* Device EP0 SETUP 0							*/
#define rcD_EP0SETUP_1			(*(UBI9032_C_REG_ADD16(0xE0)))	/* Device EP0 SETUP 1							*/
#define rcD_EP0SETUP_2			(*(UBI9032_C_REG_ADD16(0xE2)))	/* Device EP0 SETUP 2							*/
#define rcD_EP0SETUP_3			(*(UBI9032_C_REG_ADD16(0xE2)))	/* Device EP0 SETUP 3							*/
#define rcD_EP0SETUP_4			(*(UBI9032_C_REG_ADD16(0xE4)))	/* Device EP0 SETUP 4							*/
#define rcD_EP0SETUP_5			(*(UBI9032_C_REG_ADD16(0xE4)))	/* Device EP0 SETUP 5							*/
#define rcD_EP0SETUP_6			(*(UBI9032_C_REG_ADD16(0xE6)))	/* Device EP0 SETUP 6							*/
#define rcD_EP0SETUP_7			(*(UBI9032_C_REG_ADD16(0xE6)))	/* Device EP0 SETUP 7							*/
#define rcD_USB_Address 		(*(UBI9032_C_REG_ADD16(0xE8)))	/* Device USB Address							*/
#define rcD_SETUP_Control		(*(UBI9032_C_REG_ADD16(0xEA)))	/* Device SETUP Control 						*/
#define rcD_FrameNumber_H		(*(UBI9032_C_REG_ADD16(0xEE)))	/* Device FrameNumber High						*/
#define rcD_FrameNumber_L		(*(UBI9032_C_REG_ADD16(0xEE)))	/* Device FrameNumber Low						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcD_EP0MaxSize			(*(UBI9032_C_REG_ADD16(0xF0)))	/* Device EP0 Max Packet Size					*/
#define rcD_EP0Control			(*(UBI9032_C_REG_ADD16(0xF0)))	/* Device EP0 Control							*/
#define rcD_EP0ControlIN		(*(UBI9032_C_REG_ADD16(0xF2)))	/* Device EP0 Control IN						*/
#define rcD_EP0ControlOUT		(*(UBI9032_C_REG_ADD16(0xF2)))	/* Device EP0 Control OUT						*/
#define rcD_EPaMaxSize_H		(*(UBI9032_C_REG_ADD16(0xF8)))	/* Device EPa Max Packet Size Low				*/
#define rcD_EPaMaxSize_L		(*(UBI9032_C_REG_ADD16(0xF8)))	/* Device EPa Max Packet Size Low				*/
#define rcD_EPaConfig			(*(UBI9032_C_REG_ADD16(0xFA)))	/* Device EPa Configuration 					*/
#define rcD_EPaControl			(*(UBI9032_C_REG_ADD16(0xFC)))	/* Device EPa Control							*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcD_EPbMaxSize_H		(*(UBI9032_C_REG_ADD16(0x100)))	/* Device EPb Max Packet Size High				*/
#define rcD_EPbMaxSize_L		(*(UBI9032_C_REG_ADD16(0x100)))	/* Device EPb Max Packet Size Low				*/
#define rcD_EPbConfig			(*(UBI9032_C_REG_ADD16(0x102)))	/* Device EPb Configuration 					*/
#define rcD_EPbControl			(*(UBI9032_C_REG_ADD16(0x104)))	/* Device EPb Control							*/
#define rcD_EPcMaxSize_H		(*(UBI9032_C_REG_ADD16(0x108)))	/* Device EPc Max Packet Size High				*/
#define rcD_EPcMaxSize_L		(*(UBI9032_C_REG_ADD16(0x108)))	/* Device EPc Max Packet Size Low				*/
#define rcD_EPcConfig			(*(UBI9032_C_REG_ADD16(0x10A)))	/* Device EPc Configuration 					*/
#define rcD_EPcControl			(*(UBI9032_C_REG_ADD16(0x10C)))	/* Device EPc Control							*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcD_EPdMaxSize_H		(*(UBI9032_C_REG_ADD16(0x110)))	/* Device EPd Max Packet Size High				*/
#define rcD_EPdMaxSize_L		(*(UBI9032_C_REG_ADD16(0x110)))	/* Device EPd Max Packet Size Low				*/
#define rcD_EPdConfig			(*(UBI9032_C_REG_ADD16(0x112)))	/* Device EPd Configuration 					*/
#define rcD_EPdControl			(*(UBI9032_C_REG_ADD16(0x114)))	/* Device EPd Control							*/
#define rcD_EPeMaxSize_H		(*(UBI9032_C_REG_ADD16(0x118)))	/* Device EPe Max Packet Size High				*/
#define rcD_EPeMaxSize_L		(*(UBI9032_C_REG_ADD16(0x118)))	/* Device EPe Max Packet Size Low				*/
#define rcD_EPeConfig			(*(UBI9032_C_REG_ADD16(0x11A)))	/* Device EPe Configuration 					*/
#define rcD_EPeControl			(*(UBI9032_C_REG_ADD16(0x11C)))	/* Device EPe Control							*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcD_DescAdrs_H			(*(UBI9032_C_REG_ADD16(0x120)))	/* Device Descriptor Address High				*/
#define rcD_DescAdrs_L			(*(UBI9032_C_REG_ADD16(0x120)))	/* Device Descriptor Address Low				*/
#define rcD_DescSize_H			(*(UBI9032_C_REG_ADD16(0x122)))	/* Device Descriptor Size High					*/
#define rcD_DescSize_L			(*(UBI9032_C_REG_ADD16(0x122)))	/* Device Descriptor Size Low					*/
#define rcD_EP_DMA_Ctrl 		(*(UBI9032_C_REG_ADD16(0x126)))	/* Device Endpont DMA Control					*/
#define rcD_EnEP_IN_H			(*(UBI9032_C_REG_ADD16(0x128)))	/* Device Enable Endpoint IN High				*/
#define rcD_EnEP_IN_L			(*(UBI9032_C_REG_ADD16(0x128)))	/* Device Enable Endpoint IN Low				*/
#define rcD_EnEP_OUT_H			(*(UBI9032_C_REG_ADD16(0x12A)))	/* Device Enable Endpoint OUT High				*/
#define rcD_EnEP_OUT_L			(*(UBI9032_C_REG_ADD16(0x12A)))	/* Device Enable Endpoint OUT Low				*/
#define rcD_EnEP_IN_ISO_H		(*(UBI9032_C_REG_ADD16(0x12C)))	/* Device Enable Endpoint ISO High				*/
#define rcD_EnEP_IN_ISO_L		(*(UBI9032_C_REG_ADD16(0x12C)))	/* Device Enable Endpoint ISO Low				*/
#define rcD_EnEP_OUT_ISO_H		(*(UBI9032_C_REG_ADD16(0x12E)))	/* Device Enable Endpoint ISO High				*/
#define rcD_EnEP_OUT_ISO_L		(*(UBI9032_C_REG_ADD16(0x12E)))	/* Device Enable Endpoint ISO Low				*/

/*-----------------/
/ Host Registers /
/-----------------*/

#define rcH_SIE_IntStat_0		(*(UBI9032_C_REG_ADD16(0x140)))	/* Host SIE Interrupt Status 0					*/
#define rcH_SIE_IntStat_1		(*(UBI9032_C_REG_ADD16(0x140)))	/* Host SIE Interrupt Status 1					*/
#define rcH_FIFO_IntStat		(*(UBI9032_C_REG_ADD16(0x142)))	/* Host Frame Interrupt Status					*/
#define rcH_FrameIntStat		(*(UBI9032_C_REG_ADD16(0x142)))	/* Host Frame Interrupt Status					*/
#define rcH_CHrIntStat			(*(UBI9032_C_REG_ADD16(0x144)))	/* Host CHr Interrupt Status					*/
#define rcH_CH0IntStat			(*(UBI9032_C_REG_ADD16(0x144)))	/* Host CH0 Interrupt Status					*/
#define rcH_CHaIntStat			(*(UBI9032_C_REG_ADD16(0x146)))	/* Host CHa Interrupt Status					*/
#define rcH_CHbIntStat			(*(UBI9032_C_REG_ADD16(0x146)))	/* Host CHb Interrupt Status					*/
#define rcH_CHcIntStat			(*(UBI9032_C_REG_ADD16(0x148)))	/* Host CHc Interrupt Status					*/
#define rcH_CHdIntStat			(*(UBI9032_C_REG_ADD16(0x148)))	/* Host CHd Interrupt Status					*/
#define rcH_CHeIntStat			(*(UBI9032_C_REG_ADD16(0x14A)))	/* Host CHe Interrupt Status					*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcH_SIE_IntEnb_0		(*(UBI9032_C_REG_ADD16(0x150)))	/* Host SIE Interrupt Enable 0					*/
#define rcH_SIE_IntEnb_1		(*(UBI9032_C_REG_ADD16(0x150)))	/* Host SIE Interrupt Enable 1					*/
#define rcH_FIFO_IntEnb 		(*(UBI9032_C_REG_ADD16(0x152)))	/* Host Frame Interrupt Enable					*/
#define rcH_FrameIntEnb 		(*(UBI9032_C_REG_ADD16(0x152)))	/* Host Frame Interrupt Enable					*/
#define rcH_CHrIntEnb			(*(UBI9032_C_REG_ADD16(0x154)))	/* Host CHr Interrupt Enable					*/
#define rcH_CH0IntEnb			(*(UBI9032_C_REG_ADD16(0x154)))	/* Host CH0 Interrupt Enable					*/
#define rcH_CHaIntEnb			(*(UBI9032_C_REG_ADD16(0x156)))	/* Host CHa Interrupt Enable					*/
#define rcH_CHbIntEnb			(*(UBI9032_C_REG_ADD16(0x156)))	/* Host CHb Interrupt Enable					*/
#define rcH_CHcIntEnb			(*(UBI9032_C_REG_ADD16(0x158)))	/* Host CHc Interrupt Enable					*/
#define rcH_CHdIntEnb			(*(UBI9032_C_REG_ADD16(0x158)))	/* Host CHd Interrupt Enable					*/
#define rcH_CHeIntEnb			(*(UBI9032_C_REG_ADD16(0x15A)))	/* Host CHe Interrupt Enable					*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcH_NegoControl_0		(*(UBI9032_C_REG_ADD16(0x160)))	/* Host Negotiation Control 0					*/
#define rcH_NegoControl_1		(*(UBI9032_C_REG_ADD16(0x162)))	/* Host Negotiation Control 1					*/
#define rcH_USB_Test			(*(UBI9032_C_REG_ADD16(0x164)))	/* Host USB Test								*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcH_CH0SETUP_0			(*(UBI9032_C_REG_ADD16(0x170)))	/* Host CH0 SETUP 0 							*/
#define rcH_CH0SETUP_1			(*(UBI9032_C_REG_ADD16(0x170)))	/* Host CH0 SETUP 1 							*/
#define rcH_CH0SETUP_2			(*(UBI9032_C_REG_ADD16(0x172)))	/* Host CH0 SETUP 2 							*/
#define rcH_CH0SETUP_3			(*(UBI9032_C_REG_ADD16(0x172)))	/* Host CH0 SETUP 3 							*/
#define rcH_CH0SETUP_4			(*(UBI9032_C_REG_ADD16(0x174)))	/* Host CH0 SETUP 4 							*/
#define rcH_CH0SETUP_5			(*(UBI9032_C_REG_ADD16(0x174)))	/* Host CH0 SETUP 5 							*/
#define rcH_CH0SETUP_6			(*(UBI9032_C_REG_ADD16(0x176)))	/* Host CH0 SETUP 6 							*/
#define rcH_CH0SETUP_7			(*(UBI9032_C_REG_ADD16(0x176)))	/* Host CH0 SETUP 7 							*/
#define rcH_FrameNumber_H		(*(UBI9032_C_REG_ADD16(0x17E)))	/* Host Frame Number High						*/
#define rcH_FrameNumber_L		(*(UBI9032_C_REG_ADD16(0x17E)))	/* Host Frame Number Low						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcH_CH0Config_0 		(*(UBI9032_C_REG_ADD16(0x180)))	/* Host CH0 Configuration 0 					*/
#define rcH_CH0Config_1 		(*(UBI9032_C_REG_ADD16(0x180)))	/* Host CH0 COnfiguration 1 					*/
#define rcH_CH0MaxPktSize		(*(UBI9032_C_REG_ADD16(0x182)))	/* Host CH0 MaxPacketSize						*/
#define rcH_CH0TotalSize_H		(*(UBI9032_C_REG_ADD16(0x186)))	/* Host CH0 Total Size High 					*/
#define rcH_CH0TotalSize_L		(*(UBI9032_C_REG_ADD16(0x186)))	/* Host CH0 Total Size Low						*/
#define rcH_CH0HubAdrs			(*(UBI9032_C_REG_ADD16(0x188)))	/* Host CH0 Hub Address 						*/
#define rcH_CH0FuncAdrs 		(*(UBI9032_C_REG_ADD16(0x188)))	/* Host CH0 Function Address					*/
#define rcH_CTL_SupportControl	(*(UBI9032_C_REG_ADD16(0x18A)))	/* Host CONTROL Support Control 				*/
#define rcH_CH0ConditionCode	(*(UBI9032_C_REG_ADD16(0x18E)))	/* Host CH0 Condition Code						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcH_CHaConfig_0 		(*(UBI9032_C_REG_ADD16(0x190)))	/* Host CHa Configuration 0 					*/
#define rcH_CHaConfig_1 		(*(UBI9032_C_REG_ADD16(0x190)))	/* Host CHa COnfiguration 1 					*/
#define rcH_CHaMaxPktSize_H 	(*(UBI9032_C_REG_ADD16(0x192)))	/* Host CHa MaxPacketSize High					*/
#define rcH_CHaMaxPktSize_L 	(*(UBI9032_C_REG_ADD16(0x192)))	/* Host CHa MaxPacketSize Low					*/
#define rcH_CHaTotalSize_HH 	(*(UBI9032_C_REG_ADD16(0x194)))	/* Host CHa Total Size High 					*/
#define rcH_CHaTotalSize_HL 	(*(UBI9032_C_REG_ADD16(0x194)))	/* Host CHa Total Size Low						*/
#define rcH_CHaTotalSize_LH 	(*(UBI9032_C_REG_ADD16(0x196)))	/* Host CHa Total Size High 					*/
#define rcH_CHaTotalSize_LL 	(*(UBI9032_C_REG_ADD16(0x196)))	/* Host CHa Total Size Low						*/
#define rcH_CHaHubAdrs			(*(UBI9032_C_REG_ADD16(0x198)))	/* Host CHa Hub Address 						*/
#define rcH_CHaFuncAdrs 		(*(UBI9032_C_REG_ADD16(0x198)))	/* Host CHa Function Address					*/
#define rcH_BO_SupportControl	(*(UBI9032_C_REG_ADD16(0x19A)))	/* Host CHa BulkOnly Support Control			*/
#define rcH_CSW_RcvDataSize 	(*(UBI9032_C_REG_ADD16(0x19A)))	/* Host CHa BulkOnly CSW Receive Data Size		*/
#define rcH_OUT_EP_Control		(*(UBI9032_C_REG_ADD16(0x19C)))	/* Host CHa BulkOnly OUT EndPoint Control		*/
#define rcH_CHaBO_IN_EP_Ctl 	(*(UBI9032_C_REG_ADD16(0x19C)))	/* Host Cha BulkOnly IN EndPoint Control		*/
#define rcH_CHaConditionCode	(*(UBI9032_C_REG_ADD16(0x19E)))	/* Host CHa Condition Code						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcH_CHbConfig_0 		(*(UBI9032_C_REG_ADD16(0x1A0)))	/* Host CHb Configuration 0 					*/
#define rcH_CHbConfig_1 		(*(UBI9032_C_REG_ADD16(0x1A0)))	/* Host CHb COnfiguration 1 					*/
#define rcH_CHbMaxPktSize_H 	(*(UBI9032_C_REG_ADD16(0x1A2)))	/* Host CHb MaxPacketSize High					*/
#define rcH_CHbMaxPktSize_L 	(*(UBI9032_C_REG_ADD16(0x1A2)))	/* Host CHb MaxPacketSize Low					*/
#define rcH_CHbTotalSize_HH 	(*(UBI9032_C_REG_ADD16(0x1A4)))	/* Host CHb Total Size High 					*/
#define rcH_CHbTotalSize_HL 	(*(UBI9032_C_REG_ADD16(0x1A4)))	/* Host CHb Total Size Low						*/
#define rcH_CHbTotalSize_LH 	(*(UBI9032_C_REG_ADD16(0x1A6)))	/* Host CHb Total Size High 					*/
#define rcH_CHbTotalSize_LL 	(*(UBI9032_C_REG_ADD16(0x1A6)))	/* Host CHb Total Size Low						*/
#define rcH_CHbHubAdrs			(*(UBI9032_C_REG_ADD16(0x1A8)))	/* Host CHb Hub Address 						*/
#define rcH_CHbFuncAdrs 		(*(UBI9032_C_REG_ADD16(0x1A8)))	/* Host CHb Function Address					*/
#define rcH_CHbInterval_H		(*(UBI9032_C_REG_ADD16(0x1AA)))	/* Host CHb Interval High						*/
#define rcH_CHbInterval_L		(*(UBI9032_C_REG_ADD16(0x1AA)))	/* Host CHb Interval Low						*/
#define rcH_CHbConditionCode	(*(UBI9032_C_REG_ADD16(0x1AE)))	/* Host CHb Condition Code						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////

#define rcH_CHcConfig_0 		(*(UBI9032_C_REG_ADD16(0x1B0)))	/* Host CHc Configuration 0 					*/
#define rcH_CHcConfig_1 		(*(UBI9032_C_REG_ADD16(0x1B0)))	/* Host CHc COnfiguration 1 					*/
#define rcH_CHcMaxPktSize_H 	(*(UBI9032_C_REG_ADD16(0x1B2)))	/* Host CHc MaxPacketSize High					*/
#define rcH_CHcMaxPktSize_L 	(*(UBI9032_C_REG_ADD16(0x1B2)))	/* Host CHc MaxPacketSize Low					*/
#define rcH_CHcTotalSize_HH 	(*(UBI9032_C_REG_ADD16(0x1B4)))	/* Host CHc Total Size High 					*/
#define rcH_CHcTotalSize_HL 	(*(UBI9032_C_REG_ADD16(0x1B4)))	/* Host CHc Total Size Low						*/
#define rcH_CHcTotalSize_LH 	(*(UBI9032_C_REG_ADD16(0x1B6)))	/* Host CHc Total Size High 					*/
#define rcH_CHcTotalSize_LL 	(*(UBI9032_C_REG_ADD16(0x1B6)))	/* Host CHc Total Size Low						*/
#define rcH_CHcHubAdrs			(*(UBI9032_C_REG_ADD16(0x1B8)))	/* Host CHc Hub Address 						*/
#define rcH_CHcFuncAdrs 		(*(UBI9032_C_REG_ADD16(0x1B8)))	/* Host CHc Function Address					*/
#define rcH_CHcInterval_H		(*(UBI9032_C_REG_ADD16(0x1BA)))	/* Host CHc Interval High						*/
#define rcH_CHcInterval_L		(*(UBI9032_C_REG_ADD16(0x1BA)))	/* Host CHc Interval Low						*/
#define rcH_CHcConditionCode	(*(UBI9032_C_REG_ADD16(0x1BE)))	/* Host CHc Condition Code						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////
#define rcH_CHdConfig_0 		(*(UBI9032_C_REG_ADD16(0x1C0)))	/* Host CHd Configuration 0 					*/
#define rcH_CHdConfig_1 		(*(UBI9032_C_REG_ADD16(0x1C0)))	/* Host CHd COnfiguration 1 					*/
#define rcH_CHdMaxPktSize_H 	(*(UBI9032_C_REG_ADD16(0x1C2)))	/* Host CHd MaxPacketSize High					*/
#define rcH_CHdMaxPktSize_L 	(*(UBI9032_C_REG_ADD16(0x1C2)))	/* Host CHd MaxPacketSize Low					*/
#define rcH_CHdTotalSize_HH 	(*(UBI9032_C_REG_ADD16(0x1C4)))	/* Host CHd Total Size High 					*/
#define rcH_CHdTotalSize_HL 	(*(UBI9032_C_REG_ADD16(0x1C4)))	/* Host CHd Total Size Low						*/
#define rcH_CHdTotalSize_LH 	(*(UBI9032_C_REG_ADD16(0x1C6)))	/* Host CHd Total Size High 					*/
#define rcH_CHdTotalSize_LL 	(*(UBI9032_C_REG_ADD16(0x1C6)))	/* Host CHd Total Size Low						*/
#define rcH_CHdHubAdrs			(*(UBI9032_C_REG_ADD16(0x1C8)))	/* Host CHd Hub Address 						*/
#define rcH_CHdFuncAdrs 		(*(UBI9032_C_REG_ADD16(0x1C8)))	/* Host CHd Function Address					*/
#define rcH_CHdInterval_H		(*(UBI9032_C_REG_ADD16(0x1CA)))	/* Host CHd Interval High						*/
#define rcH_CHdInterval_L		(*(UBI9032_C_REG_ADD16(0x1CA)))	/* Host CHd Interval Low						*/
#define rcH_CHdConditionCode	(*(UBI9032_C_REG_ADD16(0x1CE)))	/* Host CHd Condition Code						*/

//////////////////////////////////////////////////////////////////////////////////////////////////////
#define rcH_CHeConfig_0 		(*(UBI9032_C_REG_ADD16(0x1D0)))	/* Host CHe Configuration 0 					*/
#define rcH_CHeConfig_1 		(*(UBI9032_C_REG_ADD16(0x1D0)))	/* Host CHe COnfiguration 1 					*/
#define rcH_CHeMaxPktSize_H 	(*(UBI9032_C_REG_ADD16(0x1D2)))	/* Host CHe MaxPacketSize High					*/
#define rcH_CHeMaxPktSize_L 	(*(UBI9032_C_REG_ADD16(0x1D2)))	/* Host CHe MaxPacketSize Low					*/
#define rcH_CHeTotalSize_HH 	(*(UBI9032_C_REG_ADD16(0x1D4)))	/* Host CHe Total Size High 					*/
#define rcH_CHeTotalSize_HL 	(*(UBI9032_C_REG_ADD16(0x1D4)))	/* Host CHe Total Size Low						*/
#define rcH_CHeTotalSize_LH 	(*(UBI9032_C_REG_ADD16(0x1D6)))	/* Host CHe Total Size High 					*/
#define rcH_CHeTotalSize_LL 	(*(UBI9032_C_REG_ADD16(0x1D6)))	/* Host CHe Total Size Low						*/
#define rcH_CHeHubAdrs			(*(UBI9032_C_REG_ADD16(0x1D8)))	/* Host CHe Hub Address 						*/
#define rcH_CHeFuncAdrs 		(*(UBI9032_C_REG_ADD16(0x1D8)))	/* Host CHe Function Address					*/
#define rcH_CHeInterval_H		(*(UBI9032_C_REG_ADD16(0x1DA)))	/* Host CHe Interval High						*/
#define rcH_CHeInterval_L		(*(UBI9032_C_REG_ADD16(0x1DA)))	/* Host CHe Interval Low						*/
#define rcH_CHeConditionCode	(*(UBI9032_C_REG_ADD16(0x1DE)))	/* Host CHe Condition Code						*/

#endif	/* _U9032REG_H_ */


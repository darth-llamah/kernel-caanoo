/*
 * linux/drivers/video/pollux_fb.h
 * 	Copyright (c) MagicEyes co.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    POLLUX Frame Buffer Driver
 *
 * ChangeLog
 *
 * 2007-06-18: Jonathan <mesdigital.com
 *	- Adding operations for IOCTLs
 *
 * 2007-05-08: Rohan <mesdigital.com>
 *	- First version
 */

#ifndef __POLLUX_FB_H
#define __POLLUX_FB_H


#include <asm/arch/sys/sys.h>
#include "pollux_fb_ioctl.h"

#define	USE_FB_IOCTL 		1

/* Screen Info */
#define DISP_PRI_X_RESOL	DISPLAY_PRI_MAX_X_RESOLUTION
#define DISP_PRI_Y_RESOL	DISPLAY_PRI_MAX_Y_RESOLUTION

#define DISP_SEC_X_RESOL	DISPLAY_SEC_MAX_X_RESOLUTION
#define DISP_SEC_Y_RESOL	DISPLAY_SEC_MAX_Y_RESOLUTION

#define DISP_BPP			DISPLAY_MLC_BYTE_PER_PIXEL

/* FB Info */
#define POLLUX_FB_SIZE		( DISP_PRI_X_RESOL* DISP_PRI_Y_RESOL * DISP_BPP )

/* RGB Info */
#if (DISP_BPP == 2)
#define RGB_R_LEN		5
#define RGB_G_LEN		6
#define RGB_B_LEN		5
#else
#define RGB_R_LEN		8
#define RGB_G_LEN		8
#define RGB_B_LEN		8
#endif


#define BOARD_WIZ       0xAA
#define BOARD_F300      0xBB
#define BOARD_F300PLUS  0xCC
#define BOARD_N35  		0xCC

/*3D Flag (by bnjang 2009.09.22)*/
#define WIZ_3D          0x11
#define F300_3D         0x22
#define N35_3D         	0x22

/* For internal use (in the device driver) */
struct pollux_disp_info {

	/* Screen info */
	int		xres;
	int		yres;
	int		bpp;

	/* Primary display info */
	int		pri_xres;
	int		pri_yres;
	int		pri_bpp;

	/* Primary display info */
	int		sec_xres;
	int		sec_yres;
	int		sec_bpp;

	/* Layer info */
	int		cursor_layer;
	int		video_layer;
	int 	screen_layer;
	int		three_d_layer;

	/* Dual display status info */
	int		use_extend_display;
	int		secondary_video_on;
	int		primary_video_on;
};


#if ( USE_FB_IOCTL == 1 )
/* 
 * Structure for IOCTL
 */
struct  pollux_fb_ioctl_ops{
	int (*video_update)(struct fb_info *info, LPFB_VMEMINFO vmem, u32 dev);
	int (*setup_video)(struct fb_info *info, int width, int height, int left, int top, int right, int bottom, u32 tpcol, u32 fourcc, u32 flags, u32 position);
	int (*video_on)(struct fb_info *info, u32 dev);
	int (*video_off)(struct fb_info *info, u32 dev);
	int (*check_video_on)(struct fb_info *info, u32 viddev);
	int (*video_move)(struct fb_info *info, int sx, int sy, int ex, int ey, u32 dev);
	int (*video_scale)(struct fb_info *info, int srcw, int srch, int dstw, int dsth, u32 dev);
	int (*set_layer_priority)(u32 priority, u32 dev);
	int (*set_luminance_enhance)(int bright, int contrast, u32 dev);
	int (*set_chrominance_enhance)(int cba, int cbb, int cra, int crb, u32 dev);
	int (*set_layer_tpcolor)(u32 layer, u32 color, u32 on, u32 dev);
	int (*Set_layer_invcolor)(u32 layer, u32 color, u32 on, u32 dev);
	int (*set_layer_alpha_blend)(u32 layer, u32 degree, u32 on, u32 dev);
	int (*set_disp_device_enable)(int layer, u32 on, u32 dev);
	int (*dirtflag)(int layer, u32 dev, u32 enable, u32 set_flag);
	int (*setup_rgb_layer)(int layer, u32 base, int left, int top, int right, int bottom,
	                       u32 hStride, u32 vStride, int en3d, u32 backcol, u32 dev);
	int (*set_rgb_power)(int layer, u32 on, u32 dev);
	int (*lcd_change_set)(u32 cmd, u32 value);
	int (*run_idct) (U16 *InputData, U16 *QuantMatrix, U16 *OutputData );
	int (*tv_configuration) (u8 command, u8 mode, u32 width, u32 Height);
	u16 (*R8G8B8_to_R5G6B5)(u32 rgb);
	u32 (*R5G6B5_to_R8G8B8)(u16 rgb);
	/* FIXME: lars - below function is only to change tvout bits per pixel rate */
	int (*set_bpp)(u32 value);		// 8, 16, 24... 
};
#endif /* USE_FB_IOCTL */

struct pollux_fb_info {
	struct fb_info		*fb;
	struct device		*dev;
	struct clk		*clk;

	struct pollux_disp_info disp_info;

	/* raw memory addresses */
	u_int			map_phys;	/* physical */
	u_char *		map_virt;	/* virtual  */
	u_int			map_size;

	/* addresses of pieces placed in raw buffer */
	u_char *		screen_virt;	/* virtual address of buffer */
	u_int			screen_phys;	/* physical address of buffer */
	unsigned int	palette_ready;

	/* keep these registers in case we need to re-write palette */
	u32			palette_buffer[256];
	u32			pseudo_pal[16];
};


#define PALETTE_BUFF_CLEAR (0x80000000)	/* entry is clear/invalid */

/* Debugging stuff */
#ifdef  CONFIG_FB_POLLUX_DEBUG
#define DPRINTK(msg...)	{ printk(KERN_INFO "pollux_fb: " msg); }
#define	EPRINTK(msg...)	{ printk(KERN_ERR  "pollux_fb: " msg); }
#else
#define DPRINTK(msg...)		do {} while (0)	
#define	EPRINTK(msg...)		do {} while (0)	
#endif

#endif	/* __S3C2410FB_H */


/*
 * linux/drivers/video/pollux_fb.c
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
 * 2007-06-20: Jonathan <mesdigital.com>
 * 	- Adding IOCTL
 * 2007-05-08: Rohan <mesdigital.com>
 *	- First version
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>


#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#ifdef CONFIG_PM
#undef CONFIG_PM
#endif

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

//#include <module/mes_mlc.h>

#include <asm/arch/aesop-pollux.h> 
#include <asm/arch/map.h>

#include "pollux_fb.h"

extern int pollux_ioctl_init(struct pollux_fb_ioctl_ops *ioctl_ops);

static struct pollux_disp_info *disp_info;
static u32    pollux_fb_vaddr = 0;
static u32    pollux_fb_paddr = 0;
static int nocursor = 0;

static unsigned char board_num = BOARD_N35;
static unsigned char num_3d = N35_3D;

/* useful functions: Implement */
static void pollux_fb_set_base(u32 phys)
{	
	// hw register base address. 
	/* HYUN_DEBUG */
	u32 *fb_base  = (u32 *)(POLLUX_VA_MLC_PRI + 0x6C);	
	u32 *fb_dirty = (u32 *)(POLLUX_VA_MLC_PRI + 0x58);
	u32  tmp = *fb_dirty;

	// set mlc base address register.
	*fb_base = phys;

	// set mlc dirty flag register.
	*fb_dirty = tmp | 0x00000010;

	msleep(10);
}

/*
 *	pollux_fb_check_var():
 *	Get the video params out of 'var'. If a value doesn't fit, round it up,
 *	if it's too big, return -EINVAL.
 *
 */
static int pollux_fb_check_var(struct fb_var_screeninfo *var,
			       struct fb_info *info)
{
#ifdef  CONFIG_FB_POLLUX_DEBUG
	struct pollux_fb_info *fbi = info->par;
	DPRINTK("pollux_fb_check_var(fbi=%p, var=%p, info=%p)\n", fbi, var, info);
#endif

	/* set r/g/b positions */
	switch (var->bits_per_pixel) {
		case 1:
		case 2:
		case 4:
			var->red.offset    	= 0;
			var->red.length    	= var->bits_per_pixel;
			var->green         	= var->red;
			var->blue          	= var->red;
			var->transp.offset 	= 0;
			var->transp.length 	= 0;
			break;

		case 8:
			var->red.offset    	= 0;
			var->red.length    	= var->bits_per_pixel;
			var->green         	= var->red;
			var->blue          	= var->red;
			var->transp.offset 	= 0;
			var->transp.length 	= 0;
			break;

		case 16:
			/* 16 bpp, 565 format */
			var->red.length		= 5;
			var->red.offset		= 11;
			var->green.length	= 6;
			var->green.offset	= 5;
			var->blue.length	= 5;
			var->blue.offset	= 0;
			var->transp.length	= 0;
			break;

		case 24:
			/* 24 bpp 888 */
			var->red.length		= 8;
			var->red.offset		= 16;
			var->green.length	= 8;
			var->green.offset	= 8;
			var->blue.length	= 8;
			var->blue.offset	= 0;
			var->transp.length	= 0;
			break;


		case 32: // add for sdl (rohan)
			/* 32 bpp 888 */
			var->red.length		= 10;
			var->red.offset		= 16;
			var->green.length	= 10;
			var->green.offset	= 8;
			var->blue.length	= 10;
			var->blue.offset	= 0;
			var->transp.length	= 0;
			break;

		default:
			printk(KERN_ERR"pollux_fb_check_var: current not support pit per pixel =%d\n", 
				var->bits_per_pixel);

	}
	return 0;
}


/* pollux_fb_activate_var
 *
 * activate (set) the controller from the given framebuffer
 * information
*/

static void pollux_fb_activate_var(struct pollux_fb_info *fbi,
				   struct fb_var_screeninfo *var)
{
	DPRINTK("%s: var->xres  	= %d\n", __FUNCTION__, var->xres);
	DPRINTK("%s: var->yres  	= %d\n", __FUNCTION__, var->yres);
	DPRINTK("%s: var->xres_virtual	= %d\n", __FUNCTION__, var->xres_virtual);
	DPRINTK("%s: var->yres_virtual  = %d\n", __FUNCTION__, var->yres_virtual);
	DPRINTK("%s: var->xoffset       = %d\n", __FUNCTION__, var->xoffset);
	DPRINTK("%s: var->yoffset       = %d\n", __FUNCTION__, var->yoffset);
	DPRINTK("%s: var->bits_per_pixel= %d\n", __FUNCTION__, var->bits_per_pixel);
	DPRINTK("%s: var->grayscale     = %d\n", __FUNCTION__, var->grayscale);
	DPRINTK("%s: var->nonstd        = %d\n", __FUNCTION__, var->nonstd);
	DPRINTK("%s: var->activate      = %d\n", __FUNCTION__, var->activate);
	DPRINTK("%s: var->height        = %d\n", __FUNCTION__, var->height);
	DPRINTK("%s: var->width         = %d\n", __FUNCTION__, var->width);
	DPRINTK("%s: var->accel_flags   = %d\n", __FUNCTION__, var->accel_flags);
	DPRINTK("%s: var->pixclock      = %d\n", __FUNCTION__, var->pixclock);
	DPRINTK("%s: var->left_margin   = %d\n", __FUNCTION__, var->left_margin);
	DPRINTK("%s: var->right_margin  = %d\n", __FUNCTION__, var->right_margin);
	DPRINTK("%s: var->upper_margin  = %d\n", __FUNCTION__, var->upper_margin);
	DPRINTK("%s: var->lower_margin  = %d\n", __FUNCTION__, var->lower_margin);
	DPRINTK("%s: var->hsync_len     = %d\n", __FUNCTION__, var->hsync_len);
	DPRINTK("%s: var->vsync_len     = %d\n", __FUNCTION__, var->vsync_len);
	DPRINTK("%s: var->sync          = %d\n", __FUNCTION__, var->sync);
	DPRINTK("%s: var->vmode         = %d\n", __FUNCTION__, var->vmode);
	DPRINTK("%s: var->rotate        = %d\n", __FUNCTION__, var->rotate);

	DPRINTK("%s: var->red.offset    	= %d\n", __FUNCTION__, var->red.offset);
	DPRINTK("%s: var->red.length  		= %d\n", __FUNCTION__, var->red.length);
	DPRINTK("%s: var->red.msb_right 	= %d\n", __FUNCTION__, var->red.msb_right);
	DPRINTK("%s: var->green.offset    	= %d\n", __FUNCTION__, var->green.offset);
	DPRINTK("%s: var->green.length  	= %d\n", __FUNCTION__, var->green.length);
	DPRINTK("%s: var->green.msb_right 	= %d\n", __FUNCTION__, var->green.msb_right);
	DPRINTK("%s: var->blue.offset    	= %d\n", __FUNCTION__, var->blue.offset);
	DPRINTK("%s: var->blue.length  		= %d\n", __FUNCTION__, var->blue.length);
	DPRINTK("%s: var->blue.msb_right 	= %d\n", __FUNCTION__, var->blue.msb_right);
	DPRINTK("%s: var->transp.offset    	= %d\n", __FUNCTION__, var->transp.offset);   
	DPRINTK("%s: var->transp.length  	= %d\n", __FUNCTION__, var->transp.length);   
	DPRINTK("%s: var->transp.msb_right 	= %d\n", __FUNCTION__, var->transp.msb_right);
}


/*
 *      pollux_fb_set_par - Optional function. Alters the hardware state.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 */
static int pollux_fb_set_par(struct fb_info *info)
{
	struct pollux_fb_info *fbi;
	struct fb_var_screeninfo *var;

	fbi = info->par;
	var = &info->var;

	DPRINTK("pollux_fb_set_par\n");

	switch (var->bits_per_pixel)
	{
		case 24:
		case 16:
			fbi->fb->fix.visual = FB_VISUAL_TRUECOLOR;
			break;
		case 1:
			 fbi->fb->fix.visual = FB_VISUAL_MONO01;
			 break;
		default:
			 fbi->fb->fix.visual = FB_VISUAL_PSEUDOCOLOR;
			 break;
	}

	fbi->fb->fix.line_length     = (var->width*var->bits_per_pixel)/8;

	/* activate this new configuration */
	pollux_fb_activate_var(fbi, var);

	return 0;
}

static void schedule_palette_update(struct pollux_fb_info *fbi,
				    unsigned int regno, unsigned int val)
{
	unsigned long flags;

	local_irq_save(flags);

	fbi->palette_buffer[regno] = val;

	local_irq_restore(flags);
}

/* from pxafb.c */
static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int pollux_fb_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	struct pollux_fb_info *fbi = info->par;
	unsigned int val;

	DPRINTK("pollux_fb_setcolreg\n");
	DPRINTK("setcol: regno=%d, rgb=%d,%d,%d\n", regno, red, green, blue);

	switch (fbi->fb->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/* true-colour, use pseuo-palette */

		if (regno < 16) {
			u32 *pal = fbi->fb->pseudo_palette;

			val  = chan_to_field(red,   &fbi->fb->var.red);
			val |= chan_to_field(green, &fbi->fb->var.green);
			val |= chan_to_field(blue,  &fbi->fb->var.blue);

			pal[regno] = val;
		}
		break;

	case FB_VISUAL_PSEUDOCOLOR:
		if (regno < 256) {
			/* currently assume RGB 5-6-5 mode */

			val  = ((red   >>  0) & 0xf800);
			val |= ((green >>  5) & 0x07e0);
			val |= ((blue  >> 11) & 0x001f);

			schedule_palette_update(fbi, regno, val);
		}

		break;

	default:
		DPRINTK("pollux_fb_setcolreg return unknown type\n");
		return 1;   /* unknown type */
	}

	return 0;
}


int pollux_fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    if(nocursor)
        return 0;
    else
        return -EINVAL;
}

/**
 *      pollux_fb_blank
 *	@blank_mode: the blank mode we want.
 *	@info: frame buffer structure that represents a single frame buffer
 *
 *	Blank the screen if blank_mode != 0, else unblank. Return 0 if
 *	blanking succeeded, != 0 if un-/blanking failed due to e.g. a
 *	video mode which doesn't support it. Implements VESA suspend
 *	and powerdown modes on hardware that supports disabling hsync/vsync:
 *	blank_mode == 2: suspend vsync
 *	blank_mode == 3: suspend hsync
 *	blank_mode == 4: powerdown
 *
 *	Returns negative errno on error, or zero on success.
 *
 */
static int pollux_fb_blank(int blank_mode, struct fb_info *info)
{
	DPRINTK("pollux_fb_blank(mode=%d, info=%p)\n", blank_mode, info);

	return 0;
}

#ifdef  CONFIG_FB_POLLUX_DEBUG
static int    debug = 1;
#else
static int    debug = 0;
#endif

static int pollux_fb_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", debug ? "on" : "off");
}

static int pollux_fb_debug_store(struct device *dev, struct device_attribute *attr,
					   const char *buf, size_t len)
{
	if (disp_info == NULL)
		return -EINVAL;

	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 ||
	    strnicmp(buf, "1", 1) == 0) {
		debug = 1;
		printk(KERN_DEBUG "pollux_fb: Debug On");
	} else if (strnicmp(buf, "off", 3) == 0 ||
		   strnicmp(buf, "0", 1) == 0) {
		debug = 0;
		printk(KERN_DEBUG "pollux_fb: Debug Off");
	} else {
		return -EINVAL;
	}

	return len;
}


static int pollux_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{

	DPRINTK("pollux_fb_mmap - user reserved memory\n");
	DPRINTK("request start:0x%08x, end:0x%08x, off:0x%08x, len:%d\n",
		(u_int)vma->vm_start, (u_int)vma->vm_end, (u_int)vma->vm_pgoff, (u_int)(vma->vm_end - vma->vm_start) );

	vma->vm_pgoff = (pollux_fb_paddr >> PAGE_SHIFT);
	vma->vm_end = vma->vm_start + PAGE_ALIGN(POLLUX_FB_SIZE + PAGE_SIZE);

	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	remap_pfn_range(vma, 
			vma->vm_start, 		// vm start			
			vma->vm_pgoff,		// physical
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot);

	DPRINTK("remaped start:0x%08x, end:0x%08x, off:0x%08x, len:%d\n",
		(u_int)vma->vm_start, (u_int)vma->vm_end, (u_int)vma->vm_pgoff, (int)(vma->vm_end - vma->vm_start) );

	return 0;

}


#if ( USE_FB_IOCTL == 1 )

static struct pollux_fb_ioctl_ops ioctl_ops;

/*
 * pollux_ioctl
 *
 * Driver specific IOCTL functions
 */
static int pollux_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int result = 0;
	int size = 0;
	void *pdata = NULL;
	struct pollux_fb_info *fbi;

	/* Check if IOCTL code is valid */
	if(_IOC_TYPE(cmd) != FBIO_MAGIC)
		return -EINVAL;
	
	/* Allocate argument buffer for READ/WRITE ioctl commands */
	if(_IOC_DIR(cmd) != _IOC_NONE)
	{		
		size = _IOC_SIZE(cmd);
		pdata = vmalloc(size);
	}
	
	/* Get base address of the device driver specific information */
	fbi = info->par;

	/* To do: Where can I get the color depth? */
	/* To do: Missing color key variable */

	switch (cmd)
	{
	case FBIO_INFO:
		if(pdata)
		{
			LPFB_INFO pOut = pdata;
			pOut->ScreenWidth   = info->var.width;			
			pOut->ScreenHeight  = info->var.height;
			pOut->Frequency     = 0;	/* Not prepared yet */
			pOut->ColorDepth    = info->var.bits_per_pixel;
			pOut->ColorKey      = 0;	/* Not prepared yet */
			
			// set information for 3D.
			pOut->CHIPID   	    = MES_CHIPID_POLLUX;	
			pOut->FrameMemPBase = fbi->screen_phys;
			pOut->FrameMemVBase = (u32)fbi->screen_virt;
			
			pOut->FrameMemSize  = (POLLUX_FB_MAP_SIZE >> POLLUX_MPEG2D3D_BANK_SHIFT);
			pOut->BlockMemPBase = POLLUX_3D_PIO_BASE;
			pOut->BlockMemVBase = 0;	/* Not prepared yet */
			pOut->BlockMemSize  = (POLLUX_3D_MAP_SIZE >> POLLUX_MPEG2D3D_BANK_SHIFT);
			pOut->BlockMemStride = POLLUX_MPEG2D3D_STRIDE;
			// set information for dual display.
			pOut->IsDualDisplay = CFALSE;

            /* Copy IOCTL data to userspace */
			if(copy_to_user((void *)arg, (const void *)pdata, size))
			{
				result = -EFAULT;
				break;
			}
		}
		else
			result = -EFAULT;
		break;
	case FBIO_VIDEO_INIT:
		if(pdata)
		{
			u32 	   flags;
			u32 	  fourcc;
			u32 	   tpcol;
			int 	    srcw;
			int 	    srch;
			int 	    dstw;
			int 	    dsth;
			int 	    left;
			int 	     top;
			int 	   right;
			int 	  bottom;
			
			u32 position;

			LPFB_VIDEO_CONF vconf;

			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			vconf = (LPFB_VIDEO_CONF)pdata;

			flags = vconf->Flags;				
			fourcc = vconf->FourCC;
			tpcol = vconf->ColorKey;
			srcw = vconf->SrcWidth;
			srch = vconf->SrcHeight;
			dstw = vconf->DstWidth;
			dsth = vconf->DstHeight;				
			left = vconf->Left;		
			top = vconf->Top;
			right = vconf->Right;		
			bottom = vconf->Bottom;

			position = vconf->VideoDev;
			if(flags & SET_TPCOLOR)	
			{
				if(info->var.bits_per_pixel == 16)
					tpcol = ioctl_ops.R5G6B5_to_R8G8B8((U16)tpcol);
//				m_ColorKey = tpcol;	// Not completed yet: A status variable is needed
			} 
			else 
			{
//				tpcol = m_ColorKey; // A status variable is needed
			}
			
			result = ioctl_ops.setup_video(info, srcw, srch, left, top, right, bottom, tpcol, fourcc, flags, position);	
		}
		else
			result = -EFAULT;
		break;

	case FBIO_VIDEO_START:
		if(pdata)
		{
			/*
			 * Following data should be given to run video_update()
			 *
			 * 	LuStride
			 * 	LuOffset
			 * 	CbStride
			 * 	CbOffset
			 * 	CrStride
			 * 	CrOffset
			 */
			u32 dev;
			LPFB_VMEMINFO vmem;

			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			vmem = (LPFB_VMEMINFO)pdata;
			dev = vmem->VideoDev;

			result = ioctl_ops.video_update(info, vmem, dev);
			result = ioctl_ops.video_on(info, dev);
		}
		else
			result = -EFAULT;
		break;

	/* Update position, size */
	case FBIO_VIDEO_UPDATE:
		if(pdata)
		{
			LPFB_VIDEO_CONF vconf;
			int 	srcw;
			int 	srch;
			int 	dstw;
			int 	dsth;
			int 	left;
			int 	top;
			int 	right;
			int 	bottom;
			u32		dev;

			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			vconf = (LPFB_VIDEO_CONF)pdata;		

			srcw  = vconf->SrcWidth;
			srch  = vconf->SrcHeight;
			dstw  = vconf->DstWidth;
			dsth  = vconf->DstHeight;				
			left  = vconf->Left;		
			top   = vconf->Top;
			right = vconf->Right;		
			bottom= vconf->Bottom;
			
			dev = vconf->VideoDev;
			
			result = ioctl_ops.video_move (info, left, top, right, bottom, dev);
			result = ioctl_ops.video_scale (info, srcw, srch, dstw, dsth, dev); 
		}
		else
			result = -EFAULT;
		break;

	case FBIO_VIDEO_STOP:
		if(pdata)
		{
			u32 mlc_dev;	/* primary or secondary */
	
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			mlc_dev = *(u32 *)pdata;

			result = ioctl_ops.video_off(info, mlc_dev);
		}
		else
			result = -EFAULT;
		break;                                        
			                            
	case FBIO_VIDEO_MEMORY_UPDATE:
 		if(pdata)
		{
			/*
			 * Following data should be given to run video_update()
			 *
			 * 	LuStride
			 * 	LuOffset
			 * 	CbStride
			 * 	CbOffset
			 * 	CrStride
			 * 	CrOffset
			 */
			u32 dev;
			LPFB_VMEMINFO vmem;

			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			vmem = (LPFB_VMEMINFO)pdata;
			dev = vmem->VideoDev;

			result = ioctl_ops.video_update(info, vmem, dev);
		}
		else
			result = -EFAULT;
		break;
                                       
	case FBIO_VIDEO_PRIORITY:
		if(pdata)
		{
			u32 priority;		/* new priority value */
			u32 mlc_dev;	/* primary or secondary */
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			priority = *(u32 *)pdata;
			mlc_dev = *((u32 *)pdata + 1);
	
			result = ioctl_ops.set_layer_priority(priority, mlc_dev);
		}
		else
			result = -EFAULT;
		break;                                        

	case FBIO_VIDEO_LUMINAENHANCE:
		if(pdata)
		{
			int brightness;
			int contrast;
			u32 mlc_dev;

			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			brightness = *(int *)pdata;
			contrast = *((int *)pdata + 1);
			mlc_dev = *((u32 *)pdata + 2);
			
			result = ioctl_ops.set_luminance_enhance(brightness, contrast, mlc_dev);
		}
		else
			result = -EFAULT;
		break;

	case FBIO_VIDEO_CHROMAENHANCE:
		if(pdata)
		{
			int cba;
			int cbb;
			int cra;
			int crb;
			u32 mlc_dev;
		
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			cba = *(int *)pdata;
			cbb = *((int *)pdata + 1);
			cra = *((int *)pdata + 2);
			crb = *((int *)pdata + 3);
			mlc_dev = *((u32 *)pdata + 4);
			
			result = ioctl_ops.set_chrominance_enhance(cba, cbb, cra, crb, mlc_dev);
		}
		else
			result = -EFAULT;
		break;

	case FBIO_LAYER_TPCOLOR:
		if(pdata)
		{
			u32 layer;
			u32 colkey;
			u32 on;
			u32 mlc_dev;
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			layer = *(u32 *)pdata;
			colkey = *((u32 *)pdata + 1);
			on = *((u32 *)pdata + 2);
			mlc_dev = *((u32 *)pdata + 3);

			if(1)	/* To do: you must substitute with a color depth variable */
			{
				colkey = ioctl_ops.R8G8B8_to_R5G6B5((U32)colkey);
				colkey = ioctl_ops.R5G6B5_to_R8G8B8((U16)colkey);
			}
			if(layer == LAYER_DISPLAY_SCREEN_RGB)	
			{
			/*	m_ColorKey = colkey; // To do: you must substitute with a color key variable */
			}

			result = ioctl_ops.set_layer_tpcolor(layer, colkey, on, mlc_dev);
		}
		else
			result = -EFAULT;
		break;

	case FBIO_LAYER_INVCOLOR:		
		if(pdata)
		{
			u32 layer;
			u32 colkey;
			u32 on;
			u32 mlc_dev;

			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			layer  = *(u32 *)pdata;
			colkey = *((u32 *)pdata + 1);
			on     = *((u32 *)pdata + 2);
			mlc_dev    = *((u32 *)pdata + 3);

			if(1)	/* To do: you must substitute with a color depth variable */
			{
				colkey = ioctl_ops.R8G8B8_to_R5G6B5((U32)colkey);
				colkey = ioctl_ops.R5G6B5_to_R8G8B8((U16)colkey);
			}	
						
			result = ioctl_ops.Set_layer_invcolor(layer, colkey, on, mlc_dev);
		}
		else
			result = -EFAULT;
		break;

	case FBIO_LAYER_ALPHABLD:	
		if(pdata)
		{
			u32	layer;
			u32	value;
			u32	on;
			u32	mlc_dev;
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			layer 	= *(u32 *)pdata;      
			value 	= *((u32 *)pdata + 1);
			on    	= *((u32 *)pdata + 2);
			mlc_dev	= *((u32 *)pdata + 3);

			result = ioctl_ops.set_layer_alpha_blend(layer, value, on, mlc_dev);
		}
		else
			result = -EFAULT;
		break;

	case FBIO_DEVICE_ENABLE:	
		if(pdata)
		{
			u32 	layer;
			u32 	enable;
			u32 	mlc_dev;
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			layer	= *(u32 *)pdata; 
			enable	= *((u32 *)pdata + 1);
			mlc_dev	= *((u32 *)pdata + 2);

			result = ioctl_ops.set_disp_device_enable(layer, enable, mlc_dev);
		}
		else
			result = -EFAULT;
		break;
	case FBIO_DIRTFLAG :
		if(pdata)
		{
			u32 	layer;
			u32 	enable;
			u32 	mlc_dev;
			u32 	dir;
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			
			layer	= *(u32 *)pdata; 
			enable	= *((u32 *)pdata + 1);
			mlc_dev	= *((u32 *)pdata + 2);
			dir		= *((u32 *)pdata + 3);
			
			if(dir == 0 ) { //get
				*((u32 *)pdata + 3) = ioctl_ops.dirtflag(layer, mlc_dev, enable, dir);
				if(copy_to_user((void *)arg, (const void *)pdata, size))
	            	return -EFAULT;
			}else {  //set
				ioctl_ops.dirtflag(layer, mlc_dev, enable, dir);	
			}
			
			result =0;
		
		}
		else
			result = -EFAULT;
		break;
	case  RGB_LAYER_POWER:
		if(pdata)
		{
			u32 	layer;
			u32 	enable;
			u32 	mlc_dev;
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			layer	= *(u32 *)pdata; 
			enable	= *((u32 *)pdata + 1);
			mlc_dev	= *((u32 *)pdata + 2);

			result = ioctl_ops.set_rgb_power(layer, enable, mlc_dev);
		}
		else
			result = -EFAULT;
		break;
	case FBIO_IDCT_RUN:
	    if(pdata)
		{
			u16 	*InputData;
			u16 	*QuantMatrix;
			u16 	*OutputData;
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			
			InputData	= (u16 *)pdata; 
			QuantMatrix	= (u16 *)(pdata + 1);
			OutputData	= (u16 *)(pdata + 2);
            
            result = ioctl_ops.run_idct(InputData, QuantMatrix, OutputData);
		}
		else
			result = -EFAULT;
	    break;	
	case FBIO_RGB_CONTROL:
		if(pdata)
		{
			LPFB_RGBSET pRgbSet;

			u32	layer;
			u32	addrs;
			int	left;
			int	top;
			int	right;
			int	bottom;
			u32	hStride;	
	        u32	vStride;
			int en3D;
			u32	bakcol;
			u32	mlc_dev;
			
			/* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
			
#if 0			
			layer   = *(u32 *)pdata;
			addrs   = *((u32 *)pdata + 1);
			left    = *((u32 *)pdata + 2);
			top     = *((u32 *)pdata + 3);
			right   = *((u32 *)pdata + 4);
			bottom  = *((u32 *)pdata + 5);
			bakcol  = *((u32 *)pdata + 6);
			mlc_dev = *((u32 *)pdata + 7);
#else

			pRgbSet = (LPFB_RGBSET)pdata;
			
			layer   = pRgbSet->Layer;
			addrs   = fbi->screen_phys; 	//pRgbSet->Addrs;
			
		    left    = pRgbSet->Left;
			top     = pRgbSet->Top;
			right   = pRgbSet->Right;
			bottom  = pRgbSet->Bottom;
			hStride = pRgbSet->HStride;
			vStride = pRgbSet->VStride;
			en3D    = pRgbSet->enable3D;
			bakcol  = pRgbSet->Bakcol;
			mlc_dev = pRgbSet->Mlc_dev;
            
#endif
			
			result = ioctl_ops.setup_rgb_layer(layer, addrs, left, top, right, bottom,
			                                   hStride, vStride, en3D, bakcol, mlc_dev);
		}
		else
			result = -EFAULT;
		break;
	case FBIO_SET_TVCFG_MODE:
	    if(pdata)
		{
	        LPFB_TVCONF tv_config;
	        
	        u8 command;
	        u8 mode;
	        u32 width;
	        u32 Height; 
	        /* Copy IOCTL data from userspace */
			if(copy_from_user((void *)pdata, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
	        tv_config = (LPFB_TVCONF)pdata;
            
            command = tv_config->command;
            mode    = tv_config->tv_mode;
            width   = tv_config->SecScreenWidth;
            Height  = tv_config->SecScreenHeight;
            
            result = ioctl_ops.tv_configuration(command, mode, width, Height);
	    }
	    else
			result = -EFAULT;
		break;
	case FBIO_LCD_CHANGE_CONTROL:
	    if(pdata)
	    {
	        u32 cmd;
	        u32 value;
	        if(copy_from_user((void *)pdata, (const void *)arg, size))
	        {
	            result = -EFAULT;
				break;
	        }
	        
	        cmd	= *(u32 *)pdata; 
	        value	= *((u32 *)pdata + 1);
	        
	        result = ioctl_ops.lcd_change_set(cmd, value);

        }else
	        result = -EFAULT;
	    break;    
	case FBIO_GET_BOARD_NUMBER:
	    {
	        if( copy_to_user((unsigned char*)arg, &board_num, sizeof(unsigned char)) )
	            return -EFAULT;
	        result = 0;
	    }
	    break;
    case FBIO_SET_BOARD_NUMBER:
        {
            if(copy_from_user((void *)pdata, (const void *)arg, size))
                return -EFAULT;
            board_num = *(unsigned char *)pdata;
            result = 0;
        }
        break;   
    case FBIO_GET_3D_NUMBER:
        {
			if( copy_to_user((unsigned char*)arg, &num_3d, sizeof(unsigned char)) )
				return -EFAULT;
			result = 0; 
		}
        break;
	case FBIO_SET_BPP:
	    if(pdata)
		{
			int value;
	        /* Copy IOCTL data from userspace */
			if(copy_from_user((void *)&value, (const void *)arg, size))
			{
				result = -EFAULT;
				break;
			}
            result = ioctl_ops.set_bpp(value);
	    }
	    else
			result = -EFAULT;
    
    default:
		result = -EINVAL;
	}
	
	if(pdata != NULL)
		vfree(pdata);
	return result;
}

#endif /* USE_FB_IOCTL */

static DEVICE_ATTR(debug, 0666,
		   pollux_fb_debug_show,
		   pollux_fb_debug_store);

static struct fb_ops pollux_fb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= pollux_fb_check_var,
	.fb_set_par	= pollux_fb_set_par,
	.fb_blank	= pollux_fb_blank,
	.fb_setcolreg	= pollux_fb_setcolreg,
	.fb_mmap	= pollux_fb_mmap,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
    .fb_cursor      = pollux_fb_cursor,
#if ( USE_FB_IOCTL == 1)
	.fb_ioctl	= pollux_ioctl,
#endif
};


/*
 * pollux_fb_map_video_memory():
 *	Allocates the DRAM memory for the frame buffer.  This buffer is
 *	remapped into a non-cached, non-buffered, memory region to
 *	allow palette and pixel writes to occur without flushing the
 *	cache.  Once this area is remapped, all virtual memory
 *	access to the video memory should occur at the new region.
 */

static int __init pollux_fb_map_video_memory(struct pollux_fb_info *fbi)
{
	
	fbi->map_size = PAGE_ALIGN(fbi->fb->fix.smem_len + PAGE_SIZE);

	/* allocate from io memory */
	fbi->map_phys = POLLUX_FB_PIO_BASE;
	fbi->map_virt = POLLUX_FB_VIO_BASE;

	fbi->map_size = fbi->fb->fix.smem_len;

	if (fbi->map_virt) {
		/* prevent initial garbage on screen */
		DPRINTK("map_video_memory: virt=0x%08x:%d clear\n", (u_int)fbi->map_virt, fbi->map_size);

		//memset(fbi->map_virt, 0xf0, fbi->map_size);

		fbi->screen_phys	= fbi->map_phys;
		fbi->fb->screen_base	= fbi->map_virt;
		fbi->fb->fix.smem_start = fbi->screen_phys;

		DPRINTK("map_video_memory: phys=%08x virt=0x%08x size=%d\n",
			(u_int)fbi->map_phys, (u_int)fbi->map_virt, fbi->fb->fix.smem_len);

		pollux_fb_vaddr = (u32)fbi->map_virt;
		pollux_fb_paddr = (u32)fbi->map_phys;

	}
	
	/* HYUN_DEBUG */
	pollux_fb_set_base(fbi->screen_phys);

	return fbi->map_virt ? 0 : -ENOMEM;
}

static inline void pollux_fb_unmap_video_memory(struct pollux_fb_info *fbi)
{
	DPRINTK("pollux_fb_unmap_video_memory \n");
	iounmap(fbi->map_virt);
}

/*
 * pollux_fb_init_registers - Initialise all LCD-related registers
 */

static int pollux_fb_init_registers(struct pollux_fb_info *fbi)
{
	return 0;
}


// added by smhong for export tvout status to userspace(sysfs) 
extern int pollux_tv_display_status(void);
static ssize_t tvout_status_show(struct device * dev, struct device_attribute * attr, char * buf) 
{
	return sprintf(buf, "%d\n", pollux_tv_display_status());
}
static DEVICE_ATTR(tvout, S_IRUGO, tvout_status_show, NULL);


static int __init pollux_fb_probe(struct platform_device *pdev)
{
	struct pollux_fb_info *mfbinf;
	struct fb_info	       *fbinfo;
	int    ret;
	int    i;

	
	if(pdev->name)
		DPRINTK("pollux_fb_probe: %s\n", pdev->name);

	/* 
 	 * get memory for fb_info struct region.
 	 * link fbinfo->dev = &pdev->dev.
	 * (drivers/video/fbsysfs.c) 
	 */
	fbinfo = framebuffer_alloc(sizeof(struct pollux_fb_info), &pdev->dev);
	if (!fbinfo) {
		return -ENOMEM;
	}

	mfbinf = fbinfo->par;

	/* link fbinfo to machine fb info's fb */
	mfbinf->fb = fbinfo;

	/* pollux_fb_device: pdev->dev->driver_data = fbinfo */
	platform_set_drvdata(pdev, fbinfo);

	/*set name filed in fb_fix_screeninfo fix */
	strcpy(fbinfo->fix.id,  pdev->name);


	/*
 	 *  pdev->dev.platform_data : pollux_fb_device->dev.platform_data is not set,
 	 *  use platform_device_add_data to set pdev->dev->platform_data.  
 	 */	

	/* fb_fix_screeninfo */
	fbinfo->fix.type	    = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux	= 0;
	fbinfo->fix.xpanstep	= 0;
	fbinfo->fix.ypanstep	= 0;
	fbinfo->fix.ywrapstep	= 0;
	fbinfo->fix.accel	    = FB_ACCEL_NONE;

	/* fb_var_screeninfo */
	fbinfo->var.nonstd	    	= 0;
	fbinfo->var.activate	    = FB_ACTIVATE_NOW;
	fbinfo->var.accel_flags     = 0;
	fbinfo->var.vmode	    	= FB_VMODE_NONINTERLACED;

	/* modify point. set platform_data rohan */
	fbinfo->var.height	    	= DISP_PRI_Y_RESOL;
	fbinfo->var.width	    	= DISP_PRI_X_RESOL;

	fbinfo->var.xres	    	= DISP_PRI_X_RESOL;
	fbinfo->var.xres_virtual    = DISP_PRI_X_RESOL;
	fbinfo->var.yres	    	= DISP_PRI_Y_RESOL;
	fbinfo->var.yres_virtual    = DISP_PRI_Y_RESOL;
	fbinfo->var.bits_per_pixel  = DISP_BPP*8;
	fbinfo->fix.smem_len        = POLLUX_FB_SIZE;

	DPRINTK("xres:%d, yres:%d, bpp;%d, mem:%d\n",
		fbinfo->var.xres, fbinfo->var.yres, fbinfo->var.bits_per_pixel, fbinfo->fix.smem_len);

	/* RGB format: RGB565, RGB888 */	
	fbinfo->var.red.offset      = RGB_G_LEN + RGB_B_LEN;
	fbinfo->var.green.offset    = RGB_B_LEN;
	fbinfo->var.blue.offset     = 0;

	fbinfo->var.transp.offset   = 0;
	fbinfo->var.red.length      = RGB_R_LEN;
	fbinfo->var.green.length    = RGB_G_LEN;
	fbinfo->var.blue.length     = RGB_B_LEN;
	fbinfo->var.transp.length   = 0;

	/* link machind file operation to fb_info's file operation */
	fbinfo->fbops		        = &pollux_fb_ops;

	fbinfo->flags		    	= FBINFO_FLAG_DEFAULT;
	fbinfo->pseudo_palette  	= &mfbinf->pseudo_pal;

	/* clear palette_buffer, RGB565, modify point RGB888 support rohan */
	for (i = 0; i < 256; i++)
		mfbinf->palette_buffer[i] = PALETTE_BUFF_CLEAR;

	/* Initialize the global status value */
	mfbinf->disp_info.video_layer        = 2;	/* This value is fixed */
	mfbinf->disp_info.cursor_layer       = LAYER_DISPLAY_CURSOR_RGB;
	mfbinf->disp_info.screen_layer       = LAYER_DISPLAY_SCREEN_RGB;
	mfbinf->disp_info.three_d_layer      = LAYER_DISPLAY_3DIMAGE_RGB;
	mfbinf->disp_info.use_extend_display = CFALSE;
	mfbinf->disp_info.secondary_video_on = CFALSE;
	mfbinf->disp_info.primary_video_on   = CFALSE;

	/* allocate frame buffer memory */
	ret = pollux_fb_map_video_memory(mfbinf);
	if (ret) {
		EPRINTK("Failed to allocate Frame Buffer memory: %d\n", ret);
		ret = -ENOMEM;
		goto dealloc_fb;
	}
	DPRINTK("memory: phys=0x%08x virt=0x%08x size:%d\n",mfbinf->map_phys, (u_int)mfbinf->map_virt, mfbinf->map_size);


#if ( USE_FB_IOCTL == 1)
	/* Initialize IOCTL operations */
	ret = pollux_ioctl_init(&ioctl_ops);
#endif

	/* initialize pollux display hw */ 
	ret = pollux_fb_init_registers(mfbinf);

	ret = pollux_fb_check_var(&fbinfo->var, fbinfo);

	/* 
 	 * device_create '/proc/fb0' & fb class 
 	 * register machine file operation to frame buffer file operation 
 	 * registered_fb[]
 	 * (drivers/video/fbmem.c)
 	 */
	ret = register_framebuffer(fbinfo);
	if (ret < 0) {
		EPRINTK("Failed to register framebuffer device: %d\n", ret);
		goto free_video_memory;
	}

	/* create device files */
	ret = device_create_file(&pdev->dev, &dev_attr_debug);

#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
	printk(KERN_INFO "%s: fb%d frame buffer device\n", fbinfo->fix.id, fbinfo->node);
	printk(KERN_INFO "%s: (%d*%d:%d) phys=0x%08x virt=0x%08x size=%d\n",
		fbinfo->fix.id,	fbinfo->var.xres, fbinfo->var.yres, fbinfo->var.bits_per_pixel,
		mfbinf->map_phys, (u_int)mfbinf->map_virt, mfbinf->map_size);
#endif
	ret = device_create_file(&pdev->dev, &dev_attr_tvout);
	return 0;

free_video_memory:
	pollux_fb_unmap_video_memory(mfbinf);

dealloc_fb:
	framebuffer_release(fbinfo);

	return ret;
}


/*
 *  Cleanup
 */
static int pollux_fb_remove(struct platform_device *pdev)
{
	struct fb_info	   *fbinfo;
	struct pollux_fb_info *info;
	
	DPRINTK("pollux_fb_remove\n");

	fbinfo = platform_get_drvdata(pdev);
	info = fbinfo->par;

	pollux_fb_unmap_video_memory(info);
	unregister_framebuffer(fbinfo);

	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM

/* suspend and resume support for the lcd controller */
static int pollux_fb_suspend(struct platform_device *dev, pm_message_t state)
{
	DPRINTK("pollux_fb_suspend\n");
	/* implement */
	struct fb_info	   *fbinfo = platform_get_drvdata(dev);
	struct pollux_fb_info *info = fbinfo->par;
	return 0;
}

static int pollux_fb_resume(struct platform_device *dev)
{
	DPRINTK("pollux_fb_reume\n");
	struct fb_info	   *fbinfo = platform_get_drvdata(dev);
	struct pollux_fb_info *info = fbinfo->par;
	return 0;
}

#else

#define pollux_fb_suspend NULL
#define pollux_fb_resume  NULL

#endif

static struct platform_driver pollux_fb_driver = {
	.probe		= pollux_fb_probe,
	.remove		= pollux_fb_remove,
	.suspend	= pollux_fb_suspend,
	.resume		= pollux_fb_resume,
	.driver		= {
		.name	= POLLUX_FB_NAME,
		.owner	= THIS_MODULE,
	},
};

int __devinit pollux_fb_init(void)
{
	int ret = 0;

	DPRINTK("pollux_fb_init\n");
    
    /* register platform driver, exec platform_driver probe */
	ret =  platform_driver_register(&pollux_fb_driver);
	if(ret){
		EPRINTK("failed to add platrom driver %s (%d) \n", pollux_fb_driver.driver.name, ret);
	}

	return ret;
}

static void __exit pollux_fb_exit(void)
{
	DPRINTK("pollux_fb_exit\n");
	platform_driver_unregister(&pollux_fb_driver);
}


module_init(pollux_fb_init);
module_exit(pollux_fb_exit);

MODULE_AUTHOR("Rohan <mesdigital.com>");
MODULE_DESCRIPTION("Framebuffer driver for the POLLUX");
MODULE_LICENSE("GPL");

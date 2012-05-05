/*
 * Clock change interface for Pollux cpu
 * 
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#include <asm/arch/pollux_clkpwr.h>
#include <asm/arch/regs-clock-power.h>

#define PLL0_P      20
#define PLL0_S      0


struct pollux_clk{
	unsigned long   p;	
	unsigned long	m;
	unsigned long	s;
};

unsigned long pll0_m_tb[] = { 161, 230, 299, 345, 393, 439, 460, 483, 506, 529, 552, 577,   
                                    598, 621, 625, 648, 667};

struct pollux_clk pll1_tb[] ={
	{ 	/* 261MHZ */
		.p 	= 9,
		.m  = 174,
		.s 	= 1,
	},	
	{   /* 132.75 MHZ */
		.p 	= 27,
		.m  = 1000,
		.s 	= 2, 
	},
	{	/* 124.2MHZ */
		.p 	= 9,
		.m  = 256,
		.s 	= 1,
	},
};

enum _pollux_clock_ioctl {
    IOCTL_SET_PLL0,
    IOCTL_SET_PLL1,
    IOCTL_GET_PLL0,
    IOCTL_GET_PLL1,
    IOCTL_SET_CLK_DEFUALTS
};    

enum _pollux_pll0_index {
    PLL0_200 = 0,
    PLL0_300,
    PLL0_400,
	PLL0_470,
	PLL0_530,
	PLL0_600,
	PLL0_620,
	PLL0_650,
	PLL0_680,
	PLL0_710,
	PLL0_750,
	PLL0_780,
	PLL0_800,
	PLL0_830,
	PLL0_850,
	PLL0_870,
	PLL0_900
};    

enum _pollux_pll1_index {
    PLL1_261 = 0,
    PLL1_132_75,
    PLL1_124_2	
};    

void set_pll0(int p, int m, int s)
{
    unsigned long timeo = jiffies;
    
    MES_CLKPWR_SetPLLPMS( 0, p, m, s);
	MES_CLKPWR_DoPLLChange();        	
    timeo += 50;
	while (time_before(jiffies, timeo)){
	    if(MES_CLKPWR_IsPLLStable()){
	        break;
	    }
	    cond_resched();	        				
    }	 
    mdelay(1);
}

void set_pll1(int p, int m, int s)
{
    unsigned long timeo = jiffies;
    
    MES_CLKPWR_SetPLLPowerOn( CTRUE );
    MES_CLKPWR_SetPLLPMS( 1, p, m, s);
	
	MES_CLKPWR_DoPLLChange();        	
	timeo += 50;
	while (time_before(jiffies, timeo)){
	    if(MES_CLKPWR_IsPLLStable()){
	        break;
	    }
	    cond_resched();	        				
    }	 
    mdelay(1);
}
     
void defualt_clockAndPower(void)
{
	unsigned long timeo = jiffies;
	
	// initialize PLL0  
	MES_CLKPWR_SetPLLPMS( 0, PLL0_P, pll0_m_tb[PLL0_530], PLL0_S);
			
	// initialize PLL1
	MES_CLKPWR_SetPLLPowerOn( CTRUE );
	MES_CLKPWR_SetPLLPMS( 1, pll1_tb[PLL1_261].p, pll1_tb[PLL1_261].m, 
				        pll1_tb[PLL1_261].s);	

	// PLL change and wait PLL stable state
	MES_CLKPWR_DoPLLChange();
	timeo += 50;
	while (time_before(jiffies, timeo)){
	    if(MES_CLKPWR_IsPLLStable()){
	        break;
	    }
	    cond_resched();	        				
    }	 
    mdelay(1);
}

static int pollux_clock_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	switch(cmd) 
	{ 
	    case IOCTL_SET_PLL0:
	        {
	        	int clk_index;
	          	
	          	if( copy_from_user(&clk_index, (unsigned char*)arg, sizeof(clk_index)) )
	            	return -EFAULT; 
			    				
				set_pll0(PLL0_P, pll0_m_tb[clk_index], PLL0_S);
	        }        
	        break;
        case IOCTL_SET_PLL1:
            {
                int clk_index;
	          	
	          	if( copy_from_user(&clk_index, (unsigned char*)arg, sizeof(clk_index)) )
	            	return -EFAULT; 
			    				
				set_pll1(pll1_tb[clk_index].p, pll1_tb[clk_index].m, 
				        pll1_tb[clk_index].s);
            }
	        break;
        case IOCTL_GET_PLL0:
            {
                unsigned  long pll;
              	pll = get_pll_clk(CLKSRC_PLL0);
				if( copy_to_user((unsigned char*)arg, &pll, sizeof(unsigned long)) ) 
	                return -EFAULT; 
            }
            break;        
        case IOCTL_GET_PLL1:
            {
                unsigned  long pll;
              	pll = get_pll_clk(CLKSRC_PLL1);
				if( copy_to_user((unsigned char*)arg, &pll, sizeof(unsigned long)) ) 
	                return -EFAULT; 
            }
            break;
        case IOCTL_SET_CLK_DEFUALTS:
            defualt_clockAndPower();
            break;    		
	}
	return 0;
}

static int pollux_clock_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 *	The various file operations we support.
 */

static struct file_operations pollux_clock_fops = {	
	ioctl:      pollux_clock_ioctl,
	open:       pollux_clock_open,
};

static struct miscdevice pollux_clock_dev = {
	MISC_DYNAMIC_MINOR,
	"pollux_clock",
	&pollux_clock_fops
};


static int pollux_clock_init(void)
{
	int ret;
	unsigned long data;
    
    ret = misc_register(&pollux_clock_dev);
	if (ret) {
		printk(KERN_WARNING "Clock Unable to register misc device.\n");
		return ret;
	}
    
    MES_CLKPWR_Initialize();
	MES_CLKPWR_SelectModule( 0 );
	MES_CLKPWR_SetBaseAddress( POLLUX_VA_CLKPWR);
	
	return 0;
}

static void __exit pollux_clock_exit(void)
{
	misc_deregister(&pollux_clock_dev);
}

module_init(pollux_clock_init);
module_exit(pollux_clock_exit);

MODULE_LICENSE("GPL");

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
#include <linux/irq.h>
#include <linux/workqueue.h>

#include <asm/mach-types.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/regs-clock-power.h>	
#include <asm/arch/regs-gpio.h>	
#include <asm/arch/aesop-pollux.h>	

#include "mes_sdhc.h"

//#define POLLUX_DUAL_SLOT
//#define POLLUX_AUTO_STOP_MODE
#define POLLUX_SDI0_NAME	"pollux_sdi0"
#define POLLUX_SDI1_NAME	"pollux_sdi1"



struct pollux_sdi_host {
	struct mmc_host		*mmc;
	
	char        hname[32];
	int         clock; /* clock source , GET_PCLK */
	int			irq;
	int			irq_cd;
	int         cdpin;
	int         wppin;
	u32         baseaddr;
	int			dma;
	
	int         channel;
	int			size;		/* Total size of transfer */

	int			cd_state;	/* Card state: 1 when card is inserted, 0 when card is removed */
	int			readonly;	/* Write protect state: 1 when write-protection is enabled, 0 when disabled */
	struct mmc_request	*mrq;
	u8          *buffer;
	int          length; // data read/write length ==> sg->length
	int          bytes_process; // data send/receive bytes check
	int          cmdflags; // command에 따른 command register에 손대야 하는 값을 저장하는 부분

	unsigned char		bus_width;	/* Current bus width */

	u16 imask;
	spinlock_t		complete_lock;
	struct completion	complete_request;
	struct completion	complete_dma;
	enum   pollux_waitfor complete_what;
	struct work_struct	card_detect_work;

	struct MES_SDHC_RegisterSet *sdi;
};




/*
 * Initialize pollux SD/MMC controller
 */
static int sdmmc_init(struct pollux_sdi_host *host)
{
    int ret = 0;
    MES_SDHC_SetBaseAddress(host->channel, host->baseaddr); // memory base setting for sdmmc channel 0
    MES_SDHC_SetClockPClkMode(host->channel, MES_PCLKMODE_ALWAYS);
    MES_SDHC_SetClockSource(host->channel, 0, SDHC_CLKSRC);  // 0, pclk
    MES_SDHC_SetClockDivisor(host->channel, 0, SDHC_CLKDIV); // 3, 
    MES_SDHC_SetOutputClockDivider(host->channel, SDCLK_DIVIDER_400KHZ); 
    MES_SDHC_SetOutputClockEnable(host->channel, CTRUE); // clock register control......==> enable
    MES_SDHC_SetClockDivisorEnable(host->channel, CTRUE); // clock divider enable
    MES_SDHC_OpenModule(host->channel); 
    MES_SDHC_ResetController(host->channel);    // Reset the controller.
    while (MES_SDHC_IsResetController(host->channel));
    MES_SDHC_ResetDMA(host->channel);   // Reset the DMA interface.
    while (MES_SDHC_IsResetDMA(host->channel)); // Wait until the DMA reset is
    MES_SDHC_ResetFIFO(host->channel);  // Reset the FIFO.
    while (MES_SDHC_IsResetFIFO(host->channel));    // Wait until the FIFO reset is 
    MES_SDHC_SetDMAMode(host->channel, CFALSE);
    MES_SDHC_SetLowPowerClockMode(host->channel, CFALSE);
    MES_SDHC_SetDataTimeOut(host->channel, 0xFFFFFF);
    MES_SDHC_SetResponseTimeOut(host->channel, 0xff);   // 0x64 );
    MES_SDHC_SetDataBusWidth(host->channel, 1);
    MES_SDHC_SetBlockSize(host->channel, BLOCK_LENGTH);
    MES_SDHC_SetFIFORxThreshold(host->channel, 8 - 1);  // Issue when RX FIFO Count  >= 8 x 4 bytes
    MES_SDHC_SetFIFOTxThreshold(host->channel, 8);      // Issue when TX FIFO Count <= 8 x 4 bytes
    MES_SDHC_SetInterruptEnableAll(host->channel, CFALSE);
	MES_SDHC_ClearInterruptPendingAll(host->channel);
    
    host->sdi = (struct MES_SDHC_RegisterSet *)host->baseaddr;
    
    return ret;
}



/*
 * Main interrupt handler of MP2530F SD/MMC controller driver
 *
 * This is used for state check.
 * If interrupt occurrs as intended, just call complete() of the wait event
 */
static irqreturn_t pollux_sdi_irq(int irq, void *dev_id)
{
	unsigned long iflags;
	struct pollux_sdi_host *host = (struct pollux_sdi_host *)dev_id;
	struct MES_SDHC_RegisterSet *p = host->sdi;
	u32    sdi_mint;
	u32    sdi_pclear; // pend clear variable
	int    i;

    sdi_mint = p->MINTSTS;
 	
 	spin_lock_irqsave( &host->complete_lock, iflags);
	
	if( host->complete_what==COMPLETION_NONE ) 
	{
		goto clear_imask;
	}
	
	sdi_pclear = 0; 

#ifdef POLLUX_AUTO_STOP_MODE    
	if(host->complete_what == COMPLETION_WAIT_STOP)
	{
	    if( sdi_mint & SDI_INTSTS_ACD )
	        goto transfer_closed;
	}
#endif
	
	if( sdi_mint & SDI_INTSTS_HLE )
	{
        printk("ERROR: HLE\n");		
		host->mrq->cmd->error = MMC_ERR_FAILED;
		goto transfer_closed;
	}

	if( sdi_mint & SDI_INTSTS_CD )
	{
			
		if(host->complete_what == COMPLETION_CMDDONE) 
		{
			host->mrq->cmd->error = MMC_ERR_NONE;
			goto transfer_closed;
		}

		if(host->complete_what == COMPLETION_XFERFINISH_RSPFIN) 
		{
			host->mrq->cmd->error = MMC_ERR_NONE;
			host->complete_what = COMPLETION_XFERFINISH;
		}

		sdi_pclear |= SDI_INTSTS_CD; 
	}
	
	if( sdi_mint & SDI_INTSTS_RCRC ) 
	{
		host->mrq->cmd->error = MMC_ERR_BADCRC;
		goto transfer_closed;
	}
	
	if( sdi_mint & SDI_INTSTS_RTO )
	{
		printk("ERROR: Response Timeout.........\n");
		host->mrq->cmd->error = MMC_ERR_TIMEOUT;
		goto transfer_closed;
	}

	if( sdi_mint & SDI_INTSTS_FRUN )
    {
        printk("ERROR : MES_SDCARD_ReadSectors - MES_SDHC_INT_FRUN\n");
        host->mrq->data->error = MMC_ERR_FIFO;
		goto transfer_closed;
    }
    
    
    if( host->mrq->data ) 
    {
    	if(host->mrq->data->flags & MMC_DATA_READ)
    	{
			if( host->length < 32 ) 
			{
				*((u32 *)(host->buffer + host->bytes_process)) = 0x00002501;
				host->bytes_process += 4;
				*((u32 *)(host->buffer + host->bytes_process)) = 0x00000000;
				
				
				if(host->complete_what == COMPLETION_XFERFINISH) 
				{
					host->mrq->cmd->error = MMC_ERR_NONE;
					host->mrq->data->error = MMC_ERR_NONE;
					p->RINTSTS = 0xFFFFFFFF; 
					goto transfer_closed;
				}
			}
        	
        	if( sdi_mint & SDI_INTSTS_RXDR )
        	{
            	for( i=0; i<8; i++ )
            	{
					*((u32 *)(host->buffer + host->bytes_process)) = p->DATA;
					host->bytes_process += 4;
				}
				
				sdi_pclear |= SDI_INTSTS_RXDR;
		        		
		            
			    if( host->length == host->bytes_process )
				{
					while(!MES_SDHC_IsFIFOEmpty(host->channel));
                    while(MES_SDHC_IsDataTransferBusy(host->channel));
					
					if(host->complete_what == COMPLETION_XFERFINISH) 
					{
						host->mrq->cmd->error = MMC_ERR_NONE;
						host->mrq->data->error = MMC_ERR_NONE;
						
						p->RINTSTS = 0xFFFFFFFF; 
				        
						goto transfer_closed;
					}
				}// last bytes process
			} // SDI_INTSTS_RXDR
			
			if( sdi_mint & SDI_INTSTS_DTO )
            {
            	for( i=0; i<8; i++ )
            	{
				    *((u32 *)(host->buffer + host->bytes_process)) = p->DATA;
					host->bytes_process += 4;
				}
        
				if( (host->length - host->bytes_process) == 32 ) 
				{
				    for( i=0; i<8; i++ )
            	    {
				        *((u32 *)(host->buffer + host->bytes_process)) = p->DATA;
					    host->bytes_process += 4;
				    }
				}
				
				if( host->length == host->bytes_process )
				{
                    while(!MES_SDHC_IsFIFOEmpty(host->channel));
                    while(MES_SDHC_IsDataTransferBusy(host->channel));
					
					if(host->complete_what == COMPLETION_XFERFINISH) 
					{
					    host->mrq->cmd->error = MMC_ERR_NONE;
						host->mrq->data->error = MMC_ERR_NONE;
					    p->RINTSTS = 0xFFFFFFFF; 
					    goto transfer_closed;
					}
				}// last bytes process
			}// SDI_INTSTS_DTO
		} // flags & MMC_DATA_READ
		else
		{

 	        if( sdi_mint & SDI_INTSTS_TXDR )
        	{
                
                for( i=0; i<8; i++ )
            	{
					p->DATA = *((u32 *)(host->buffer + host->bytes_process));
					host->bytes_process += 4;
				}
				
	            sdi_pclear |= SDI_INTSTS_TXDR;
			    
			    if( host->length == host->bytes_process )
				{
					while(!MES_SDHC_IsFIFOEmpty(host->channel));
                    while(MES_SDHC_IsDataTransferBusy(host->channel));
                    
					if(host->complete_what == COMPLETION_XFERFINISH) 
					{
						host->mrq->cmd->error = MMC_ERR_NONE;
						host->mrq->data->error = MMC_ERR_NONE;
						p->RINTSTS = 0xFFFFFFFF;
				        goto transfer_closed;
					}
		        }// last bytes process
		    } // SDI_INTSTS_TXDR
        } // flags & MMC_DATA_READ else ==> MMC_DATA_WRITE
	} // mrq->data

    
	p->RINTSTS = sdi_pclear; 
	spin_unlock_irqrestore( &host->complete_lock, iflags);
	return IRQ_HANDLED;
	
transfer_closed:
	host->complete_what = COMPLETION_NONE;
	complete(&host->complete_request);
	p->INTMASK = 0;
	spin_unlock_irqrestore( &host->complete_lock, iflags);
	return IRQ_HANDLED;

clear_imask:
	p->INTMASK = 0;
	spin_unlock_irqrestore( &host->complete_lock, iflags);
	return IRQ_HANDLED;
	
}



static int sdi_set_command_flag(struct pollux_sdi_host *host)
{
	struct MES_SDHC_RegisterSet *p = host->sdi;	
	int ret = 0;
	u32 flag = 0;
	int command = host->mrq->cmd->opcode;
	
    switch( command )
    {
    case GO_IDLE_STATE:        
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_SENDINIT |
            MES_SDHC_CMDFLAG_WAITPRVDAT;
		
        break;
    case IO_SEND_OP_COND:      // R4
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_SENDINIT |
            MES_SDHC_CMDFLAG_WAITPRVDAT | MES_SDHC_CMDFLAG_SHORTRSP;
        break;
    case ALL_SEND_CID:         // R2
    case SEND_CSD:             // R2
    case SEND_CID:             // R2
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_LONGRSP;
        break;
    case SEND_STATUS:          // R1
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_CHKRSPCRC |
            MES_SDHC_CMDFLAG_SHORTRSP;
        break;
    case SET_BLOCKLEN:         // R1
    case APP_CMD:              // R1
    case SET_CLR_CARD_DETECT:  // R1
    case SWITCH_FUNC:          // R1
    case SELECT_CARD:          // R1b
    case SEND_RELATIVE_ADDR:   // R6
    case SEND_IF_COND:         // R7
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_SHORTRSP;
        break;
    case SEND_SCR:             // R1, ghcstop fix for debugging
    	command = ((APP_CMD<<8) | 51);
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_SHORTRSP;
        break;
    case READ_SINGLE_BLOCK:    // R1
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_SHORTRSP |
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_RXDATA;
        break;
    case READ_MULTIPLE_BLOCK:  // R1
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_SHORTRSP |
#ifdef POLLUX_AUTO_STOP_MODE
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_RXDATA | 
            MES_SDHC_CMDFLAG_SENDAUTOSTOP;
#else
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_RXDATA;
#endif       
       break;
    case WRITE_BLOCK:          // R1
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_SHORTRSP |
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_TXDATA;
        break;
    case WRITE_MULTIPLE_BLOCK: // R1
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_SHORTRSP |
#ifdef POLLUX_AUTO_STOP_MODE            
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_TXDATA |
            MES_SDHC_CMDFLAG_SENDAUTOSTOP;    
#else        
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_TXDATA;
#endif        
        break;
    case STOP_TRANSMISSION:    // R1
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_CHKRSPCRC |
#ifdef POLLUX_AUTO_STOP_MODE
            MES_SDHC_CMDFLAG_SHORTRSP | MES_SDHC_CMDFLAG_STOPABORT |
            MES_SDHC_CMDFLAG_WAITPRVDAT;
#else
            MES_SDHC_CMDFLAG_SHORTRSP | MES_SDHC_CMDFLAG_STOPABORT;
#endif
        break;
    case SD_SEND_OP_COND:      // R3
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_SHORTRSP;
        break;
    default:
		flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_SHORTRSP;
        break;
    }
    
    
    host->cmdflags = flag;

	return ret;
}


/*
 * pollux_sdi_request()
 * Callback function for command processing 
 */
static void pollux_sdi_request(struct mmc_host *mmc, struct mmc_request *mrq) 
{
	int ret;
 	struct pollux_sdi_host *host = mmc_priv(mmc);
	struct scatterlist *obsg;
	struct MES_SDHC_RegisterSet *p = host->sdi;
	struct device *dev = mmc_dev(host->mmc); // 2.6.20-rc1
	u32    sdi_imsk = 0;
	u32    timeout ,Response;
	
    //hyun_debug 
  	if(host->cd_state != SD_INSERTED) mrq->cmd->retries = 0;
    
    host->mrq = mrq;
    host->cmdflags      = 0;
	host->complete_what = COMPLETION_CMDDONE; 
	ret = sdi_set_command_flag(host);
	if(ret < 0 )
	{
        printk("Command error occurred\n");
		goto request_done;
	}
	
	sdi_imsk = SDI_INTMSK_HLE|SDI_INTMSK_CD|SDI_INTMSK_RE;

	if (host->mrq->data) 
	{
		host->bytes_process = 0;
		obsg = &mrq->data->sg[0];
		host->length = host->mrq->data->blocks * host->mrq->data->blksz;
		MES_SDHC_SetByteCount( host->channel, host->length);
		host->buffer = page_address(sg_page(obsg)) + obsg->offset;
		
		if(mrq->data->flags & MMC_DATA_READ)
		{
			host->complete_what = COMPLETION_XFERFINISH_RSPFIN;
			sdi_imsk |= (SDI_INTMSK_RXDR|SDI_INTMSK_DTO|SDI_INTMSK_HTO|SDI_INTMSK_FRUN);
		}
		else if(mrq->data->flags & MMC_DATA_WRITE)
		{
			host->complete_what = COMPLETION_XFERFINISH_RSPFIN;
			sdi_imsk |= (SDI_INTMSK_TXDR|SDI_INTMSK_DTO|SDI_INTMSK_HTO|SDI_INTMSK_FRUN);
        }
	}
    
    init_completion(&host->complete_request);

	p->RINTSTS = 0xFFFFFFFF; 
	p->INTMASK = sdi_imsk;  
	p->CMDARG  = host->mrq->cmd->arg;
	p->CMD     = (host->mrq->cmd->opcode & 0xff)|(host->cmdflags);

    timeout = 1000; // 3000ms = 3sec
	ret = wait_for_completion_timeout(&host->complete_request, msecs_to_jiffies(timeout));
    if(!ret) 
	{
		printk("Timeout occurred during waiting for the interrupt\n");
		host->mrq->cmd->error = MMC_ERR_TIMEOUT;
		ret = -MMC_ERR_TIMEOUT;
		goto request_done;
	}
    
 
    //Cleanup controller
	p->RINTSTS = 0xFFFFFFFF; 
	p->INTMASK = 0;   
	p->CMDARG  = 0;
	p->CMD     = 0;


    // Read response
	if( host->mrq->cmd->flags & MMC_RSP_136 ) 
	{
		mrq->cmd->resp[0] = p->RESP3;
		mrq->cmd->resp[1] = p->RESP2;
		mrq->cmd->resp[2] = p->RESP1;
		mrq->cmd->resp[3] = p->RESP0;
    }
	else
	{
		mrq->cmd->resp[0] = p->RESP0;
		mrq->cmd->resp[1] = p->RESP1;
	}
    
    
    // If we have no data transfer we are finished here
	if(!host->mrq->data){ 
	    host->mrq = NULL;	
		goto request_done;
    }
	
	// Calulate the amout of bytes transfer, but only if there was no error
	if(mrq->data->error == MMC_ERR_NONE) 
	{
		mrq->data->bytes_xfered = (mrq->data->blocks * mrq->data->blksz);
	} 
	else 
	{
		mrq->data->bytes_xfered = 0;
	}

#ifdef POLLUX_AUTO_STOP_MODE
	if(host->mrq->data->stop)
	{
	    host->complete_what = COMPLETION_WAIT_STOP;
	    init_completion(&host->complete_request);
	    MES_SDHC_SetInterruptEnable(host->channel, SDI_INTMSK_ACD, CTRUE);
	    timeout = 1000;
	    ret = wait_for_completion_timeout(&host->complete_request, msecs_to_jiffies(timeout));
	    if(!ret) 
	    {
		    printk("Timeout occurred during waiting for the auto command interrupt\n");
		    host->mrq->data->error = MMC_ERR_TIMEOUT;
		    goto END_RW;
	    }
	    
	    p->RINTSTS = 0xFFFFFFFF;
	    
	    Response = MES_SDHC_GetAutoStopResponse(host->channel);
	    if(Response & 0xFDF98008)
	    {
		    host->mrq->data->error = MMC_ERR_TIMEOUT;
		    printk("Error, Auto stop response=%08Xh\n",Response);
			goto END_RW;
		}  
	}
	
END_RW:
	
    if(mrq->data->error != MMC_ERR_NONE)
    {
        if(host->mrq->data->stop) 
        {
            mmc_wait_for_cmd(mmc, mrq->data->stop, 3);
        }
        
        if( CFALSE == MES_SDHC_IsFIFOEmpty(host->channel) )       
        {
            MES_SDHC_ResetFIFO(host->channel);  // Reest the FIFO.
            while (MES_SDHC_IsResetFIFO(host->channel));   // Wait until the FIFO reset is completed.
        }
    }    

#else	
	
	// If we had an error while transfering data we reset the FIFO to clear out any garbage
	if(mrq->data->error != MMC_ERR_NONE) 
	{
		printk("mrq->data->error processing\n");
        MES_SDHC_ResetFIFO(host->channel);  // Reest the FIFO.
        while (MES_SDHC_IsResetFIFO(host->channel));   // Wait until the FIFO reset is completed.
    }

    /* Issue stop command, if required */
	if(host->mrq->data->stop) 
	{
        mmc_wait_for_cmd(mmc, mrq->data->stop, 3);
    }
#endif	
	
	host->mrq = NULL;


request_done:
	/* Notify end of the request processing by calling mmc_request_done() */
	mmc_request_done(host->mmc, mrq);
    
}


int sdmmc_SetClock(int channel, U32 divider )
{
	U32 timeout;
	
	MES_ASSERT( (1==divider) || (0==(divider&1)) );		// 1 or even number ==> even number error
	MES_ASSERT( (0<divider) && (510>=divider) );		// between 1 and 510
	
	// 1. Confirm that no card is engaged in any transaction.
	//    If there's a transaction, wait until it finishes.
	if(  MES_SDHC_IsCardDataBusy(channel) )
	{
		printk("MES_SDCARD_SetClock : ERROR - Data is busy\n");
		return CFALSE;
	}
	
	if( MES_SDHC_IsDataTransferBusy(channel) )
	{
		printk("MES_SDCARD_SetClock : ERROR - Data Transfer is busy\n");
		return CFALSE;
	}
	
	// 2. Disable the output clock.
	MES_SDHC_SetOutputClockEnable( channel, CFALSE );
	
	// 3. Program the clock divider as required.
	MES_SDHC_SetOutputClockDivider( channel, divider );
	
	// 4. Start a command with MES_SDHC_CMDFLAG_UPDATECLKONLY flag.
repeat_4 :
	MES_SDHC_SetCommand( channel, 0, MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_UPDATECLKONLY | MES_SDHC_CMDFLAG_WAITPRVDAT );
	
	// 5. Wait until a update clock command is taken by the SDHC module.
	//    If a HLE is occurred, repeat 4.
	timeout = 0;
	while( MES_SDHC_IsCommandBusy(channel) )
	{
		if( ++timeout > 0x1000000 )
		{
			printk("MES_SDCARD_SetClock : ERROR - TIme-out to update clock.\n");
			return CFALSE;
		}
	}
	
	if( MES_SDHC_GetInterruptPending( channel, MES_SDHC_INT_HLE ) )
	{
		MES_SDHC_ClearInterruptPending( channel, MES_SDHC_INT_HLE );
		goto repeat_4;
	}
	
	// 6. Enable the output clock.
	MES_SDHC_SetOutputClockEnable( channel, CTRUE );
	
	// 7. Start a command with MES_SDHC_CMDFLAG_UPDATECLKONLY flag.
repeat_7 :
	MES_SDHC_SetCommand( channel, 0, MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_UPDATECLKONLY | MES_SDHC_CMDFLAG_WAITPRVDAT );
	
	// 8. Wait until a update clock command is taken by the SDHC module.
	//    If a HLE is occurred, repeat 7.
	timeout = 0;
	while( MES_SDHC_IsCommandBusy(channel) )
	{
		if( ++timeout > 0x1000000 )
		{
			printk("MES_SDCARD_SetClock : ERROR - TIme-out to update clock2.\n");
			return CFALSE;
		}
	}
	
	if( MES_SDHC_GetInterruptPending( channel, MES_SDHC_INT_HLE ) )
	{
		MES_SDHC_ClearInterruptPending( channel, MES_SDHC_INT_HLE );
		goto repeat_7;
	}
	
	
	return CTRUE;
}



/*
 * Callback function for clock enable/disable and clock rate setting
 */
static void pollux_sdi_set_ios(struct mmc_host *mmc, struct mmc_ios *ios) 
{
	int prescaler;
	struct pollux_sdi_host *host = mmc_priv(mmc);;

#if 0	
	if (ios->power_mode == MMC_POWER_OFF) 
	{
        MES_SDHC_SetOutputClockEnable(host->channel, CFALSE);
		//mdelay(100);
		
	}
	else if (ios->power_mode == MMC_POWER_ON) 
	{
		MES_SDHC_SetOutputClockEnable(host->channel, CTRUE);
	}
#endif	

    if( ios->clock == 400000 && host->clock == 400000 ) goto iosend;
    
    if( ios->clock == 400000 )
	{
		prescaler = SDCLK_DIVIDER_400KHZ;
		sdmmc_SetClock(host->channel, prescaler);
	}
	else if( ios->clock == 25000000 )
	{
		prescaler = SDCLK_DIVIDER_25MHZ;
		sdmmc_SetClock(host->channel, prescaler);
    }
	else
		goto iosend;

#if 0	
    MES_SDHC_ResetFIFO(host->channel);  // Reset the FIFO.
    while (MES_SDHC_IsResetFIFO(host->channel));    // Wait until the FIFO reset is 
#endif
                                                
	/* Wait after clock setting */
	mdelay(100);
	
iosend:
    host->clock = ios->clock;
    host->bus_width = ios->bus_width; // ios->bus_width is set automatically by reading CSR(card status register)

#if 1
	if( host->bus_width == 0 ) // 1bit
		MES_SDHC_SetDataBusWidth(host->channel, 1);
	else if( host->bus_width == 2 ) // 4bit
		MES_SDHC_SetDataBusWidth(host->channel, 4);
#endif

}


/*
 * Get write-protect tab state by reading GPIO pin
 */
static int pollux_sdi_get_ro(struct mmc_host *mmc)
{
	struct pollux_sdi_host *host = mmc_priv(mmc);
	
	host->readonly =  pollux_gpio_getpin(host->wppin); 
	return (host->readonly);
}



/*
 * ISR for the CardDetect Pin
 *
 * New card state is written in host->cd_state
 * Schedule workqueue to call mmc_detect_change()
 * (This function cannot be called by interrupt handler because
 *  interrupt handler cannot be scheduled)
 */

/*             
                       
Inserted high: 1  1    
Interted low : 1  0    
Blank    high: 0  1    
Blank    low : 0  0    

*/

static irqreturn_t pollux_sdi_irq_cd(int irq, void *dev_id)
{
	struct pollux_sdi_host *host = (struct pollux_sdi_host *)dev_id;
	int new_cd_state;

	disable_irq(irq);

	new_cd_state = pollux_gpio_getpin(host->cdpin); 
	
	if( host->cd_state == SD_REMOVED && new_cd_state == 0) // removed/low ==> inserted
	{
		host->cd_state = SD_INSERTED;		
		set_irq_type(irq, IRQT_RISING);	/* Current GPIO value is low: wait for raising high */
		schedule_work(&host->card_detect_work);
	}
    else if( host->cd_state == SD_INSERTED && new_cd_state == 1) // inserted/high ==> removed
	{
		host->cd_state = SD_REMOVED;
    	set_irq_type(irq, IRQT_FALLING);	/* Current GPIO value is hight: wait for raising low */
		schedule_work(&host->card_detect_work);
    }
    else
    {
    	if( new_cd_state == 1 )
    		set_irq_type( irq, IRQT_FALLING); 
    	else
    		set_irq_type( irq, IRQT_RISING);
    }

	enable_irq(irq);
	
	return IRQ_HANDLED;	
}

/*
 * This function is scheduled by card detect interrupt handler.
 *
 * When card state is changed, this function call mmc_detect_change().
 * Upper layer of the mmc driver recognizes insertion/removal of the card
 * when mmc_detect_change() function is called.
 */
static void pollux_sdi_card_detect(struct work_struct *work)
{
	struct pollux_sdi_host *host;
	int delay = 0;
	int ret;

	host = container_of(work, struct pollux_sdi_host, card_detect_work);

	/*
	 * Initialize pollux SDI
	 */
	ret = sdmmc_init(host);
	if(ret != 0)
	{
		printk(KERN_ERR "pollux SD %d channel Initialize error\n", host->channel);
		return;
	}

	
	if(host->cd_state == SD_INSERTED)
		delay = 200;
	else
		delay = 50;
 	
 	mmc_detect_change(host->mmc, msecs_to_jiffies(delay)); // number of jiffies to wait before queueing
}



/* Callback functions */
static const struct mmc_host_ops pollux_sdi_ops = {
	.request	= pollux_sdi_request,
	.set_ios	= pollux_sdi_set_ios,
	.get_ro		= pollux_sdi_get_ro,
};


/*
 * Initialize MP2530F SD/MMC host controller driver
 */

#if 0
static int pollux_sdi_probe(struct platform_device *pdev)
{
	struct mmc_host 	*mmc;
	struct pollux_sdi_host 	*host;
	int ret = 0;
	struct resource *res;


	/* 
	 * Allocate memory space for host specific data structure 
	 */
	
    mmc = mmc_alloc_host(sizeof(struct pollux_sdi_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_out;
	}
	
	/*
	 * Set host specific pointer to allocated space
	 */
	
	host = mmc_priv(mmc);
	
	/* 
	 * Initialize host specific data
	 */
	
	/* Set value of mmc structure */
	mmc->ops			= &pollux_sdi_ops;
	mmc->f_min			=   400000;
	mmc->f_max			= 33000000;
	mmc->caps			= MMC_CAP_4_BIT_DATA;	
		/*
		 * Set the maximum segment size.  Since we aren't doing DMA
		 * we are only limited by the data length register.
		 */
	mmc->max_seg_size	= 64 * 512;
	mmc->max_phys_segs	= 1;
	mmc->max_blk_size   = 2048;
	mmc->max_blk_count  = 64;
	mmc->ocr_avail = MMC_VDD_32_33;

	spin_lock_init( &host->complete_lock );

	/* 
	 * Set host specific structure 
	 */
	host->mmc				= mmc;
	host->dma				= 0;
	host->size				= 0;
	host->irq_cd			= 0;
	host->cd_state			= 0;
	host->clock				= 0;
	host->cd_state			= SD_INSERTED;
	host->readonly			= 0;
	host->bus_width 		= 0;
	host->channel           = pdev->id;

	if( host->channel == 0 )
		strcpy(host->hname, POLLUX_SDI0_NAME);
	else
		strcpy(host->hname, POLLUX_SDI1_NAME);
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0); // MEM resource #0, Virtual memory of sdmmc base address
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get memory base resources\n");
		ret = -EINVAL;
		goto probe_out;
	}
	host->baseaddr = (u32)res->start;
	

	res = platform_get_resource(pdev, IORESOURCE_IO, 0); // IO resource #0, card detection gpio number
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get resources 2\n");
		ret = -EINVAL;
		goto probe_out;
	}
	
	host->cdpin = (int)res->start;
	
    res = platform_get_resource(pdev, IORESOURCE_IO, 1); // IO resource #1, write protection gpio number
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get resources 3\n");
		ret = -EINVAL;
		goto probe_out;
	}
	
	host->wppin = (int)res->start;


	/*
	 * Get SD/MMC controller IRQ
	 */
	
	host->irq = platform_get_irq(pdev, 0);
	if (host->irq == 0) 
	{
		printk(KERN_INFO "failed to get interrupt resouce.\n");
		ret = -EINVAL;
		goto probe_out;
	}
	
	/*
	 * Initialize GPIO prototype.
	 * GPIO pin is used for card detection and write protection state detection 
	 */
	host->irq_cd = platform_get_irq(pdev, 1);
	if( host->irq_cd == 0 ) 
	{
		printk(KERN_INFO "failed to get card detection interrupt resource.\n");
		ret = -EINVAL;
		goto probe_out;
	}

    /*
	 * Initialize SD controller prototype  from sdmmc.cpp of wince bsp
	 */
	
	MES_SDHC_Initialize(); 
	
    /*
	 * Initialize MP2530F SDI
	 */
	ret = sdmmc_init(host);
	if(ret != 0)
	{
		ret = -EINVAL;
		goto probe_out;
	}

    /*
	 * Register handler for host interupt
	 */
	if(request_irq(host->irq, pollux_sdi_irq, 0, POLLUX_SDI0_NAME, host)) 
	{
		printk(KERN_INFO "failed to request sdi interrupt.\n");
		ret = -ENOENT;
		goto probe_out;
	}


	/*
	 * Read current card state 
	 */
    host->readonly =  pollux_gpio_getpin(host->wppin); 
	host->cd_state = !pollux_gpio_getpin(host->cdpin); 
	
    /*
	 * Register interrupt handler for card detection 
	 */
	INIT_WORK(&host->card_detect_work, pollux_sdi_card_detect);

	if( host->cd_state ) // inserted
		set_irq_type(host->irq_cd, IRQT_RISING);
	else
		set_irq_type( host->irq_cd, IRQT_FALLING);
		
		
    if(request_irq(host->irq_cd, pollux_sdi_irq_cd, 0, host->hname, host)) 
	{
		printk(KERN_WARNING "failed to request card detect interrupt.\n" );
		ret = -ENOENT;
		goto probe_free_irq;
	}

    /*
	 * Register MP2530F host driver
	 */
	if((ret = mmc_add_host(mmc)))
	{
		printk(KERN_INFO "failed to add mmc host.\n");
		goto free_resource;
	}

	platform_set_drvdata(pdev, mmc);
    return ret;

free_resource:
    free_irq(host->irq_cd, host);	/* unregister card detection interrupt handler */

probe_free_irq:
 	free_irq(host->irq, host);	/* unregister host controller interrupt handler */

probe_out:
	return ret;
}

#else  /* **************************************************************** */
struct platform_device *pbdev;

static int pollux_sdi_probe(struct platform_device *pdev)
{
    pbdev = pdev;
    return 0;
}


int pollux_sdi_probe1(void)
{
	struct mmc_host 	*mmc;
	struct pollux_sdi_host 	*host;
	int ret = 0;
	struct resource *res;


	/* 
	 * Allocate memory space for host specific data structure 
	 */
	mmc = mmc_alloc_host(sizeof(struct pollux_sdi_host), &pbdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_out;
	}
	
	/*
	 * Set host specific pointer to allocated space
	 */
	host = mmc_priv(mmc);
	
	
    /* 
	 * Initialize host specific data
	 */
	
	/* Set value of mmc structure */
	mmc->ops			= &pollux_sdi_ops;
	mmc->f_min			=   400000;
	mmc->f_max			= 33000000;
	mmc->caps			= MMC_CAP_4_BIT_DATA;	
	/*
     * Set the maximum segment size.  Since we aren't doing DMA
	 * we are only limited by the data length register.
     */
	mmc->max_seg_size	= 64 * 512;
	mmc->max_phys_segs	= 1;
	mmc->max_blk_size   = 2048;
	mmc->max_blk_count  = 64;
	mmc->ocr_avail = MMC_VDD_32_33;

	spin_lock_init( &host->complete_lock );

	/* 
	 * Set host specific structure 
	 */
	host->mmc				= mmc;
	host->dma				= 0;
	host->size				= 0;
	host->irq_cd			= 0;
	host->cd_state			= 0;
	host->clock				= 0;
	host->cd_state			= SD_INSERTED;
	host->readonly			= 0;
	host->bus_width 		= 0;
	host->channel           = pbdev->id;

	
	if( host->channel == 0 )
		strcpy(host->hname, POLLUX_SDI0_NAME);
	else
		strcpy(host->hname, POLLUX_SDI1_NAME);
	
	res = platform_get_resource(pbdev, IORESOURCE_MEM, 0); // MEM resource #0, Virtual memory of sdmmc base address
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get memory base resources\n");
		ret = -EINVAL;
		goto probe_out;
	}
	host->baseaddr = (u32)res->start;
	

	res = platform_get_resource(pbdev, IORESOURCE_IO, 0); // IO resource #0, card detection gpio number
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get resources 2\n");
		ret = -EINVAL;
		goto probe_out;
	}
	
	host->cdpin = (int)res->start;
	res = platform_get_resource(pbdev, IORESOURCE_IO, 1); // IO resource #1, write protection gpio number
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get resources 3\n");
		ret = -EINVAL;
		goto probe_out;
	}
	
	host->wppin = (int)res->start;


	/*
	 * Get SD/MMC controller IRQ
	 */
	
	host->irq = platform_get_irq(pbdev, 0);
	if (host->irq == 0) 
	{
		printk(KERN_INFO "failed to get interrupt resouce.\n");
		ret = -EINVAL;
		goto probe_out;
	}
	
	/*
	 * Initialize GPIO prototype.
	 * GPIO pin is used for card detection and write protection state detection 
	 */
	host->irq_cd = platform_get_irq(pbdev, 1);
	if( host->irq_cd == 0 ) 
	{
		printk(KERN_INFO "failed to get card detection interrupt resource.\n");
		ret = -EINVAL;
		goto probe_out;
	}


	/*
	 * Initialize SD controller prototype  from sdmmc.cpp of wince bsp
	 */
	MES_SDHC_Initialize(); 
	

	/*
	 * Initialize MP2530F SDI
	 */
	ret = sdmmc_init(host);
	if(ret != 0)
	{
		ret = -EINVAL;
		goto probe_out;
	}


    /*
	 * Register handler for host interupt
	 */
	if(request_irq(host->irq, pollux_sdi_irq, 0, POLLUX_SDI0_NAME, host)) 
	{
		printk(KERN_INFO "failed to request sdi interrupt.\n");
		ret = -ENOENT;
		goto probe_out;
	}


	/*
	 * Read current card state 
	 */

	host->readonly =  pollux_gpio_getpin(host->wppin); 
	host->cd_state = !pollux_gpio_getpin(host->cdpin); 
	
    /*
	 * Register interrupt handler for card detection 
	 */
	INIT_WORK(&host->card_detect_work, pollux_sdi_card_detect);

	if( host->cd_state ) // inserted
		set_irq_type(host->irq_cd, IRQT_RISING);
	else
		set_irq_type( host->irq_cd, IRQT_FALLING);
		
	if(request_irq(host->irq_cd, pollux_sdi_irq_cd, 0, host->hname, host)) 
	{
		printk(KERN_WARNING "failed to request card detect interrupt.\n" );
		ret = -ENOENT;
		goto probe_free_irq;
	}



	/*
	 * Register MP2530F host driver
	 */

	if((ret = mmc_add_host(mmc)))
	{
		printk(KERN_INFO "failed to add mmc host.\n");
		goto free_resource;
	}

	platform_set_drvdata(pbdev, mmc);

    return ret;

free_resource:
    free_irq(host->irq_cd, host);	/* unregister card detection interrupt handler */

probe_free_irq:
 	free_irq(host->irq, host);	/* unregister host controller interrupt handler */

probe_out:
	return ret;
}

EXPORT_SYMBOL_GPL(pollux_sdi_probe1);

#endif


static int pollux_sdi_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "pollux SD/MMC Driver - Removing\n");
	return 0;
}


#ifdef CONFIG_PM
static int pollux_sdi_suspend(struct platform_device *dev, pm_message_t state)
{
	int ret = 0;

	printk(KERN_INFO "pollux sdi suspend\n");

	return ret;
}

static int pollux_sdi_resume(struct platform_device *dev)
{
	int ret = 0;

	printk(KERN_INFO "pollux sdi resume\n");

	return ret;
}
#else
#define pollux_sdi_suspend	NULL
#define pollux_sdi_resume	NULL
#endif

static struct resource pollux_sdi0_resource[] = {
    [0] = { // SD/MMC register virtual address ==> not use ioremap
        .start = POLLUX_VA_SDI0,
        .end   = POLLUX_VA_SDI0 + 0x3f,
        .flags = IORESOURCE_MEM,
    },  
    [1] = { // irq no.
        .start = IRQ_SDMMC0,	
        .end   = IRQ_SDMMC0,	
        .flags = IORESOURCE_IRQ,
    } ,
    [2] = { // card detection irq
        .start = IRQ_SD0_DETECT,	
        .end   = IRQ_SD0_DETECT,	
        .flags = IORESOURCE_IRQ,
    },
    [3] = { // card detection state read gpio
        .start = GPIO_SD0_CD ,	
        .end   = GPIO_SD0_CD ,	
        .flags = IORESOURCE_IO,
    },
    [4] = { // card write protection read gpio
        .start = GPIO_SD0_WP,	
        .end   = GPIO_SD0_WP,	
        .flags = IORESOURCE_IO,
    } 
};


static struct platform_device pollux_sdi0_device = {
	.name	= POLLUX_SDI0_NAME,
	.id	= 0,	// -1: set 'platform_device->device.bus_id' with 'platform_device->name' &  strlcpy(,,BUS_ID_SIZE)
			//  0: set 'platform_device->device.bus_id' with 'platform_device->name' &  snprintf
	.num_resources = ARRAY_SIZE(pollux_sdi0_resource),
	.resource = pollux_sdi0_resource,
	
};


static struct platform_driver pollux_sdi0_driver =
{
	.driver		= {
		.name	= POLLUX_SDI0_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= pollux_sdi_probe,
	.remove		= pollux_sdi_remove,
	.suspend	= pollux_sdi_suspend,
	.resume		= pollux_sdi_resume,
};



static struct resource pollux_sdi1_resource[] = {
    [0] = { // SD/MMC register virtual address ==> not use ioremap
        .start = POLLUX_VA_SDI1,
        .end   = POLLUX_VA_SDI1 + 0x3f,
        .flags = IORESOURCE_MEM,
    },  
    [1] = { // irq no.
        .start = IRQ_SDMMC1,	
        .end   = IRQ_SDMMC1,	
        .flags = IORESOURCE_IRQ,
    } ,
    [2] = { // card detection irq
        .start = IRQ_SD1_DETECT,	
        .end   = IRQ_SD1_DETECT,	
        .flags = IORESOURCE_IRQ,
    },
    [3] = { // card detection state read gpio
        .start = GPIO_SD1_CD ,	
        .end   = GPIO_SD1_CD ,	
        .flags = IORESOURCE_IO,
    },
    [4] = { // card write protection read gpio
        .start = GPIO_SD1_WP,	
        .end   = GPIO_SD1_WP,	
        .flags = IORESOURCE_IO,
    } 
};

static struct platform_device pollux_sdi1_device = {
	.name	= POLLUX_SDI1_NAME,
	.id	= 1,	// -1: set 'platform_device->device.bus_id' with 'platform_device->name' &  strlcpy(,,BUS_ID_SIZE)
			//  0: set 'platform_device->device.bus_id' with 'platform_device->name' &  snprintf
	.num_resources = ARRAY_SIZE(pollux_sdi1_resource),
	.resource = pollux_sdi1_resource,
	
};


static struct platform_driver pollux_sdi1_driver =
{
	.driver		= {
		.name	= POLLUX_SDI1_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= pollux_sdi_probe,
	.remove		= pollux_sdi_remove,
	.suspend	= pollux_sdi_suspend,
	.resume		= pollux_sdi_resume,
};

//


static int __init pollux_sdi_init(void)
{
	int ret;

    /* register platform device */
	ret = platform_device_register(&pollux_sdi0_device);
	if(ret)
	{
		printk(KERN_WARNING "failed to add platform device %s (%d) \n", pollux_sdi0_device.name, ret);
		return ret;
	}

	/* register platform driver, exec platform_driver probe */
	ret =  platform_driver_register(&pollux_sdi0_driver);
	if(ret)
	{
		printk(KERN_WARNING "failed to add platrom driver %s (%d) \n", pollux_sdi0_driver.driver.name, ret);
	}
	
#ifdef POLLUX_DUAL_SLOT	
	/* register platform device */
	ret = platform_device_register(&pollux_sdi1_device);
	if(ret)
	{
		printk(KERN_WARNING "failed to add platform device %s (%d) \n", pollux_sdi1_device.name, ret);
		return ret;
	}

	/* register platform driver, exec platform_driver probe */
	ret =  platform_driver_register(&pollux_sdi1_driver);
	if(ret){
		printk(KERN_WARNING "failed to add platrom driver %s (%d) \n", pollux_sdi1_driver.driver.name, ret);
	}
#endif	
    
    return ret;
}


static void __exit pollux_sdi_exit(void)
{
	platform_driver_unregister(&pollux_sdi0_driver);
	platform_driver_unregister(&pollux_sdi1_driver);
}


module_init(pollux_sdi_init);
module_exit(pollux_sdi_exit);

MODULE_DESCRIPTION("pollux MP2530F SD/MMC Interface driver");
MODULE_LICENSE("GPL");

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

#define POLLUX_SDI0_NAME	"pollux_sdi0"
#define POLLUX_SDI1_NAME	"pollux_sdi1"


#if 0
	#define dprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define dprintk(x...) do { } while (0)
#endif


#if 0
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif


#if 0
	#define aprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define aprintk(x...) do { } while (0)
#endif

static int test_check=0;

//static int m_channel = 0; // 0 channel로 고정

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


inline char *disp32bit(unsigned int data)
{
    unsigned int mask = 0x80000000;
    int j, bufidx=0;
    static char buf[32+8];
    //char buf[32+8];
    
    for(j=0; j<32; j++)
    {
        if( data&mask ) // !0
        {
        	if( (j%4) == 3 )
        	{
               buf[bufidx++] = '1';
               buf[bufidx++] = ' ';
            }
            else
            {
            	buf[bufidx++] = '1';
            }
        }
        else // 0
        {
        	if( (j%4) == 3 )
        	{
               buf[bufidx++] = '0';
               buf[bufidx++] = ' ';
            }
            else
            {
            	buf[bufidx++] = '0';
            }
        }
        
        mask >>= 1;
    }
    
    buf[bufidx] = 0;
    
    return buf;   
}


static void dump_sdi(struct pollux_sdi_host *host)
{
	struct MES_SDHC_RegisterSet *p = host->sdi;

	dprintk("CTRL    =  0x%08x\n", p->CTRL);       				// 0x000 : Control
	dprintk("CLKDIV  = 	0x%08x\n", p->CLKDIV);  				// 0x008 : Clock Divider 
	dprintk("CLKENA  = 	0x%08x\n", p->CLKENA);  				// 0x010 : Clock Enable
	dprintk("TMOUT   = 	0x%08x\n", p->TMOUT);					// 0x014 : Time-Out
	dprintk("CTYPE   = 	0x%08x\n", p->CTYPE);					// 0x018 : Card Type
	dprintk("BLKSIZ  = 	0x%08x\n", p->BLKSIZ);  				// 0x01C : Block Size
	dprintk("BYTCNT  = 	0x%08x\n", p->BYTCNT);  				// 0x020 : Byte Count
	dprintk("INTMASK =	0x%08x\n", p->INTMASK);   				// 0x024 : Interrupt Mask
	dprintk("CMDARG  = 	0x%08x\n", p->CMDARG);  				// 0x028 : Command Argument
	dprintk("CMD     = 	0x%08x\n", p->CMD);	    				// 0x02C : Command
	//dprintk("RESP0   = 	0x%08x\n", p->RESP0);					// 0x030 : Response 0
	//dprintk("RESP1   = 	0x%08x\n", p->RESP1);					// 0x034 : Response 1
	//dprintk("RESP2   = 	0x%08x\n", p->RESP2);					// 0x038 : Response 2
	//dprintk("RESP3   = 	0x%08x\n", p->RESP3);					// 0x03C : Response 3
	dprintk("MINTSTS = 	0x%08x\n", p->MINTSTS);   				// 0x040 : Masked Interrupt
	dprintk("RINTSTS =	0x%08x\n", p->RINTSTS);  				// 0x044 : Raw Interrupt
	dprintk("STATUS  = 	0x%08x\n", p->STATUS);  				// 0x048 : Status
	dprintk("FIFOTH  = 	0x%08x\n", p->FIFOTH);  				// 0x04C : FIFO Threshold Watermark
	dprintk("TCBCNT  = 	0x%08x\n", p->TCBCNT);  				// 0x05C : Transferred Card Byte Count
	dprintk("TBBCNT  = 	0x%08x\n", p->TBBCNT);  				// 0x060 : Transferred Host Byte Count
	//dprintk("DATA    = 	0x%08x\n", p->DATA);					// 0x100 : Data
	dprintk("CLKENB  = 	0x%08x\n", p->CLKENB);  				// 0x7C0 : Clock Enable for CLKGEN module
	dprintk("CLKGEN  = 	0x%08x\n", p->CLKGEN);  				// 0x7C4 : Clock Generator for CLKGEN module
}


/*
 * Initialize MP2530F SD/MMC controller
 */
static int sdmmc_init(struct pollux_sdi_host *host)
{
    int ret = 0;
    MES_SDHC_SetBaseAddress(host->channel, host->baseaddr); // memory base setting for sdmmc channel 0
    // SDHC_Def.h, 
    MES_SDHC_SetClockPClkMode(host->channel, MES_PCLKMODE_ALWAYS);
    MES_SDHC_SetClockSource(host->channel, 0, SDHC_CLKSRC);  // 0, pclk
    MES_SDHC_SetClockDivisor(host->channel, 0, SDHC_CLKDIV); // 3, 
    MES_SDHC_SetOutputClockDivider(host->channel, SDCLK_DIVIDER_400KHZ); 
    MES_SDHC_SetOutputClockEnable(host->channel, CTRUE); // clock register control......==> enable
    MES_SDHC_SetClockDivisorEnable(host->channel, CTRUE); // clock divider enable

    MES_SDHC_OpenModule(host->channel); // sdi control register에서 interrupt enable bit를 세팅한다. c000_9800, c000_c800

	// dmarst/fiforst를 0으로(이상함), controller reset, sdi control register
    MES_SDHC_ResetController(host->channel);    // Reset the controller.
    
    
    
    // controller reset bit가 normal mode인지 체킹
    while (MES_SDHC_IsResetController(host->channel));  // Wait until the
                                                    // controller reset is
                                                    // completed.

	// fifo랑 controller는 normal mode로 세팅, DMA만 reset, contol register
    MES_SDHC_ResetDMA(host->channel);   // Reset the DMA interface.
    // dma가 normal mode인지 검사
    while (MES_SDHC_IsResetDMA(host->channel)); // Wait until the DMA reset is
                                            // completed.

	// 위의 두 놈이랑 비슷함
    MES_SDHC_ResetFIFO(host->channel);  // Reset the FIFO.
    while (MES_SDHC_IsResetFIFO(host->channel));    // Wait until the FIFO reset is 
                                                // completed.

	// dma mode disable: 초기라서 그런가....ㅋㅋ
    MES_SDHC_SetDMAMode(host->channel, CFALSE);
    // low power는 안함
    MES_SDHC_SetLowPowerClockMode(host->channel, CFALSE);
    //MES_SDHC_SetLowPowerClockMode(host->channel, CTRUE);
	// data timeout 세팅해주고    
    MES_SDHC_SetDataTimeOut(host->channel, 0xFFFFFF);
    // command response timeout setting해주고
    MES_SDHC_SetResponseTimeOut(host->channel, 0xff);   // 0x64 );
    
    MES_SDHC_SetDataBusWidth(host->channel, 1);


    //MES_SDHC_SetOutputClockEnable(host->channel, CTRUE); // clock register control......==> enable
    //MES_SDHC_SetClockDivisorEnable(host->channel, CTRUE); // clock divider enable


	// sdhc_def.h, block length는 역쉬 512
    MES_SDHC_SetBlockSize(host->channel, BLOCK_LENGTH);
    
    // 정확히 8이 의미하는 바를 모르겠음...메뉴얼에도 안나와 있음.
    // 32bytes의 half full에 대한 interrupt level을 말하는것 같기는 한데....웃긴다.
    MES_SDHC_SetFIFORxThreshold(host->channel, 8 - 1);  // Issue when RX FIFO Count  >= 8 x 4 bytes
    //MES_SDHC_SetFIFORxThreshold(host->channel, 2);  // Issue when RX FIFO Count  >= 2 x 4 bytes  ==> 안먹는다....이상함...
    MES_SDHC_SetFIFOTxThreshold(host->channel, 8);      // Issue when TX FIFO Count <= 8 x 4 bytes

	// 0으로 write한다....즉, 모든 mask clear ==> interrupt all enable?: 함수에서의 설명과는 틀린 듯...하여간 몽땅 enable시키는 것이기는 함
    MES_SDHC_SetInterruptEnableAll(host->channel, CFALSE);
	// interrupt status register all clear(write all bit to 1)
    MES_SDHC_ClearInterruptPendingAll(host->channel);
    
    host->sdi = (struct MES_SDHC_RegisterSet *)host->baseaddr;

	dump_sdi(host);

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
	
	sdi_pclear = 0; // pend clear variable

	// command 관련 error check
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
			gprintk(" ==> nodata\n");		
			host->mrq->cmd->error = MMC_ERR_NONE;
			goto transfer_closed;
		}

		if(host->complete_what == COMPLETION_XFERFINISH_RSPFIN) 
		{
			gprintk(" ==> DATA\n");		
			host->mrq->cmd->error = MMC_ERR_NONE;
			host->complete_what = COMPLETION_XFERFINISH;
		}

		sdi_pclear |= SDI_INTSTS_CD; // pend clear하고 data 처리루틴으로 가야함
	}
	
	if( sdi_mint & SDI_INTSTS_RCRC ) // Check Response Error
	{
		host->mrq->cmd->error = MMC_ERR_BADCRC;
		goto transfer_closed;
	}
	
	if( sdi_mint & SDI_INTSTS_RTO )
	{
		dprintk("ERROR: Response Timeout.........\n");
		host->mrq->cmd->error = MMC_ERR_TIMEOUT;
		goto transfer_closed;
	}

	if( sdi_mint & SDI_INTSTS_FRUN )
    {
        dprintk("ERROR : MES_SDCARD_ReadSectors - MES_SDHC_INT_FRUN\n");
        gprintk("ERROR : MES_SDCARD_ReadSectors - MES_SDHC_INT_FRUN\n");
	    host->mrq->data->error = MMC_ERR_FIFO;
		goto transfer_closed;
    }


	// 실제 data process
    if( host->mrq->data ) 
    {
    	if(host->mrq->data->flags & MMC_DATA_READ)
    	{
			if( host->length < 32 ) // 8bytes read 명령이 내려오는데 처리가 안된다....데이터가 들어오지 않음
			{
				*((u32 *)(host->buffer + host->bytes_process)) = 0x00002501;
				host->bytes_process += 4;
				*((u32 *)(host->buffer + host->bytes_process)) = 0x00000000;
				
				
				if(host->complete_what == COMPLETION_XFERFINISH) 
				{
					host->mrq->cmd->error = MMC_ERR_NONE;
					host->mrq->data->error = MMC_ERR_NONE;
					
					dprintk("done...\n");
					p->RINTSTS = 0xFFFFFFFF; // all interrupt pend clear
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
					dprintk("RXDR: last bytes_process = %d\n", host->bytes_process);
					
                    if( CFALSE == MES_SDHC_IsFIFOEmpty(host->channel) )
                    {
                    	dprintk("Fifo is not empty...........1\n");
                    	host->mrq->data->error = MMC_ERR_FIFO;
                    	goto transfer_closed;
                    }
                    
                    if( CTRUE == MES_SDHC_IsDataTransferBusy(host->channel) )
                    {
                    	dprintk("Data transfer busy...........1\n");
                    	host->mrq->data->error = MMC_ERR_FIFO;
                    	goto transfer_closed;
                    }
					
					
					if(host->complete_what == COMPLETION_XFERFINISH) 
					{
						host->mrq->cmd->error = MMC_ERR_NONE;
						host->mrq->data->error = MMC_ERR_NONE;
						
						p->RINTSTS = 0xFFFFFFFF; // all interrupt pend clear
				        
						goto transfer_closed;
					}
				}// last bytes process
				
			} // SDI_INTSTS_RXDR
			
			
			// Data Transfer Over, 에러가 생겨도 Data transfer completed 일 경우 생김. Data Timeout 시에도 이비트는 체크됨
			// 이쒸....==> 마지막 32bytes의 인터럽트일때는 이것이 떨어진다. 위의 SDI_INTSTS_RXDR이 떨어지는게 아니라....ㅠ.ㅠ
			if( sdi_mint & SDI_INTSTS_DTO )
            {
            	if( (host->length - host->bytes_process) >= 64 ) // 맨 마지막일때 발생한다, 즉 32bytes만 남아있을 경우 이녀석 발생함 해서 64일 경우만 에러처리
            	{
                	dprintk("ERROR : MES_SDCARD_ReadSectors -MES_SDHC_INT_DTO fifocount = %d\n", MES_SDHC_GetFIFOCount(host->channel) * 4);
                	dprintk("ERROR : MES_SDCARD_ReadSectors dwReadSize = %d, D.TRCBCNT = %d\n", host->length, MES_SDHC_GetDataTransferSize(host->channel) );
                	dprintk("ERROR : MES_SDCARD_ReadSectors dwReadSize = %d, D.TRFBCNT = %d\n", host->length, MES_SDHC_GetFIFOTransferSize(host->channel) );
            
                	dump_sdi(host);
                
			    	host->mrq->data->error = MMC_ERR_FIFO;
					goto transfer_closed;
				}
				else 
				{
			    	for( i=0; i<8; i++ )
            		{
						*((u32 *)(host->buffer + host->bytes_process)) = p->DATA;
						host->bytes_process += 4;
					}
        
					dprintk("bytes_process = %d ==> DTO\n", host->bytes_process);
					gprintk("bytes_process = %d ==> DTO\n", host->bytes_process);
					
					if( host->length == host->bytes_process )
					{
                        if( CFALSE == MES_SDHC_IsFIFOEmpty(host->channel) )
                        {
                        	dprintk("Fifo is not empty...........1\n");
                        	host->mrq->data->error = MMC_ERR_FIFO;
                        	goto transfer_closed;
                        }
                        
                        if( CTRUE == MES_SDHC_IsDataTransferBusy(host->channel) )
                        {
                        	dprintk("Data transfer busy...........1\n");
                        	host->mrq->data->error = MMC_ERR_FIFO;
                        	goto transfer_closed;
                        }
						
						
						if(host->complete_what == COMPLETION_XFERFINISH) 
						{
							host->mrq->cmd->error = MMC_ERR_NONE;
							host->mrq->data->error = MMC_ERR_NONE;
							
							p->RINTSTS = 0xFFFFFFFF; // all interrupt pend clear
					        
							goto transfer_closed;
						}
					}// last bytes process
				}
			} // SDI_INTSTS_DTO
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
						p->RINTSTS = 0xFFFFFFFF; // all interrupt pend clear
				        
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
	
	dprintk("host->mrq->cmd->opcode = 0x%x: %d\n", host->mrq->cmd->opcode, host->mrq->cmd->opcode);
    switch( command )
    {
    case GO_IDLE_STATE:        // No Response(이 녀석은 원래 mmc/sd 명령에 없다) 
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_SENDINIT |
            MES_SDHC_CMDFLAG_WAITPRVDAT;
		//dprintk("go........idle......state\n");            
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
    // ghcstop: 이 두개는 mmc쪽 명령과 동일하다. 처리도 동일하기 때문에 그냥 없앤다.
    //case SET_BUS_WIDTH:        // R1
    //case SD_STATUS:            // R1
    case SEND_SCR:             // R1, ghcstop fix for debugging
    	command = ((APP_CMD<<8) | 51);
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_WAITPRVDAT |
            MES_SDHC_CMDFLAG_CHKRSPCRC | MES_SDHC_CMDFLAG_SHORTRSP;
        //printk("send......scr\n");
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
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_RXDATA;
        dprintk("=================================== multiple_block, arg = %d\n", host->mrq->cmd->arg );
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
            MES_SDHC_CMDFLAG_BLOCK | MES_SDHC_CMDFLAG_TXDATA;
        break;
    case STOP_TRANSMISSION:    // R1
        flag =
            MES_SDHC_CMDFLAG_STARTCMD | MES_SDHC_CMDFLAG_CHKRSPCRC |
            MES_SDHC_CMDFLAG_SHORTRSP | MES_SDHC_CMDFLAG_STOPABORT;
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
	u32    timeout;
	struct mmc_request mrq_s;
	
	
	//hyun_debug 
  	if(host->cd_state != SD_INSERTED) mrq->cmd->retries = 0;
    
    host->mrq = mrq;
START_CMD: 	
	host->cmdflags      = 0;
	host->complete_what = COMPLETION_CMDDONE; 
	ret = sdi_set_command_flag(host);
	if(ret < 0 )
	{
		dprintk("Command error occurred\n");
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

	p->RINTSTS = 0xFFFFFFFF; // all interrupt pend clear
	p->INTMASK = sdi_imsk;   // interrupt enable setting(1로 write한 것은 enable)
	p->CMDARG  = host->mrq->cmd->arg;
	p->CMD     = (host->mrq->cmd->opcode & 0xff)|(host->cmdflags);

    timeout = 3000; // 3000ms = 3sec
	ret = wait_for_completion_timeout(&host->complete_request, msecs_to_jiffies(timeout));
    if(!ret) 
	{
		printk("Timeout occurred during waiting for the interrupt\n");
		host->mrq->cmd->error = MMC_ERR_TIMEOUT;
		ret = -MMC_ERR_TIMEOUT;
		goto request_done;
	}
    
 
    //Cleanup controller
	p->RINTSTS = 0xFFFFFFFF; // all interrupt pend clear
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
#if 0
	    mmc_wait_for_cmd(mmc, mrq->data->stop, 3);
#else
        mrq_s.cmd = mrq->data->stop;
        mrq_s.cmd->retries = 0;
        mrq_s.data = NULL;
        host->mrq = &mrq_s;
        goto START_CMD;
#endif    
    
    }
	
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
	dprintk("+SetClock\n");
	
	// 1. Confirm that no card is engaged in any transaction.
	//    If there's a transaction, wait until it finishes.
//	while( MES_SDHC_IsDataTransferBusy() );
//	while( MES_SDHC_IsCardDataBusy() );
	if(  MES_SDHC_IsCardDataBusy(channel) )
	{
		dprintk("MES_SDCARD_SetClock : ERROR - Data is busy\n");
		return CFALSE;
	}
	
	if( MES_SDHC_IsDataTransferBusy(channel) )
	{
		dprintk("MES_SDCARD_SetClock : ERROR - Data Transfer is busy\n");
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
			dprintk("MES_SDCARD_SetClock : ERROR - TIme-out to update clock.\n");
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
			dprintk("MES_SDCARD_SetClock : ERROR - TIme-out to update clock2.\n");
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
	//unsigned long pclk = pollux_get_pclk();
	//unsigned long sdclk = 0;
    
#if 0	
	if (ios->power_mode == MMC_POWER_OFF) 
	{
		dprintk("MMC_POWER_OFF pre\n");
		MES_SDHC_SetOutputClockEnable(host->channel, CFALSE);
		//mdelay(100);
		dprintk("MMC_POWER_OFF aft\n");
	}
	else if (ios->power_mode == MMC_POWER_ON) 
	{
		dprintk("MMC_POWER_ON\n");
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
	dprintk("bus_width = %d\n", ios->bus_width);
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
	
	//host->readonly = WRITE_ENABLED; // just fix
	host->readonly =  pollux_gpio_getpin(host->wppin); // low면 write protected, high면 write enable ==> low active
	
	gprintk("Read only state: %d (1 if write protected)\n", host->readonly);
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

	new_cd_state = pollux_gpio_getpin(host->cdpin); // low/high인지 읽어냄
	aprintk("host->cd_state = %s, New card state: %d\n", (host->cd_state)?"Inserted":"Removed", new_cd_state);
	
	if( host->cd_state == SD_REMOVED && new_cd_state == 0) // removed/low ==> inserted
	{
		aprintk("CD IRQ insert =====================> change to rising\n");
		host->cd_state = SD_INSERTED;		
		set_irq_type(irq, IRQT_RISING);	/* Current GPIO value is low: wait for raising high */
		schedule_work(&host->card_detect_work);
	}
    else if( host->cd_state == SD_INSERTED && new_cd_state == 1) // inserted/high ==> removed
	{
		aprintk("CD IRQ eject  =====================> change to falling\n");
    	host->cd_state = SD_REMOVED;
    	set_irq_type(irq, IRQT_FALLING);	/* Current GPIO value is hight: wait for raising low */
		schedule_work(&host->card_detect_work);
    }
    else
    {
    	aprintk("CD IRQ - Unexpected state\n");
    	if( new_cd_state == 1 ) // high 상태이면
    		set_irq_type( irq, IRQT_FALLING); // falling으로 바꾼다.
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

	aprintk("Card detect state changed\n");
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
	dprintk("sdi: 1\n");
	mmc = mmc_alloc_host(sizeof(struct pollux_sdi_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_out;
	}
	
	printk(KERN_INFO "======================mmc: %p, private: %p, sizeof(private) = %d\n", mmc, &(mmc->private), sizeof(mmc->private) );


	/*
	 * Set host specific pointer to allocated space
	 */
	dprintk("sdi: 2\n");
	host = mmc_priv(mmc);
	
	printk(KERN_INFO "==================host: %p\n", host);


	/* 
	 * Initialize host specific data
	 */
	dprintk("sdi: 3\n");
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
	
#if 0	
	aprintk("pdev.id = %d\n", pdev->id);
	host->channel           = 0; // sdmmc channel 0로 일단 세팅 
#else
	aprintk("pdev.id = %d\n", pdev->id);
	host->channel           = pdev->id; // sdmmc channel 0로 일단 세팅 
#endif	
	
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
	gprintk("cd gpio = %d\n", res->start );
	host->cdpin = (int)res->start;
	
	
	res = platform_get_resource(pdev, IORESOURCE_IO, 1); // IO resource #1, write protection gpio number
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get resources 3\n");
		ret = -EINVAL;
		goto probe_out;
	}
	gprintk("wp gpio = %d\n", res->start );
	host->wppin = (int)res->start;


	/*
	 * Get SD/MMC controller IRQ
	 */
	dprintk("sdi: 5 \n");	
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
	aprintk("SDI___________________IRQ_CD = %d\n", host->irq_cd );
	if( host->irq_cd == 0 ) 
	{
		printk(KERN_INFO "failed to get card detection interrupt resource.\n");
		ret = -EINVAL;
		goto probe_out;
	}


	/*
	 * Initialize SD controller prototype  from sdmmc.cpp of wince bsp
	 */
	
	MES_SDHC_Initialize(); //mes_sdhc.c 의 해당 레지스터 block 초기화 루틴, 두번 호출이 안되도록 세팅이 되어 있음.
	
#if 1	
	/*
	 * Initialize MP2530F SDI
	 */
	ret = sdmmc_init(host);
	if(ret != 0)
	{
		ret = -EINVAL;
		goto probe_out;
	}
#endif	

//return 0; // ghcstop for debugging



	/*
	 * Register handler for host interupt
	 */
	dprintk("sdi: 6 - SD IRQ number is %d\n", host->irq);
	if(request_irq(host->irq, pollux_sdi_irq, 0, POLLUX_SDI0_NAME, host)) 
	{
		printk(KERN_INFO "failed to request sdi interrupt.\n");
		ret = -ENOENT;
		goto probe_out;
	}


	/*
	 * Read current card state 
	 */

	host->readonly =  pollux_gpio_getpin(host->wppin); // low면 write protected, high면 write enable ==> low active
	host->cd_state = !pollux_gpio_getpin(host->cdpin); // low면 Inserted, high면 ejected. ==> 그러므로 !로 뒤집음
	
    gprintk("host->readonly = %d, host->cd_state = %d, pollux_get_gpio_func(host->cdpin) = %d\n", host->readonly, host->cd_state, pollux_get_gpio_func(host->cdpin) );

	/*
	 * Register interrupt handler for card detection 
	 */
	INIT_WORK(&host->card_detect_work, pollux_sdi_card_detect);
#if 1
	if( host->cd_state ) // inserted
		set_irq_type(host->irq_cd, IRQT_RISING);
	else
		set_irq_type( host->irq_cd, IRQT_FALLING);
		
		
	//if(request_irq(host->irq_cd, pollux_sdi_irq_cd, 0, POLLUX_SDI0_NAME, host)) 
	if(request_irq(host->irq_cd, pollux_sdi_irq_cd, 0, host->hname, host)) 
	{
		printk(KERN_WARNING "failed to request card detect interrupt.\n" );
		ret = -ENOENT;
		goto probe_free_irq;
	}
#endif


	/*
	 * Register MP2530F host driver
	 */
	dprintk("sdi: 8\n");
	if((ret = mmc_add_host(mmc)))
	{
		printk(KERN_INFO "failed to add mmc host.\n");
		goto free_resource;
	}

	platform_set_drvdata(pdev, mmc);

	printk(KERN_INFO "initialization done.\n");
	
	
	//mmc_detect_change(host->mmc, 100); // number of jiffies to wait before queueing	

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
	dprintk("sdi: 1\n");
	mmc = mmc_alloc_host(sizeof(struct pollux_sdi_host), &pbdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_out;
	}
	
	printk(KERN_INFO "======================mmc: %p, private: %p, sizeof(private) = %d\n", mmc, &(mmc->private), sizeof(mmc->private) );


	/*
	 * Set host specific pointer to allocated space
	 */
	dprintk("sdi: 2\n");
	host = mmc_priv(mmc);
	
	printk(KERN_INFO "==================host: %p\n", host);


	/* 
	 * Initialize host specific data
	 */
	dprintk("sdi: 3\n");
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
	
#if 0	
	aprintk("pdev.id = %d\n", pdev->id);
	host->channel           = 0; // sdmmc channel 0로 일단 세팅 
#else
	aprintk("pdev.id = %d\n", pbdev->id);
	host->channel           = pbdev->id; // sdmmc channel 0로 일단 세팅 
#endif	
	
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
	gprintk("cd gpio = %d\n", res->start );
	host->cdpin = (int)res->start;
	
	
	res = platform_get_resource(pbdev, IORESOURCE_IO, 1); // IO resource #1, write protection gpio number
	if( res == 0 )
	{
		printk("POLLUX_SDI: error......get resources 3\n");
		ret = -EINVAL;
		goto probe_out;
	}
	gprintk("wp gpio = %d\n", res->start );
	host->wppin = (int)res->start;


	/*
	 * Get SD/MMC controller IRQ
	 */
	dprintk("sdi: 5 \n");	
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
	aprintk("SDI___________________IRQ_CD = %d\n", host->irq_cd );
	if( host->irq_cd == 0 ) 
	{
		printk(KERN_INFO "failed to get card detection interrupt resource.\n");
		ret = -EINVAL;
		goto probe_out;
	}


	/*
	 * Initialize SD controller prototype  from sdmmc.cpp of wince bsp
	 */
	
	MES_SDHC_Initialize(); //mes_sdhc.c 의 해당 레지스터 block 초기화 루틴, 두번 호출이 안되도록 세팅이 되어 있음.
	
#if 1	
	/*
	 * Initialize MP2530F SDI
	 */
	ret = sdmmc_init(host);
	if(ret != 0)
	{
		ret = -EINVAL;
		goto probe_out;
	}
#endif	

//return 0; // ghcstop for debugging



	/*
	 * Register handler for host interupt
	 */
	dprintk("sdi: 6 - SD IRQ number is %d\n", host->irq);
	if(request_irq(host->irq, pollux_sdi_irq, 0, POLLUX_SDI0_NAME, host)) 
	{
		printk(KERN_INFO "failed to request sdi interrupt.\n");
		ret = -ENOENT;
		goto probe_out;
	}


	/*
	 * Read current card state 
	 */

	host->readonly =  pollux_gpio_getpin(host->wppin); // low면 write protected, high면 write enable ==> low active
	host->cd_state = !pollux_gpio_getpin(host->cdpin); // low면 Inserted, high면 ejected. ==> 그러므로 !로 뒤집음
	
	
	gprintk("host->readonly = %d, host->cd_state = %d, pollux_get_gpio_func(host->cdpin) = %d\n", host->readonly, host->cd_state, pollux_get_gpio_func(host->cdpin) );

	/*
	 * Register interrupt handler for card detection 
	 */
	INIT_WORK(&host->card_detect_work, pollux_sdi_card_detect);
#if 1
	if( host->cd_state ) // inserted
		set_irq_type(host->irq_cd, IRQT_RISING);
	else
		set_irq_type( host->irq_cd, IRQT_FALLING);
		
		
	//if(request_irq(host->irq_cd, pollux_sdi_irq_cd, 0, POLLUX_SDI0_NAME, host)) 
	if(request_irq(host->irq_cd, pollux_sdi_irq_cd, 0, host->hname, host)) 
	{
		printk(KERN_WARNING "failed to request card detect interrupt.\n" );
		ret = -ENOENT;
		goto probe_free_irq;
	}
#endif


	/*
	 * Register MP2530F host driver
	 */
	dprintk("sdi: 8\n");
	if((ret = mmc_add_host(mmc)))
	{
		printk(KERN_INFO "failed to add mmc host.\n");
		goto free_resource;
	}

	platform_set_drvdata(pbdev, mmc);

	printk(KERN_INFO "initialization done.\n");
	
	
	//mmc_detect_change(host->mmc, 100); // number of jiffies to wait before queueing	

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
	printk(KERN_INFO "MP2530F SD/MMC Driver - Removing\n");
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


//#if 0
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


//#else
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
	dprintk(KERN_INFO "MagicEyes Pollux SD/MMC Interface driver\n");

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
	
#if 0	
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
	dprintk(KERN_INFO "MagicEyes Pollux SD/MMC Interface driver: EXIT\n");
	platform_driver_unregister(&pollux_sdi0_driver);
	
	platform_driver_unregister(&pollux_sdi1_driver);
}


module_init(pollux_sdi_init);
module_exit(pollux_sdi_exit);

MODULE_DESCRIPTION("MagicEyes MP2530F SD/MMC Interface driver");
MODULE_LICENSE("GPL");

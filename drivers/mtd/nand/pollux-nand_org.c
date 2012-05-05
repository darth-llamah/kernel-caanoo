/*
 * drivers/mtd/nand/pollux.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This is a device driver for the NAND flash device found on the
 *   MP2530F NRK board which utilizes a Samsung 8 Mbyte part.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/delay.h>


#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>

#include <asm/arch/regs-gpio.h>
#define BD_VER_LSB              POLLUX_GPC18
#define BD_VER_MSB             POLLUX_GPC17

#define MTD0_SIZE		16
#define MTD1_SIZE		128
#define MTD2_SIZE		2560
#define MTD4_SIZE		32
#define MTD5_SIZE		32
#define BED_TABLE_SIZE		40
#define MTD3_SIZE		( 4095 - ( MTD0_SIZE + MTD1_SIZE + MTD2_SIZE + MTD4_SIZE + MTD5_SIZE + BED_TABLE_SIZE ) )

#undef GDEBUG
//#define GDEBUG
#ifdef GDEBUG
	#define gprintk(fmt, x... ) printk( "%s: " fmt, __FUNCTION__ , ## x)
#else
	#define gprintk(x...) do { } while (0)
#endif

#undef WINCE_ECC				


typedef volatile struct _tag_NAND_ECC 
{
	volatile u32	reg_NFCONTROL;			// 0x5874
	volatile u32	reg_NFECCL;				// 0x5878
	volatile u32	reg_NFECCH;				// 0x587c
	volatile u32	reg_NFORGECCL;			// 0x5880
	volatile u32	reg_NFORGECCH;			// 0x5884
	volatile u32	reg_NFCNT;				// 0x5888
	volatile u32	reg_NFECCSTATUS;		// 0x588c
	volatile u32	reg_NFSYNDRONE31;		// 0x5890
	volatile u32	reg_NFSYNDRONE75;		// 0x5894
} NANDECC , *PNANDECC;

extern short BCH_AlphaToTable[8192];
extern short BCH_IndexOfTable[8192];

#define NAND_WRENCDONE		0x1
#define NAND_RDDECDONE		0x2
#define NAND_RDCHECKERR		0x4

volatile 	PNANDECC		pECC;


#define CLEAR_RnB()		pECC->reg_NFCONTROL |= 0x8000	
#define CHECK_RnB()		while(1)								\
	{															\
		if (pECC->reg_NFCONTROL & 0x8000)							\
		{														\
			CLEAR_RnB();										\
			break;												\
		}														\
	}		
#define NF_WAITRB()		{ while(!(pECC->reg_NFCONTROL & 0x8000)); }		//add by bnjang [2008.12.31]

//#define NF_CLEARECC()                       { pECC->reg_NFCONTROL |= 0x0800; }	//add by bnjang [2009. 02. 02]
#define NF_CLEARECC() { pECC->reg_NFCONTROL | 0x0800; udelay(1); }

static int getDeviceVersing(void);

int cur_ecc_mode = 0;
//board version 20090131 [bnjang]
int boardVer = 0;


void ReadOddSyndrome(int  *s)
{
    int syn_val13, syn_val57;
    
    syn_val13 = pECC->reg_NFSYNDRONE31;
    syn_val57 = pECC->reg_NFSYNDRONE75;

    s[0] = syn_val13 & 0x1fff;
    s[2] = (syn_val13>>13) & 0x1fff;
    s[4] = syn_val57 & 0x1fff;
    s[6] = (syn_val57>>13) & 0x1fff;
}

//------------------------------------------------------------------------------
int		MES_NAND_GetErrorLocation(  int  *s, int *pLocation )
{
	register int i, j, elp_sum ;
	int count;
	int r;				// Iteration steps
	int Delta; 			// Discrepancy value
	int elp[8+1][8+2]; 	// Error locator polynomial (ELP)
	int L[8+1];			// Degree of ELP 
	int B[8+1][8+2];	// Scratch polynomial
	int reg[8+1];		// Register state


	// Even syndrome = (Odd syndrome) ** 2
	for( i=1,j=0 ; i<8 ; i+=2, j++ )
	{ 	
		if( s[j] == 0 )		s[i] = 0;
		else				s[i] = BCH_AlphaToTable[(2 * BCH_IndexOfTable[s[j]]) % 8191];
	}
	
	// Initialization of elp, B and register
	for( i=0 ; i<=8 ; i++ )
	{	
		L[i] = 0 ;
		for( j=0 ; j<=8 ; j++ )
		{	
			elp[i][j] = 0 ;
			B[i][j] = 0 ;
		}
	}

	for( i=0 ; i<=4 ; i++ )
	{	
		reg[i] = 0 ;
	}
	
	elp[1][0] = 1 ;
	elp[1][1] = s[0] ;

	L[1] = 1 ;
	if( s[0] != 0 )
		B[1][0] = BCH_AlphaToTable[(8191 - BCH_IndexOfTable[s[0]]) % 8191];
	else
		B[1][0] = 0;
	
	for( r=3 ; r<=8-1 ; r=r+2 )
	{	
		// Compute discrepancy
		Delta = s[r-1] ;
		for( i=1 ; i<=L[r-2] ; i++ )
		{	
			if( (s[r-i-1] != 0) && (elp[r-2][i] != 0) )
				Delta ^= BCH_AlphaToTable[(BCH_IndexOfTable[s[r-i-1]] + BCH_IndexOfTable[elp[r-2][i]]) % 8191];
		}
		
		if( Delta == 0 )
		{	
			L[r] = L[r-2] ;
			for( i=0 ; i<=L[r-2] ; i++ )
			{	
				elp[r][i] = elp[r-2][i];
				B[r][i+2] = B[r-2][i] ;
			}
		}
		else
		{	
			// Form new error locator polynomial
			for( i=0 ; i<=L[r-2] ; i++ )
			{	
				elp[r][i] = elp[r-2][i] ;
			}

			for( i=0 ; i<=L[r-2] ; i++ )
			{	
				if( B[r-2][i] != 0 )
					elp[r][i+2] ^= BCH_AlphaToTable[(BCH_IndexOfTable[Delta] + BCH_IndexOfTable[B[r-2][i]]) % 8191];
			}
			
			// Form new scratch polynomial and register length
			if( 2 * L[r-2] >= r )
			{	
				L[r] = L[r-2] ;
				for( i=0 ; i<=L[r-2] ; i++ )
				{	
					B[r][i+2] = B[r-2][i];
				}
			}
			else
			{	
				L[r] = r - L[r-2];
				for( i=0 ; i<=L[r-2] ; i++ )
				{	
					if( elp[r-2][i] != 0 )
						B[r][i] = BCH_AlphaToTable[(BCH_IndexOfTable[elp[r-2][i]] + 8191 - BCH_IndexOfTable[Delta]) % 8191];
					else
						B[r][i] = 0;
				}
			}
		}
	}
	
	if( L[8-1] > 4 )
	{	
		//return L[8-1];
		return -1;
	}
	else
	{	
		// Chien's search to find roots of the error location polynomial
		// Ref: L&C pp.216, Fig.6.1
		for( i=1 ; i<=L[8-1] ; i++ )
			reg[i] = elp[8-1][i];
		
		count = 0;
		for( i=1 ; i<=8191 ; i++ )
		{ 	
			elp_sum = 1;
			for( j=1 ; j<=L[8-1] ; j++ )
			{	
				if( reg[j] != 0 )
				{ 	
					reg[j] = BCH_AlphaToTable[(BCH_IndexOfTable[reg[j]] + j) % 8191] ;
					elp_sum ^= reg[j] ;
				}
			}
			
			if( !elp_sum )		// store root and error location number indices
			{ 	
				// Convert error location from systematic form to storage form 
				pLocation[count] = 8191 - i;
				
				if (pLocation[count] >= 52)
				{	
					// Data Bit Error
					pLocation[count] = pLocation[count] - 52;
					pLocation[count] = 4095 - pLocation[count];
				}
				else
				{
					// ECC Error
					pLocation[count] = pLocation[count] + 4096;
				}

				if( pLocation[count] < 0 )	return -1;

				count++;
			}
		}
		
		if( count == L[8-1] )	// Number of roots = degree of elp hence <= 4 errors
		{	
			return L[8-1];
		}
		else	// Number of roots != degree of ELP => >4 errors and cannot solve
		{	
			return -1;
		}
	}
}




/*
 * returns 0 if the nand is busy, 1 if it is ready
 */
static int pollux_nand_devready(struct mtd_info *mtd)
{
	if (pECC->reg_NFCONTROL & 0x8000) // RnB irq pend?
	{
		//gprintk("dev_ready\n");
		CLEAR_RnB(); // pend clear
		return 1;
	}
	else 
	{
		//gprintk("dev_busy\n");
		return 0;
	}
			
	return 1;
}


void pollux_clear_RnB(void)
{
	gprintk("clear_rnb\n");
	CLEAR_RnB(); // pend clear
}




/*
 * Function for checking ECCEncDone
 */
static void pollux_nand_wait_enc(void)
{
    unsigned long timeo = jiffies;
 
    timeo += 64;    /* when Hz=200,  jiffies interval 1/200=5mS, waiting for 80mS  80/5 = 16 */
 
    /* Apply this short delay always to ensure that we do wait tWB in
     * any case on any machine. */

    while (time_before(jiffies, timeo)) 
    {
		if( (pECC->reg_NFECCSTATUS & NAND_WRENCDONE) )
			break;
		cond_resched();
    }
}

/*
 * Function for checking ECCDecDone
 */
static void pollux_nand_wait_dec(void)
{
	unsigned long timeo = jiffies;
 
    timeo += 64;    /* when Hz=200,  jiffies interval  1/200=5mS, waiting for 80mS  80/5 = 16 */
 
    /* Apply this short delay always to ensure that we do wait tWB in
     * any case on any machine. */

    while (time_before(jiffies, timeo)) 
    {			
		if( (pECC->reg_NFECCSTATUS & (NAND_RDDECDONE)) )
			break;
		cond_resched();
    }
}

static int pollux_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf)
{
	int      i, j;
	int      eccsize = chip->ecc.size;  // 512
	uint8_t *p = buf; // data buffer
	unsigned int *p_oob;
	int col = 0;
	unsigned int *pdwSectorAddr;

	col = mtd->writesize;
	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize); // spare area read;

	p_oob = (chip->oob_poi+32);		//warning: assignment from incompatible pointer type		

	col = 0;
	
	for(  i=0; i<4; i++ ) // 2k page, p는 512단위로 증가
	{
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
		
		pECC->reg_NFORGECCL	= *p_oob++;
		pECC->reg_NFORGECCH	= *p_oob++;
		
       NF_CLEARECC();
		
		chip->read_buf( mtd, p, eccsize );
		pollux_nand_wait_dec(); // ecc 계산될 동안 대기
		
		if( pECC->reg_NFECCSTATUS & NAND_RDCHECKERR ) // error check 했는데, error가 있을 경우는
		{
			int dSyndrome[8];
			int ErrCnt, ErrPos[4];

		    ReadOddSyndrome(dSyndrome);
		    pdwSectorAddr = (unsigned int *)p;

		    ErrCnt = MES_NAND_GetErrorLocation( dSyndrome, ErrPos );

		    for( j=0 ; j<ErrCnt ; j++ )
		    {
		    	pdwSectorAddr[ErrPos[j]/32] ^= 1<<(ErrPos[j]%32);
 		    }
		}
		p   += eccsize;
		col += eccsize;
	}
	
	return 0;
}

static int pollux_read_page_hwecc_4K(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf)
{
	int      i, j;
	int      eccsize = chip->ecc.size;  // 512
	uint8_t *p = buf; // data buffer
	unsigned int *p_oob;
	int col = 0;
	unsigned int *pdwSectorAddr;

	col = mtd->writesize;
	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize); // spare area read;

	p_oob = (chip->oob_poi+64);		//warning: assignment from incompatible pointer type

	col = 0;
	
	for(  i=0; i<8; i++ ) // 4k page, p는 512단위로 증가
	{
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
		
		pECC->reg_NFORGECCL	= *p_oob++;
		pECC->reg_NFORGECCH	= *p_oob++;
		
       NF_CLEARECC();

		chip->read_buf( mtd, p, eccsize );
		pollux_nand_wait_dec(); // ecc 계산될 동안 대기
		
		if( pECC->reg_NFECCSTATUS & NAND_RDCHECKERR ) // error check 했는데, error가 있을 경우는
		{
			int dSyndrome[8];
			int ErrCnt, ErrPos[4];

		    ReadOddSyndrome(dSyndrome);
		    pdwSectorAddr = (unsigned int *)p;

		    ErrCnt = MES_NAND_GetErrorLocation( dSyndrome, ErrPos );

		    for( j=0 ; j<ErrCnt ; j++ )
		    {
		    	pdwSectorAddr[ErrPos[j]/32] ^= 1<<(ErrPos[j]%32);
 		    }
		}
		p   += eccsize;
		col += eccsize;
	}
	
	return 0;
}

// you must randomin command per 512/after write
void pollux_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	int	i; 
	int	eccsize = chip->ecc.size;  // 512
	uint8_t *p = buf; // data buffer	warning: initialization discards qualifiers from pointer target type
	int	col = 0;
	unsigned int *p_oob;
	
 	CLEAR_RnB();

	/* Send command to begin page programming */
	chip->cmdfunc(mtd, NAND_CMD_RNDIN, col, -1);

	p_oob = (chip->oob_poi+32);		//warning: assignment from incompatible pointer type

	for( i=0; i<4; i++) // 2k page, p는 512단위로 증가
	{
		NF_CLEARECC();		
		chip->write_buf(mtd, p, eccsize);
		
		pollux_nand_wait_enc();
		
        *p_oob++ = pECC->reg_NFECCL;
        *p_oob++ = pECC->reg_NFECCH;
		
		p   += eccsize;
		col += eccsize;
		
		chip->cmdfunc(mtd, NAND_CMD_RNDIN, col, -1);
	}
	
//    for (i = 0; i < 32; i++)
//		chip->oob_poi[i] = 0xff;
	chip->oob_poi[0] = 0xff;
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
	
	//col = mtd->writesize;

#if 1
	#if 1	
	col = mtd->writesize+mtd->oobsize;
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, col, -1);
    #else	
	/* Send command actually program the data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
    #endif
#endif

	NF_WAITRB();	//bnjang [2008.12.31]
}

void pollux_nand_write_page_4K(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	int	i; 
	int	eccsize = chip->ecc.size;  // 512
	uint8_t *p = buf; // data buffer	warning: initialization discards qualifiers from pointer target type
	int	col = 0;
	unsigned int *p_oob;
	
 	CLEAR_RnB();

	/* Send command to begin page programming */
	chip->cmdfunc(mtd, NAND_CMD_RNDIN, col, -1);
	p_oob = (chip->oob_poi+64);		//warning: assignment from incompatible pointer type	

	for( i=0; i<8; i++) // 4k page, p는 512단위로 증가
	{
		NF_CLEARECC();
		chip->write_buf(mtd, p, eccsize);

		pollux_nand_wait_enc();

		*p_oob++ = pECC->reg_NFECCL;
       *p_oob++ = pECC->reg_NFECCH;
	
		p   += eccsize;
		col += eccsize;
		
		chip->cmdfunc(mtd, NAND_CMD_RNDIN, col, -1);
	}
	
//    for (i = 0; i < 32; i++)
//		chip->oob_poi[i] = 0xff;
	chip->oob_poi[0] = 0xff;
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
	
	//col = mtd->writesize;

#if 1
	#if 1	
	col = mtd->writesize+mtd->oobsize;
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, col, -1);
    #else	
	/* Send command actually program the data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
    #endif
#endif

	NF_WAITRB();	//bnjang [2008.12.31]
}


static struct mtd_info *pollux_mtd = NULL;

#ifdef CONFIG_MTD_PARTITIONS
	#ifdef CONFIG_MTD_NAND_SLCMAP_POLLUX
		#define NUM_PARTITIONS	3
		static const char *part_probes[] = { "cmdlinepart", NULL };
		struct mtd_partition partition_info32[] = {
			[0] = {
				.name	= "Boot  Partition",
				.size	= (SZ_1M),					// 1M: nandboot/u-boot 
				.offset	= 0x0,						// BootLoad + Config = 240K + 16K = 256Kbyte
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
			[1] = {
				.name	= "Data0 Partition",
				.size	= 20*SZ_1M,
				.offset	= MTDPART_OFS_APPEND, 		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.	
			},
			[2] = {
				.name	= "Data1 Partition",
				.size	= MTDPART_SIZ_FULL,         // device의 끝까지 지정을 한다.
				.offset	= MTDPART_OFS_APPEND,		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
		};
	#else
		/*
		#define NUM_PARTITIONS	3
		static const char *part_probes[] = { "cmdlinepart", NULL };
		struct mtd_partition partition_info32[] = {
			[0] = {
				.name	= "Boot  Partition",
				.size	= (SZ_1M),					// 1M: nandboot/u-boot 
				.offset	= 0x0,						// BootLoad + Config = 240K + 16K = 256Kbyte
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
			[1] = {
				.name	= "Data0 Partition",
				.size	= 64*SZ_1M,
				.offset	= MTDPART_OFS_APPEND, 		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.	
			},
			[2] = {
				.name	= "Data1 Partition",
				.size	= MTDPART_SIZ_FULL,         // device의 끝까지 지정을 한다.
				.offset	= MTDPART_OFS_APPEND,		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
		};
		*/
		
		// ubi test total partition
		/*#define NUM_PARTITIONS	1
		static const char *part_probes[] = { "cmdlinepart", NULL };
		struct mtd_partition partition_info32[] = {
			[0] = {
				.name	= "data",
				.size	= MTDPART_SIZ_FULL,         // device의 끝까지 지정을 한다.
				.offset	= MTDPART_OFS_APPEND,		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
		};
		*/
		
		
		// ubi test 2 partition ==> ubi/ubifs done
		/* 
		#define NUM_PARTITIONS	2
		static const char *part_probes[] = { "cmdlinepart", NULL };
		struct mtd_partition partition_info32[] = {
			[0] = {
				.name	= "d0",
				.size	= 128*(SZ_1M),					// 1M: nandboot/u-boot 
				.offset	= 0x0,						// BootLoad + Config = 240K + 16K = 256Kbyte
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
			[1] = {
				.name	= "d1",
				.size	= MTDPART_SIZ_FULL,         // device의 끝까지 지정을 한다.
				.offset	= MTDPART_OFS_APPEND,		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
		};
		*/

		// ubi test 3 partition ==> ubi/ubifs/ubiblk done
		/*#define NUM_PARTITIONS	3
		static const char *part_probes[] = { "cmdlinepart", NULL };
		struct mtd_partition partition_info32[] = {
			[0] = {
				.name	= "d0",
				.size	= 128*(SZ_1M),					// 1M: nandboot/u-boot 
				.offset	= 0x0,						// BootLoad + Config = 240K + 16K = 256Kbyte
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
			[1] = {
				.name	= "d1",
				.size	= 256*SZ_1M,
				.offset	= MTDPART_OFS_APPEND, 		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.	
			},
			[2] = {
				.name	= "d2",
				.size	= MTDPART_SIZ_FULL,         // device의 끝까지 지정을 한다.
				.offset	= MTDPART_OFS_APPEND,		// 이것을 넣어주어야 연속해서 맵이 지정된다.
				.mask_flags = 0, 					// 0 으로 해주어야 쓰기 금지가 풀린다.
			},
		};
		*/
		
		#define NUM_PARTITIONS	6
		static const char *part_probes[] = { "cmdlinepart", NULL };
		struct mtd_partition partition_info32[] = {
			[0] = {
				.name	= "d0",
				.size	= MTD0_SIZE*(SZ_1M),         // 4M boot
				.offset	= 0x0,			
				.mask_flags = MTD_WRITEABLE,        // read-only area		
			},
			[1] = {
				.name	= "d1",
				.size	= MTD1_SIZE*(SZ_1M),       // 128M bytes
				.offset	= MTDPART_OFS_APPEND,			
				.mask_flags = 0, 		
			},
			[2] = {
				.name	= "d2",
				.size	= MTD2_SIZE*(SZ_1M),  
				.offset	= MTDPART_OFS_APPEND,
				.mask_flags = 0, 
			},
			[3] = { 
				.name = "d3",
				.size = MTD3_SIZE*(SZ_1M),
				.offset = MTDPART_OFS_APPEND,
				.mask_flags = 0,
			},		
			[4] = { 
				.name = "d4",
				.size = MTD4_SIZE*(SZ_1M),
				.offset = MTDPART_OFS_APPEND,
				.mask_flags = 0,
			},		
			[5] = { 
				.name = "d5",
				.size = MTD5_SIZE*(SZ_1M),
				.offset = MTDPART_OFS_APPEND,
				.mask_flags = 0,
			},		
		};
		
	#endif // MLC
#endif


#define MASK_CLE	0x10
#define MASK_ALE	0x18

static void pollux_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	void __iomem* addr = chip->IO_ADDR_W;

	if (cmd == NAND_CMD_NONE)
		return;

	if(ctrl & NAND_CLE)
		addr += MASK_CLE;
	else if(ctrl & NAND_ALE)
		addr += MASK_ALE;

	//printk("addr/cmd : 0x%x/0x%x\n",addr,cmd);
	writeb(cmd, addr);
}

#if 0
static struct nand_ecclayout pollux_nand_oob_mlc_64 = {
	.eccbytes = 32,
	.eccpos = {
		   32, 33, 34, 35, 36, 37, 38, 39,
		   40, 41, 42, 43, 44, 45, 46, 47,
 		   48, 49, 50, 51, 52, 53, 54, 55,
   		   56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 28}}
};
#else
	static struct nand_ecclayout pollux_nand_oob_mlc_64 = {
		.eccbytes = 32,
		.eccpos = {
			 8, 9,10,11,12,13,14,15,
			16,17,18,19,20,21,22,23,
			24,25,26,27,28,29,30,31,
			32,33,34,35,36,37,38,39},
		.oobfree = {{0, 8}}   // file system reserved area지정, 일단 8개는 제낀다. ==> wince랑 맞추자....
	};
	static struct nand_ecclayout pollux_nand_oob_mlc_128 = {
		.eccbytes = 64,
		.eccpos = {
			 8, 9,10,11,12,13,14,15,
			16,17,18,19,20,21,22,23,
			24,25,26,27,28,29,30,31,
			32,33,34,35,36,37,38,39,
			40,41,42,43,44,45,46,47,
			48,49,50,51,52,53,54,55,
			56,57,58,59,60,61,62,63,						
			64,65,66,67,68,69,70,71},
		.oobfree = {{0, 8}}   // file system reserved area지정, 일단 8개는 제낀다. ==> wince랑 맞추자....
	};	
#endif


void pollux_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
}

int pollux_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
	return 0;
}

int pollux_nand_correct_data(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
	return 0;
}


void pollux_select_chip(struct mtd_info *mtd, int chipnr)
{
	//printk("p.....s.....s\n");
}


/*
 * Main initialization routine
 */
static int __init pollux_nand_init(void)
{
	struct nand_chip *this;
	const char *part_type = 0;
	int mtd_parts_nb = 0;
	struct mtd_partition *mtd_parts = 0;
	void __iomem* nandaddr=0;
	
	//boardVer = getDeviceVersing();
	boardVer = 3;
	
#if 1 // for now....just remap
	nandaddr = ioremap(POLLUX_PA_NAND, 0x1000);
#else	
	nandaddr = (void __iomem *)POLLUX_VA_NAND;
#endif	
	if(!nandaddr)
	{
		printk("failed to ioremap nand flash\n");
		return -ENOMEM;
	}

	/* Allocate memory for MTD device structure and private data */
	pollux_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	if (!pollux_mtd) {
		printk("Unable to allocate NAND MTD device structure.\n");
		return -ENOMEM;
	}

	/* Get pointer to private data */
	this = (struct nand_chip *)(&pollux_mtd[1]);

	/* Initialize structures */
	memset(pollux_mtd, 0, sizeof(struct mtd_info));
	memset(this, 0, sizeof(struct nand_chip));

	/* Link the private data with the MTD structure */
	pollux_mtd->priv = this;
	pollux_mtd->owner = THIS_MODULE;
	
	
	pECC = (PNANDECC)POLLUX_VA_NANDECC;
	
	printk(KERN_INFO "[Device Version]>> %d  \n",  boardVer);
	
	/* insert callbacks */
	this->IO_ADDR_R = nandaddr;
	this->IO_ADDR_W = nandaddr;
	this->cmd_ctrl  = pollux_nand_hwcontrol;
	this->dev_ready = pollux_nand_devready;
	this->select_chip = pollux_select_chip;
	//this->dev_ready = NULL;
#if 1	
	this->chip_delay   = 15;
#else	
	this->chip_delay   = 25; //bnjang [2008.12.30]
#endif	

#if 0	
	this->options = 0;
	this->ecc.mode = NAND_ECC_SOFT;
#else
	//this->options        = 0;
	this->options       |= NAND_NO_SUBPAGE_WRITE;	/* NOP = 1 if MLC */
	this->ecc.hwctl		= pollux_nand_enable_hwecc;
	this->ecc.calculate	= pollux_nand_calculate_ecc;
	this->ecc.correct	= pollux_nand_correct_data;
	
	this->ecc.mode		 = NAND_ECC_HW;
	this->ecc.size       = 512;



    this->ecc.bytes      = 64;
	this->ecc.layout     = &pollux_nand_oob_mlc_128	;
	this->ecc.read_page  = pollux_read_page_hwecc_4K;
	this->ecc.write_page = pollux_nand_write_page_4K;		

#endif	


	
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
	printk("Searching for NAND flash...\n");
#endif	
	/* Scan to find existence of the device */
	if (nand_scan(pollux_mtd, 1)) {
		kfree(pollux_mtd);
		return -ENXIO;
	}
#ifdef CONFIG_MTD_PARTITIONS
	pollux_mtd->name = "pollux-nand";
	mtd_parts_nb = parse_mtd_partitions(pollux_mtd, part_probes, &mtd_parts, 0);
	if (mtd_parts_nb > 0)
		part_type = "command line";
	else
		mtd_parts_nb = 0;
#endif
	if (mtd_parts_nb == 0) {
		mtd_parts = partition_info32;
		mtd_parts_nb = NUM_PARTITIONS;
		part_type = "static";
	}

	/* Register the partitions */
#ifdef 	CONFIG_POLLUX_KERNEL_BOOT_MESSAGE_ENABLE
	printk(KERN_NOTICE "Pollux Nand: Using %s partition definition\n", part_type);
#endif	
	add_mtd_partitions(pollux_mtd, mtd_parts, mtd_parts_nb);

	/* Return happy */
	return 0;
}

/*
 * Clean up routine
 */
static void __exit pollux_nand_cleanup(void)
{
	/* Unregister the device */
	del_mtd_device(pollux_mtd);

	/* Free the MTD device structure */
	kfree(pollux_mtd);
}

module_init(pollux_nand_init);
module_exit(pollux_nand_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Claude <claude@mesdigital.com>");
MODULE_DESCRIPTION("NAND MTD map driver for MP2530F NRK board");

static int getDeviceVersing(void)
{
	int bd_rev;
	
	pollux_set_gpio_func(BD_VER_LSB,POLLUX_GPIO_MODE_GPIO);
	pollux_gpio_set_inout(BD_VER_LSB, POLLUX_GPIO_INPUT_MODE);	
	
	pollux_set_gpio_func(BD_VER_MSB,POLLUX_GPIO_MODE_GPIO);
	pollux_gpio_set_inout(BD_VER_MSB, POLLUX_GPIO_INPUT_MODE);	
	
	if( pollux_gpio_getpin(BD_VER_LSB) && (!pollux_gpio_getpin(BD_VER_MSB)) )
		bd_rev = 1;
	else if( (!pollux_gpio_getpin(BD_VER_LSB)) && pollux_gpio_getpin(BD_VER_MSB) )
		bd_rev = 2;
	else if( pollux_gpio_getpin(BD_VER_LSB) && pollux_gpio_getpin(BD_VER_MSB) )
		bd_rev = 3;
	else
		bd_rev = 0;

	return bd_rev;
}

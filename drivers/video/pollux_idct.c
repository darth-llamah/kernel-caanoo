#include <linux/module.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <linux/mutex.h>
#include "pollux_idct.h"

//#define IDCT_BASE_VIRTUAL_ADDRESS	0xc000F800

struct MES_IDCT_RegisterSet  
{
	volatile U32	BUF[32];	// 16bit, 8x8 Data block	
	volatile U32	CONT;
	volatile U32	INT_ENB;
	volatile U32	INT_PEND;
};

static  struct
{
    struct MES_IDCT_RegisterSet *pRegister;

} __g_ModuleVariables[NUMBER_OF_IDCT_MODULE] = { CNULL, };


static  U32 __g_CurModuleIndex = 0;
static  struct MES_IDCT_RegisterSet *__g_pRegister = CNULL;


CBOOL   MES_IDCT_Initialize( void )
{
	static CBOOL bInit = CFALSE;
	U32 i;
	
	if( CFALSE == bInit )
	{
		for( i=0; i < NUMBER_OF_IDCT_MODULE; i++ )
		{
			__g_ModuleVariables[i].pRegister = CNULL;
		}
		
		bInit = CTRUE;
	}
	
	return CTRUE;
}


U32     MES_IDCT_GetNumberOfModule( void )
{
    return NUMBER_OF_IDCT_MODULE;
}


void    MES_IDCT_SetBaseAddress( U32 BaseAddress )
{
    MES_ASSERT( CNULL != BaseAddress );
    MES_ASSERT( NUMBER_OF_IDCT_MODULE > __g_CurModuleIndex );

    __g_ModuleVariables[__g_CurModuleIndex].pRegister = (struct MES_IDCT_RegisterSet *)BaseAddress;
    __g_pRegister = (struct MES_IDCT_RegisterSet *)BaseAddress;
}


U32     MES_IDCT_GetBaseAddress( void )
{
    MES_ASSERT( NUMBER_OF_IDCT_MODULE > __g_CurModuleIndex );

    return (U32)__g_ModuleVariables[__g_CurModuleIndex].pRegister;    
}


U32    MES_IDCT_SelectModule( U32 ModuleIndex )
{
	U32 oldindex = __g_CurModuleIndex;

    MES_ASSERT( NUMBER_OF_IDCT_MODULE > ModuleIndex );

    __g_CurModuleIndex = ModuleIndex;
    __g_pRegister = __g_ModuleVariables[ModuleIndex].pRegister;    
    
    return oldindex;
}



void MES_IDCT( U16 *pSrc, U16 *pDst )
{
	register struct MES_IDCT_RegisterSet*   pRegister;
	
	MES_ASSERT( CNULL != __g_pRegister );
	
	pRegister = __g_pRegister;
	// Write to IDCT module
	memcpy((void*)pRegister->BUF, (void *)pSrc, 64*2);
	// Run IDCT 
	pRegister->CONT= 0x0001;		// Set Run bit
	// Wait for finishing ... It takes about 300 BCLK cycles to process it.
	while(1)
	{
		if (pRegister->INT_PEND & 0x1)
		{
			pRegister->INT_PEND= 1;
			break;
		}
	}

	// Read From IDCT module
	memcpy((void *)pDst, (void *)pRegister->BUF, 64*2);	
}


#if 0
static void dequant_slow( ogg_int16_t * dequant_coeffs,
                   ogg_int16_t * quantized_list,
                   ogg_int16_t * DCT_block) 
{
	int i;

	for(i=0;i<64;i++)
		DCT_block[dezigzag_index[i]] = quantized_list[i] * dequant_coeffs[i] >> 2;
}
#else

static void dequant_slow( U16 *dequant_coeffs, U16 *quantized_list, U16 *DCT_block) 
{
	int i;

	for(i=0;i<64;i++)
		DCT_block[dezigzag_index[i]] = quantized_list[i] * dequant_coeffs[i] >> 2;
}

#endif



#if 0
void IDctSlow(  Q_LIST_ENTRY * InputData,
                ogg_int16_t *QuantMatrix,
                ogg_int16_t * OutputData ) 
{
	ogg_int16_t IntermediateData[64];

	dequant_slow( QuantMatrix, InputData, IntermediateData);
	MES_IDCT(IntermediateData, OutputData);  
	return;
}
#else
void MES_IDctSlow(  U16 *InputData, U16 *QuantMatrix, U16 *OutputData ) 
{
	U16 IntermediateData[64];

	dequant_slow( QuantMatrix, InputData, IntermediateData);
	MES_IDCT(IntermediateData, OutputData);  
	return;
}
#endif



/************************
  x  x  x  x  0  0  0  0
  x  x  x  0  0  0  0  0
  x  x  0  0  0  0  0  0
  x  0  0  0  0  0  0  0
  0  0  0  0  0  0  0  0
  0  0  0  0  0  0  0  0
  0  0  0  0  0  0  0  0
  0  0  0  0  0  0  0  0
*************************/
#if 0
static void dequant_slow10( ogg_int16_t * dequant_coeffs,
                     ogg_int16_t * quantized_list,
                     ogg_int16_t * DCT_block)
{
	int i;

	memset(DCT_block,0, 128);
	for(i=0;i<10;i++)
		DCT_block[dezigzag_index[i]] = quantized_list[i] * dequant_coeffs[i] >> 2;
}
#else
static void MES_dequant_slow10( U16 *dequant_coeffs, U16 *quantized_list, U16* DCT_block)
{
	int i;

	memset(DCT_block,0, 128);
	for(i=0;i<10;i++)
		DCT_block[dezigzag_index[i]] = quantized_list[i] * dequant_coeffs[i] >> 2;
}

#endif


#if 0
void IDct10( Q_LIST_ENTRY * InputData,
             ogg_int16_t *QuantMatrix,
             ogg_int16_t * OutputData )
{
	ogg_int16_t IntermediateData[64];
	
	dequant_slow10( QuantMatrix, InputData, IntermediateData);
	MES_IDCT(IntermediateData, OutputData);
	return;
}
#else
void MES_IDct10( U16 *InputData, U16 *QuantMatrix, U16 * OutputData )
{
	U16 IntermediateData[64];
	
	MES_dequant_slow10( QuantMatrix, InputData, IntermediateData);
	MES_IDCT(IntermediateData, OutputData);
	return;
}
#endif


/***************************
  x   0   0  0  0  0  0  0
  0   0   0  0  0  0  0  0
  0   0   0  0  0  0  0  0
  0   0   0  0  0  0  0  0
  0   0   0  0  0  0  0  0
  0   0   0  0  0  0  0  0
  0   0   0  0  0  0  0  0
  0   0   0  0  0  0  0  0
**************************/
#if 0
void IDct1( Q_LIST_ENTRY * InputData,
            ogg_int16_t *QuantMatrix,
            ogg_int16_t * OutputData )
{
  ogg_int16_t  OutD;
  int	loop;

  OutD=(ogg_int16_t) ((ogg_int32_t)(InputData[0]*QuantMatrix[0]+15)>>5);

  for(loop=0;loop<64;loop++)
    OutputData[loop]=OutD;

}
#else
void IDct1( U16 *InputData, U16 *QuantMatrix, U16* OutputData )
{
  U16  OutD;
  int	loop;

  OutD=(U16) ((U32)(InputData[0]*QuantMatrix[0]+15)>>5);

  for(loop=0;loop<64;loop++)
    OutputData[loop]=OutD;

}
#endif



#ifndef __POLLUX_IDCT_H__
#define __POLLUX_IDCT_H__

#include <asm/arch/wincetype.h>

#ifdef  __cplusplus
extern "C"
{
#endif


#define NUMBER_OF_IDCT_MODULE 1
#define OFFSET_OF_IDCT_MODULE 0 


static const U32 dezigzag_index[64] = { 
   0,  1,  8,  16,  9,  2,  3, 10, 
   17, 24, 32, 25, 18, 11,  4,  5, 
   12, 19, 26, 33, 40, 48, 41, 34, 
   27, 20, 13,  6,  7, 14, 21, 28, 
   35, 42, 49, 56, 57, 50, 43, 36, 
   29, 22, 15, 23, 30, 37, 44, 51, 
   58, 59, 52, 45, 38, 31, 39, 46, 
   53, 60, 61, 54, 47, 55, 62, 63 
}; 

CBOOL   MES_IDCT_Initialize( void );
U32     MES_IDCT_GetNumberOfModule( void );
void    MES_IDCT_SetBaseAddress( U32 BaseAddress );
U32     MES_IDCT_GetBaseAddress( void );
U32     MES_IDCT_SelectModule( U32 ModuleIndex );
void    MES_IDct10( U16 *InputData, U16 *QuantMatrix, U16 * OutputData );


#ifdef  __cplusplus
}
#endif


#endif // __POLLUX_IDC_H__
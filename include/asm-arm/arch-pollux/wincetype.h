/*
 * ghcstop add: for wince bsp prototype routine append
 *
 * ghcstop@gmail.com
 *
 */
#ifndef __WINCETYPE_H__
#define __WINCETYPE_H__


typedef          char       S8;
typedef unsigned char       U8;
typedef          short      S16;
typedef unsigned short      U16;
typedef int                 S32;
typedef unsigned int        U32;
typedef enum boolean { CFALSE, CTRUE } CBOOL;
#define CFALSE 0
#define CTRUE 1

#define CNULL (NULL)

#define MES_ASSERT( x ) do { if( !(x) ) printk("%s.%d: MES_ASSERT error\n", __FUNCTION__, __LINE__); } while(0)


///	@brief	type for PCLK control mode
typedef enum 
{
	MES_PCLKMODE_DYNAMIC = 0UL,	    ///< PCLK is provided only when CPU has access to registers of this module.
	MES_PCLKMODE_ALWAYS  = 1UL  	///< PCLK is always provided for this module.
} MES_PCLKMODE ;

///	@brief type for BCLK control mode
typedef enum 
{
	MES_BCLKMODE_DISABLE  = 0UL,	///< BCLK is disabled.
	MES_BCLKMODE_DYNAMIC  = 2UL,	///< BCLK is provided only when this module requests it.
	MES_BCLKMODE_ALWAYS   = 3UL		///< BCLK is always provided for this module.
} MES_BCLKMODE ;



#endif /*__MMSP20_H__*/

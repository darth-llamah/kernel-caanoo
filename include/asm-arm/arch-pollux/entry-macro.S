/*
 * include/asm-arm/arch-pollux/entry-macro.S
 *
 * Low-level IRQ helper macros for POLLUX-based platforms
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <asm/hardware.h>

	.macro  get_irqnr_preamble, base, tmp
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm


 		.macro	disable_fiq
		.endm

// 2008-03-12, ghcstop fix ==> full search irq number & fast search(from LF), 이 녀석은 mask가 되어 있는지 아닌지 검사하지 않는군, 즉 mask register를 신경안씀.
		.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp
		ldr	\base, =0xf0000800
		ldr	\irqstat, [\base, #0x20]	@ low pend
		mov	\irqnr, #0
		teq	\irqstat, #0
		beq	1003f

1001:		tst	\irqstat, #15
		bne	1002f
		add	\irqnr, \irqnr, #4
		movs	\irqstat, \irqstat, lsr #4
		bne	1001b
1002:		tst	\irqstat, #1
		bne	2003f
		add	\irqnr, \irqnr, #1
		movs	\irqstat, \irqstat, lsr #1
		bne	1002b
1003:
		ldr	\irqstat, [\base, #0x24]	@ high pend
		teq	\irqstat, #0
		beq	2003f
		@add	\irqnr, #32
		add	\irqnr, \irqnr, #32  @ghcstop fix, for gnu gcc(not using EABI compiler)

2001:		tst	\irqstat, #15
		bne	2002f
		add	\irqnr, \irqnr, #4
		movs	\irqstat, \irqstat, lsr #4
		bne	2001b
2002:		tst	\irqstat, #1
		bne	2003f
		add	\irqnr, \irqnr, #1
		movs	\irqstat, \irqstat, lsr #1
		bne	2002b
2003:
		teq	\irqnr, #0
		.endm



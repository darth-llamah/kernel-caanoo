
#
# Makefile for the linux kernel.
#

# Object file lists.
obj-m			:=
obj-n			:=
obj-			:=
obj-dma-y		:=
obj-dma-n		:=

#obj-y			:= pollux.o irq.o time.o devs.o cpu.o clock_control.o gpio.o pollux-dma-proto.o dma.o gpioalv.o pollux-staticbus-proto.o
#obj-y			:= pollux.o irq.o time.o devs.o cpu.o clock_control.o gpio.o mp2530f-dma-proto.o dma.o gpio-alive.o
obj-y			:= pollux.o irq.o time.o devs.o cpu.o gpio.o mp2530f-dma-proto.o dma.o gpio-alive.o

# machine specific support

obj-$(CONFIG_MACH_AESOP_POLLUX) += mach-aesop-pollux.o

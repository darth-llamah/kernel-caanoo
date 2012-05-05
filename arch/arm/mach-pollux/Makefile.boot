ifeq ($(CONFIG_POLLUX_SHADOW_ONE),y) # nand boot mode

   zreladdr-y	:= 0x00008000
params_phys-y	:= 0x00000100

else # nor boot mode

   zreladdr-y	:= 0x80008000
params_phys-y	:= 0x80000100

endif



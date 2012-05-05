#ifndef _POLLUX_KEYIF_H
#define _POLLUX_KEYIF_H


#define FW_VERSION_WIZ      100
#define FW_VERSION_VOCA     100


/* pollux_key ioctl command */
enum _pollux_keyif_ioctl {
	IOCTL_GET_TIME,
	IOCTL_SET_TIME,
    IOCTL_SET_POWER_OFF,
    IOCTL_GET_INFO_LSB,
    IOCTL_GET_INFO_MSB,
    IOCTL_GET_USB_CHECK,
    IOCTL_SET_SOFT_DISCONNECT,
    IOCTL_SET_SOFT_CONNECT,
    IOCTL_GET_FW_VERSION,
    IOCTL_GET_INSERT_SDMMC,
    IOCTL_GET_BOARD_VERSION,
    IOCTL_GET_HOLD_STATUS,
    IOCTL_GET_PWR_STATUS,
    IOCTL_GET_ID_NUM,
    IOCTL_SET_ID_NUM,
    IOCTL_GET_ID_STATUS,
	IOCTL_GET_USB_WIRELESS_LAN,
	IOCTL_SET_USB_MEMORY_STICK_MOUNT,
	IOCTL_GET_USB_MEMORY_STICK_MOUNT,
	IOCTL_GET_USB_MEMORY_STICK_ENABLE,
	IOCTL_GET_PRODUCT_VERSION
};



#endif // _POLLUX_KEYIF_H
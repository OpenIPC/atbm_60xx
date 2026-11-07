#ifndef CONF_H
#define CONF_H

#define BLUETOOTH_UART_DEVICE_PORT  "/dev/atbm_ioctl"
#define FW_PATCHFILE_LOCATION   "/vendor/etc/firmware/"
#define VENDOR_LIB_CONF_FILE   "/vendor/etc/bluetooth/bt_vendor.conf"
#define UART_TARGET_BAUD_RATE   1500000
#define FW_PATCH_SETTLEMENT_DELAY_MS   200
#define USERIAL_VENDOR_SET_BAUD_DELAY_US   200000
#define LPM_IDLE_TIMEOUT_MULTIPLE   5


#define BTVND_DBG   FALSE
#define BTHW_DBG   TRUE
#define VNDUSERIAL_DBG   FALSE
#define UPIO_DBG   FALSE
#define USE_CONTROLLER_BDADDR   FALSE
#define FW_AUTO_DETECTION   TRUE
#define BT_WAKE_VIA_PROC   TRUE
#define SCO_PCM_ROUTING   0
#define SCO_PCM_IF_CLOCK_RATE   2
#define SCO_PCM_IF_FRAME_TYPE   0
#define SCO_PCM_IF_SYNC_MODE   0
#define SCO_PCM_IF_CLOCK_MODE   0
#define PCM_DATA_FMT_SHIFT_MODE   0
#define PCM_DATA_FMT_FILL_BITS   0
#define PCM_DATA_FMT_FILL_METHOD   0
#define PCM_DATA_FMT_FILL_NUM   0
#define PCM_DATA_FMT_JUSTIFY_MODE   0


#endif
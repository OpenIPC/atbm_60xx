#include "apollo.h"
#include "fwio.h"
#include "hwio.h"

#ifdef CONFIG_USE_FW_H
/*
if have ble
config ONLY_COMB_FIRMWARE
	bool "Only wifi_ble comb firmware,There is no pure WiFi firmware"
	depends on ATBM_BLE_WIFI6
	default n
   	help
        if [yes] just include ble comb firmware
        if [no] include ble comb firmware && wifi only firmware;
				firmware will usb insmod atbm_xxx.ko wifi_bt_comb=0?1 to Choice which on to use 
				insmod atbm_xxx.ko wifi_bt_comb=1 usb ble comb firmware
				insmod atbm_xxx.ko wifi_bt_comb=0 usb wifi only firmware
else if not have ble
	just include wifi only firmware
*/
#ifdef CONFIG_ATBM_BLE //###################################// need have ble 
#ifdef CONFIG_ATBM_ONLY_COMB_FIRMWARE // 
/***************************wifi+BLE ********************************************************/

#ifdef USB_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX
#include "firmware_usb_wifi_bt_comb_clite.h" /* CRONOUSLITE */
#endif//ATBM_WIFI6_COMPAT_6062_CX

#ifdef ATBM_WIFI6_COMPAT_6162X
#include "firmware_usb_wifi_bt_comb_ocea.h" /* OCEANUS */
#endif//ATBM_WIFI6_COMPAT_6062_CX

#ifdef ATBM_WIFI6_COMPAT_6062X
#include "firmware_usb_wifi_bt_comb_cronus.h" /* CRONOUS */
#endif//ATBM_WIFI6_COMPAT_6062X
#endif//USB_BUS

#ifdef SDIO_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX /* CRONOUSLITE */
#include "firmware_sdio_wifi_bt_comb_clite.h"
#endif //ATBM_WIFI6_COMPAT_6062_CX

#ifdef ATBM_WIFI6_COMPAT_6162X/* OCEANUS */
#include "firmware_sdio_wifi_bt_comb_ocea.h"
#endif//ATBM_WIFI6_COMPAT_6162X

#ifdef ATBM_WIFI6_COMPAT_6062X/* CRONOUS */
#include "firmware_sdio_wifi_bt_comb_cronus.h"
#endif//ATBM_WIFI6_COMPAT_6062X
#endif//SDIO_BUS

#ifdef SPI_BUS
#include "firmware_spi_wifi_bt_comb_cronus.h"
#endif //SPI_BUS
/***************************wifi+BLE end********************************************************/
#else  //not CONFIG_ATBM_ONLY_COMB_FIRMWARE

/****************************wifi only/ wifi+BLE start********************************************************/

#ifdef USB_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX
#include "firmware_usb_clite.h" /* CRONOUSLITE */
#include "firmware_usb_wifi_bt_comb_clite.h" /* CRONOUSLITE */
#endif //ATBM_WIFI6_COMPAT_6062_CX
#ifdef ATBM_WIFI6_COMPAT_6162X
#include "firmware_usb_ocea.h" /* OCEANUS */
#include "firmware_usb_wifi_bt_comb_ocea.h" /* OCEANUS */
#endif //ATBM_WIFI6_COMPAT_6162X
#ifdef ATBM_WIFI6_COMPAT_6062X
#include "firmware_usb_cronus.h" /* CRONOUS */
#include "firmware_usb_wifi_bt_comb_cronus.h" /* CRONOUS */
#endif//ATBM_WIFI6_COMPAT_6062X
#endif//USB_BUS

#ifdef SDIO_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX
#include "firmware_sdio_clite.h"
#include "firmware_sdio_wifi_bt_comb_clite.h"
#endif//ATBM_WIFI6_COMPAT_6062_CX
#ifdef ATBM_WIFI6_COMPAT_6162X
#include "firmware_sdio_ocea.h"
#include "firmware_sdio_wifi_bt_comb_ocea.h"
#endif//ATBM_WIFI6_COMPAT_6162X
#ifdef ATBM_WIFI6_COMPAT_6062X
#include "firmware_sdio_cronus.h"
#include "firmware_sdio_wifi_bt_comb_cronus.h"
#endif//ATBM_WIFI6_COMPAT_6062X
#endif//SDIO_BUS

#ifdef SPI_BUS
#include "firmware_spi.h"
#include "firmware_spi_wifi_bt_comb_cronus.h"
#endif
/***************************wifi only/ wifi+BLE end********************************************************/

#endif //CONFIG_ATBM_ONLY_COMB_FIRMWARE
#else //###################################//not CONFIG_ATBM_BLE

/***************************wifi only********************************************************/

#ifdef USB_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX
#include "firmware_usb_clite.h" /* CRONOUSLITE */
#endif //ATBM_WIFI6_COMPAT_6062_CX
#ifdef ATBM_WIFI6_COMPAT_6162X
#include "firmware_usb_ocea.h" /* OCEANUS */
#endif //ATBM_WIFI6_COMPAT_6162X
#ifdef ATBM_WIFI6_COMPAT_6062X
#include "firmware_usb_cronus.h" /* CRONOUS */
#endif//ATBM_WIFI6_COMPAT_6062X
#endif//USB_BUS

#ifdef SDIO_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX
#include "firmware_sdio_clite.h"
#endif//ATBM_WIFI6_COMPAT_6062_CX
#ifdef ATBM_WIFI6_COMPAT_6162X
#include "firmware_sdio_ocea.h"
#endif//ATBM_WIFI6_COMPAT_6162X
#ifdef ATBM_WIFI6_COMPAT_6062X
#include "firmware_sdio_cronus.h"
#endif//ATBM_WIFI6_COMPAT_6062X
#endif//SDIO_BUS

#ifdef SPI_BUS
#include "firmware_spi.h"
#endif //SPI_BUS
	/* OCEANUS */
#endif //###################################//CONFIG_ATBM_BLE


#if defined(ATBM_WIFI6_COMPAT_6062_CX) || defined(ATBM_WIFI6_COMPAT_6062X)|| defined(ATBM_WIFI6_COMPAT_6162X)
#else

#ifdef SDIO_BUS
#include "firmware_sdio.h"
#endif //
#ifdef USB_BUS
#include "firmware_usb.h"
#endif //

#endif //

extern int atbm_wifi_bt_comb_get(void);

int atbm_load_wifi_bt_firmware(struct firmware_altobeam *fw_altobeam)
	{
	
	atbm_printk_debug("load_wifi_bt_firmware from *.h\n");
	struct firmware_headr* hdr = NULL;
	switch(fw_altobeam->chip_catogery){
#ifdef ATBM_WIFI6_COMPAT_6162X
		case OCEANUS_CATEGORY:			
		
#ifdef CONFIG_ATBM_BLE
#ifdef CONFIG_ATBM_ONLY_COMB_FIRMWARE
			atbm_printk_always("firmware_usb_wifi_bt_comb_ocea.h\n");
			hdr = (struct firmware_headr*)firmware_headrOCEANUSBLE;
			fw_altobeam->hdr.iccm_len = sizeof(fw_codeOCEANUSBLE);
			fw_altobeam->hdr.dccm_len = sizeof(fw_dataOCEANUSBLE);
			fw_altobeam->hdr.sram_len = sizeof(fw_sramOCEANUSBLE);
			fw_altobeam->fw_sram = (u8 *)&fw_sramOCEANUSBLE[0];
			fw_altobeam->fw_iccm = (u8 *)&fw_codeOCEANUSBLE[0];
			fw_altobeam->fw_dccm = (u8 *)&fw_dataOCEANUSBLE[0];
#else //not CONFIG_ATBM_ONLY_COMB_FIRMWARE
			if(atbm_wifi_bt_comb_get()){
				atbm_printk_always("firmware_usb_wifi_bt_comb_ocea.h\n");
				hdr = (struct firmware_headr*)firmware_headrOCEANUSBLE;
				fw_altobeam->hdr.iccm_len = sizeof(fw_codeOCEANUSBLE);
				fw_altobeam->hdr.dccm_len = sizeof(fw_dataOCEANUSBLE);
				fw_altobeam->hdr.sram_len = sizeof(fw_sramOCEANUSBLE);
				fw_altobeam->fw_sram = (u8 *)&fw_sramOCEANUSBLE[0];
				fw_altobeam->fw_iccm = (u8 *)&fw_codeOCEANUSBLE[0];
				fw_altobeam->fw_dccm = (u8 *)&fw_dataOCEANUSBLE[0];
			}
			else {
				atbm_printk_always("firmware_usb_ocea.h\n");
				hdr = (struct firmware_headr*)firmware_headrOCEANUS;
		fw_altobeam->hdr.iccm_len = sizeof(fw_codeOCEANUS);
		fw_altobeam->hdr.dccm_len = sizeof(fw_dataOCEANUS);
				fw_altobeam->hdr.sram_len = sizeof(fw_sramOCEANUS);
				fw_altobeam->fw_sram = (u8 *)&fw_sramOCEANUS[0];
		fw_altobeam->fw_iccm = (u8 *)&fw_codeOCEANUS[0];
		fw_altobeam->fw_dccm = (u8 *)&fw_dataOCEANUS[0];
	}
#endif  //CONFIG_ATBM_ONLY_COMB_FIRMWARE
#else //#ifndef CONFIG_ATBM_BLE
			atbm_printk_always("firmware_usb_ocea.h\n");
			hdr = (struct firmware_headr*)firmware_headrOCEANUS;
			fw_altobeam->hdr.iccm_len = sizeof(fw_codeOCEANUS);
			fw_altobeam->hdr.dccm_len = sizeof(fw_dataOCEANUS);
			fw_altobeam->hdr.sram_len = sizeof(fw_sramOCEANUS);
			fw_altobeam->fw_sram = (u8 *)&fw_sramOCEANUS[0];
			fw_altobeam->fw_iccm = (u8 *)&fw_codeOCEANUS[0];
			fw_altobeam->fw_dccm = (u8 *)&fw_dataOCEANUS[0];
#endif //#ifdef CONFIG_ATBM_BLE
			break;
#endif //#ifdef ATBM_WIFI6_COMPAT_6162X
#ifdef ATBM_WIFI6_COMPAT_6062_CX
		case CLITE_CATEGORY:
#ifdef CONFIG_ATBM_BLE
#ifdef CONFIG_ATBM_ONLY_COMB_FIRMWARE
			atbm_printk_always("firmware_usb_wifi_bt_comb_clite.h\n");
			hdr = (struct firmware_headr*)firmware_headrCRONUS_LITEBLE;
			fw_altobeam->hdr.iccm_len = sizeof(fw_codeCRONUS_LITEBLE);
			fw_altobeam->hdr.dccm_len = sizeof(fw_dataCRONUS_LITEBLE);
			fw_altobeam->hdr.sram_len = sizeof(fw_sramCRONUS_LITEBLE);
			fw_altobeam->fw_sram = (u8 *)&fw_sramCRONUS_LITEBLE[0];
			fw_altobeam->fw_iccm = (u8 *)&fw_codeCRONUS_LITEBLE[0];
			fw_altobeam->fw_dccm = (u8 *)&fw_dataCRONUS_LITEBLE[0];
#else //not CONFIG_ATBM_ONLY_COMB_FIRMWARE
			if(atbm_wifi_bt_comb_get()){
				atbm_printk_always("firmware_usb_wifi_bt_comb_clite.h\n");
				hdr = (struct firmware_headr*)firmware_headrCRONUS_LITEBLE;
				fw_altobeam->hdr.iccm_len = sizeof(fw_codeCRONUS_LITEBLE);
				fw_altobeam->hdr.dccm_len = sizeof(fw_dataCRONUS_LITEBLE);
				fw_altobeam->hdr.sram_len = sizeof(fw_sramCRONUS_LITEBLE);
				fw_altobeam->fw_sram = (u8 *)&fw_sramCRONUS_LITEBLE[0];
				fw_altobeam->fw_iccm = (u8 *)&fw_codeCRONUS_LITEBLE[0];
				fw_altobeam->fw_dccm = (u8 *)&fw_dataCRONUS_LITEBLE[0];
			}
			else {
				atbm_printk_always("firmware_usb_clite.h\n");
				hdr = (struct firmware_headr*)firmware_headrCRONUS_LITE;
		fw_altobeam->hdr.iccm_len = sizeof(fw_codeCRONUS_LITE);
		fw_altobeam->hdr.dccm_len = sizeof(fw_dataCRONUS_LITE);
				fw_altobeam->hdr.sram_len = sizeof(fw_sramCRONUS_LITE);
				fw_altobeam->fw_sram = (u8 *)&fw_sramCRONUS_LITE[0];
		fw_altobeam->fw_iccm = (u8 *)&fw_codeCRONUS_LITE[0];
		fw_altobeam->fw_dccm = (u8 *)&fw_dataCRONUS_LITE[0];
			}
#endif  //CONFIG_ATBM_ONLY_COMB_FIRMWARE
#else //#ifndef CONFIG_ATBM_BLE
			atbm_printk_always("firmware_usb_clite.h\n");
			hdr = (struct firmware_headr*)firmware_headrCRONUS_LITE;
			fw_altobeam->hdr.iccm_len = sizeof(fw_codeCRONUS_LITE);
			fw_altobeam->hdr.dccm_len = sizeof(fw_dataCRONUS_LITE);
			fw_altobeam->hdr.sram_len = sizeof(fw_sramCRONUS_LITE);
			fw_altobeam->fw_sram = (u8 *)&fw_sramCRONUS_LITE[0];
			fw_altobeam->fw_iccm = (u8 *)&fw_codeCRONUS_LITE[0];
			fw_altobeam->fw_dccm = (u8 *)&fw_dataCRONUS_LITE[0];
#endif //#ifdef CONFIG_ATBM_BLE

			break;
#endif //#ifdef ATBM_WIFI6_COMPAT_6062_CX
#ifdef ATBM_WIFI6_COMPAT_6062X
		case CRONUS_CATEGORY:
#ifdef CONFIG_ATBM_BLE
#ifdef CONFIG_ATBM_ONLY_COMB_FIRMWARE
			atbm_printk_always("firmware_usb_wifi_bt_comb_cronus.h\n");
			hdr = (struct firmware_headr*)firmware_headrBLE;
			fw_altobeam->hdr.iccm_len = sizeof(fw_codeBLE);
			fw_altobeam->hdr.dccm_len = sizeof(fw_dataBLE);
			fw_altobeam->hdr.sram_len = sizeof(fw_sramBLE);
			fw_altobeam->fw_sram = (u8 *)&fw_sramBLE[0];
			fw_altobeam->fw_iccm = (u8 *)&fw_codeBLE[0];
			fw_altobeam->fw_dccm = (u8 *)&fw_dataBLE[0];
#else //not CONFIG_ATBM_ONLY_COMB_FIRMWARE
			if(atbm_wifi_bt_comb_get()){
				atbm_printk_always("firmware_usb_wifi_bt_comb_cronus.h\n");
				hdr = (struct firmware_headr*)firmware_headrBLE;
				fw_altobeam->hdr.iccm_len = sizeof(fw_codeBLE);
				fw_altobeam->hdr.dccm_len = sizeof(fw_dataBLE);
				fw_altobeam->hdr.sram_len = sizeof(fw_sramBLE);
				fw_altobeam->fw_sram = (u8 *)&fw_sramBLE[0];
				fw_altobeam->fw_iccm = (u8 *)&fw_codeBLE[0];
				fw_altobeam->fw_dccm = (u8 *)&fw_dataBLE[0];
			}
			else {
				atbm_printk_always("firmware_usb_cronus.h\n");
				hdr = (struct firmware_headr*)firmware_headr;
		fw_altobeam->hdr.iccm_len = sizeof(fw_code);
		fw_altobeam->hdr.dccm_len = sizeof(fw_data);
				fw_altobeam->hdr.sram_len = sizeof(fw_sram);
				fw_altobeam->fw_sram = (u8 *)&fw_sram[0];
		fw_altobeam->fw_iccm = (u8 *)&fw_code[0];
		fw_altobeam->fw_dccm = (u8 *)&fw_data[0];
	}
#endif  //CONFIG_ATBM_ONLY_COMB_FIRMWARE
#else //#ifndef CONFIG_ATBM_BLE
			atbm_printk_always("firmware_usb_cronus.h\n");
			hdr = (struct firmware_headr*)firmware_headr;
			fw_altobeam->hdr.iccm_len = sizeof(fw_code);
			fw_altobeam->hdr.dccm_len = sizeof(fw_data);
			fw_altobeam->hdr.sram_len = sizeof(fw_sram);
			fw_altobeam->fw_sram = (u8 *)&fw_sram[0];
			fw_altobeam->fw_iccm = (u8 *)&fw_code[0];
			fw_altobeam->fw_dccm = (u8 *)&fw_data[0];
#endif //#ifdef CONFIG_ATBM_BLE
			break;
#endif //#ifdef ATBM_WIFI6_COMPAT_6062X
		default:
#if 0//ndef CONFIG_ATBM_ONLY_COMB_FIRMWARE
			atbm_printk_always("other firmware_usb_cronus.h\n");
			hdr = (struct firmware_headr*)firmware_headr;
			fw_altobeam->hdr.iccm_len = sizeof(fw_code);
			fw_altobeam->hdr.dccm_len = sizeof(fw_data);
			fw_altobeam->hdr.sram_len = sizeof(fw_sram);
			fw_altobeam->fw_sram = (u8 *)&fw_sram[0];
			fw_altobeam->fw_iccm = (u8 *)&fw_code[0];
			fw_altobeam->fw_dccm = (u8 *)&fw_data[0];
#else 
			atbm_printk_always("can't find firmware.h\n");
#endif //#ifdef CONFIG_ATBM_ONLY_COMB_FIRMWARE
			break;

	}
	
	BUG_ON((hdr == NULL));
	BUG_ON((hdr->sram_len != fw_altobeam->hdr.sram_len));
	BUG_ON((hdr->flags != ALTOBEAM_WIFI_HDR_FLAG_V2)&&(fw_altobeam->hdr.sram_len != 0));


	if (hdr->flags == ALTOBEAM_WIFI_HDR_FLAG_V2)
		fw_altobeam->hdr.sram_addr = hdr->sram_addr;
	else
		fw_altobeam->hdr.sram_addr = DOWNLOAD_BLE_SRAM_ADDR;

	printk("atbm_load_wifi_bt_firmware: fwhdr->flags %x sram_addr %x,chip_cat %d\n", hdr->flags, fw_altobeam->hdr.sram_addr,fw_altobeam->chip_catogery);

	return 0;
}


#endif // CONFIG_USE_FW_H

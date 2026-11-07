#ifdef CONFIG_ATBM_BLE_CODE_SRAM

#include "apollo.h"
#include "fwio.h"
#ifdef CONFIG_USE_FW_H

#ifdef USB_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX//#ifdef ATBM_WIFI6_VARIANT_CLITE
#include "firmware_usb_wifi_bt_comb_clite.h" /* CRONOUSLITE */
#endif

#ifdef ATBM_WIFI6_COMPAT_6162X//#elif ATBM_WIFI6_VARIANT_OCEANUS
#include "firmware_usb_wifi_bt_comb_ocea.h" /* OCEANUS */
#endif

#ifdef ATBM_WIFI6_COMPAT_6062X//#else
#include "firmware_usb_wifi_bt_comb.h" /* CRONOUS */
#endif
#endif

#ifdef SDIO_BUS
#ifdef ATBM_WIFI6_COMPAT_6062_CX//#ifdef ATBM_WIFI6_VARIANT_CLITE
#include "firmware_sdio_wifi_bt_comb_clite.h"
#endif

#ifdef ATBM_WIFI6_COMPAT_6162X//#elif ATBM_WIFI6_VARIANT_OCEANUS
#include "firmware_sdio_wifi_bt_comb_ocea.h"
#endif

#ifdef ATBM_WIFI6_COMPAT_6062X//#else
#include "firmware_sdio_wifi_bt_comb.h"
#endif
#endif

#ifdef SPI_BUS
#include "firmware_spi_wifi_bt_comb.h"
#endif




int load_usb_wifi_bt_comb_firmware(struct firmware_altobeam *fw_altobeam)
{
	struct firmware_headr* hdr = NULL;//move out for safe check
	if(fw_altobeam->chip_catogery == OCEANUS_CATEGORY)
	{
#ifdef ATBM_WIFI6_COMPAT_6162X
		hdr = (struct firmware_headr*)firmware_headrOCEANUSBLE;
#endif
	}
	else if(fw_altobeam->chip_catogery == CLITE_CATEGORY)
	{
#ifdef ATBM_WIFI6_COMPAT_6062_CX
		hdr = (struct firmware_headr*)firmware_headrCRONUS_LITEBLE;
#endif
	}
	else if(fw_altobeam->chip_catogery == CRONUS_CATEGORY)
	{
#ifdef ATBM_WIFI6_COMPAT_6062X
		hdr = (struct firmware_headr*)firmware_headr;
#endif
	}

/* OCEANUS */
//#ifdef ATBM_WIFI6_VARIANT_OCEANUS
	if(fw_altobeam->chip_catogery == OCEANUS_CATEGORY)
	{
#ifdef ATBM_WIFI6_COMPAT_6162X
		fw_altobeam->hdr.iccm_len = sizeof(fw_codeOCEANUSBLE);
		fw_altobeam->hdr.dccm_len = sizeof(fw_dataOCEANUSBLE);
		fw_altobeam->hdr.sram_len = sizeof(fw_sramOCEANUSBLE);
		fw_altobeam->fw_sram = (u8 *)&fw_sramOCEANUSBLE[0];
		fw_altobeam->fw_iccm = (u8 *)&fw_codeOCEANUSBLE[0];
		fw_altobeam->fw_dccm = (u8 *)&fw_dataOCEANUSBLE[0];
#endif
	}
/* CRONOUSLITE */
//#elif ATBM_WIFI6_VARIANT_CLITE
	else if(fw_altobeam->chip_catogery == CLITE_CATEGORY)
	{
#ifdef ATBM_WIFI6_COMPAT_6062_CX
		fw_altobeam->hdr.iccm_len = sizeof(fw_codeCRONUS_LITEBLE);
		fw_altobeam->hdr.dccm_len = sizeof(fw_dataCRONUS_LITEBLE);
		fw_altobeam->hdr.sram_len = sizeof(fw_sramCRONUS_LITEBLE);
		fw_altobeam->fw_sram = (u8 *)&fw_sramCRONUS_LITEBLE[0];
		fw_altobeam->fw_iccm = (u8 *)&fw_codeCRONUS_LITEBLE[0];
		fw_altobeam->fw_dccm = (u8 *)&fw_dataCRONUS_LITEBLE[0];
#endif
	}
/* CRONOUS */
//#else
	else if(fw_altobeam->chip_catogery == CRONUS_CATEGORY)
	{
#ifdef ATBM_WIFI6_COMPAT_6062X
		fw_altobeam->hdr.iccm_len = sizeof(fw_code);
		fw_altobeam->hdr.dccm_len = sizeof(fw_data);
		fw_altobeam->hdr.sram_len = sizeof(fw_sram);
		fw_altobeam->fw_sram = (u8 *)&fw_sram[0];
		fw_altobeam->fw_iccm = (u8 *)&fw_code[0];
		fw_altobeam->fw_dccm = (u8 *)&fw_data[0];
#endif
	}
//#endif

	if (hdr->flags == ALTOBEAM_WIFI_HDR_FLAG_V2)
		fw_altobeam->hdr.sram_addr = hdr->sram_addr;
	else
		fw_altobeam->hdr.sram_addr = DOWNLOAD_BLE_SRAM_ADDR;

	printk("load_usb_wifi_bt_comb_firmware: fwhdr->flags %x sram_addr %x,chip_cat %d\n", hdr->flags, fw_altobeam->hdr.sram_addr,fw_altobeam->chip_catogery);

	return 0;
}
#endif

#else
int load_usb_wifi_bt_comb_firmware(struct firmware_altobeam *fw_altobeam)
{
	return 0;
}

#endif


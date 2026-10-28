/*
 * Firmware API for mac80211 altobeam APOLLO drivers
 * *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 * Based on apollo code
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * Based on:
 * ST-Ericsson UMAC CW1200 driver which is
 * Copyright (c) 2010, ST-Ericsson
 * Author: Ajitpal Singh <ajitpal.singh@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FWIO_H_INCLUDED
#define FWIO_H_INCLUDED

#ifndef CONFIG_FW_NAME
#if ((ATBM_WIFI_PLATFORM == 13/*PLATFORM_AMLOGIC_S805*/) || (ATBM_WIFI_PLATFORM == 8))
#define FIRMWARE_DEFAULT_PATH	"../wifi/atbm/fw.bin"
#else // PLATFORM_AMLOGIC_S805
#define FIRMWARE_DEFAULT_PATH	"fw.bin"
#endif //PLATFORM_AMLOGIC_S805
#else 
#define FIRMWARE_DEFAULT_PATH CONFIG_FW_NAME 
#define FIRMWARE_BLE_PATH CONFIG_FW_BLE_NAME 
#endif
#define SDD_FILE_DEFAULT_PATH	("sdd.bin")



struct firmware_headr {
	u32 flags; /*0x34353677*/
	u32 version;
	u32 iccm_len;
	u32 dccm_len;
	u32 sram_len;
	u32 sram_addr;
	u32 reserve[1];
	u16 reserve2;
	u16 checksum;
};

struct firmware_altobeam {
	struct firmware_headr hdr;
	u8 *fw_iccm;
	u8 *fw_dccm;
	u8 *fw_sram;
	u8 chip_catogery;
};

struct atbm_common;



int atbm_load_firmware(struct atbm_common *hw_priv);
void  atbm_get_chiptype(struct atbm_common *hw_priv);
void atbm_release_firmware(void);
int atbm_init_firmware(void);
char * atbm_get_chip_type(void);
void atbm_set_chip_type(const char *chip);

void  atbm_efuse_read_byte(struct atbm_common *priv,u32 byteIndex, u32 *value);
u32 atbm_efuse_read_chip_flag(struct atbm_common *hw_priv);

int atbm_load_wifi_bt_firmware(struct firmware_altobeam *fw_altobeam);
#ifdef ATBM_ADD_CRERT_FIRMWARE
int atbm_load_wifi_bt_cert_firmware(struct firmware_altobeam *fw_altobeam);
#endif //CONFIG_ATBM_BLE_CODE_SRAM

#ifdef CONFIG_PM_SLEEP
int atbm_cache_fw_before_suspend(struct device	 *pdev);
#endif
#endif

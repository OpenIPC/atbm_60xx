/* *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 *Based on apollo code
 * Copyright (C) ST-Ericsson SA 2011
 *
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef ATBM_APOLLO_PLAT_H_INCLUDED
#define ATBM_APOLLO_PLAT_H_INCLUDED

/*
*********************************************************
*
*PLATFORM_XUNWEI: based on linux3.0
*
*PLATFORM_SUN6I: based on linux3.3
*
*PLATFORM_FRIENDLY:based on linux3.086
*
*********************************************************
*/
#define PLATFORM_XUNWEI    					1
#define PLATFORM_SUN6I						2
#define PLATFORM_FRIENDLY					3
#define PLATFORM_SUN6I_64					4
#define PLATFORM_HI3798M					5
#define PLATFORM_HI3518E					6
#define PLATFORM_X86PC        				7
#define PLATFORM_AMLOGIC					8
#define PLATFORM_AMLOGIC_905				9	
#define PLATFORM_AMLOGIC_905X				9
#define PLATFORM_ROCKCHIP      				10
#define PLATFORM_MSTAR						11
#define PLATFORM_CDLINUX					12
#define PLATFORM_AMLOGIC_S805				13
#define PLATFORM_HIS_LINUX_3_4				14
#define PLATFORM_ROCKCHIP_3229				15
#define PLATFORM_ROCKCHIP_3229_ANDROID8		16
#define PLATFORM_HS_IPC						17
#define PLATFORM_SIGMASTAR                  18
#define PLATFORM_HI3516EV200                19
#define PLATFORM_XUNWEI_2G                  20
#define PLATFORM_NVT98517       			21
#define PLATFORM_AK_IPC       				22
#define PLATFORM_SUN8I       				23
#define PLATFORM_INGENICT31					24
#define PLATFORM_INGENICT41					25
#define PLATFORM_SUN50IWP1					26
#define PLATFORM_KALI                       27
#define PLATFORM_XM530						28
#define PLATFORM_GK7202V330         		29



#ifndef  ATBM_WIFI_PLATFORM
#define ATBM_WIFI_PLATFORM			PLATFORM_SIGMASTAR
#endif

#define APOLLO_1505  0
#define APOLLO_1601  1
#define APOLLO_1606  0
#define APOLLO_C     2
#define APOLLO_D     3
#define APOLLO_E     4
#define APOLLO_F     5
#define ATHENA_B     6
#define ATHENA_LITE  7
#define ATHENA_LITE_ECO  8
#define ARES_A  	 9
#define ARES_B  	 10
#define HERA         11
#define CRONUS       12
#define CRONUS_NO_HT40   13
#define CRONUS_NO_HT40_LDPC   14
#define CRONUS_NO_BLE   15
#define RHEA         16
#define OCEANUS      17
#define CRONUS_LITE      18
#define OCEANUS_NO_HT40   19
#define OCEANUS_NO_HT40_LDPC   20
#define OCEANUS_NO_BLE   21
#define CRONUS_LITE_NO_HT40   22
#define CRONUS_LITE_NO_HT40_LDPC   23
#define CRONUS_LITE_NO_BLE   24
#define OCEANUS_NO_WIFI6   25

#define CHIP_VERSION_MAX     (OCEANUS_NO_WIFI6)


enum CHIP_CATEGORY{
	CRONUS_CATEGORY,
	OCEANUS_CATEGORY,
	CLITE_CATEGORY
};




#ifndef PROJ_TYPE
#define PROJ_TYPE  CRONUS
#endif

#define CRYSTAL_MODE 0
#define EXTERNAL_MODE 1
#ifndef DCXO_TYPE
#define DCXO_TYPE   CRYSTAL_MODE
#endif

#define DPLL_CLOCK_26M 0
#define DPLL_CLOCK_40M 1
#define DPLL_CLOCK_24M 2

#ifndef DPLL_CLOCK
#define DPLL_CLOCK DPLL_CLOCK_40M
#endif

#include <linux/ioport.h>

struct atbm_platform_data {
	const char *mmc_id;
	const int irq_gpio;
	const int power_gpio;
	const int reset_gpio;
	int (*power_ctrl)(const struct atbm_platform_data *pdata,
			  bool enable);
	int (*clk_ctrl)(const struct atbm_platform_data *pdata,
			  bool enable);
	int (*insert_ctrl)(const struct atbm_platform_data *pdata,
			  bool enable);
};

/* Declaration only. Should be implemented in arch/xxx/mach-yyy */
struct atbm_platform_data *atbm_get_platform_data(void);



#endif /* ATBM_APOLLO_PLAT_H_INCLUDED */

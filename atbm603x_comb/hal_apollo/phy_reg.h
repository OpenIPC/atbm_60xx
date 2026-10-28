#ifndef ATBM_APOLLO_PHY_REG_H_INCLUDED
#define ATBM_APOLLO_PHY_REG_H_INCLUDED
#include "pac_defs_wlan.h"
#include "apollo.h"

/* 驱动初始化寄存器过程中需要的宏 */
#define ATBM_PHY_REG_INIT_BUF_MAX_SIZE 1024
#define ATBM_PHY_MAX_BUF_LEN 1550

/* 120 * sizeof(wsm_regval_bit) must less than ATBM_PHY_MAX_BUF_LEN */
#define ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME 120

#define GET_LMAC_FPGA(_compileMacro) 							(((_compileMacro) >> 0) & 0x00000001)
#define GET_LMAC_REDUCE_TX_POWER_CONSUMPTION(_compileMacro)		(((_compileMacro) >> 1) & 0x00000001)
#define GET_LMAC_ATE_MODE(_compileMacro)						(((_compileMacro) >> 2) & 0x00000001)
#define GET_LMAC_OFDM_USE_CLOCK_LOCK(_compileMacro)				(((_compileMacro) >> 3) & 0x00000001)
#define GET_LMAC_MODEM_TX_RX_ETF(_compileMacro)					(((_compileMacro) >> 4) & 0x00000001)
#define GET_LMAC_TWO_CHIP_ONE_SOC(_compileMacro)				(((_compileMacro) >> 5) & 0x00000001)
#define GET_LMAC_NORMAL_SIFS(_compileMacro)						(((_compileMacro) >> 6) & 0x00000001)
#define GET_LMAC_TX_CFO_PPM_CORRECTION(_compileMacro)			(((_compileMacro) >> 7) & 0x00000001)
#define GET_LMAC_ENABLE_CALI_CFO(_compileMacro)					(((_compileMacro) >> 8) & 0x00000001)
#define GET_LMAC_CFO_DCXO_CORRECTION(_compileMacro)				(((_compileMacro) >> 9) & 0x00000001)
#define GET_LMAC_MOD_PHY_ETF_SUPPORT(_compileMacro)				(((_compileMacro) >> 10) & 0x00000001)
#define GET_LMAC_SUPPORT_MULTI_MAC_ADDR(_compileMacro)			(((_compileMacro) >> 11) & 0x00000001)
#define GET_LMAC_ENABLE_A4_FORMAT_ECO(_compileMacro)			(((_compileMacro) >> 12) & 0x00000001)
#define GET_LMAC_ASIC_1250_CUT_1(_compileMacro)					(((_compileMacro) >> 13) & 0x00000001)
#define GET_LMAC_ENABLE_AGG_END_ECO(_compileMacro)				(((_compileMacro) >> 14) & 0x00000001)
#define GET_LMAC_SW_SET_NAV(_compileMacro)						(((_compileMacro) >> 15) & 0x00000001)
#define GET_LMAC_STA_MONITOR(_compileMacro)						(((_compileMacro) >> 16) & 0x00000001)
#define GET_LMAC_USE_NORMAL_RAMP_AND_DOWN(_compileMacro)		(((_compileMacro) >> 17) & 0x00000001)
#define GET_LMAC_REDUCE_POWER_CONSUMPTION(_compileMacro)		(((_compileMacro) >> 18) & 0x00000001)

#define HW_WRITE_REG(addr, data) ROM_HW_WRITE_REG_BIT((addr), 31, 0, (data))

#define PHY_BB_HW_WRITE_REG HW_WRITE_REG

void atbm_phy_init(struct atbm_common *hw_priv);
void ROM_HW_WRITE_REG_BIT(u32 addr, u8 end, u8 start, u32 data);
void ROM_lmac_Wait(u32 us);


#endif /* ATBM_APOLLO_PHY_REG_H_INCLUDED */


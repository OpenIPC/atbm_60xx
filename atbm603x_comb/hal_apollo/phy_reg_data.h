#ifndef ATBM_APOLLO_PHY_REG_DATA_H_INCLUDED
#define ATBM_APOLLO_PHY_REG_DATA_H_INCLUDED

#include "phy_reg.h"
#define Ares_AZLC				18
#define Ares_AY					15
#define Ares_AX					14
#define RF_SUBTYPE_DEFINE  		Ares_AX
#define CFG_DPLL_CLOCK			24
#define REDUCE_POWER_CONSUMPTION 0

/* 此文件中均为Lmac中定义的宏或数据，需要和Lmac保持一致 */
#define PHY_EOT 0xffffffff
#define HB_EOT 0xffff
#define PHY_WAIT	0xFFFFFFFE

#define D11_TYPE_MASK 0x0F /* for all protocal versions, 0x0c */
#define D11_TYPE_MASK_ALLVER 0x0C
#define D11_PROT_VER 0x00
#define D11_PROT_MASK 0x03
#define D11_MGMT_TYPE 0x00
#define D11_CNTL_TYPE 0x04
#define D11_DATA_TYPE 0x08
#define D11_RESERVED_TYPE 0x0c
#define D11_SUB_TYPE_ONLY_MASK 0xF0
#define D11_SUB_TYPE_MASK (D11_SUB_TYPE_ONLY_MASK | D11_TYPE_MASK)

#define D11_GET_TYPE(_x) ((_x) & D11_TYPE_MASK)
#define D11_GET_SUB_TYPE(_x) ((_x) & D11_SUB_TYPE_MASK)
#define D11_IS_MGMT(_x) (D11_GET_TYPE(_x) D11_MGMT_TYPE)
#define D11_IS_CNTL(_x) (D11_GET_TYPE(_x) D11_CNTL_TYPE)
#define D11_IS_DATA(_x) (D11_GET_TYPE(_x) & D11_DATA_TYPE)

#define D11_SUB_MGMT_ASRQ (0x00 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_ASRSP (0x10 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_RSRQ (0x20 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_RSRSP (0x30 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_PBRQ (0x40 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_PBRSP (0x50 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_BCN (0x80 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_ATIM (0x90 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_DAS (0xa0 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_AUTH (0xb0 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_DAUTH (0xc0 + D11_MGMT_TYPE)
#define D11_SUB_MGMT_ACTION (0xd0 + D11_MGMT_TYPE)

#define D11_SUB_CNTL_WRAPPER (0x70 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_BLKACK_REQ (0x80 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_BLKACK (0x90 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_PSPOLL (0xa0 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_RTS (0xb0 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_CTS (0xc0 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_ACK (0xd0 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_CFEND (0xe0 + D11_CNTL_TYPE)
#define D11_SUB_CNTL_CFEACK (0xf0 + D11_CNTL_TYPE)

#define D11_SUB_DATA (0x00 + D11_DATA_TYPE)
#define D11_SUB_DATA_CFACK (0x10 + D11_DATA_TYPE)
#define D11_SUB_DATA_CFPOLL (0x20 + D11_DATA_TYPE)
#define D11_SUB_DATA_CFAKPL (0x30 + D11_DATA_TYPE)
#define D11_SUB_DNUL (0x40 + D11_DATA_TYPE)
#define D11_SUB_DNUL_CFACK (0x50 + D11_DATA_TYPE)
#define D11_SUB_DNUL_CFPOLL (0x60 + D11_DATA_TYPE)
#define D11_SUB_DNUL_CFAKPL (0x70 + D11_DATA_TYPE)

/*QOS Changes - START */
#define D11_SUB_QDATA_BIT (0x80)
#define D11_SUB_QTYPE_MASK (D11_SUB_QDATA_BIT | D11_TYPE_MASK)
#define D11_SUB_QTYPE_MASK_ALLVER (D11_SUB_QDATA_BIT | D11_TYPE_MASK_ALLVER)
#define D11_SUB_QTYPE_VAL (D11_SUB_QDATA_BIT | D11_DATA_TYPE)
#define D11_SUB_QDATA (0x80 + D11_DATA_TYPE)
#define D11_SUB_QDATA_CFACK (0x90 + D11_DATA_TYPE)
#define D11_SUB_QDATA_CFPOLL (0xA0 + D11_DATA_TYPE)
#define D11_SUB_QDATA_CFAKPL (0xB0 + D11_DATA_TYPE)
#define D11_SUB_QDNUL (0xC0 + D11_DATA_TYPE)
#define D11_SUB_QDNUL_CFACK (0xD0 + D11_DATA_TYPE)
#define D11_SUB_QDNUL_CFPOLL (0xE0 + D11_DATA_TYPE)
#define D11_SUB_QDNUL_CFAKPL (0xF0 + D11_DATA_TYPE)
	/*QOS Changes - END */

/* Bit usage in data frames */
#define D11_NULL_BIT 0x40
#define D11_CFPOLL_BIT 0x20
#define D11_CFACK_BIT 0x10
#define D11_DATA_BIT 0x00

/* Remaining bits in frame control */
#define D11_TO_DS 0x0100
#define D11_FROM_DS 0x0200
#define D11_DS_DS 0x0300
#define D11_MORE_FRAG 0x0400
#define D11_RETRY 0x0800
#define D11_PWR_MGMT 0x1000
#define D11_MORE_DATA 0x2000
#define D11_PRIVACY 0x4000
#define D11_ORDER 0x8000

/*A4UPD: Masks & Bitmap for FromDs ToDS bits*/
#define D11_WDS_FRM_IDENTIFIER 0x03
#define D11_FROM_DS_TO_DS_MASK 0x03

/* action frame categories */
#define D11_CATEGORY_11H_SPECTRUM_MGMT 0
#define D11_CATEGORY_11E_QOS 1
#define D11_CATEGORY_BLOCK_ACK 3
#define D11_CATEGORY_PUBLIC 4
#define D11_CATEGORY_11K_RADIO_MEASUREMENT 5
#define D11_CATEGORY_HT 7
#define D11_CATEGORY_WME_QOS 17
#define D11_CATEGORY_VENDOR_SPECIFIC 127

#define D11_ACK_POLICY_NORMAL 0x0000
#define D11_ACK_POLICY_MASK 0x0060

#define SECOND_INFADDR_0_31 0x0ad0006c
#define SECOND_INFADDR_32_47 0x0ad00070

#define PAC_BASE_ADDRESS (0x09000000)
#define PAC_REGS (PAC_BASE_ADDRESS + 0xC00000) // 0x9c00000

const wsm_regval PHY_INIT_COMMON_REG_TABLE[] = {
	// bmodem registors
	{0xaca803c, 	0x77671F7B	}, // BRAGCT2
	{0xaca8574, 	0x090a0f0f	}, // BRDLYSCCA
	{0xacb0078, 	0x00480035	}, // BRCALC2
	{0xaca8540, 	0x0000017f	}, // BRCCACALC
	{0xaca8048, 	0x0342D6F0	}, // BRAGCT1
	{0xaca8040, 	0x00077bbd	}, // BRCONF
 // {0xaca805c, 	0x430c2620	}, // BRCALC
	{0xac80234, 	((0x7f)<<10)}, // params_imb_gain[9:0], params_imb_phase[19:10], 8*16/512/pi*180 = 13 degree
	{0xACB81A0, 	0x10000000	}, // RIRXIQIMB
	{0xac80da0, 	0xffff3000	}, // rx data watch dog 9.8ms; rx end watch dog is 3.2ms
	{0xac80d98, 	0x04000300	}, // wait_end_sig:1024 points;  VALIDATE_HT_SIG: 768;
	{0xac80d9c, 	0x03000300	}, // noise_update: 768 points;  VALIDATE_SYNC: 768
	{0xACA0010, 	0x0000000a	}, // Radio On Counter Register
	{0xac80cfc, 	0x0000000f	},
	{0xaca805c, 	0x330C2624	}, // 0x430c261c},   // dsss reg
	{PHY_EOT  , 	PHY_EOT		}
};

const wsm_regval_bit PHY_INIT_FEM_TABLE[] = {
	{0xACB8308,		5,		0,		0x0		}, // FEM_CTRL_MATRIX_2, Rx
	{0xACB8310,		5,		0,		0x1		}, // FEM_CTRL_MATRIX_4, Tx
	{0xACB838C,		2,		0,		0x4		}, // pta_priority��force to 0.
	{0xAC80CAC,		1,		1,		0x0		},
	{0xac8043c,		15,		15,		0x0		},
	{0xACB8A48,		0,		0,		0x0		},
	{0xAc80248,		17,		17,		0x0		},
	{0xac80ca0,		26,		17,		0x3ff	},
	{0xac80ca4,		9,		0,		0x3ff	},
	{PHY_EOT,		0,		0,		HB_EOT	}
};

/* 根据编译宏拆分了PHY_INIT_AGC_REG_TABLE */
const u16 PHY_INIT_AGC_REG_TABLE_1[40] = {
	0xD5D, // 0xAC80800
	0xD5D, // 0xAC80808
	0xD5D, // 0xAC80810
	0xD5D, // 0xAC80818
	0xD5D, // 0xAC80820
	0xD5D, // 0xAC80828
	0xD5D, // 0xAC80830
	0xD5D, // 0xAC80838
	0xD5D, // 0xAC80840
	0xD5D, // 0xAC80848
	0xD5D, // 0xAC80850
	0xD5D, // 0xAC80858
	0xD5D, // 0xAC80860
	0xD5D, // 0xAC80868
	0xD5D, // 0xAC80870
	0xD5D, // 0xAC80878
	0xD5D, // 0xAC80880
	0xD5D, // 0xAC80888
	0xD5D, // 0xAC80890
	0xD5D, // 0xAC80898
	0xD5D, // 0xAC808A0
	0xD5D, // 0xAC808A8
	0xD5D, // 0xAC808B0
	0xC5E, // 0xAC808B8
	0xB50, // 0xAC808C0
	0xA51, // 0xAC808C8
	0x952, // 0xAC808D0
	0x854, // 0xAC808D8
	0x71D, // 0xAC808E0
	0x61E, // 0xAC808E8
	0x510, // 0xAC808F0
	0x411, // 0xAC808F8
	0x312, // 0xAC80900
	0x214, // 0xAC80908
	0x115, // 0xAC80910
	0x16, // 0xAC80918
	0x3F18, // 0xAC80920
	0x3E19, // 0xAC80928
	0x3D1A, // 0xAC80930
	0x3D1A, // 0xAC80938
};

const u16 PHY_INIT_AGC_REG_TABLE_2[40] = {
	0x196E, // 0xAC80800
	0x196E, // 0xAC80808
	0x196E, // 0xAC80810
	0x196E, // 0xAC80818
	0x196E, // 0xAC80820
	0x196E, // 0xAC80828
	0x196E, // 0xAC80830
	0x196E, // 0xAC80838
	0x196E, // 0xAC80840
	0x196E, // 0xAC80848
	0x196E, // 0xAC80850
	0x187C, // 0xAC80858
	0x177D, // 0xAC80860
	0x167E, // 0xAC80868
	0x1570, // 0xAC80870
	0x1471, // 0xAC80878
	0x1372, // 0xAC80880
	0x1274, // 0xAC80888
	0x1175, // 0xAC80890
	0x1076, // 0xAC80898
	0xF78, // 0xAC808A0
	0xE79, // 0xAC808A8
	0xD5D, // 0xAC808B0
	0xC5E, // 0xAC808B8
	0xB50, // 0xAC808C0
	0xA51, // 0xAC808C8
	0x952, // 0xAC808D0
	0x854, // 0xAC808D8
	0x755, // 0xAC808E0
	0x656, // 0xAC808E8
	0x558, // 0xAC808F0
	0x459, // 0xAC808F8
	0x35A, // 0xAC80900
	0x212, // 0xAC80908
	0x114, // 0xAC80910
	0x15, // 0xAC80918
	0x3F16, // 0xAC80920
	0x3E18, // 0xAC80928
	0x3D19, // 0xAC80930
	0x3C1A, // 0xAC80938
};

const u16 PHY_INIT_AGC_REG_TABLE_3[40] = {
	0x1E60, // 0xAC80800
	0x1E60, // 0xAC80808
	0x1E60, // 0xAC80810
	0x1E60, // 0xAC80818
	0x1E60, // 0xAC80820
	0x1E60, // 0xAC80828
	0x1D64, // 0xAC80830
	0x1C68, // 0xAC80838
	0x1B6C, // 0xAC80840
	0x1A6D, // 0xAC80848
	0x196E, // 0xAC80850
	0x187C, // 0xAC80858
	0x177D, // 0xAC80860
	0x167E, // 0xAC80868
	0x1570, // 0xAC80870
	0x1471, // 0xAC80878
	0x1372, // 0xAC80880
	0x1274, // 0xAC80888
	0x1175, // 0xAC80890
	0x104d, // 0xAC80898
	0xF4e, // 0xAC808A0
	0xE5c, // 0xAC808A8
	0xD5D, // 0xAC808B0
	0xC5E, // 0xAC808B8
	0xB50, // 0xAC808C0
	0xA51, // 0xAC808C8
	0x952, // 0xAC808D0
	0x854, // 0xAC808D8
	0x71D, // 0xAC808E0
	0x61E, // 0xAC808E8
	0x510, // 0xAC808F0
	0x411, // 0xAC808F8
	0x312, // 0xAC80900
	0x214, // 0xAC80908
	0x115, // 0xAC80910
	0x16, // 0xAC80918
	0x3F18, // 0xAC80920
	0x3E19, // 0xAC80928
	0x3D1A, // 0xAC80930
	0x3D1A, // 0xAC80938
};

const u16 PHY_INIT_AGC_REG_TABLE_4[40] = {
	0xD20, // 0xAC80800
	0xD20, // 0xAC80808
	0xD20, // 0xAC80810
	0xD20, // 0xAC80818
	0xD20, // 0xAC80820
	0xD20, // 0xAC80828
	0xD20, // 0xAC80830
	0xD20, // 0xAC80838
	0xD20, // 0xAC80840
	0xD20, // 0xAC80848
	0xD20, // 0xAC80850
	0xD20, // 0xAC80858
	0xD20, // 0xAC80860
	0xD20, // 0xAC80868
	0xD20, // 0xAC80870
	0xD20, // 0xAC80878
	0xD20, // 0xAC80880
	0xD20, // 0xAC80888
	0xD20, // 0xAC80890
	0xD20, // 0xAC80898
	0xD20, // 0xAC808A0
	0xD20, // 0xAC808A8
	0xD20, // 0xAC808B0
	0xC24, // 0xAC808B8
	0xB28, // 0xAC808C0
	0xA2C, // 0xAC808C8
	0x92D, // 0xAC808D0
	0x82E, // 0xAC808D8
	0x73C, // 0xAC808E0
	0x63D, // 0xAC808E8
	0x53E, // 0xAC808F0
	0x430, // 0xAC808F8
	0x331, // 0xAC80900
	0x232, // 0xAC80908
	0x134, // 0xAC80910
	0x35, // 0xAC80918
	0x3F36, // 0xAC80920
	0x3E38, // 0xAC80928
	0x3D39, // 0xAC80930
	0x3C3A, // 0xAC80938
};

const u16 PHY_INIT_AGC_REG_TABLE_5[40] = {
	0x1E60, // 0xAC80800
	0x1E60, // 0xAC80808
	0x1E60, // 0xAC80810
	0x1E60, // 0xAC80818
	0x1E60, // 0xAC80820
	0x1E60, // 0xAC80828
	0x1D64, // 0xAC80830
	0x1C68, // 0xAC80838
	0x1B6C, // 0xAC80840
	0x1A6D, // 0xAC80848
	0x196E, // 0xAC80850
	0x187C, // 0xAC80858
	0x177D, // 0xAC80860
	0x167E, // 0xAC80868
	0x1570, // 0xAC80870
	0x1440, // 0xAC80878
	0x1344, // 0xAC80880
	0x1248, // 0xAC80888
	0x114C, // 0xAC80890
	0x104D, // 0xAC80898
	0xF4E, // 0xAC808A0
	0xE5C, // 0xAC808A8
	0xD20, // 0xAC808B0
	0xC24, // 0xAC808B8
	0xB28, // 0xAC808C0
	0xA2C, // 0xAC808C8
	0x92D, // 0xAC808D0
	0x82E, // 0xAC808D8
	0x73C, // 0xAC808E0
	0x63D, // 0xAC808E8
	0x53E, // 0xAC808F0
	0x430, // 0xAC808F8
	0x331, // 0xAC80900
	0x232, // 0xAC80908
	0x134, // 0xAC80910
	0x35, // 0xAC80918
	0x3F36, // 0xAC80920
	0x3E38, // 0xAC80928
	0x3D39, // 0xAC80930
	0x3C3A, // 0xAC80938
};
#define DigGain_Table_0dB_Index 2
#define DigGain_Table_Max_Index 71
const u8 ROM_ro_txdigital_gain_magic_table[] = {
	0x20,
0x21,
0x22,
0x23,
0x24,
0x25,
0x26,
0x27,
0x28,
0x29,
0x2B,
0x2C,
0x2D,
0x2F,
0x30,
0x31,
0x33,
0x34,
0x36,
0x37,
0x39,
0x3B,
0x3C,
0x3E,
0x40,
0x42,
0x44,
0x46,
0x48,
0x4A,
0x4C,
0x4E,
0x50,
0x53,
0x55,
0x58,
0x5A,
0x5D,
0x60,
0x62,
0x65,
0x68,
0x6B,
0x6E,
0x72,
0x75,
0x78,
0x7C,
0x7F,
0x83,
0x87,
0x8B,
0x8F,
0x93,
0x97,
0x9C,
0xA0,
0xA5,
0xAA,
0xAF,
0xB4,
0xB9,
0xBF,
0xC4,
0xCA,
0xD0,
0xD6,
0xDC,
0xE3,
0xE9,
0xF0,
0xF7,
//0xFE,
};
#define MAX_POWER_INDEX_ENTRIES	11
const u8 u16PpaGainValTbl_20M_40M[MAX_POWER_INDEX_ENTRIES] ={	
	0x3d,//1M/2M/
	0x3d,//5.5M/11M/
	0x1d, //6M/MCS0 /
	0x1d, //9M/
	0x1d, //12M/MCS1/
	0x1d, //18M/MCS2/
	0x1d, //24M/MCS3/
	0x1d, //36M/MCS4/
	0x1d, //48M/MCS5/
	0x1d, //54M/MCS6/
	0x1d,  //MCS7/	
};
#define AresB_delta_gain_20M 0

const u8 S8DigGainInitTbl_20M[MAX_POWER_INDEX_ENTRIES] ={	
#if (RF_SUBTYPE_DEFINE == Ares_AY)
    60, /*1M/2M*/
	60, /*5.5M/11M*/
	65, /*6M/MCS0 */
	65, /*9M*/
	65, /*12M/MCS1*/
	65, /*18M/MCS2*/
	65, /*24M/MCS3*/
	65, /*36M/MCS4*/
	65, /*48M/MCS5*/
	65, /*54M/MCS6*/
	55, /*MCS7*/
#else
	95 + AresB_delta_gain_20M, //1M/2M  17dBm /
	95 + AresB_delta_gain_20M, //5.5M/11M, 17dBm/
	85 + AresB_delta_gain_20M, //6M/MCS0  15dbm / 
	85 + AresB_delta_gain_20M, //9M//15dbm
	85 + AresB_delta_gain_20M, //12M/MCS1/
	85 + AresB_delta_gain_20M, //18M/MCS2/
	85 + AresB_delta_gain_20M, //24M/MCS3/
	85 + AresB_delta_gain_20M, //36M/MCS4/
	85 + AresB_delta_gain_20M, //48M/MCS5/
	85 + AresB_delta_gain_20M, //54M/MCS6/
	75 + AresB_delta_gain_20M, //MCS7/ 14dBm

#endif
};

#define AresB_delta_gain_40M 0

const u8 S8DigGainInitTbl_40M[MAX_POWER_INDEX_ENTRIES] ={	
	105 + AresB_delta_gain_40M, //1M/2M/
	105 + AresB_delta_gain_40M, //5.5M/11M/
	92 + AresB_delta_gain_40M, //6M/MCS0 /14dbm
	92 + AresB_delta_gain_40M, //9M/
	92 + AresB_delta_gain_40M, //12M/MCS1/
	92 + AresB_delta_gain_40M, //18M/MCS2/
	92 + AresB_delta_gain_40M, //24M/MCS3/
	92 + AresB_delta_gain_40M, //36M/MCS4/
	92 + AresB_delta_gain_40M, //48M/MCS5/
	92 + AresB_delta_gain_40M, //54M/MCS6/
	82 + AresB_delta_gain_40M, //MCS7/13dbM
};

#endif /* ATBM_APOLLO_PHY_REG_DATA_H_INCLUDED */


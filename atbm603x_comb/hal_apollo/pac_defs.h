/* -*-H-*-
*******************************************************************************
* altobeam
* Reproduction and Communication of this document is strictly prohibited
* unless specifically authorized in writing by altobeam
******************************************************************************/
/**
* \file
* \ingroup PAS
*
* \brief Protocol accelerator bit definitions
*/
/*
***************************************************************************
* Copyright (c) 2007 altobeam Ltd
* Copyright altobeam, 2009 – All rights reserved.
*
* This information, source code and any compilation or derivative thereof are
* the proprietary information of altobeam and/or its licensors and are
* confidential in nature. Under no circumstances is this software to be exposed
* to or placed under an Open Source License of any type without the expressed
* written permission of altobeam.
*
* Although care has been taken to ensure the accuracy of the information and the
* software, altobeam assumes no responsibility therefore.
*
* THE INFORMATION AND SOFTWARE ARE PROVIDED "AS IS" AND "AS AVAILABLE".
*
* altobeam MAKES NO REPRESENTATIONS OR WARRANTIES OF ANY KIND, EITHER EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES OR MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS,
*
****************************
* Updates
* 16/04/2009 - WBF00003856 - http://cqweb.zav.st.com/cqweb/url/default.asp?db=WBF&id=WBF00003856
* 27/04/2009 - WBF00003694 - http://cqweb.zav.st.com/cqweb/url/default.asp?db=WBF&id=WBF00003694
****************************************************************************/

#ifndef PAC_DEFS_H
#define PAC_DEFS_H
/***********************************************************************/
/***                        Data Types and Constants                 ***/
/***********************************************************************/

/* ----------------------------------------------------------------*/
/* ST - "Event response enable /request" - Pg 50 | Tality - "event_resp_enable" - 1.9.1 */
/* Masked Values */
#define PAC_NTD_EVT_RSP_EN__SET_TSF_HW_0      (1<<0)  // 0x00000001
#define PAC_NTD_EVT_RSP_EN__SET_TSF_HW_1      (1<<1)  // 0x00000002
#define PAC_NTD_EVT_RSP_EN__SET_SW_TX_0       (1<<16) // 0x00010000
#define PAC_NTD_EVT_RSP_EN__SET_SW_TX_1       (1<<17) // 0x00020000
#define PAC_NTD_EVT_RSP_EN__RESET_SW_TX_0     (1<<20) // 0x00100000
#define PAC_NTD_EVT_RSP_EN__RESET_SW_TX_1     (1<<21) // 0x00200000
#define PAC_NTD_EVT_RSP_EN__SET_EBM_SEQ_REQ   (1<<24) // 0x01000000
#define PAC_NTD_EVT_RSP_EN__RESET_EBM_SEQ_REQ (1<<28) // 0x10000000

#define PAC_NTD_EVT_RSP_EN__RESET_VALUE     ( PAC_NTD_EVT_RSP_EN__RESET_EBM_SEQ_REQ \
                                           | PAC_NTD_EVT_RSP_EN__RESET_SW_TX_1 \
                                           | PAC_NTD_EVT_RSP_EN__RESET_SW_TX_0)

/* ----------------------------------------------------------------*/
/* ST - "Rx event response enable" - Pg 52 | Tality - "rx_event_resp_enable" - 1.9.2 */
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_00        (1<<0)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_01        (1<<1)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_02        (1<<2)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_03        (1<<3)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_04        (1<<4)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_05        (1<<5)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_06        (1<<6)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_07        (1<<7)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_08        (1<<8)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_09        (1<<9)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_10        (1<<10)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_11        (1<<11)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_12        (1<<12)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_13        (1<<13)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_14        (1<<14)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_15        (1<<15)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_16        (1<<16)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_17        (1<<17)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_18        (1<<18)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_19        (1<<19)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_20        (1<<20)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_21        (1<<21)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_22        (1<<22)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_23        (1<<23)
#define PAC_NTD_RX_EVT_RSP_EN__RX_EVENT_24        (1<<24)
#define PAC_NTD_RX_EVT_RSP_EN__RESPONSE_TIMEOUT_A (1<<30)
#define PAC_NTD_RX_EVT_RSP_EN__RESPONSE_TIMEOUT_B (1<<31)

/* ----------------------------------------------------------------*/
/* ST - "Rx event response multi-shot" - Pg 53 (section number missing) | Tality - "rx_event_resp_multi_shot" - 1.9.3 */
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_00        (1<<0)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_01        (1<<1)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_02        (1<<2)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_03        (1<<3)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_04        (1<<4)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_05        (1<<5)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_06        (1<<6)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_07        (1<<7)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_08        (1<<8)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_09        (1<<9)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_10        (1<<10)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_11        (1<<11)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_12        (1<<12)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_13        (1<<13)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_14        (1<<14)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_15        (1<<15)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_16        (1<<16)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_17        (1<<17)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_18        (1<<18)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_19        (1<<19)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_20        (1<<20)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_21        (1<<21)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_22        (1<<22)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_23        (1<<23)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RX_EVENT_24        (1<<24)
#define PAC_NTD_RX_EVT_RSP_MULTISHOT__RESPONSE_TIMEOUT    (1<<31)


/* ----------------------------------------------------------------*/
/* Receiver control registers */
#define PAC_RXC_RX_CONTROL__ENABLE                    0x00000001
#define PAC_RXC_DISABLE_RX__ENABLE                    BIT(5)

#define PAC_RXC_RX_CONTROL__BAP_PHY_DISABLE_STATUS    0x00000002
#define PAC_RXC_RX_CONTROL__APPEND_ERROR              0x00000004
#define PAC_RXC_RX_CONTROL__APPEND_BAP_STATUS         0x00000008
#define PAC_RXC_RX_CONTROL__BT_PHY_DISABLE            0x00000010

#define PAC_RXC_RX_CONTROL__KEEP_BAD                  0x00008000
#define PAC_RXC_RX_CONTROL__AUTO_DISCARD              0x00010000
#define PAC_RXC_RX_CONTROL__AUTO_KEEP                 0x00020000
#define PAC_RXC_RX_CONTROL__CCA                       0x00040000
#define PAC_RXC_RX_CONTROL__CCA_VALID                 0x00080000
#define PAC_RXC_RX_CONTROL__BUFFER_NRFULL             0x00100000
#define PAC_RXC_RX_CONTROL__BUFFER_OFLOW              0x00200000
#define PAC_RXC_RX_CONTROL__FIFO_OFLOW                0x00400000
#define PAC_RXC_RX_CONTROL__BUSY                      0x00800000
#define PAC_RXC_RX_CONTROL__APPEND_STATUS             0x01000000
#define PAC_RXC_RX_CONTROL__APPEND_TSF                0x04000000
#define PAC_RXC_RX_CONTROL__SAVE_CSI_DATA             0x08000000
#define PAC_RXC_RX_CONTROL__DISABLE_DEAGGREGATION     0x40000000

#define PAC_RXC_RX_CONTROL__BUFFER_SIZE(__x) (((__x)<<4) & 0xF0)
#define PAC_RXC_RX_CONTROL__RESV_SIZE(__x)   (((__x)<<8) & 0x3F00)

/* ----------------------------------------------------------------*/

#if 0
#define PAC_DUR_MAP_M_PRIMARY       0x7F
#define PAC_DUR_MAP_V_PRIMARY       0
#define PAC_DUR_MAP_M_ALTERNATE     0x7F00
#define PAC_DUR_MAP_V_ALTERNATE     8
#define PAC_DUR_MAP_M_RATE          0xF0000
#define PAC_DUR_MAP_V_RATE          16
#define PAC_DUR_MAP_M_MODE          0x7000000
#define PAC_DUR_MAP_V_MODE          24
#define PAC_DUR_MAP_M_READ          0x80000000
#endif
// for 8601 second level DUR mapping register
#define PAC_DUR_WRITE_MODE_RATE_MAPPING_VAL(_mode,_rate,_pri_idx,_alt_idx) ( (((_mode)&0x07)<<24) | (((_rate)&0x0F)<<16) | (((_pri_idx)&0x7F)<<8)  | ((_alt_idx)&0x7F) )

/* ----------------------------------------------------------------*/
/* Next transmit decision registers */
#define PAC_NTD_TIMEOUT_BIT         0x80000000
#define PAC_NTD_TIMEOUT_A_BIT       0x40000000
#define PAC_NTD_TIMEOUT_B_BIT       0x80000000

/* ----------------------------------------------------------------*/
/* ST - "NTD control" - Pg 60 | Tality - "control" - 1.9.11 */
/* PAC_NTD_CONTROL fields */
#define PAC_NTD_CONTROL__EBM_SEQ_REQ        0x00000001

/* ----------------------------------------------------------------*/
/* ST - NTD: "Rx SW event enable" - Pg 55 | Tality - "rx_sw_event_enable" - 1.9.8 */
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_00        (1<<0)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_01        (1<<1)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_02        (1<<2)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_03        (1<<3)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_04        (1<<4)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_05        (1<<5)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_06        (1<<6)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_07        (1<<7)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_08        (1<<8)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_09        (1<<9)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_10        (1<<10)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_11        (1<<11)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_12        (1<<12)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_13        (1<<13)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_14        (1<<14)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_15        (1<<15)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_16        (1<<16)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_17        (1<<17)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_18        (1<<18)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_19        (1<<19)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_20        (1<<20)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_21        (1<<21)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_22        (1<<22)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_23        (1<<23)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_24        (1<<24)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_UNRECOGNISEDRX    (1<<25)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_RESPONSE_TIMEOUT    (1<<31)
#define PAC_NTD_RX_SW_EVT_EN__RX_EVENT_ALL_DISABLED    (0)

/* PAC_NTD_CONTROL fields */
#define PAC_NTD_CONTROL__NO_TX_0       0x00000001
#define PAC_NTD_CONTROL__NO_TX_1       0x00000002

/* ----------------------------------------------------------------*/
/* ST - "NTD status" - Pg 57 | Tality - "status" - 1.9.9 */
/* PAC_NTD_STATUS fields */

/* NTD Status - Empty [31] */
#define PAC_NTD_STATUS__EMPTY         ((uint32)(1<<31))  // 0x80000000  uint32 added as otherwise it was giving sign bit errors

/* NTD Status  - Overflow [30] */
#define PAC_NTD_STATUS__OVERFLOW               (1<<30)  // 0x40000000

/* NTD Status  - TSF Request Switch Status [29] */
#define PAC_NTD_STATUS__TSF_REQ_SWITCH_STATUS  (1<<29)  // 0x20000000

/* NTD Status  - TSF Request SwitchStatus [28] */
#define PAC_NTD_STATUS__TSF_REQ_SWITCH         (1<<28)  // 0x10000000

/* NTD Status  - TSF Hw Event 1 [27] */
#define PAC_NTD_STATUS__TSF_HW_EVENT_1         (1<<27)  // 0x08000000

/* NTD Status  - TSF Hw Event 0 [26] */
#define PAC_NTD_STATUS__TSF_HW_EVENT_0         (1<<26)  // 0x04000000

/* NTD Status  - PTCS event [25] */
#define PAC_NTD_STATUS__PTCS_EVENT             (1<<25)  // 0x02000000

/* NTD Status  - Response timeout/Rx event [24] */
#define PAC_NTD_STATUS__RESP_RX_EVENT          (1<<24)  // 0x01000000

/* NTD Status  - EBM Status / Int Conten [23] */
#define PAC_NTD_STATUS__EBM_STATUS__INT_CONTEN (1<<23)  // 0x00800000

/* NTD Status  - Ptcs status - Ptcs abort status [22:20] */
/* The Mask */
#define PAC_NTD_STATUS__PTCS_ABORT_STATUS__MASK    ((1<<22)|(1<<21)|(1<<20)) // 0x00700000
/* Masked Values */
#define PAC_NTD_STATUS__PTCS_ABORT_STATUS__READING_TTCS      (0<<20) // 0x00000000
#define PAC_NTD_STATUS__PTCS_ABORT_STATUS__IFS_RUNNING       (1<<20) // 0x00100000
#define PAC_NTD_STATUS__PTCS_ABORT_STATUS__IFS_COMPLETE      (2<<20) // 0x00200000
#define PAC_NTD_STATUS__PTCS_ABORT_STATUS__MEDIUM_BUSY       (3<<20) // 0x00300000
#define PAC_NTD_STATUS__PTCS_ABORT_STATUS__BACKOFF_COMPLETE  (4<<20) // 0x00400000
#define PAC_NTD_STATUS__PTCS_ABORT_STATUS__PHY_ERROR         (5<<20) // 0x00500000

/* NTD Status  - PTCS Status - Winning AC [19:18] */
/* The Mask */
#define PAC_NTD_STATUS__WINNING_AC__MASK     ((1<<19) |(1<<18)) // 0x000C0000
#define PAC_WINNING_AC_SHIFT 18

/* NTD Status - PTCS status - Event [17:16] */
/* Mask */
#define PAC_NTD_STATUS__PTCS_EVENT_STATUS__MASK  ((1<<17)|(1<<16)) // 0x00030000
/* Masked values */
#define PAC_NTD_STATUS__PTCS_EVENT_STATUS__NONE         (0<<16) // 0x00000000
#define PAC_NTD_STATUS__PTCS_EVENT_STATUS__ABORT        (1<<16) // 0x00010000
#define PAC_NTD_STATUS__PTCS_EVENT_STATUS__TTCS_END     (2<<16) // 0x00020000
#define PAC_NTD_STATUS__PTCS_EVENT_STATUS__RECIPE_END   (3<<16) // 0x00030000
/* transmit aborted by ePTA due to BT access */
#define PAC_NTD_STATUS__PTCS_EVENT_STATUS__BT_ABORT     (1<<14)
/* NTD Status  - PTCS Status - In_Txop [15] */
#define PAC_NTD_STATUS__IN_TXOP                (1<<15) // 0x00008000

/* NTD Status  - PTCS Status - BT_Abort [14] */
#define PAC_NTD_STATUS__BT_ABORT               (1<<14) // 0x00004000

/* NTD Status - PTCS Status - Request [13:8] */
/* Mask */
#define PAC_NTD_STATUS__PTCS_REQUEST__MASK              0x00003F00
#define PAC_NTD_STATUS__PTCS_REQUEST__OFFSET            8 /* NTD Status: For >> shifting [13:8] by 8 bits */
/* Values */
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_00               0
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_01               1
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_02               2
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_03               3
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_04               4
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_05               5
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_06               6
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_07               7
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_08               8
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_09               9
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_10              10
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_11              11
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_12              12
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_13              13
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_14              14
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_15              15
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_16              16
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_17              17
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_18              18
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_19              19
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_20              20
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_21              21
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_22              22
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_23              23
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_HIGH_24              24
#define PAC_NTD_STATUS__PTCS_REQUEST__TSF_HW_EVENT_0                25 /*tx bcn for ibss*/
#define PAC_NTD_STATUS__PTCS_REQUEST__TSF_HW_EVENT_1                26
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_00               27
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_01               28
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_02               29
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_03               30
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_04               31
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_05               32
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_06               33
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_07               34
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_08               35
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_09               36
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_10               37
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_11               38
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_12               39
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_13               40
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_14               41
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_15               42
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_16               43
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_17               44
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_18               45
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_19               46
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_20               47
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_21               48
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_22               49
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_23               50
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_24               51
#define PAC_NTD_STATUS__PTCS_REQUEST__RX_EVENT_LOW_RESPONSE_TIMEOUT 52
#define PAC_NTD_STATUS__PTCS_REQUEST__SW_TX_REQUEST_0               53 /* medium protection CTS-to-itself*/
#define PAC_NTD_STATUS__PTCS_REQUEST__SW_TX_REQUEST_1               54
#define PAC_NTD_STATUS__PTCS_REQUEST__EBM_SEQ_REQ                   55 /*request txop at rts*/
#define PAC_NTD_STATUS__PTCS_REQUEST__IDLE                          57 /* noticed as being inaccurate during CM00024855 work */

/* NTD Status - Response Timeout / Rx status - Stored/Response timeout event A [7] */
#define PAC_NTD_STATUS__RESP_RX_STORED       (1<<7)  // 0x00000080

/* NTD Status - Response Timeout / Rx status - Event [5:0] */
/* Mask */
#define PAC_NTD_STATUS__RESP_RX_EVENT_STATUS  0x0000003F
/* Values */
#define    PAC_NTD_STATUS__RX_EVENT_00                 0
#define    PAC_NTD_STATUS__RX_EVENT_01                 1
#define    PAC_NTD_STATUS__RX_EVENT_02                 2
#define    PAC_NTD_STATUS__RX_EVENT_03                 3
#define    PAC_NTD_STATUS__RX_EVENT_04                 4
#define    PAC_NTD_STATUS__RX_EVENT_05                 5
#define    PAC_NTD_STATUS__RX_EVENT_06                 6
#define    PAC_NTD_STATUS__RX_EVENT_07                 7
#define    PAC_NTD_STATUS__RX_EVENT_08                 8
#define    PAC_NTD_STATUS__RX_EVENT_09                 9
#define    PAC_NTD_STATUS__RX_EVENT_10                10
#define    PAC_NTD_STATUS__RX_EVENT_11                11
#define    PAC_NTD_STATUS__RX_EVENT_12                12
#define    PAC_NTD_STATUS__RX_EVENT_13                13
#define    PAC_NTD_STATUS__RX_EVENT_14                14
#define    PAC_NTD_STATUS__RX_EVENT_15                15
#define    PAC_NTD_STATUS__RX_EVENT_16                16
#define    PAC_NTD_STATUS__RX_EVENT_17                17
#define    PAC_NTD_STATUS__RX_EVENT_18                18
#define    PAC_NTD_STATUS__RX_EVENT_19                19
#define    PAC_NTD_STATUS__RX_EVENT_20                20
#define    PAC_NTD_STATUS__RX_EVENT_21                21
#define    PAC_NTD_STATUS__RX_EVENT_22                22
#define    PAC_NTD_STATUS__RX_EVENT_23                23
#define    PAC_NTD_STATUS__RX_EVENT_24                24
#define    PAC_NTD_STATUS__RX_EVENT_RESPONSE_TIMEOUT  25
#define    PAC_NTD_STATUS__RX_EVENT_UNRECOGNISEDRX    26


/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

//If the register offset is 29 ie 1<<29=bit 30
// return the offset 30 making the bit set the top bit.
#define PAC_THUNK_FOR_TALITY(_a) \
    ((_a==30)?(31):(_a))


/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* ST - RXD missing | Tality - "event_enable" - 1.6.1 */
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_00       (1<<0)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_01       (1<<1)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_02       (1<<2)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_03       (1<<3)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_04       (1<<4)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_05       (1<<5)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_06       (1<<6)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_07       (1<<7)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_08       (1<<8)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_09       (1<<9)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_10       (1<<10)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_11       (1<<11)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_12       (1<<12)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_13       (1<<13)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_14       (1<<14)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_15       (1<<15)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_16       (1<<16)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_17       (1<<17)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_18       (1<<18)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_19       (1<<19)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_20       (1<<20)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_21       (1<<21)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_22       (1<<22)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_23       (1<<23)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_24       (1<<24)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_25_BAB   (1<<25)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_26_BAB   (1<<26)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_27_DUR   (1<<27)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_28_DUR   (1<<28)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_29_DUR   (1<<29)
#define PAC_RXD_EVENT_ENABLE__RX_EVENT_30_DUR   (1<<30)

/* ST - RXD missing | Tality - "rx_enable" - 1.6.2 */
#define PAC_RXD_RX_ENABLE__RX_EVENT_00        (1<<0)
#define PAC_RXD_RX_ENABLE__RX_EVENT_01        (1<<1)
#define PAC_RXD_RX_ENABLE__RX_EVENT_02        (1<<2)
#define PAC_RXD_RX_ENABLE__RX_EVENT_03        (1<<3)
#define PAC_RXD_RX_ENABLE__RX_EVENT_04        (1<<4)
#define PAC_RXD_RX_ENABLE__RX_EVENT_05        (1<<5)
#define PAC_RXD_RX_ENABLE__RX_EVENT_06        (1<<6)
#define PAC_RXD_RX_ENABLE__RX_EVENT_07        (1<<7)
#define PAC_RXD_RX_ENABLE__RX_EVENT_08        (1<<8)
#define PAC_RXD_RX_ENABLE__RX_EVENT_09        (1<<9)
#define PAC_RXD_RX_ENABLE__RX_EVENT_10        (1<<10)
#define PAC_RXD_RX_ENABLE__RX_EVENT_11        (1<<11)
#define PAC_RXD_RX_ENABLE__RX_EVENT_12        (1<<12)
#define PAC_RXD_RX_ENABLE__RX_EVENT_13        (1<<13)
#define PAC_RXD_RX_ENABLE__RX_EVENT_14        (1<<14)
#define PAC_RXD_RX_ENABLE__RX_EVENT_15        (1<<15)
#define PAC_RXD_RX_ENABLE__RX_EVENT_16        (1<<16)
#define PAC_RXD_RX_ENABLE__RX_EVENT_17        (1<<17)
#define PAC_RXD_RX_ENABLE__RX_EVENT_18        (1<<18)
#define PAC_RXD_RX_ENABLE__RX_EVENT_19        (1<<19)
#define PAC_RXD_RX_ENABLE__RX_EVENT_20        (1<<20)
#define PAC_RXD_RX_ENABLE__RX_EVENT_21        (1<<21)
#define PAC_RXD_RX_ENABLE__RX_EVENT_22        (1<<22)
#define PAC_RXD_RX_ENABLE__RX_EVENT_23        (1<<23)
#define PAC_RXD_RX_ENABLE__RX_EVENT_24        (1<<24)
#define PAC_RXD_RX_ENABLE__RX_EVENT_UNRECOGNISEDRX    (1<<25)

/* ST - RXD missing | Tality - "all_events" - 1.6.3 */
#define PAC_RXD_ALL_EVENTS__RX_EVENT_00        (1<<0)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_01        (1<<1)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_02        (1<<2)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_03        (1<<3)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_04        (1<<4)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_05        (1<<5)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_06        (1<<6)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_07        (1<<7)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_08        (1<<8)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_09        (1<<9)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_10        (1<<10)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_11        (1<<11)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_12        (1<<12)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_13        (1<<13)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_14        (1<<14)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_15        (1<<15)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_16        (1<<16)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_17        (1<<17)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_18        (1<<18)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_19        (1<<19)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_20        (1<<20)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_21        (1<<21)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_22        (1<<22)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_23        (1<<23)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_24         (1<<24)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_25_BAB     (1<<25)
#define PAC_RXD_ALL_EVENTS__RX_EVENT_26_BAB     (1<<26)

/* ----------------------------------------------------------------*/
/* ST - RXD missing | Tality - "frame_abort" - 1.6.4 */
#define PAC_RXD_FRAME_ABORT__OVERFLOW         (1<<0)
#define PAC_RXD_FRAME_ABORT__PHY_ERROR        (1<<1)
#define PAC_RXD_FRAME_ABORT__CRC_ERROR        (1<<2)


/* ----------------------------------------------------------------*/
/* Pattern Matcher Condition/Filter match values */
/* Condition A */
#define PAC_RXF_BYTE_FILTER_00__MATCH           (1<<0)
#define PAC_RXF_BYTE_FILTER_01__MATCH           (1<<1)
#define PAC_RXF_BYTE_FILTER_02__MATCH           (1<<2)
#define PAC_RXF_BYTE_FILTER_03__MATCH           (1<<3)
#define PAC_RXF_BYTE_FILTER_04__MATCH           (1<<4)
#define PAC_RXF_BYTE_FILTER_05__MATCH           (1<<5)
#define PAC_RXF_BYTE_FILTER_06__MATCH           (1<<6)
#define PAC_RXF_BYTE_FILTER_07__MATCH           (1<<7)
#define PAC_RXF_BYTE_FILTER_08__MATCH           (1<<8)
#define PAC_RXF_BYTE_FILTER_09__MATCH           (1<<9)
#define PAC_RXF_BYTE_FILTER_10__MATCH           (1<<10)
#define PAC_RXF_BYTE_FILTER_11__MATCH           (1<<11)
#define PAC_RXF_BYTE_FILTER_12__MATCH           (1<<12)
#define PAC_RXF_BYTE_FILTER_13__MATCH           (1<<13)
#define PAC_RXF_BYTE_FILTER_14__MATCH           (1<<14)
#define PAC_RXF_BYTE_FILTER_15__MATCH           (1<<15)
#define PAC_RXF_BYTE_FILTER_16__MATCH           (1<<16)
#define PAC_RXF_BYTE_FILTER_17__MATCH           (1<<17)
#define PAC_RXF_BYTE_FILTER_18__MATCH           (1<<18)
#define PAC_RXF_BYTE_FILTER_19__MATCH           (1<<19)
#define PAC_RXF_BYTE_FILTER_20__MATCH           (1<<20)
#define PAC_RXF_BYTE_FILTER_21__MATCH           (1<<21)
#define PAC_RXF_BYTE_FILTER_22__MATCH           (1<<22)
#define PAC_RXF_TATID_FILTER__MATCH             (1<<23)
#define PAC_RXF_LONG_BYTE_FILTER_0__MATCH       (1<<24)
#define PAC_RXF_LONG_BYTE_FILTER_1__MATCH       (1<<25)
#define PAC_RXF_LONG_BYTE_FILTER_2__MATCH       (1<<26)
#define PAC_RXF_NORMAL_ACK_IN_AGG_0_FILTER__MATCH (1<<27)
#define PAC_RXF_END_OF_AGGR_0_FILTER__MATCH       (1<<28)
#if 1 // (ASIC_1250_CUT_1)
#define PAC_RXF_LONG_BYTE_FILTER_0_OR_2__MATCH  (1<<29)
#else
#define PAC_RXF_TSF_COMPARE_FILTER__MATCH       (1<<29)
#endif
#define PAC_RXF_FRAME_ABORT_FILTER__MATCH       (1<<30)
#define PAC_RXF_RX_CRC_ERROR__MATCH             (u32)(1<<31)

/* Condition B */
#define PAC_RXF_WITHIN_AGGR_0_FILTER__MATCH       (1<<0)    // Condition 32 match
#define PAC_RXF_WITHIN_AGGR_1_FILTER__MATCH       (1<<1)    // Condition 33 mismatch
#define PAC_RXF_END_OF_AGGR_1_FILTER__MATCH       (1<<2)    // Condition 34
#define PAC_RXF_NORMAL_ACK_IN_AGG_1_FILTER__MATCH (1<<3)    // Condition 35
#define PAC_RXF_BYTE_FILTER_23__MATCH           (1<<4)    // Condition 36
#define PAC_RXF_BYTE_FILTER_24__MATCH           (1<<5)    // Condition 37
#define PAC_RXF_BYTE_FILTER_25__MATCH           (1<<6)    // Condition 38
#define PAC_RXF_BYTE_FILTER_26__MATCH           (1<<7)    // Condition 39

#define PAC_RXF_LONG_BYTE_FILTER_3__MATCH       (1<<12)   // Condition 44


/* ----------------------------------------------------------------*/
/* Tality: 1.6.1, 1.6.2, 1.6.3, 1.6.4 */
//First part of 64-bit condition register
#define PAC_RXD_CONDITION_00__MASK      (1<<0)
#define PAC_RXD_CONDITION_01__MASK      (1<<1)
#define PAC_RXD_CONDITION_02__MASK      (1<<2)
#define PAC_RXD_CONDITION_03__MASK      (1<<3)
#define PAC_RXD_CONDITION_04__MASK      (1<<4)
#define PAC_RXD_CONDITION_05__MASK      (1<<5)
#define PAC_RXD_CONDITION_06__MASK      (1<<6)
#define PAC_RXD_CONDITION_07__MASK      (1<<7)
#define PAC_RXD_CONDITION_08__MASK      (1<<8)
#define PAC_RXD_CONDITION_09__MASK      (1<<9)
#define PAC_RXD_CONDITION_10__MASK      (1<<10)
#define PAC_RXD_CONDITION_11__MASK      (1<<11)
#define PAC_RXD_CONDITION_12__MASK      (1<<12)
#define PAC_RXD_CONDITION_13__MASK      (1<<13)
#define PAC_RXD_CONDITION_14__MASK      (1<<14)
#define PAC_RXD_CONDITION_15__MASK      (1<<15)
#define PAC_RXD_CONDITION_16__MASK      (1<<16)
#define PAC_RXD_CONDITION_17__MASK      (1<<17)
#define PAC_RXD_CONDITION_18__MASK      (1<<18)
#define PAC_RXD_CONDITION_19__MASK      (1<<19)
#define PAC_RXD_CONDITION_20__MASK      (1<<20)
#define PAC_RXD_CONDITION_21__MASK      (1<<21)
#define PAC_RXD_CONDITION_22__MASK      (1<<22)
#define PAC_RXD_CONDITION_23__MASK      (1<<23)
#define PAC_RXD_CONDITION_24__MASK      (1<<24)
#define PAC_RXD_CONDITION_25__MASK      (1<<25)
#define PAC_RXD_CONDITION_26__MASK      (1<<26)
#define PAC_RXD_CONDITION_27__MASK      (1<<27)
#define PAC_RXD_CONDITION_28__MASK      (1<<28)
#define PAC_RXD_CONDITION_29__MASK      (1<<29)
#define PAC_RXD_CONDITION_30__MASK      (1<<30)
#define PAC_RXD_CONDITION_31__MASK      (1<<31)

//Second part of 64-bit condition register
#define PAC_RXD_CONDITIONB_00__MASK     (1<<0)        /* Condition 32 */
#define PAC_RXD_CONDITIONB_01__MASK     (1<<1)        /* Condition 33 */
#define PAC_RXD_CONDITIONB_02__MASK     (1<<2)        /* Condition 34 */
#define PAC_RXD_CONDITIONB_03__MASK     (1<<3)        /* Condition 35 */
#define PAC_RXD_CONDITIONB_04__MASK     (1<<4)        /* Condition 36 */
#define PAC_RXD_CONDITIONB_05__MASK     (1<<5)        /* Condition 37 */
#define PAC_RXD_CONDITIONB_06__MASK     (1<<6)        /* Condition 38 */
#define PAC_RXD_CONDITIONB_07__MASK     (1<<7)        /* Condition 39 */
#define PAC_RXD_CONDITIONB_08__MASK     (1<<8)        /* Condition 40 */
#define PAC_RXD_CONDITIONB_09__MASK     (1<<9)        /* Condition 41 */
#define PAC_RXD_CONDITIONB_10__MASK     (1<<10)       /* Condition 42 */
#define PAC_RXD_CONDITIONB_11__MASK     (1<<11)       /* Condition 43 */
#define PAC_RXD_CONDITIONB_12__MASK     (1<<12)       /* Condition 44 */
#if 0
#define PAC_RXD_CONDITIONB_13__MASK     (1<<13)       /* Condition 45 */
#define PAC_RXD_CONDITIONB_14__MASK     (1<<14)       /* Condition 46 */
#define PAC_RXD_CONDITIONB_15__MASK     (1<<15)       /* Condition 47 */
#define PAC_RXD_CONDITIONB_16__MASK     (1<<16)       /* Condition 48 */
#define PAC_RXD_CONDITIONB_17__MASK     (1<<17)       /* Condition 49 */
#define PAC_RXD_CONDITIONB_18__MASK     (1<<18)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_19__MASK     (1<<19)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_20__MASK     (1<<20)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_21__MASK     (1<<21)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_22__MASK     (1<<22)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_23__MASK     (1<<23)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_24__MASK     (1<<24)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_25__MASK     (1<<25)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_26__MASK     (1<<26)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_27__MASK     (1<<27)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_28__MASK     (1<<28)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_29__MASK     (1<<29)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_30__MASK     (1<<30)       /* Condition 32 */
#define PAC_RXD_CONDITIONB_31__MASK     (1<<31)       /* Condition 32 */
#endif

#define PAC_RXF__BYTE_FILTER_REFERENCE   0x000000FF
#define PAC_RXF__BYTE_FILTER_MASK        0x0000FF00
#define PAC_RXF__BYTE_FILTER_OFFSET      0x00FF0000

#define USE_DYNAMIC_FILTERS                1
#define DONT_USE_DYNAMIC_FILTERS           0
#define ENABLE_DYNAMIC_FILTER              1
#define DISABLE_DYNAMIC_FILTER             0

// for 8601 extractors - will be set by default after initial fpga versions
#define PAC_RXF_MAKE_EXTRACT_CONTROL_VAL(_offset, _dyn)    (  (( (_offset) & 0xFF) << 16) |  (( (_dyn) & 0x01)<<31))

//If _dyn is TRUE, means that dynamic offset is used, FALSE means it is not used.
#define PAC_RXD_MAKE_BYTE_FILTER_VAL(_r,_m,_o, _dyn)    (( (_r) & 0xFF) | (( (_m) & 0xFF) << 8) | (( (_o) & 0xFF) << 16) |  (( (_dyn) & 0x01)<<31))
#define PAC_RXD_MAKE_DYNAMIC_FILTER_VAL(_r,_m,_o,_o1,_en)    (( (_r) & 0xFF) | (( (_m) & 0xFF) << 8) | (( (_o) & 0xFF) << 16) | (( (_o1) & 0x7F) << 24) | (((_en) & 0x01)<<31))

#define PAC_LF__USE_EXTRACTOR_DEFAULT       0
#define PAC_LF__USE_EXTRACTOR_A1            1
#define PAC_LF__USE_EXTRACTOR_A2            2
#define PAC_LF__USE_EXTRACTOR_A3            3

#define PAC_LF__MATCH_PATTERN_0             0
#define PAC_LF__MATCH_PATTERN_1             1
#define PAC_LF__MATCH_PATTERN_DEFAULT       1

/*Macro to setup Extractor and Reference select as per CUT-2 register changes*/
#define PAC_RXD_MAKE_LONG_FILTER_CTL(_ext,_ref) ((_ref & 0x01) | (( (_ext) & 0x3) << 8))

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/
/* Timer registers */
#define PAC_TIM_TSF_NAV_LOAD__ENABLE      0x40000000
#define PAC_TIM_TSF_NAV_LOAD__TIME_MASK   0x00FFFFFF
#define PAC_TIM_RXD_TSF_COMP__ENABLE      0x80000000
#define PAC_TIM_TICK_TIMER__ENABLE        0x80000000
#define PAC_TIM_TICK_TIMER__MASK          0xFFFF
#define PAC_TIM_TIMER_CONTROL__EVT_MASK   0x1F000000
#define PAC_TIM_TIMER_CONTROL__EVT_4      0x10000000
#define PAC_TIM_TIMER_CONTROL__EVT_3      0x08000000
#define PAC_TIM_TIMER_CONTROL__EVT_2      0x04000000
#define PAC_TIM_TIMER_CONTROL__EVT_1      0x02000000
#define PAC_TIM_TIMER_CONTROL__EVT_0      0x01000000
#define PAC_TIM_TIMER_CONTROL__TSF_ADJUST 0x0001FF00
#define PAC_TIM_TIMER_CONTROL__TSF_NUDGE  0x4
#define PAC_TIM_TIMER_CONTROL__TSF_ENABLE 0x2
#define PAC_TIM_TIMER_CONTROL__NAV_ENABLE 0x1
#define PAC_TIM_TIMER_CONTROL__DEFAULT    (PAC_TIM_TIMER_CONTROL__TSF_ENABLE|PAC_TIM_TIMER_CONTROL__NAV_ENABLE)
#define PAC_TIM_SW_ENABLE                 0x80000000
#define PAC_TIM_HW_ENABLE                 0x40000000
#define PAC_TIM_TSF_TIME_MASK             0x00FFFFFF

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* Block Acknowledgement Registers */
#define PAC_BAB_BLOCK_ENABLE         (0x01000000)
#define PAC_BAB_MODE_DISABLED        (0x00)
#define PAC_BAB_MODE_BASIC           (0x01)
#define PAC_BAB_MODE_COMPRESSED      (0x02)
#define PAC_BAB_MODE_HTI             (0x03)

//BA Control Field definitions
#define PAC_COMPRESSED_MULTITID      (1 << 1)
#define PAC_COMPRESSED_BITMAP        (1 << 2)

/* 11.16.4.1    Mode Control Register */
/* a. SMC Offsets  - at 32 x 32bit Offsets = 1024 bits */
#define PAC_BAB_SMC_OFFSET__0              (0x00)
#define PAC_BAB_SMC_OFFSET__1              (0x20)
#define PAC_BAB_SMC_OFFSET__2              (0x40)
#define PAC_BAB_SMC_OFFSET__3              (0x60)

/* b. Mode */
#define PAC_BAB_MODE_CONTROL__MODE_DISABLED             (0x00)
#define PAC_BAB_MODE_CONTROL__BASIC                (0x01)
#define PAC_BAB_MODE_CONTROL__COMPRESSED           (0x02)
#define PAC_BAB_MODE_CONTROL__HTI                  (0x03)

/* c. Tid BA Control */
#define PAC_BAB_MODE_CONTROL__TIDBA_CTRL(TID, COMPRESSED, MULTITID, ACKPOLICY )     \
        (   ( ((TID) & 0xf) << 12)        \
          | ( ((COMPRESSED) & 1) << 2)    \
          | ( ((MULTITID)  & 1) << 1)      \
          |   ((ACKPOLICY) & 1)       )

//SMC Offsets at 32 x 32bit Offsets = 1024 bits
#define PAC_BAB_SMC_OFFSET_0         (0x00 << 8)
#define PAC_BAB_SMC_OFFSET_1         (0x20 << 8)
#define PAC_BAB_SMC_OFFSET_2         (0x40 << 8)
#define PAC_BAB_SMC_OFFSET_3         (0x60 << 8)

#define PAC_BAB_SMC_OFFSET           (0x20) // Size in uint32s of individual bitmap memory
#define PAC_BAB_REGISTER_OFFSET      (0x20) // Address offset between BAB Agreement registers
#define PAC_BAB_TA_TID_OFFSET        (0x0C) // Address offset between TA TID Filter Registers

#define PAC_BAB_WC_WININIT           (0x80000000)
#define PAC_BAB_WC_WINSIZE_PSN       (16)
#define PAC_BAB_WC_WINSIZE_MASK      (0x3F)
#define PAC_BAB_WC_WINSTART_SCC_PSN  (4)
#define PAC_BAB_WC_WINSTART_MASK     (0xfff)
#define PAC_BAB_MC_SMCOFFSET         (8)
#define PAC_BAB_MC_BACONTROL         (16)

#define PAC_BAB_HEAD_SC_MASK         (0xFFFF)

#if 1 // USE_CM00018794_BAP
/* Beacon Assist Processor PAC_BAP_CONTROL register definitions */
#define PAC_BAP_CTL__ENABLE                ( 1 << 4 )
#define PAC_BAP_CTL__ABRT_NO_TRFC_PEND     ( 1 << 3 )
#define PAC_BAP_CTL__DIS_NO_TRFC_PEND      ( 1 << 2 )
#define PAC_BAP_CTL__ABRT_BSSID_MISMATCH   ( 1 << 1 )
#define PAC_BAP_CTL__DIS_BSSID_MISMATCH    ( 1 << 0 )

/* Beacon Assist Processor PAC_BAP_STATUS_TIM_INFO register definitions */
#define PAC_BAP_TIM__FRAME_IS_BEACON       ( (uint32)(1 << 31))
#define PAC_BAP_TIM__NOT_OUR_BEACON        ( 1 << 30)
#define PAC_BAP_TIM__BEACON_CRC_MATCH      ( 1 << 29)
#define PAC_BAP_TIM__NO_PENDING_TRAFFIC    ( 1 << 28)
#define PAC_BAP_TIM__MCAST_PENDING         ( 1 << 27)
#define PAC_BAP_TIM__UCAST_PENDING         ( 1 << 26)
#define PAC_BAP_TIM__AID_PRESENT           ( 1 << 25)

#define PAC_BAP_TIM__BEACON_MASK           ( PAC_BAP_TIM__FRAME_IS_BEACON | PAC_BAP_TIM__NOT_OUR_BEACON )
#define PAC_BAP_TIM__CRC_CLEAR             ( 0xDFFFFFFF )

#endif

/* Sequencer Registers */
#define PAC_SEQ_CONTROL__T_FLAG      0x000000FF
#define PAC_SEQ_CONTROL__T_AR_FLAG   0x0000FF00
#define PAC_SEQ_CONTROL__T_AE_FLAG   0x00FF0000
#define PAC_SEQ_CONTROL__T_WRITE_P   0x07000000
#define PAC_SEQ_CONTROL__T_READ_P    0x38000000
#define PAC_SEQ_CONTROL__T_Q_N_FULL  0x40000000
#define PAC_SEQ_CONTROL__T_Q_FULL    0x80000000

#define PAC_SEQ_WRITEPORT__RES_EXP   (1<<13)

/* EBM Registers */
#define PAC_EBM__ENABLE              0x00001
#define PAC_EBM__CLEAR_BO            0x00002
#define PAC_EBM__RESET_ON_CCA        0x00004
#define PAC_EBM__RESET_ON_NAV        0x00008
#define PAC_EBM__ABORT_ON_CCA        0x00010
#define PAC_EBM__ABORT_ON_NAV        0x00020
#define PAC_EBM__TXOP_AUTO_EXTEND    0x00040
#define PAC_EBM__DUR_COMPLETE        0x00080
#define PAC_EBM__PRIORITY            0x00100

#define PAC_EBM__CNTL_INT_CONT           0x0000F
#define PAC_EBM__INT_CONT_MASK           0xFFFFFFFF
#define PAC_EBM__CLR_INT_CONT            0xFFFFFFF0
#define PAC_EBM__CNTL_FRAME_ERR          0x000F0
#define PAC_EBM__CNTL_ACK_FAIL           0x00F00
#define PAC_EBM__CNTL_TX_ERR             0x0F000
#define PAC_SEQ__CONTROL_REG_OFFSET      0x0020 // Control register offset from Queue Base address
#define PAC_SEQ__CONTROL_PTR_REG_OFFSET  0x001C // Control register offset from Queue Base address
#define PAC_SEQ__CONTROL_FLAGS           0x0018

#define PAC_EBM__CNTL_INT_CONT_BASE   0x00001
#define PAC_EBM__CNTL_FRAME_ERR_BASE  0x00010
#define PAC_EBM__CNTL_FRAME_ERR_MASK  0x000F0
#define PAC_EBM__CNTL_ACK_FAIL_BASE   0x00100
#define PAC_EBM__CNTL_TX_ERR_BASE     0x01000
#define PAC_EBM__CNTL_TX_ERR_MASK     0x0F000
#define PAC_EBM__WINNING_AC_MASK     0x000C0000

#define PAC_EBM__TXOP_CLEAR_OFFSET   25

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* PTCS pointer table related stuff (TTCS, PTCS, RECIPE) */
#define MAX_RECIPES    32

            /* RETRO IFS IN 1/8 Us */
#define PAC_TTCS_IFSA__RETRO_IFS_MASK                0x00001FFF
#define PAC_TTCS_IFSA__RETRO_IFS_ENABLE              0x00002000
#define PAC_TTCS_IFSA__RESET_CCA_BSY                 0x00004000
#define PAC_TTCS_IFSA__RESET_NAV_BSY                 0x00008000
            /* CRC OK IFS IN 1/8 Us */
#define PAC_TTCS_IFSA__CRC_OK_IFS_MASK               0x1FFF0000
#define PAC_TTCS_IFSA__CRC_OK_IFS_SHIFT(_a)          (_a << 16)
#define PAC_TTCS_IFSA__CRC_OK_IFS_ENABLE             0x20000000
#define PAC_TTCS_IFSA__ABORT_PTCS_ON_RX              0x40000000
#define PAC_TTCS_IFSA__ABORT_ON_NEW_NTD              0x80000000

            /* CRC ERROR IFS IN 1/8 Us */
#define PAC_TTCS_IFSB__CRC_ERROR_IFS_MASK            0x1FFF
#define PAC_TTCS_IFSB__CRC_ERROR_IFS_ENABLE          0x8000

/* BACKOFF IN SLOTS! */
#define PAC_TTCS_BACKOFF__LEGACY_SLOTS_MASK          0x03FF
#define PAC_TTCS_BACKOFF__EDCA_SLOTS_MASK            0x0FFF
#define PAC_TTCS_BACKOFF__EDCA_STARTBIT              10

#define PAC_TTCS_EXP_RX_EVENT_ENABLE                 0x0080
#define PAC_TTCS_BACKOFF__ENABLE                     0x0400
#define PAC_TTCS_BACKOFF__ABORT_ON_CCA_BSY           0x0800
#define PAC_TTCS_BACKOFF__ABORT_ON_NAV_BSY           0x1000
#define PAC_TTCS_BACKOFF__RESTART_IFS_ON_NAV_CCA_BSY 0x2000
#define PAC_TTCS_BACKOFF__ABORT_ON_NEW_NTD           0x4000
#define PAC_TTCS_BACKOFF__RESET                      0x8000

#define PAC_TTCS_TIMEOUT__MASK                       0x00001FFF
            /* RESPONSE TIMEOUT IN 1/8 Us */
#define PAC_TTCS_TIMEOUT__ENABLE                     0x00002000
#define PAC_TTCS_TIMEOUT__IFS_COMP_FLAG              0x00004000
#define PAC_TTCS_TIMEOUT__RESET_ON_CCA_BSY           0x00008000
#define PAC_TTCS_TIMEOUT__RESPONSE_EVENT_DISABLE     0x04000000
#define PAC_TTCS_TIMEOUT__ENABLE_RECIPE_END_EVENT    0x08000000
#define PAC_TTCS_TIMEOUT__ENABLE_RECIPE              0x10000000
#define PAC_TTCS_TIMEOUT__RE_Q_PTCS_IF_ABORTED       0x20000000
#define PAC_TTCS_TIMEOUT__ENABLE_TTCS_END_EVENT      0x40000000
#define PAC_TTCS_TIMEOUT__ABORT_RECIPE_ON_NEW_NTD    0x80000000

#define PAC_RECIPE_OFFSET_IN_PTCS                   0x3 //in uint32s

#define PAC_PTCS_POINTER_MASK                       0x003FFFFC


/* To get the address of the PTCS pointer the following will need to be */
/* added to the PAC_TXC_PTCS_PTR_TABLE_BASE */
/* ST - "PTCS pointer table" - Pg 49 - 11.4.4 */
#define PAC_PTCS_TABLE__TSF_HW_0            0
#define PAC_PTCS_TABLE__TSF_HW_1            1
#define PAC_PTCS_TABLE__RX_EVENT_00         2
#define PAC_PTCS_TABLE__RX_EVENT_01         3
#define PAC_PTCS_TABLE__RX_EVENT_02         4
#define PAC_PTCS_TABLE__RX_EVENT_03         5
#define PAC_PTCS_TABLE__RX_EVENT_04         6
#define PAC_PTCS_TABLE__RX_EVENT_05         7
#define PAC_PTCS_TABLE__RX_EVENT_06         8
#define PAC_PTCS_TABLE__RX_EVENT_07         9
#define PAC_PTCS_TABLE__RX_EVENT_08        10
#define PAC_PTCS_TABLE__RX_EVENT_09        11
#define PAC_PTCS_TABLE__RX_EVENT_10        12
#define PAC_PTCS_TABLE__RX_EVENT_11        13
#define PAC_PTCS_TABLE__RX_EVENT_12        14
#define PAC_PTCS_TABLE__RX_EVENT_13        15
#define PAC_PTCS_TABLE__RX_EVENT_14        16
#define PAC_PTCS_TABLE__RX_EVENT_15        17
#define PAC_PTCS_TABLE__RX_EVENT_16        18
#define PAC_PTCS_TABLE__RX_EVENT_17        19
#define PAC_PTCS_TABLE__RX_EVENT_18        20
#define PAC_PTCS_TABLE__RX_EVENT_19        21
#define PAC_PTCS_TABLE__RX_EVENT_20        22
#define PAC_PTCS_TABLE__RX_EVENT_21        23
#define PAC_PTCS_TABLE__RX_EVENT_22        24
#define PAC_PTCS_TABLE__RX_EVENT_23        25
#define PAC_PTCS_TABLE__RX_EVENT_24        26
#define PAC_PTCS_TABLE__RESPONSE_TIMEOUT_A 27
#define PAC_PTCS_TABLE__RESPONSE_TIMEOUT_B 28
#define PAC_PTCS_TABLE__SW_TX_0            29
#define PAC_PTCS_TABLE__SW_TX_1            30
#define PAC_PTCS_TABLE__EBM_SEQ_REQ        31

#define PAC_RESOLVE_ERROR 9999
#define PAC_NTD_EVENT_RESP_ENABLE_DISABLE_OFFSET 4 // related to ST - event response enable/request | Tality 1.9.1  - for converting Sw Tx request 0 "set" (16)   -to-   Sw Tx request "reset" (20)

/* ----------------------------------------------------------------*/
//#define RESOLVE_RX_EVENT_RESP_OFFSET(_a) (_a-2)
#define PAC_EVENT_IS_RX_EVENT(_a) \
    (((_a<PAC_PTCS_TABLE__RX_EVENT_00)|| \
      (_a>PAC_PTCS_TABLE__RESPONSE_TIMEOUT_B))?(FALSE):(TRUE))

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* Recipe instructios */
#define PAC_HOB__RESP_RATE                0x000
#define PAC_HOB__CTS_DUR                  0x004
#define PAC_HOB__ACK_DUR                  0x008
#define PAC_HOB__CFP_DUR_REM              0x00C
#define PAC_HOB__ACK_BIT                  0x010
#define PAC_HOB__PM_BIT                   0x011
#define PAC_HOB__TX_TIM                   0x400
#define PAC_HOB__TX_CRC                   0x600
#define PAC_DELIA__TX_CRC                 PAC_DELIA__TX_HOB(4,PAC_HOB__TX_CRC)
#define PAC_DELIA__TX_TS                  PAC_DELIA__TX_HOB(8,PAC_HOB__TX_TIM)

//Type-0
//Transmit    N12, (R+OFS12)    //E000 0RRR NNNN NNNN NNNN AAAA AAAA AAAA
#define PAC_DELIA__TX_REG(_n,_r,_o)       (0x00000000 | (((_n) & (uint32)0xFFF)<<12) | (((_r) & 0x7)<<24) | ((_o) & (uint32)0xFFF))

//Transmit    N12, (HOB+OFS12)        //E000 0111 NNNN NNNN NNNN AAAA AAAA AAAA
#define PAC_DELIA__TX_HOB(_n,_o)          (0x07000000 | (((_n) & 0xFFF)<<12) | ((_o) & (uint32)0xFFF))

//Type-2
//Add Base    R, IMM12        //E001 0RRR 0000 0000 0000 DDDD DDDD DDDD
#define PAC_DELIA__ADD_BASE(_r,_i)        (0x10000000 | (((_r) & 0x3)<<24) | ((_i) & (uint32)0xFFF))

//Type-2
//Transmit    N5, (ABS23)  //E010 NNNN NAAA AAAA AAAA AAAA AAAA AAAA
#define PAC_DELIA__TX(_n,_a)              (0x20000000 | (((_n) & 0x1F)<<23) | ((_a) & (uint32)0x007FFFFF))

//Type-3
//Transmit N2,IMM24    //E011 00NN DDDD DDDD DDDD DDDD DDDD DDDD
//Transmit 0-3 (n) bytes of data. The data is present in the DDDD part
#define PAC_DELIA__TX_IMM(_n,_i)          (0x30000000 | (((_n) & 0x3)<<24) | ((_i) & (uint32)0xFFFFFF))

//Type-4
//Load Base    R, ABS23        //E100 0RRR 0AAA AAAA AAAA AAAA AAAA AAAA
#define PAC_DELIA__LOAD_REG(_r,_a)        (0x40000000 | (((_r) & 0x7)<<24) | ((_a) & (uint32)0x7FFFFF))

//XOR    IMM8    //E100 0111 0000 0000 0000 0000 DDDD DDDD
#define PAC_DELIA__XOR(_i)                (0x47000000 | ((_i) & 0xFF))

#define PAC_DELIA__TX_CONTROL_RATE(_t,_i) (0x58000000 | (((_t) & 0x3)<<24) | ((_i) & (uint32)0xFFFF))
#define PAC_DELIA__TX_RATE_LENGTH(_r,_l)  (0x50000000 | (2<<24) | ((uint32)(_l) & 0xFFFF) | (((uint32)(_r) & 0xFF) << 16))
#define PAC_DELIA__TX_PREAMBLE(_p)        (0x50000000 | (1<<24) | ((uint32)(_p) & 0xF))
#define PAC_DELIA__SOD                    (0x53000000)

//Type-5
//TxControl    T, IMM24        //E101 00TT DDDD DDDD DDDD DDDD DDDD DDDD
//Load TxControl[0..2] register (TT) with 3 bytes of immediate data (DDDD..)
#define PAC_DELIA__TX_CONTROL(_tx_reg_num, _imm_data)      (0x50000000 | (((_tx_reg_num) & 0x3)<<24) | ((_imm_data) & (uint32)0xFFFFFF))
#define PAC_DELIA__TX_CONTROL2__RATE_LENGTH(_r,_l)  PAC_DELIA__TX_CONTROL(2, ((_r)<<16) | (_l))

//TxControl   0, L-SIG + IMM12    //E101 1000 0000 0000 0000 DDDD DDDD DDDD
//Load TxControl[0] register with 12 bits of L-SIG automatically, and 12 bits of immediate data.
//TxControl[0] == pac_phy_tx_phy_control0
#define PAC_DELIA__TX_CONTROL0__AUTOLSIG(_imm_data)      (0x58000000 |((_imm_data) & (uint32)0xFFF))

//TxControl    1, IMM3 + txpwr + IMM6 + txmode    //E101 1001 0000 0000 DDD0 00DD DDDD 0000
//TxControl[1] == pac_phy_tx_phy_control0  immmed 11 starts from bit 13

//CM00010862
//#define ACK_PTA_PRIORITY 0x30  // =6 PTA_PRIORITY bits 18:16
//#define ACK_PTA_PRIORITY 0x10  // =2 PTA_PRIORITY bits 18:16
#if 0 // USE_EPTA && USE_EPTA_HIGH_PRIO_ACK
#define ACK_PTA_PRIORITY 0x38  // =7 PTA_PRIORITY bits 18:16
#else
//#define ACK_PTA_PRIORITY 0x28  // =5 PTA_PRIORITY bits 18:16 // was killing bt pkt
#define ACK_PTA_PRIORITY 0x00  // =0 PTA_PRIORITY bits 18:16 - low prio ack
#endif

//Load TxControl[1] register with 4 bits of TxPower automatically, 6 bits of immediate data-1, 3 bits of RxMode automatically, 11 bits of immediate data-2
#define PAC_DELIA__TX_CONTROL1__AUTOTXPOWER_AUTOMODE(_imm_data11, _imm_data6 )    (0x59000000 |(((_imm_data11) & (uint32)0x7FF)<<13) |(((_imm_data6) & (uint32)0x3F)<<4) )
//TxControl    2, rate8 + IMM16    //E101 1010 0000 0000 DDDD DDDD DDDD DDDD
//Load TxControl[2] register with 8 bits of Rate automatically, and 16 bits of immediate data (Tx Packet Length).
//TxControl[2][23:16] == pac_phy_tx_phy_rate, [15:00] == pac_phy_tx_phy_length
#define PAC_DELIA__TX_CONTROL2__AUTORATE_LENGTH(_l)      (0x5A000000 | ((_l) & (uint32)0xFFFF))

//TxControl    SOD        //E101 0011 0000 0000 0000 0000 0000 0000
#define PAC_DELIA__SOD                    (0x53000000)



#define PAC_DELIA__BA_INFO(_n)            (0x60000000 | (uint32)(_n))
#define PAC_DELIA__RIFS_TIMING            (0x62000000)
#define PAC_DELIA__AJUMP(_a)              (0x65000000 | ((_a) & (uint32)0x001FFFFC))
#define PAC_DELIA__APAD                   (0x66000000)
#define PAC_DELIA__NOP                    (0x70000000)
#define PAC_DELIA__JUMP(_a)               (0x71000000 | (((_a) & (uint32)0x001FFFFF)<<2))
#define PAC_DELIA__CALL(_a)               (0x75000000 | (((_a) & (uint32)0x001FFFFF)<<2))
#define PAC_DELIA__RETURN                 (0x72000000)
#define PAC_DELIA__AEND                   (0xE4000000)
#define PAC_DELIA__END                    (0xF0000000) //Was 0x8... but this sends 4K of data before the end of recipe!
#define PAC_DELIA__TX_TID_BA_CONTROL(_channel)  (0x68000000 | ((_channel) & (uint32)0x00000007))
#define PAC_DELIA__TX_TA_ADDR(_channel)         (0x69000000 | ((_channel) & (uint32)0x00000007))

#define PAC_SET_AGGR_JUMP        (0)
#define PAC_SET_AGGR_PAD         (1)
#define PAC_SET_AGGR_END         (2)
#define PAC_SET_AGGR_CONTROL     (3)
#define PAC_SET_AGGR_ZERODEL     (4)
#define PAC_SET_AGGR_RATELENGTH  (5) /*AMPDU Ê¹ÓÃµÄRATE ÓëLEN*/

// bits definition for PAC_DEL_DELIA_STATUS
#define PAC_DEL_DELIA_STATUS__BUSY      0x01

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* Bits in the PAC_WEP_CONTROL register */
#define PAC_WEP_CONTROL__START             0x01
#define PAC_WEP_CONTROL__RC4_ENABLE        0x02
#define PAC_WEP_CONTROL__ENCRYPT           0x04
#define PAC_WEP_CONTROL__READ_IV           0x08
#define PAC_WEP_CONTROL__WRITE_IV          0x10
#define PAC_WEP_CONTROL__WRITE_ICV         0x20
#define PAC_WEP_CONTROL__WEP2              0x40
#define PAC_WEP_CONTROL__MASK              0x7F
/* The below are in the same bitfield as the ones above but relate to the AES
engine NOT the WEP */
#define PAC_WEP_CONTROL__AES               0x80
#define PAC_WEP_CONTROL__CRYPT_FAIL        0x100

/* Wired equivalent privacy registers */
/* Bits in the WEP status register */
#define PAC_WEP_STATUS__CRC_FAIL           0x02
#define PAC_WEP_STATUS__ACTIVE             0x01

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* PAC_CLC_STOP bits - not the parity of some bits is reversed */
#define PAC_CLC_STOP__REG            0x00000001    /* No purpose */
#define PAC_CLC_STOP__SMC            0x00000002    /* (STOP) Shared Memory Controller clock */
#define PAC_CLC_STOP__RAB            0x00000004    /* (STOP) Register Access Block clock */
#define PAC_CLC_STOP__UPI            0x00000008    /* (STOP) Upper Processor Interface clock */
#define PAC_CLC_STOP__API            0x00000010    /* (GO) AES Processor Interface clock */
#define PAC_CLC_STOP__PHI            0x00000020    /* (STOP) Host Interface clock */
#define PAC_CLC_STOP__WEP            0x00000040    /* (GO) Wired Equivalent Privacy block clock */
#define PAC_CLC_STOP__NTD            0x00000080    /* (GO) next transmit Decision block clock */
#define PAC_CLC_STOP__TXC            0x00000100    /* (GO) Transmit Control block clock */
#define PAC_CLC_STOP__TIM            0x00000200    /* (GO) Timer block clock */
#define PAC_CLC_STOP__DELIA          0x00000400    /* (GO) DELIA clock */
#define PAC_CLC_STOP__RXC            0x00000800    /* (GO) receive control block clock */
#define PAC_CLC_STOP__RTA            0x00001000    /* (GO) Receive-transmit Arbiter clock */
#define PAC_CLC_STOP__DUR            0x00002000    /* (GO) Duration calculation clock */
#define PAC_CLC_STOP__AES            0x00004000    /* (GO) AES calculation clock - BBIC2c onwards */
#define PAC_CLC_STOP__SPARE          0x00008000    /* This bit has no hardware function */

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* Michael code registers */
#define PAC_MIC_CONTROL_USE_PREVIOUS_KEY  0x1

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* AES Registers */
#define PAC_AES_C_DMA_WAIT_STORE    0x20000000
#define PAC_AES_C_DMA_START_STOP    0x10000000
#define PAC_AES_C_FIFO_RESET        0x00008000
#define PAC_AES_C_UC_ENABLE         0x00001000
#define PAC_AES_C_UC_BUSY           0x00001000
#define PAC_AES_C_UC_TRIGGER_3      0x00000800
#define PAC_AES_C_UC_TRIGGER_2      0x00000400
#define PAC_AES_C_UC_TRIGGER_1      0x00000200
#define PAC_AES_C_UC_TRIGGER_0      0x00000100
#define PAC_AES_C_UC_LENGTH_3       0x00000080
#define PAC_AES_C_UC_LENGTH_2       0x00000040
#define PAC_AES_C_UC_LENGTH_1       0x00000020
#define PAC_AES_C_UC_LENGTH_0       0x00000010
#define PAC_AES_C_UC_CONTROL_3      0x00000008
#define PAC_AES_C_UC_CONTROL_2      0x00000004
#define PAC_AES_C_UC_CONTROL_1      0x00000002
#define PAC_AES_C_UC_CONTROL_0      0x00000001

#define PAC_AES_C_UC_STATUS_3       0x00000008
#define PAC_AES_C_UC_STATUS_2       0x00000004
#define PAC_AES_C_UC_STATUS_1       0x00000002
#define PAC_AES_C_UC_STATUS_0       0x00000001

#define PAC_AES_C_UC_LENGTH_SHIFT(_l)   ((_l&PAC_AES_C_UC_LENGTH_MASK)<<0x4)
#define PAC_AES_C_UC_LENGTH_MASK    0xF
#define PAC_AES_C_DMA_TAG_LENGTH_SHIFT(_l)  ((_l&PAC_AES_C_DMA_TAG_LENGTH_MASK)<<0x10)
#define PAC_AES_C_DMA_TAG_LENGTH_MASK   0x1F

#define PAC_AES_MICROCODE_START_BIT 0x80000000

/* ----------------------------------------------------------------*/
/* ----------------------------------------------------------------*/

/* Set a PTCS table entry */
#define HW_WRITE_REG__PAC_PTCS(ENTRY,PTR_PTCS) \
      SM_Region.Var.PTCS_Table[(ENTRY)] = SM_PTR_TO_PAS_ADDR(&(PTR_PTCS)->Ttcs);
//    HW_WRITE_REG((uint32) (&SM_Region.Var.PTCS_Table[(ENTRY)]), SM_PTR_TO_PAS_ADDR(&(PTR_PTCS)->Ttcs) );

#define PAC_PTCS__DISABLE   0x0
#define PAC_PTCS__ENABLE    0x1
#define PAC_NTD_RESPONSE    0x2

#endif    // #ifndef PAC_DEFS_H


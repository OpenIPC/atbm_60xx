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
* \brief PAC register definitions
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
****************************************************************************/

#ifndef PAC_REGS_H
#define PAC_REGS_H

/***********************************************************************/
/***                        Include Files                            ***/
/***********************************************************************/
// #include "hw_defs.h"

/***********************************************************************/
/***                        Data Types and Constants                 ***/
/***********************************************************************/


#define PAC_SHARED_MEMORY           PAC_BASE_ADDRESS
#define PAC_ALIASED_ADDRESS_OFFSET  0x400000
#define PAC_SHARED_MEMORY_ALIASED   (PAC_SHARED_MEMORY + PAC_ALIASED_ADDRESS_OFFSET)

/*****************************
* PAS ( Protocol Accelerator Subsystem ) Base Address
******************************
* All PAS Register Documentation can be found in
*
* http://web.bri.st.com/cgi-bin/viewcvs.cgi/WBF/Documents/Digital/PAS/PAS_Hardware_Specification.doc?cvsroot=CODCVS

* 27/04/2009 - WBF00003694 - http://cqweb.zav.st.com/cqweb/url/default.asp?db=WBF&id=WBF00003694
*/


/*****************************
* PAS Sub-system Base Addresses
*****************************/

//#define PAS_SHARED_MEMORY                       (PAC_BASE_ADDRESS + 0x000000)


#define PAC_RXF_REGS                            (PAC_REGS + 0x000) //0x9c00000
#define PAC_RXF_REGS_SIZE                       0x200
#define PAC_RXD_REGS                            (PAC_REGS + 0x200)
#define PAC_RXD_REGS_SIZE                       0x200
#define PAC_DEL_REGS                            (PAC_REGS + 0x400)
#define PAC_DEL_REGS_SIZE                       0x200
#define PAC_RXC_REGS                            (PAC_REGS + 0x600)
#define PAC_RXC_REGS_SIZE                       0x200
#define PAC_DUR_REGS                            (PAC_REGS + 0x800)
#define PAC_DUR_REGS_SIZE                       0x200
#define PAC_NTD_REGS                            (PAC_REGS + 0xA00)//0x9c00a00
#define PAC_NTD_REGS_SIZE                       0x200
#define PAC_TXC_REGS                            (PAC_REGS + 0xC00)
#define PAC_TXC_REGS_SIZE                       0x200
#define PAC_TIM_REGS                            (PAC_REGS + 0xE00)
#define PAC_TIM_REGS_SIZE                       0x200
#define PAC_PTA_REGS                            (PAC_REGS + 0x1000)
#define PAC_PTA_REGS_SIZE                       0x200
#define PAC_BAB_REGS                            (PAC_REGS + 0x1200)
#define PAC_BAB_REGS_SIZE                       0x200
#define PAC_BAP_REGS                            (PAC_REGS + 0x1400)
#define PAC_BAP_REGS_SIZE                       0x200

#define PAS_SHARED_MEMORY_0                     (PAS_SHARED_MEMORY + 0x0)
#define PAS_SHARED_MEMORY_0_SIZE                0x32768 //TODO change for 8601

/*****************************
* PAC_RXF_ (Receive Filters Block) Registers
*****************************/

#define PAC_RXF_BYTE_FILTER_CONTROL_0           (PAC_RXF_REGS + 0x00)
#define PAC_RXF_BYTE_FILTER_CONTROL_1           (PAC_RXF_REGS + 0x04)
#define PAC_RXF_BYTE_FILTER_CONTROL_2           (PAC_RXF_REGS + 0x08)
#define PAC_RXF_BYTE_FILTER_CONTROL_3           (PAC_RXF_REGS + 0x0C)
#define PAC_RXF_BYTE_FILTER_CONTROL_4           (PAC_RXF_REGS + 0x10)
#define PAC_RXF_BYTE_FILTER_CONTROL_5           (PAC_RXF_REGS + 0x14)
#define PAC_RXF_BYTE_FILTER_CONTROL_6           (PAC_RXF_REGS + 0x18)
#define PAC_RXF_BYTE_FILTER_CONTROL_7           (PAC_RXF_REGS + 0x1C)
#define PAC_RXF_BYTE_FILTER_CONTROL_8           (PAC_RXF_REGS + 0x20)
#define PAC_RXF_BYTE_FILTER_CONTROL_9           (PAC_RXF_REGS + 0x24)
#define PAC_RXF_BYTE_FILTER_CONTROL_10          (PAC_RXF_REGS + 0x28)
#define PAC_RXF_BYTE_FILTER_CONTROL_11          (PAC_RXF_REGS + 0x2C)
#define PAC_RXF_LONG_FILTER_0_REF1              (PAC_RXF_REGS + 0x30)
#define PAC_RXF_LONG_FILTER_0_REF2              (PAC_RXF_REGS + 0x34)
#define PAC_RXF_LONG_FILTER_0_CONTROL           (PAC_RXF_REGS + 0x38)
#define PAC_RXF_LONG_FILTER_1_REF1              (PAC_RXF_REGS + 0x3C)
#define PAC_RXF_LONG_FILTER_1_REF2              (PAC_RXF_REGS + 0x40)
#define PAC_RXF_LONG_FILTER_1_CONTROL           (PAC_RXF_REGS + 0x44)
#define PAC_RXF_LONG_FILTER_2_REF1              (PAC_RXF_REGS + 0x48)//0x9c00000 + 0x48
#define PAC_RXF_LONG_FILTER_2_REF2              (PAC_RXF_REGS + 0x4C)
#define PAC_RXF_LONG_FILTER_2_CONTROL           (PAC_RXF_REGS + 0x50)
#define PAC_RXF_LONG_FILTER_3_REF1              (PAC_RXF_REGS + 0x1B8)
#define PAC_RXF_LONG_FILTER_3_REF2              (PAC_RXF_REGS + 0x1BC)
#define PAC_RXF_LONG_FILTER_3_CONTROL           (PAC_RXF_REGS + 0x1C0)
#define PAC_RXF_UNMATCH_ADDRESS_REF1            (PAC_RXF_REGS + 0x54)
#define PAC_RXF_UNMATCH_ADDRESS_ADDREN          (PAC_RXF_REGS + 0x58)
#define PAC_RXF_UNMATCH_ADDRESS_OFFSET          (PAC_RXF_REGS + 0x5C)
#define PAC_RXF_TA_0_REF1                       (PAC_RXF_REGS + 0x60)
#define PAC_RXF_TA_0_REF2                       (PAC_RXF_REGS + 0x64)
#define PAC_RXF_TID_0_REF                       (PAC_RXF_REGS + 0x68)
#define PAC_RXF_TA_1_REF1                       (PAC_RXF_REGS + 0x6c)
#define PAC_RXF_TA_1_REF2                       (PAC_RXF_REGS + 0x70)
#define PAC_RXF_TID_1_REF                       (PAC_RXF_REGS + 0x74)
#define PAC_RXF_TA_2_REF1                       (PAC_RXF_REGS + 0x78)
#define PAC_RXF_TA_2_REF2                       (PAC_RXF_REGS + 0x7c)
#define PAC_RXF_TID_2_REF                       (PAC_RXF_REGS + 0x80)
#define PAC_RXF_TA_3_REF1                       (PAC_RXF_REGS + 0x84)
#define PAC_RXF_TA_3_REF2                       (PAC_RXF_REGS + 0x88)
#define PAC_RXF_TID_3_REF                       (PAC_RXF_REGS + 0x8c)
#define PAC_RXF_TA_OFFSET                       (PAC_RXF_REGS + 0x90)
#define PAC_RXF_TID_C_OFFSET                    (PAC_RXF_REGS + 0x94)
#define PAC_RXF_TID_D_OFFSET                    (PAC_RXF_REGS + 0x98)
#define PAC_RXF_BYTE_FILTER_CONTROL_12          (PAC_RXF_REGS + 0xA0)
#define PAC_RXF_BYTE_FILTER_CONTROL_13          (PAC_RXF_REGS + 0xA4)
#define PAC_RXF_BYTE_FILTER_CONTROL_14          (PAC_RXF_REGS + 0xA8)
#define PAC_RXF_BYTE_FILTER_CONTROL_15          (PAC_RXF_REGS + 0xAC)
#define PAC_RXF_BYTE_FILTER_CONTROL_16          (PAC_RXF_REGS + 0xB0)
#define PAC_RXF_BYTE_FILTER_CONTROL_17          (PAC_RXF_REGS + 0xB4)
#define PAC_RXF_BYTE_FILTER_CONTROL_18          (PAC_RXF_REGS + 0xB8)
#define PAC_RXF_BYTE_FILTER_CONTROL_19          (PAC_RXF_REGS + 0xBC)
#define PAC_RXF_BYTE_FILTER_CONTROL_20          (PAC_RXF_REGS + 0xC0)
#define PAC_RXF_BYTE_FILTER_CONTROL_21          (PAC_RXF_REGS + 0xC4)
#define PAC_RXF_BYTE_FILTER_CONTROL_22          (PAC_RXF_REGS + 0xC8)
#define PAC_RXF_BYTE_FILTER_CONTROL_23          (PAC_RXF_REGS + 0xCC)
#define PAC_RXF_DYNAMIC_OFFSET_0                (PAC_RXF_REGS + 0xD0)
#define PAC_RXF_DYNAMIC_OFFSET_1                (PAC_RXF_REGS + 0xD4)
#define PAC_RXF_DYNAMIC_OFFSET_2                (PAC_RXF_REGS + 0xD8)
#define PAC_RXF_DYNAMIC_OFFSET_3                (PAC_RXF_REGS + 0xDC)
#define PAC_RXF_TPCTA_0_REF1                    (PAC_RXF_REGS + 0xE0)
#define PAC_RXF_TPCTA_0_REF2                    (PAC_RXF_REGS + 0xE4)
#define PAC_RXF_TPCTA_1_REF1                    (PAC_RXF_REGS + 0xE8)
#define PAC_RXF_TPCTA_1_REF2                    (PAC_RXF_REGS + 0xEC)
#define PAC_RXF_TPCTA_OFFSET                    (PAC_RXF_REGS + 0xF0)
#define PAC_RXF_BYTE_FILTER_CONTROL_24          (PAC_RXF_REGS + 0xF4)
#define PAC_RXF_BYTE_FILTER_CONTROL_25          (PAC_RXF_REGS + 0xF8)
#define PAC_RXF_BYTE_FILTER_CONTROL_26          (PAC_RXF_REGS + 0xFC)

// new extractors for 8601
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_0      (PAC_RXF_REGS+0x100)
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_M_OFFSET 0x00FF0000
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_V_OFFSET 16
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_1      (PAC_RXF_REGS+0x104)
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_2      (PAC_RXF_REGS+0x108)
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_3      (PAC_RXF_REGS+0x10C)
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_4      (PAC_RXF_REGS+0x110)
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_5      (PAC_RXF_REGS+0x114)
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_6      (PAC_RXF_REGS+0x118)
#define PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_7      (PAC_RXF_REGS+0x11C)

#define PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_0      (PAC_RXF_REGS+0x120)
#define PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_M_OFFSET 0x00FE0000
#define PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_V_OFFSET 16
#define PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_1      (PAC_RXF_REGS+0x128)
#define PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_2      (PAC_RXF_REGS+0x130)

#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_0       (PAC_RXF_REGS+0x180)
#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_1       (PAC_RXF_REGS+0x184)
#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_2       (PAC_RXF_REGS+0x188)
#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_3       (PAC_RXF_REGS+0x18C)
#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_4       (PAC_RXF_REGS+0x190)
#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_5       (PAC_RXF_REGS+0x194)
#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_6       (PAC_RXF_REGS+0x198)
#define PAC_RXF_TWO_BYTE_EXTRACTED_DATA_7       (PAC_RXF_REGS+0x19C)

#define PAC_RXF_SIX_BYTE_EXTRACTED_DATA_L_0     (PAC_RXF_REGS+0x1A0)
#define PAC_RXF_SIX_BYTE_EXTRACTED_DATA_H_0     (PAC_RXF_REGS+0x1A4)
#define PAC_RXF_SIX_BYTE_EXTRACTED_DATA_L_1     (PAC_RXF_REGS+0x1A8)
#define PAC_RXF_SIX_BYTE_EXTRACTED_DATA_H_1     (PAC_RXF_REGS+0x1AC)
#define PAC_RXF_SIX_BYTE_EXTRACTED_DATA_L_2     (PAC_RXF_REGS+0x1B0)
#define PAC_RXF_SIX_BYTE_EXTRACTED_DATA_H_2     (PAC_RXF_REGS+0x1B4)

// extractors used by hardware
#define PAC_RXF_EXTRACTED_FRMCTL    0
#define PAC_RXF_EXTRACTED_DURID     1
#define PAC_RXF_EXTRACTED_SEQCTL    2
#define PAC_RXF_EXTRACTED_QOSCTL    3
#define PAC_RXF_EXTRACTED_BARCTL    4
#define PAC_RXF_EXTRACTED_BABINFO   5

/*****************************
* PAC_RXD_ (Rx Frame Event Decoder Block) Registers
*****************************/

#define PAC_RXD_EVENT_ENABLE                    (PAC_RXD_REGS + 0x00)//200
#define PAC_RXD_RX_ENABLE                       (PAC_RXD_REGS + 0x04)
#define PAC_RXD_ALL_EVENTS                      (PAC_RXD_REGS + 0x08)
#define PAC_RXD_FRAME_ABORT                     (PAC_RXD_REGS + 0x0C)
#define PAC_RXD_CONDITION_0                     (PAC_RXD_REGS + 0x10)
#define PAC_RXD_CONDITION_1                     (PAC_RXD_REGS + 0x14)
#define PAC_RXD_CONDITION_2                     (PAC_RXD_REGS + 0x18)
#define PAC_RXD_CONDITION_3                     (PAC_RXD_REGS + 0x1C)
#define PAC_RXD_CONDITION_4                     (PAC_RXD_REGS + 0x20)
#define PAC_RXD_CONDITION_5                     (PAC_RXD_REGS + 0x24)
#define PAC_RXD_CONDITION_6                     (PAC_RXD_REGS + 0x28)
#define PAC_RXD_CONDITION_7                     (PAC_RXD_REGS + 0x2C)
#define PAC_RXD_CONDITION_8                     (PAC_RXD_REGS + 0x30)
#define PAC_RXD_CONDITION_9                     (PAC_RXD_REGS + 0x34)
#define PAC_RXD_CONDITION_10                    (PAC_RXD_REGS + 0x38)
#define PAC_RXD_CONDITION_11                    (PAC_RXD_REGS + 0x3C)
#define PAC_RXD_CONDITION_12                    (PAC_RXD_REGS + 0x40)
#define PAC_RXD_CONDITION_13                    (PAC_RXD_REGS + 0x44)
#define PAC_RXD_CONDITION_14                    (PAC_RXD_REGS + 0x48)
#define PAC_RXD_CONDITION_15                    (PAC_RXD_REGS + 0x4C)
#define PAC_RXD_CONDITION_16                    (PAC_RXD_REGS + 0x50)
#define PAC_RXD_CONDITION_17                    (PAC_RXD_REGS + 0x54)
#define PAC_RXD_CONDITION_18                    (PAC_RXD_REGS + 0x58)
#define PAC_RXD_CONDITION_19                    (PAC_RXD_REGS + 0x5C)
#define PAC_RXD_CONDITION_20                    (PAC_RXD_REGS + 0x60)
#define PAC_RXD_CONDITION_21                    (PAC_RXD_REGS + 0x64)
#define PAC_RXD_CONDITION_22                    (PAC_RXD_REGS + 0x68)
#define PAC_RXD_CONDITION_23                    (PAC_RXD_REGS + 0x6C)
#define PAC_RXD_CONDITION_24                    (PAC_RXD_REGS + 0x70)
#define PAC_RXD_CONDITION_25                    (PAC_RXD_REGS + 0x74)
#define PAC_RXD_CONDITION_26                    (PAC_RXD_REGS + 0x78)
#define PAC_RXD_CONDITION_INV                   (PAC_RXD_REGS + 0x7C)
#define PAC_RXD_CONDITIONB_0                    (PAC_RXD_REGS + 0x80)
#define PAC_RXD_CONDITIONB_1                    (PAC_RXD_REGS + 0x84)
#define PAC_RXD_CONDITIONB_2                    (PAC_RXD_REGS + 0x88)
#define PAC_RXD_CONDITIONB_3                    (PAC_RXD_REGS + 0x8C)
#define PAC_RXD_CONDITIONB_4                    (PAC_RXD_REGS + 0x90)
#define PAC_RXD_CONDITIONB_5                    (PAC_RXD_REGS + 0x94)
#define PAC_RXD_CONDITIONB_6                    (PAC_RXD_REGS + 0x98)
#define PAC_RXD_CONDITIONB_7                    (PAC_RXD_REGS + 0x9C)
#define PAC_RXD_CONDITIONB_8                    (PAC_RXD_REGS + 0xA0)
#define PAC_RXD_CONDITIONB_9                    (PAC_RXD_REGS + 0xA4)
#define PAC_RXD_CONDITIONB_10                   (PAC_RXD_REGS + 0xA8)
#define PAC_RXD_CONDITIONB_11                   (PAC_RXD_REGS + 0xAC)
#define PAC_RXD_CONDITIONB_12                   (PAC_RXD_REGS + 0xB0)
#define PAC_RXD_CONDITIONB_13                   (PAC_RXD_REGS + 0xB4)
#define PAC_RXD_CONDITIONB_14                   (PAC_RXD_REGS + 0xB8)
#define PAC_RXD_CONDITIONB_15                   (PAC_RXD_REGS + 0xBC)
#define PAC_RXD_CONDITIONB_16                   (PAC_RXD_REGS + 0xC0)
#define PAC_RXD_CONDITIONB_17                   (PAC_RXD_REGS + 0xC4)
#define PAC_RXD_CONDITIONB_18                   (PAC_RXD_REGS + 0xC8)
#define PAC_RXD_CONDITIONB_19                   (PAC_RXD_REGS + 0xCC)
#define PAC_RXD_CONDITIONB_20                   (PAC_RXD_REGS + 0xD0)
#define PAC_RXD_CONDITIONB_21                   (PAC_RXD_REGS + 0xD4)
#define PAC_RXD_CONDITIONB_22                   (PAC_RXD_REGS + 0xD8)
#define PAC_RXD_CONDITIONB_23                   (PAC_RXD_REGS + 0xDC)
#define PAC_RXD_CONDITIONB_24                   (PAC_RXD_REGS + 0xE0)
#define PAC_RXD_CONDITIONB_25                   (PAC_RXD_REGS + 0xE4)
#define PAC_RXD_CONDITIONB_26                   (PAC_RXD_REGS + 0xE8)
#define PAC_RXD_CONDITIONB_INV                  (PAC_RXD_REGS + 0xEC)
#define PAC_RXD_CONDITION_27                    (PAC_RXD_REGS + 0xF0)
#define PAC_RXD_CONDITION_28                    (PAC_RXD_REGS + 0xF4)
#define PAC_RXD_CONDITION_29                    (PAC_RXD_REGS + 0xF8)
#define PAC_RXD_CONDITION_30                    (PAC_RXD_REGS + 0xFC)
#define PAC_RXD_CONDITIONB_27                   (PAC_RXD_REGS + 0x100)
#define PAC_RXD_CONDITIONB_28                   (PAC_RXD_REGS + 0x104)
#define PAC_RXD_CONDITIONB_29                   (PAC_RXD_REGS + 0x108)
#define PAC_RXD_CONDITIONB_30                   (PAC_RXD_REGS + 0x10C)
#define PAC_RXD_HT_ROW_ENABLE                   (PAC_RXD_REGS + 0x110)
#define PAC_RXD_AMP_ROW_ENABLE                  (PAC_RXD_REGS + 0x114)

/*****************************
* PAC_DEL_ (DELIA) Registers
*****************************/

#define PAC_DEL_BASE_0                          (PAC_DEL_REGS + 0x00)
#define PAC_DEL_BASE_1                          (PAC_DEL_REGS + 0x04)
#define PAC_DEL_BASE_2                          (PAC_DEL_REGS + 0x08)
#define PAC_DEL_BASE_3                          (PAC_DEL_REGS + 0x0C)
#define PAC_DEL_BASE_4                          (PAC_DEL_REGS + 0x10)
#define PAC_DEL_BASE_5                          (PAC_DEL_REGS + 0x14)
#define PAC_DEL_BASE_6                          (PAC_DEL_REGS + 0x18)
#define PAC_DEL_RETURN_ADDRESS                  (PAC_DEL_REGS + 0x1C)
#define PAC_DEL_RECIPE_POINTER                  (PAC_DEL_REGS + 0x20)
#define PAC_DEL_DELIA_STATUS                    (PAC_DEL_REGS + 0x24)
#define PAC_DEL_IFS_COMPENSATION                (PAC_DEL_REGS + 0x28)
#define PAC_DEL_CCA_COMPENSATION                (PAC_DEL_REGS + 0x2C)
#define PAC_DEL_ALL_QUEUES_BASE                 (PAC_DEL_REGS + 0x30)
#define PAC_DEL_RIFS_IFS_COMPENSATION           (PAC_DEL_REGS + 0x34)
#define PAC_DEL_RIFS_CCA_COMPENSATION           (PAC_DEL_REGS + 0x38)
#define PAC_DEL_DSSS_IFS_COMPENSATION           (PAC_DEL_REGS + 0x3C)
#define PAC_DEL_DSSS_CCA_COMPENSATION           (PAC_DEL_REGS + 0x40)
#define PAC_DEL_ENABLE_COMPENSATION             (PAC_DEL_REGS + 0x44)
#define PAC_DEL_RIFS_ENABLE_COMPENSATION        (PAC_DEL_REGS + 0x48)
#define PAC_DEL_DSSS_ENABLE_COMPENSATION        (PAC_DEL_REGS + 0x4C)

/*****************************
* PAC_RXC_ (Receive Control Block) Registers
*****************************/
#define PAC_RXC_BUF_OVERFLOW_BIT			BIT(21) 
#define PAC_RXC_FIFO_OVERFLOW_BIT			BIT(22)
#define PAC_RXC_OVERFLOW_MASK			(PAC_RXC_BUF_OVERFLOW_BIT|PAC_RXC_FIFO_OVERFLOW_BIT)

#define PAC_RXC_RX_CONTROL                      (PAC_RXC_REGS + 0x00)
#define PAC_RXC_RX_BUFFER_IN_POINTER            (PAC_RXC_REGS + 0x04)
#define PAC_RXC_RX_BUFFER_OUT_POINTER           (PAC_RXC_REGS + 0x08)
#define PAC_RXC_NEAR_FULL_THRESHOLD             (PAC_RXC_REGS + 0x0C)
#define PAC_RXC_CCA_VALID_DELAY                 (PAC_RXC_REGS + 0x10)
#define PAC_RXC_CCA_IFS_COMPENSATION            (PAC_RXC_REGS + 0x14)
#define PAC_RXC_RXRDY_IFS_COMPENSATION          (PAC_RXC_REGS + 0x18)
#define PAC_RXC_PHY_EXTEND_COUNT                (PAC_RXC_REGS + 0x1C)
#define PAC_RXC_ERR_CLEAR_BIT					BIT(30) 
#define PAC_RXC_ERR_ENIRQ_BIT					BIT(29) 
#define PAC_RXC_ERR_ENIRQ_THRESHOLD_BIT			BIT(28) 
#define PAC_RXC_ERROR_CODE                      (PAC_RXC_REGS + 0x20)
#undef PAC_RXC_PHY_EXTEND_COUNT
#define PAC_RXC_BUFFER_SIZE                     (PAC_RXC_REGS+0x1C)
#define PAC_RXC_RX_ERROR_COUNTERS_0             (PAC_RXC_REGS+0x24)
#define PAC_RXC_RX_ERROR_COUNTERS_1             (PAC_RXC_REGS+0x28)
#define PAC_RXC_RX_ERROR_COUNTERS_2             (PAC_RXC_REGS+0x2C)
#define PAC_RXC_RX_DELIMITER_ERROR_COUNT        (PAC_RXC_REGS+0x30)
#define PAC_RXC_M_DELIMITER_ERROR_COUNT         0xFF;
#define PAC_RXC_PHY_ERROR_ABORT_CONFIG          (PAC_RXC_REGS+0x34)
#define PAC_RXC_RX_STATE                        (PAC_RXC_REGS+0x38)

/*****************************
* PAC_DUR_ (802.11 Specific Tx Field Formatters) Registers
*****************************/

#define PAC_DUR_RAM_BASE_ADDRESS                (PAC_DUR_REGS + 0x00)
#define PAC_DUR_CFP_END_TIME                    (PAC_DUR_REGS + 0x04)
#define PAC_DUR_TSF_START_TIME_ADJUST           (PAC_DUR_REGS + 0x08)
#define PAC_DUR_MISC                            (PAC_DUR_REGS + 0x0C)
#define PAC_DUR_NAV_ADJUST                      (PAC_DUR_REGS + 0x10)
#define PAC_DUR_POWER_CONTROL                   (PAC_DUR_REGS + 0x18)
#define PAC_DUR_MODE_RATE_MAPPING               (PAC_DUR_REGS + 0x1C) // new for 8601
#define PAC_DUR_RATE_MASK                       (PAC_DUR_REGS + 0x80)

/*****************************
* PAC_NTD_ (Next Transmit Decision Block) Registers
*****************************/

#define PAC_NTD_EVENT_RESP_ENABLE               (PAC_NTD_REGS + 0x00)
#define PAC_NTD_RX_EVENT_RESP_ENABLE            (PAC_NTD_REGS + 0x04)
#define PAC_NTD_RX_EVENT_RESP_MULTI_SHOT        (PAC_NTD_REGS + 0x08)
#define PAC_NTD_RX_EVENT_RESP_PRIORITY          (PAC_NTD_REGS + 0x0C)
#define PAC_NTD_RX_EVENT_RESP_REQUEUE_OVERRIDE  (PAC_NTD_REGS + 0x10)
#define PAC_NTD_RX_EVENT_NAV_LOADA              (PAC_NTD_REGS + 0x14)
#define PAC_NTD_RX_EVENT_NAV_LOADB              (PAC_NTD_REGS + 0x18)
#define PAC_NTD_RX_SW_EVENT_ENABLE              (PAC_NTD_REGS + 0x1C)
#define PAC_NTD_STATUS                          (PAC_NTD_REGS + 0x20)
#define PAC_NTD_STATUS_PEEK                     (PAC_NTD_REGS + 0x24)
#define PAC_NTD_CONTROL                         (PAC_NTD_REGS + 0x28)
#define PAC_NTD_TSF_HW_0_PTCS_PTR_OFFSET        (0 + 0x00)
#define PAC_NTD_TSF_HW_1_PTCS_PTR_OFFSET        (0 + 0x04)
#define PAC_NTD_CRC_ERR_0_PTCS_PTR_OFFSET       (0 + 0x08)
#define PAC_NTD_CRC_ERR_1_PTCS_PTR_OFFSET       (0 + 0x0C)
#define PAC_NTD_RX_EVENT_2_PTCS_PTR_OFFSET      (0 + 0x10)
#define PAC_NTD_RX_EVENT_3_PTCS_PTR_OFFSET      (0 + 0x14)
#define PAC_NTD_RX_EVENT_4_PTCS_PTR_OFFSET      (0 + 0x18)
#define PAC_NTD_RX_EVENT_5_PTCS_PTR_OFFSET      (0 + 0x1c)
#define PAC_NTD_RX_EVENT_6_PTCS_PTR_OFFSET      (0 + 0x20)
#define PAC_NTD_RX_EVENT_7_PTCS_PTR_OFFSET      (0 + 0x24)
#define PAC_NTD_RX_EVENT_8_PTCS_PTR_OFFSET      (0 + 0x28)
#define PAC_NTD_RX_EVENT_9_PTCS_PTR_OFFSET      (0 + 0x2c)
#define PAC_NTD_RX_EVENT_10_PTCS_PTR_OFFSET     (0 + 0x30)
#define PAC_NTD_RX_EVENT_11_PTCS_PTR_OFFSET     (0 + 0x34)
#define PAC_NTD_RX_EVENT_12_PTCS_PTR_OFFSET     (0 + 0x38)
#define PAC_NTD_RX_EVENT_13_PTCS_PTR_OFFSET     (0 + 0x3C)
#define PAC_NTD_RX_EVENT_14_PTCS_PTR_OFFSET     (0 + 0x40)
#define PAC_NTD_RX_EVENT_15_PTCS_PTR_OFFSET     (0 + 0x44)
#define PAC_NTD_RX_EVENT_16_PTCS_PTR_OFFSET     (0 + 0x48)
#define PAC_NTD_RX_EVENT_17_PTCS_PTR_OFFSET     (0 + 0x4c)
#define PAC_NTD_RX_EVENT_18_PTCS_PTR_OFFSET     (0 + 0x50)
#define PAC_NTD_RX_EVENT_19_PTCS_PTR_OFFSET     (0 + 0x54)
#define PAC_NTD_RX_EVENT_20_PTCS_PTR_OFFSET     (0 + 0x58)
#define PAC_NTD_RX_EVENT_21_PTCS_PTR_OFFSET     (0 + 0x5c)
#define PAC_NTD_RX_EVENT_22_PTCS_PTR_OFFSET     (0 + 0x60)
#define PAC_NTD_RX_EVENT_23_PTCS_PTR_OFFSET     (0 + 0x64)
#define PAC_NTD_RX_EVENT_24_PTCS_PTR_OFFSET     (0 + 0x68)
#define PAC_NTD_RESP_TIMEOUT_A_PTCS_PTR_OFFSET  (0 + 0x6c)
#define PAC_NTD_RESP_TIMEOUT_B_PTCS_PTR_OFFSET  (0 + 0x70)
#define PAC_NTD_SW_TX_REQ_0_PTCS_PTR_OFFSET     (0 + 0x74)
#define PAC_NTD_SW_TX_REQ_1_PTCS_PTR_OFFSET     (0 + 0x78)
#define PAC_NTD_EBM_SEQ_REQ_PTCS_PTR_OFFSET     (0 + 0x7c)

/*****************************
* PAC_TXC_ (Transmit Control Block) Registers
*****************************/

#define PAC_TXC_PTCS_POINTER_TABLE_BASE         (PAC_TXC_REGS + 0x00)
#define PAC_TXC_STATUS                          (PAC_TXC_REGS + 0x04)
#define PAC_TXC_ACKTIMEOUT_EN                   (PAC_TXC_REGS + 0x08)
/*PAC_TXC_STATUS TXC STATE*/
#define PAC_TXC_STATE_MASK							(0x1F)
#define PAC_TXC_STATE_IDLE							(0)
#define PAC_TXC_STATE_IDLE_MEDIUM_BUSY				(1)
#define PAC_TXC_STATE_IDLE_IFS_ACTIVE				(2)
#define PAC_TXC_STATE_IDLE_WAIT_FOR_AIR_CLEAR		(3)
#define PAC_TXC_STATUS_IDLE_DISABLE_FSM_BIT			BIT(31)

/*****************************
* PAC_PTA_ (WLAN-BLUETOOTH Co-existance Mechanism) Registers
*****************************/

#define PAC_PTA_PRB_AND_PSM_CTRL		 	    (PAC_PTA_REGS + 0   )
#define PAC_PTA_PRESCALAR  					    (PAC_PTA_REGS + 4   )
#define PAC_PTA_IF_MODE  					    (PAC_PTA_REGS + 8   )
#define PAC_PTA_PIN_DELAYS  				    (PAC_PTA_REGS + 12  )

#define PAC_PTA_PRIORITY_SAMPLING_TIME 		    (PAC_PTA_REGS + 16  )
#define PAC_PTA_PER_LINKID_SAMPLING_TIME  	    (PAC_PTA_REGS + 20  )
#define PAC_PTA_FREQ_SAMPLING_TIME  		    (PAC_PTA_REGS + 24  )
#define PAC_PTA_EPTA_OUTPUT_VALID_TIME 		    (PAC_PTA_REGS + 28  )
#define PAC_PTA_TXRX_SAMPLING_TIME   		    (PAC_PTA_REGS + 32  )
#define PAC_PTA_BT_SLOT_DELAY  				    (PAC_PTA_REGS + 36  )

#define PAC_PTA_QUOTA  						    (PAC_PTA_REGS + 40  )

#define PAC_PTA_DEFAULT_PRIORITY  			    (PAC_PTA_REGS + 44  )

#define PAC_PTA_PRIORITIZE_NEXT_LINKIDS_0  	    (PAC_PTA_REGS + 48  )
#define PAC_PTA_PRIORITIZE_NEXT_LINKIDS_1  	    (PAC_PTA_REGS + 52  )

#define PAC_PTA_F_GRANT_LINKID_0  			    (PAC_PTA_REGS + 56 	)
#define PAC_PTA_F_GRANT_LINKID_1  			    (PAC_PTA_REGS + 60 	)

#define PAC_PTA_INT_ENABLE  				    (PAC_PTA_REGS + 64  )
#define PAC_PTA_INT_STATUS  				    (PAC_PTA_REGS + 68  )
#define PAC_PTA_INT_CLEAR  					    (PAC_PTA_REGS + 72  )

#define PAC_PTA_SW_OVERRIDE  				    (PAC_PTA_REGS + 76  )
																	
#define PAC_PTA_SERIAL_IF_CONTROL  			    (PAC_PTA_REGS + 80  )
#define PAC_PTA_SERIAL_IF_TRIGGER  			    (PAC_PTA_REGS + 84  )
#define PAC_PTA_SERIAL_IF_INT_ENABLE  		    (PAC_PTA_REGS + 88  )
#define PAC_PTA_SERIAL_IF_INT_STATUS  		    (PAC_PTA_REGS + 92  )
#define PAC_PTA_SERIAL_IF_INT_CLEAR  		    (PAC_PTA_REGS + 96  )
#define PAC_PTA_SERIAL_TIMER   				    (PAC_PTA_REGS + 100 )
																	
#define PAC_PTA_BT_TO_WLAN_MESSAGE  		    (PAC_PTA_REGS + 104 )
#define PAC_PTA_BT_TO_WLAN_PARAMETER  		    (PAC_PTA_REGS + 108 )
#define PAC_PTA_WLAN_TO_BT_MESSAGE  		    (PAC_PTA_REGS + 112 )
#define PAC_PTA_WLAN_TO_BT_PARAMETER  		    (PAC_PTA_REGS + 116 )
																	
#define PAC_PTA_WLAN_SW_OVERRIDE_1  		    (PAC_PTA_REGS + 120 )
#define PAC_PTA_WLAN_SW_OVERRIDE_2  		    (PAC_PTA_REGS + 124 )
#define PAC_PTA_BT_SW_OVERRIDE 				    (PAC_PTA_REGS + 128 )
																	
#define PAC_PTA_PRIORITY_MATRIX_0  			    (PAC_PTA_REGS + 132	)
#define PAC_PTA_PRIORITY_MATRIX_1  			    (PAC_PTA_REGS + 136	)
#define PAC_PTA_PRIORITY_MATRIX_2  			    (PAC_PTA_REGS + 140	)
#define PAC_PTA_PRIORITY_MATRIX_3  			    (PAC_PTA_REGS + 144	)
#define PAC_PTA_PRIORITY_MATRIX_4  			    (PAC_PTA_REGS + 148	)
#define PAC_PTA_PRIORITY_MATRIX_5  			    (PAC_PTA_REGS + 152	)
#define PAC_PTA_PRIORITY_MATRIX_6  			    (PAC_PTA_REGS + 156	)
#define PAC_PTA_PRIORITY_MATRIX_7  			    (PAC_PTA_REGS + 160	)
#define PAC_PTA_PRIORITY_MATRIX_8  			    (PAC_PTA_REGS + 164	)
#define PAC_PTA_PRIORITY_MATRIX_9  			    (PAC_PTA_REGS + 168	)
#define PAC_PTA_PRIORITY_MATRIX_10  		    (PAC_PTA_REGS + 172	)
#define PAC_PTA_PRIORITY_MATRIX_11  		    (PAC_PTA_REGS + 176	)
#define PAC_PTA_PRIORITY_MATRIX_12  		    (PAC_PTA_REGS + 180	)
#define PAC_PTA_PRIORITY_MATRIX_13  		    (PAC_PTA_REGS + 184	)
#define PAC_PTA_PRIORITY_MATRIX_14  		    (PAC_PTA_REGS + 188	)
#define PAC_PTA_PRIORITY_MATRIX_15  		    (PAC_PTA_REGS + 192	)
																	
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_0  	    (PAC_PTA_REGS + 196 )
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_1  	    (PAC_PTA_REGS + 200 )
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_2  	    (PAC_PTA_REGS + 204 )
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_3  	    (PAC_PTA_REGS + 208 )
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_4  	    (PAC_PTA_REGS + 212 )
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_5  	    (PAC_PTA_REGS + 216 )
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_6  	    (PAC_PTA_REGS + 220 )
#define PAC_PTA_LINKIDS2MONITOR_ARRAY_7  	    (PAC_PTA_REGS + 224 )
																	
#define PAC_PTA_LINKINTERVALS_ARRAY_0	  	    (PAC_PTA_REGS + 228 )
#define PAC_PTA_LINKINTERVALS_ARRAY_1	  	    (PAC_PTA_REGS + 232 )
#define PAC_PTA_LINKINTERVALS_ARRAY_2	  	    (PAC_PTA_REGS + 236 )
#define PAC_PTA_LINKINTERVALS_ARRAY_3	  	    (PAC_PTA_REGS + 240 )
#define PAC_PTA_LINKINTERVALS_ARRAY_4	  	    (PAC_PTA_REGS + 244 )
#define PAC_PTA_LINKINTERVALS_ARRAY_5	  	    (PAC_PTA_REGS + 248 )
#define PAC_PTA_LINKINTERVALS_ARRAY_6	  	    (PAC_PTA_REGS + 252 )
#define PAC_PTA_LINKINTERVALS_ARRAY_7	  	    (PAC_PTA_REGS + 256 )
																	
#define PAC_PTA_STAT_ENABLE  				    (PAC_PTA_REGS + 260 )
#define PAC_PTA_STAT_CLEAR  				    (PAC_PTA_REGS + 264 )
#define PAC_PTA_STAT_ANTENNA_OCCUPANCY 		    (PAC_PTA_REGS + 268 )
#define PAC_PTA_STAT_BT_REQUEST  			    (PAC_PTA_REGS + 272 )
#define PAC_PTA_STAT_BT_IS_GRANTED  		    (PAC_PTA_REGS + 276 )
#define PAC_PTA_STAT_BT_IS_NOT_GRANTED 		    (PAC_PTA_REGS + 280 )
#define PAC_PTA_STAT_WLAN_ACTIVE  			    (PAC_PTA_REGS + 284 )
											    				
#define PAC_PTA_STAT_ACTIVE_LINKIDS_0  		    (PAC_PTA_REGS + 288 )
#define PAC_PTA_STAT_ACTIVE_LINKIDS_1  		    (PAC_PTA_REGS + 292 )
#define PAC_PTA_STAT_LINKIDS2MONITOR  		    (PAC_PTA_REGS + 296 )
#define PAC_PTA_STAT_LINKID_0_ANT_OCC  		    (PAC_PTA_REGS + 300 )
#define PAC_PTA_STAT_LINKID_1_ANT_OCC 		    (PAC_PTA_REGS + 304 )
#define PAC_PTA_STAT_LINKID_2_ANT_OCC  		    (PAC_PTA_REGS + 308 )
#define PAC_PTA_STAT_LINKID_3_ANT_OCC  		    (PAC_PTA_REGS + 312 )
#define PAC_PTA_STAT_LINKID_4_ANT_OCC  		    (PAC_PTA_REGS + 316 )
											 						
#define PAC_PTA_RESERVED_1  				    (PAC_PTA_REGS + 320 )
#define PAC_PTA_RESERVED_2  				    (PAC_PTA_REGS + 324 )
#define PAC_PTA_RESERVED_3  				    (PAC_PTA_REGS + 328 )
#define PAC_PTA_RESERVED_4  				    (PAC_PTA_REGS + 332 )
#define PAC_PTA_RESERVED_5  				    (PAC_PTA_REGS + 336 )
#define PAC_PTA_RESERVED_6  				    (PAC_PTA_REGS + 340 )
#define PAC_PTA_RESERVED_7  				    (PAC_PTA_REGS + 344 )
#define PAC_PTA_RESERVED_8  				    (PAC_PTA_REGS + 348 )
#define PAC_PTA_RESERVED_9  				    (PAC_PTA_REGS + 352 )
#define PAC_PTA_RESERVED_10  				    (PAC_PTA_REGS + 356 )
#define PAC_PTA_RESERVED_11 				    (PAC_PTA_REGS + 360 )
#define PAC_PTA_RESERVED_12 				    (PAC_PTA_REGS + 364 )
#define PAC_PTA_RESERVED_13 				    (PAC_PTA_REGS + 368 )
#define PAC_PTA_RESERVED_14 				    (PAC_PTA_REGS + 372 )
#define PAC_PTA_RESERVED_15 				    (PAC_PTA_REGS + 376 )
																	
#define PAC_PTA_ENABLE_OUTPUT_ARRAYS  		    (PAC_PTA_REGS + 380 )
#define PAC_PTA_WIB_OUTPUT_ARRAY_1  		    (PAC_PTA_REGS + 384 )
#define PAC_PTA_WIB_OUTPUT_ARRAY_2  		    (PAC_PTA_REGS + 388 )
#define PAC_PTA_IFB_OUTPUT_ARRAY  			    (PAC_PTA_REGS + 392 )
#define PAC_PTA_PRB_OUTPUT_ARRAY  			    (PAC_PTA_REGS + 396 )
#define PAC_PTA_PRM_OUTPUT_ARRAY  			    (PAC_PTA_REGS + 400 )
#define PAC_PTA_CPB_OUTPUT_ARRAY  			    (PAC_PTA_REGS + 404 )
#define PAC_PTA_PSM_OUTPUT_ARRAY  			    (PAC_PTA_REGS + 408 )
#define PAC_PTA_SIF_OUTPUT_ARRAY_1  		    (PAC_PTA_REGS + 412 )
#define PAC_PTA_SIF_OUTPUT_ARRAY_2  		    (PAC_PTA_REGS + 416 )
#define PAC_PTA_SIF_OUTPUT_ARRAY_3  		    (PAC_PTA_REGS + 420 )
#define PAC_PTA_DEBUG_PORT_SELECT  			    (PAC_PTA_REGS + 424 )
#define PAC_PTA_EPTA_VERSION				    (PAC_PTA_REGS + 428 )
#define PAC_PTA_RESERVED_16  				    (PAC_PTA_REGS + 428 )
#define PAC_PTA_LAST  						    (PAC_PTA_REGS + 512 )										


/*****************************
* PAC_TIM_ (Timers Block) Registers
*****************************/

#define PAC_TIM_PRESCALE0                       (PAC_TIM_REGS + 0x00)
#define PAC_TIM_PRESCALE1                       (PAC_TIM_REGS + 0x04)
#define PAC_TIM_TSF_HW_EVENT0                   (PAC_TIM_REGS + 0x08)
#define PAC_TIM_TSF_HW_EVENT1                   (PAC_TIM_REGS + 0x0C)
#define PAC_TIM_TSF_REQ_SWITCH                  (PAC_TIM_REGS + 0x10)
#define PAC_TIM_TSF_SW_EVENT2                   (PAC_TIM_REGS + 0x14)
#define PAC_TIM_TSF_SW_EVENT3                   (PAC_TIM_REGS + 0x18)
#define PAC_TIM_TSF_NAV_LOAD                    (PAC_TIM_REGS + 0x1C)
#define PAC_TIM_RX_NAV_LOAD_DURATION            (PAC_TIM_REGS + 0x20)
#define PAC_TIM_TSF_NAV_LOAD_DURATION           (PAC_TIM_REGS + 0x24)
#define PAC_TIM_RXD_TSF_COMP                    (PAC_TIM_REGS + 0x28)
#define PAC_TIM_TICK_TIMER                      (PAC_TIM_REGS + 0x2C)
#define PAC_TIM_SLOT_TIMER                      (PAC_TIM_REGS + 0x30)
#define PAC_TIM_TIMER_CONTROL                   (PAC_TIM_REGS + 0x34)
#define PAC_TIM_TSF_TIMER_LOW                   (PAC_TIM_REGS + 0x38)
#define PAC_TIM_TSF_TIMER_HIGH                  (PAC_TIM_REGS + 0x3C)
#define PAC_TIM_NAV_TIMER                       (PAC_TIM_REGS + 0x40)
#define PAC_TIM_TICK                            (PAC_TIM_REGS + 0x44)
#define PAC_TIM_SLOT                            (PAC_TIM_REGS + 0x48)
#define PAC_TIM_BACKOFF_COUNTER                 (PAC_TIM_REGS + 0x4C)
#define PAC_TIM_IFS_TIMER                       (PAC_TIM_REGS + 0x50)
#define PAC_TIM_RESP_TIMER                      (PAC_TIM_REGS + 0x54)
#define PAC_TIM_GBM_DIFS                        (PAC_TIM_REGS + 0x58)
#define PAC_TIM_GBM_EIFS                        (PAC_TIM_REGS + 0x5C)
#define PAC_TIM_GBM_SIFS                        (PAC_TIM_REGS + 0x60)
#define PAC_TIM_EBM_AIFSN                       (PAC_TIM_REGS + 0x64)
#define PAC_TIM_EBM_CWMIN01_W0                  (PAC_TIM_REGS + 0x68)
#define PAC_TIM_EBM_CWMIN23_W0                  (PAC_TIM_REGS + 0x6C)
#define PAC_TIM_EBM_TXOPLIM0                    (PAC_TIM_REGS + 0x70)
#define PAC_TIM_EBM_TXOPLIM1                    (PAC_TIM_REGS + 0x74)
#define PAC_TIM_EBM_TXOPLIM2                    (PAC_TIM_REGS + 0x78)
#define PAC_TIM_EBM_TXOPLIM3                    (PAC_TIM_REGS + 0x7C)
#define PAC_TIM_EBM_IFS_ADJUST                  (PAC_TIM_REGS + 0x80)
#define PAC_TIM_EBM_INT_CONTEN                  (PAC_TIM_REGS + 0x84)
#define PAC_TIM_EBM_EVENT_SEL                   (PAC_TIM_REGS + 0x88)
#define PAC_TIM_EBM_CONTROLS                    (PAC_TIM_REGS + 0x8C)
#define PAC_TIM_EBM_STAT                        (PAC_TIM_REGS + 0x90)
#define PAC_TIM_TSF_OFFSET                      (PAC_TIM_REGS + 0x94)
/*
"	Int contention indicates if an AC has lost an internal contention (1= contention), Bit3 AC3, bit2 AC2, bit1 AC1, bit0, AC0. 
"      Note that this bit can also be set if a tx_error has occurred - in this case the NTD status will contain a PTCS_ABORT.
"	First frame error indicates that the first frame in an AC queue will not fit within the TXOPlimit, Bit7 AC3, bit6 AC2, bit5 AC1, bit4, AC0
"	Ack_fail indicates that there has been an ack failure, Bit11 AC3, bit10 AC2, bit9 AC1, bit8, AC0
"	Tx_err indicates that there has been a transmit failure, either caused by bad PHY control or by a BT Abort (NTD status will reflect this).
"	While a bit (either int_conten, ack_fail, tx_err or first_frame_error) is set to a 1 the queue for that AC is suspended
"	Writing a 0 to clear a bit, causes the TTCS at the head of the relevant AC queue to be reloaded and then the AC queue operation will be resumed.
*/
#define PAC_TIM_EBM_TXOP_STATUS                 (PAC_TIM_REGS + 0x98)
#define PAC_TIM_EBM_TXOP_PRESCALE_LIMIT         (PAC_TIM_REGS + 0x9C)
#define PAC_TIM_EBM_STAT_2                      (PAC_TIM_REGS + 0xA0)
#define PAC_TIM_EBM_STAT_3                      (PAC_TIM_REGS + 0xA4)
#if 1 // (CHIP_TYPE==CHIP_ARES_B)
#define PAC_TIM_TX_TIMEOUT_0					(PAC_TIM_REGS + 0xAC)
#define PAC_TIM_TX_TIMEOUT_1					(PAC_TIM_REGS + 0xB0)
#define PAC_TIM_TX_TIMEOUT_2					(PAC_TIM_REGS + 0xB4)
#define PAC_TIM_TX_TIMEOUT_3					(PAC_TIM_REGS + 0xB8)
#define PAC_TIM_TX_TIMEOUT_4					(PAC_TIM_REGS + 0xBC)
#endif
/*****************************
* PAC_BAB_ (Block Acknowledge Bitmapper) Registers
*****************************/

#define PAC_BAB_GLOBAL_CON                      (PAC_BAB_REGS + 0x100)
#define PAC_BAB_FRAME_FORMAT                    (PAC_BAB_REGS + 0x104)
#define PAC_BAB_GLOBAL_STATUS                   (PAC_BAB_REGS + 0x108)
#define PAC_BAB_MODE_CONTROL_0                  (PAC_BAB_REGS + 0x00)
#define PAC_BAB_WINDOW_CONTROL_0                (PAC_BAB_REGS + 0x04)
#define PAC_BAB_MODE_CONTROL_1                  (PAC_BAB_REGS + 0x20)
#define PAC_BAB_WINDOW_CONTROL_1                (PAC_BAB_REGS + 0x24)
#define PAC_BAB_MODE_CONTROL_2                  (PAC_BAB_REGS + 0x40)
#define PAC_BAB_WINDOW_CONTROL_2                (PAC_BAB_REGS + 0x44)
#define PAC_BAB_MODE_CONTROL_3                  (PAC_BAB_REGS + 0x60)
#define PAC_BAB_WINDOW_CONTROL_3                (PAC_BAB_REGS + 0x64)
#define PAC_BAB_WINDOW_WORD_0_0                 (PAC_BAB_REGS + 0x10)
#define PAC_BAB_WINDOW_WORD_1_0                 (PAC_BAB_REGS + 0x14)
#define PAC_BAB_WINDOW_WORD_2_0                 (PAC_BAB_REGS + 0x18)
#define PAC_BAB_WINDOW_WORD_3_0                 (PAC_BAB_REGS + 0x1C)
#define PAC_BAB_WINDOW_WORD_0_1                 (PAC_BAB_REGS + 0x30)
#define PAC_BAB_WINDOW_WORD_1_1                 (PAC_BAB_REGS + 0x34)
#define PAC_BAB_WINDOW_WORD_2_1                 (PAC_BAB_REGS + 0x38)
#define PAC_BAB_WINDOW_WORD_3_1                 (PAC_BAB_REGS + 0x3C)
#define PAC_BAB_WINDOW_WORD_0_2                 (PAC_BAB_REGS + 0x50)
#define PAC_BAB_WINDOW_WORD_1_2                 (PAC_BAB_REGS + 0x54)
#define PAC_BAB_WINDOW_WORD_2_2                 (PAC_BAB_REGS + 0x58)
#define PAC_BAB_WINDOW_WORD_3_2                 (PAC_BAB_REGS + 0x5C)
#define PAC_BAB_WINDOW_WORD_0_3                 (PAC_BAB_REGS + 0x70)
#define PAC_BAB_WINDOW_WORD_1_3                 (PAC_BAB_REGS + 0x74)
#define PAC_BAB_WINDOW_WORD_2_3                 (PAC_BAB_REGS + 0x78)
#define PAC_BAB_WINDOW_WORD_3_3                 (PAC_BAB_REGS + 0x7C)

/************************************************
* BAP -Beacon Assist Processor Registers
************************************************/
#define PAC_BAP_CONTROL                         (PAC_BAP_REGS + 0x00)
#define PAC_BAP_BSSID_LOW                       (PAC_BAP_REGS + 0x04)
#define PAC_BAP_BSSID_HIGH                      (PAC_BAP_REGS + 0x08)
#define PAC_BAP_AID                             (PAC_BAP_REGS + 0x0C)
#define PAC_BAP_LAST_BEACON_CRC                 (PAC_BAP_REGS + 0x10)
#define PAC_BAP_NEW_BEACON_CRC                  (PAC_BAP_REGS + 0x14)
#define PAC_BAP_STATUS_TIM_INFO                 (PAC_BAP_REGS + 0x18)
#define PAC_BAP_STATUS_STATE                    (PAC_BAP_REGS + 0x1C)

/*****************************
* WEP_ (Wired Equivalent Privacy) Registers
*****************************/

#define WEP_CONTROL                             (WEP_REGS + 0x00)
#define WEP_STATUS                              (WEP_REGS + 0x04)
#define WEP_SRC_ADDRESS                         (WEP_REGS + 0x08)
#define WEP_DST_ADDRESS                         (WEP_REGS + 0x0C)
#define WEP_CUR_ADDRESS_OFFSET                  (WEP_REGS + 0x10)
#define WEP_LENGTH                              (WEP_REGS + 0x14)
#define WEP_KEY_LENGTH                          (WEP_REGS + 0x18)
#define WEP_KEY_31TO0                           (WEP_REGS + 0x1C)
#define WEP_KEY_63TO32                          (WEP_REGS + 0x20)
#define WEP_KEY_95TO64                          (WEP_REGS + 0x24)
#define WEP_KEY_127TO96                         (WEP_REGS + 0x28)
#define WEP_CRC_RESULT                          (WEP_REGS + 0x2C)
#define WEP_RC4_DEBUG_WR_EN                     (WEP_REGS + 0x30)
#define WEP_RC4_DEBUG_ADDRESS                   (WEP_REGS + 0x34)
#define WEP_RC4_DEBUG_WDATA                     (WEP_REGS + 0x38)
#define WEP_RC4_DEBUG_RDATA                     (WEP_REGS + 0x3C)


/*****************************
* RAB_ (Register Access Block) Registers
*****************************/

#define RAB_SMC_ACCESS_DISABLE                  (RAB_REGS + 0x00)
#define RAB_RESERVED                            (RAB_REGS + 0x04)
#define RAB_UPI_INTERRUPT                       (RAB_REGS + 0x08)
#define RAB_UPI_INTERRUPT_CLEAR                 (RAB_REGS + 0x0c)
#define RAB_LPI_INTERRUPT                       (RAB_REGS + 0x10)
#define RAB_LPI_INTERRUPT_CLEAR                 (RAB_REGS + 0x14)
#define RAB_PHI_INTERRUPT                       (RAB_REGS + 0x18)
#define RAB_PHI_INTERRUPT_CLEAR                 (RAB_REGS + 0x1c)
#define RAB_REVISION                            (RAB_REGS + 0x40)

/*****************************
* MIC_ (Message Integrity Check) Registers
*****************************/

#define MIC_K0                                  (MIC_REGS + 0x00)
#define MIC_K1                                  (MIC_REGS + 0x04)
#define MIC_WORD_COUNT                          (MIC_REGS + 0x08)
#define MIC_START_DATA                          (MIC_REGS + 0x0C)
#define MIC_START                               (MIC_REGS + 0x10)
#define MIC_MIC0                                (MIC_REGS + 0x14)
#define MIC_MIC1                                (MIC_REGS + 0x18)
#define MIC_CONTROL                             (MIC_REGS + 0x1c)
#define MIC_START_COEFF                         (MIC_REGS + 0x20)
#define MIC_MMH_KEY                             (MIC_REGS + 0x24)

/*****************************
* AES_ (Advanced Encryption Standard) Registers
*****************************/

#define AES_CTRL_STAT                           (AES_REGS + 0x00)
#define AES_DATA_FIFO                           (AES_REGS + 0x04)
#define AES_UCODE_RAM                           (AES_REGS + 0x08)
#define AES_RESERVED                            (AES_REGS + 0x0C)
#define AES_DMA_SRC                             (AES_REGS + 0x10)
#define AES_DMA_DEST                            (AES_REGS + 0x14)
#define AES_DMA_LEN                             (AES_REGS + 0x18)

/*****************************
* PAC_SEQ_ (PAC Sequencer) Registers
*****************************/

#define PAC_SEQ_QUEUE_0_WRITE_PORT              (PAC_SEQ_REGS + 0x00)//0x9c60000
#define PAC_SEQ_QUEUE_0_PTCS_POINTER            (PAC_SEQ_REGS + 0x0C)
#define PAC_SEQ_QUEUE_0_PTCS_SIZE               (PAC_SEQ_REGS + 0x10)
#define PAC_SEQ_QUEUE_0_ENABLE                  (PAC_SEQ_REGS + 0x14)
#define PAC_SEQ_QUEUE_0_CONTROL_FLAGS           (PAC_SEQ_REGS + 0x18)
#define PAC_SEQ_QUEUE_0_CONTROL_POINTER         (PAC_SEQ_REGS + 0x1c)
#define PAC_SEQ_QUEUE_0_CONTROL                 (PAC_SEQ_REGS + 0x20)
#define PAC_SEQ_QUEUE_0_ENTRY_0                 (PAC_SEQ_REGS + 0x24)
#define PAC_SEQ_QUEUE_0_ENTRY_1                 (PAC_SEQ_REGS + 0x28)
#define PAC_SEQ_QUEUE_0_ENTRY_2                 (PAC_SEQ_REGS + 0x2C)
#define PAC_SEQ_QUEUE_0_ENTRY_3                 (PAC_SEQ_REGS + 0x30)
#define PAC_SEQ_QUEUE_1_WRITE_PORT              (PAC_SEQ_REGS + 0x80)
#define PAC_SEQ_QUEUE_1_PTCS_POINTER            (PAC_SEQ_REGS + 0x8C)
#define PAC_SEQ_QUEUE_1_PTCS_SIZE               (PAC_SEQ_REGS + 0x90)
#define PAC_SEQ_QUEUE_1_ENABLE                  (PAC_SEQ_REGS + 0x94)
#define PAC_SEQ_QUEUE_1_CONTROL_FLAGS           (PAC_SEQ_REGS + 0x98)
#define PAC_SEQ_QUEUE_1_CONTROL_POINTER         (PAC_SEQ_REGS + 0x9c)
#define PAC_SEQ_QUEUE_1_CONTROL                 (PAC_SEQ_REGS + 0xA0)
#define PAC_SEQ_QUEUE_1_ENTRY_0                 (PAC_SEQ_REGS + 0xA4)
#define PAC_SEQ_QUEUE_1_ENTRY_1                 (PAC_SEQ_REGS + 0xA8)
#define PAC_SEQ_QUEUE_1_ENTRY_2                 (PAC_SEQ_REGS + 0xAC)
#define PAC_SEQ_QUEUE_1_ENTRY_3                 (PAC_SEQ_REGS + 0xB0)
#define PAC_SEQ_QUEUE_2_WRITE_PORT              (PAC_SEQ_REGS + 0x100)
#define PAC_SEQ_QUEUE_2_PTCS_POINTER            (PAC_SEQ_REGS + 0x10C)
#define PAC_SEQ_QUEUE_2_PTCS_SIZE               (PAC_SEQ_REGS + 0x110)
#define PAC_SEQ_QUEUE_2_ENABLE                  (PAC_SEQ_REGS + 0x114)
#define PAC_SEQ_QUEUE_2_CONTROL_FLAGS           (PAC_SEQ_REGS + 0x118)
#define PAC_SEQ_QUEUE_2_CONTROL_POINTER         (PAC_SEQ_REGS + 0x11c)
#define PAC_SEQ_QUEUE_2_CONTROL                 (PAC_SEQ_REGS + 0x120)
#define PAC_SEQ_QUEUE_2_ENTRY_0                 (PAC_SEQ_REGS + 0x124)
#define PAC_SEQ_QUEUE_2_ENTRY_1                 (PAC_SEQ_REGS + 0x128)
#define PAC_SEQ_QUEUE_2_ENTRY_2                 (PAC_SEQ_REGS + 0x12C)
#define PAC_SEQ_QUEUE_2_ENTRY_3                 (PAC_SEQ_REGS + 0x130)
#define PAC_SEQ_QUEUE_3_WRITE_PORT              (PAC_SEQ_REGS + 0x180)
#define PAC_SEQ_QUEUE_3_PTCS_POINTER            (PAC_SEQ_REGS + 0x18C)
#define PAC_SEQ_QUEUE_3_PTCS_SIZE               (PAC_SEQ_REGS + 0x190)
#define PAC_SEQ_QUEUE_3_ENABLE                  (PAC_SEQ_REGS + 0x194)
#define PAC_SEQ_QUEUE_3_CONTROL_FLAGS           (PAC_SEQ_REGS + 0x198)
#define PAC_SEQ_QUEUE_3_CONTROL_POINTER         (PAC_SEQ_REGS + 0x19c)
#define PAC_SEQ_QUEUE_3_CONTROL                 (PAC_SEQ_REGS + 0x1A0)
#define PAC_SEQ_QUEUE_3_ENTRY_0                 (PAC_SEQ_REGS + 0x1A4)
#define PAC_SEQ_QUEUE_3_ENTRY_1                 (PAC_SEQ_REGS + 0x1A8)
#define PAC_SEQ_QUEUE_3_ENTRY_2                 (PAC_SEQ_REGS + 0x1AC)
#define PAC_SEQ_QUEUE_3_ENTRY_3                 (PAC_SEQ_REGS + 0x1B0)

#endif


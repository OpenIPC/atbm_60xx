/* -*-C-*-
*******************************************************************************
* altobeam
* Reproduction and Communication of this document is strictly prohibited
* unless specifically authorized in writing by altobeam
******************************************************************************/
/**
* \file
* \ingroup PHY CONTROL MODULE
*
* \brief File contains register configuration functionality and information.
*/
/*
***************************************************************************
*  Copyright (c) 2008     altobeam R & D Ltd.
* Copyright altobeam, 2009 锟紸ll rights reserved.
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
*
****************************************************************************/

/***********************************************************************/
/***                        Include Files                            ***/
/***********************************************************************/
#include "phy_regs.h"
#include "Rf_Rx_Tx_Psm.h"

#define PHY_RF_HW_WRITE_REG	HW_WRITE_REG
#define PHY_RI_HW_WRITE_REG	HW_WRITE_REG

//NORELOC const REGVAL_Bit RF_INIT_REG_FULLMASK_BIT_TABLE[];

/***********************************************************************/
/***               Externally Visible Static Data                    ***/
/***********************************************************************/

//extern const REGVAL BBDIG_SETUP_Table[];

/***********************************************************************/
/***                Internally Visible Functions                     ***/
/***********************************************************************/
static void PHY_RF_SetReg_Table(const rf_reg *psm_table)
{
	u32 iRegAddr;
	u32 iRegVal;
	u32 const *p;
	
	p = (u32*)psm_table;
			
	while (*p != PHY_RFIP_INVALID)
	{
		iRegAddr = *p++;
		iRegVal  = *p++; 
			
		PHY_RF_HW_WRITE_REG(iRegAddr, iRegVal);
		
	}	
}
static void ROM_PHY_RF_WriteReg_Bit_u16(const rf_reg_bit_u16* rf_reg_bit_table)
{
	rf_reg_bit_u16 *pReg;
	pReg = (rf_reg_bit_u16*)rf_reg_bit_table;

	while (pReg->addr != PHY_RFIP_INVALID)
	{		
		ROM_HW_WRITE_REG_BIT(pReg->addr,pReg->endBit,pReg->startBit,pReg->data);
		pReg++;
	}
}

static void ROM_PHY_TX_Psm_addr_value(const u16 *reg_addr_vec,const u32 *reg_value_vec, u32 addr_num)
{
	u32 addr_idx;
	for(addr_idx=0;addr_idx<addr_num;addr_idx++)
	{
		PHY_RF_HW_WRITE_REG(reg_addr_vec[addr_idx] + RFIP_BASE_ADDR,reg_value_vec[addr_idx]);
	}
	
}

/**************************************************************************
**
** NAME         PHY_RF_TX_Config_Cut1
**
** PARAMETERS:	PHY_MODEM_T mode
**
** RETURNS:     void
**
** DESCRIPTION  Initializes the cut1 Tx.
**
**************************************************************************/
static void PHY_RF_TX_Config( void )
{
	struct wsm_phy_regval_sets *regval_sets = phy_reg_table_get();
	//u32 uiRegValue;
	// Original test pattern: reg w							RFIPTXCALCFG		=0030,0040000
	//PHY_RF_HW_WRITE_REG(PHY_RFIP_TXCALCFG_ADDR, 0x40000);
	//uiRegValue=PHY_RF_HW_READ_REG(PHY_RFIP_TXRF_REG0_ADDR);
	//uiRegValue&=~0xe;
	//PHY_RF_HW_WRITE_REG(PHY_RFIP_TXRF_REG0_ADDR,uiRegValue);
	ROM_HW_WRITE_REG_BIT(PHY_RFIP_TXRF_REG0_ADDR,3,1,0);
    {
    	// Original test pattern: reg w	txForceLdoOn=1 -> FrameDetect bypassed		RFIPTXLDOCFG		=001C,00000001
        PHY_RF_HW_WRITE_REG(PHY_RFIP_TXLDOCFG_ADDR,PHY_RFIP_TXLDOCFG_TXFORCELDOON_EN);//value=0x9=1001
    }
	
	/*Psm status reg value*/
	if(regval_sets->phy_params.rfSubtypeDefine == regval_sets->phy_params.aresAzlc){
		ROM_PHY_TX_Psm_addr_value(Rf_Tx_Psm_addr_offset,Rf_Tx_Psm_valueAresAZLC,14);
	}else {
		ROM_PHY_TX_Psm_addr_value(Rf_Tx_Psm_addr_offset,Rf_Tx_Psm_valueAres,14);
	}

}

static void PHY_RF_PA_Reg(void)
{
	struct wsm_phy_regval_sets *regval_sets = phy_reg_table_get();
	if(regval_sets->phy_params.rfSubtypeDefine == regval_sets->phy_params.aresAy){
		ROM_PHY_RF_WriteReg_Bit_u16(Rf_TxPa_TableAresAy);
	}else if(regval_sets->phy_params.rfSubtypeDefine == regval_sets->phy_params.aresAzlc){
		ROM_PHY_RF_WriteReg_Bit_u16(Rf_TxPa_TableAresAZLC);
	}else {
		ROM_PHY_RF_WriteReg_Bit_u16(Rf_TxPa_TableAres);
	}
	
	ROM_HW_WRITE_REG_BIT(PHY_RFIP_TXPA_VSWR,12, 9, 0x3);
	ROM_HW_WRITE_REG_BIT(PHY_RFIP_TXPA_REG1_ADDR,13, 11, 0x7);
	
}
static void PHY_RF_Temp_Reg(void)
{
	//9:0 		Delay, c8 = 200, means delay 12.5ns*200 = 2.5us
	//27:16			Accumulation Time: 5
	PHY_RI_HW_WRITE_REG(PHY_RI_TEMPDSSS_ADDR, 0x400C8); // set number of 10MHz samples and delay 0ac38440
	PHY_RI_HW_WRITE_REG(PHY_RI_TEMPOFDM_ADDR, 0x400C8); //
}

static void PHY_RF_Init_Reg(void)
{
	struct wsm_phy_regval_sets *regval_sets = phy_reg_table_get();
	//u32 uRegValue;	
	
	/*Init some important reg*/	
	//uRegValue=PHY_RF_HW_READ_REG(PHY_RFIP_SOFT_CFG_TXLO_ADD);
	///uRegValue&=~BIT(0);
	//PHY_RF_HW_WRITE_REG(PHY_RFIP_SOFT_CFG_TXLO_ADD,uRegValue);
	ROM_HW_WRITE_REG_BIT(PHY_RFIP_SOFT_CFG_TXLO_ADD,0,0,0);
	PHY_RF_SetReg_Table(Rf_init_value);	
	if(regval_sets->phy_params.rfSubtypeDefine == regval_sets->phy_params.aresAzlc)
		HW_WRITE_REG(PHY_RFIP_RSV_RW1_ADDR,0xff6f8f1f);
	ROM_HW_WRITE_REG_BIT(PHY_RFIP_RSV_RW0_ADDR, 31, 31, 1);
}
static void PHY_RF_RX_Config(void)
{
	 //0x18001c = 0b110000000000000011100
	 //u32 uiRegValue;
	 //uiRegValue=PHY_RF_HW_READ_REG(PHY_RFIP_RXRF_REG0_ADDR);
	 ///uiRegValue&=~0x18001c;
	 ///PHY_RF_HW_WRITE_REG(PHY_RFIP_RXRF_REG0_ADDR,uiRegValue);
	/*Psm status reg value*/
	ROM_HW_WRITE_REG_BIT(PHY_RFIP_RXRF_REG0_ADDR,4,2,0);
	ROM_HW_WRITE_REG_BIT(PHY_RFIP_RXRF_REG0_ADDR,20,19,0);
	PHY_RF_SetReg_Table(Rf_Rx_Psm);
}

static void PHY_RF_RxAdc_Init(void)
{	
	struct wsm_phy_regval_sets *regval_sets = phy_reg_table_get();
	
	ROM_PHY_RF_WriteReg_Bit_u16(Rf_RxAdc_Table);
	{
		if(GET_LMAC_REDUCE_POWER_CONSUMPTION(__le32_to_cpu(regval_sets->phy_params.compileMacro))){
			ROM_HW_WRITE_REG_BIT(0x0acc0140,14,8,7);
		}else {
			ROM_HW_WRITE_REG_BIT(0x0acc0140,14,8,3);
		}
		
	}
	/*
#if REDUCE_POWER_CONSUMPTION
	{0x0acc0140, 14 , 8 , 7},    //rxadc_ibcomp_q[1:0],bit[9:8],bit[111:10]rxadc_input_buffer_ibias_adj													 
	//{0x0acc0140, 14, 10, 0 },	//rxadc_input_buffer_ibias_adj[1:0],bit[14:10] 	
#else
	{0x0acc0140, 14, 8 , 3},	//rxadc_ibcomp_q[1:0],bit[9:8]													 
	//{0x0acc0140, 14, 10, 0 },	//rxadc_input_buffer_ibias_adj[1:0],bit[14:10] 
#endif

*/
}


static void PHY_RF_Abb_RcCal_Init(void)
{
	/*rx_lpf_ccal_offset<4:0>=rccali_result<4:0>*/
		//	ROM_HW_WRITE_REG_BIT(0xacc00A4,8,4,0xf);	
	/*tx_lpf_ccal_offset<4:0>=rccali_result<4:0>*/
		//	ROM_HW_WRITE_REG_BIT(0xacc00A4,13,9,0xf);

	ROM_HW_WRITE_REG_BIT(0xacc00A4,13,4,(0x1f |(0x1f<<5)));
	ROM_HW_WRITE_REG_BIT(0xacc0134,30,30,0);
	ROM_HW_WRITE_REG_BIT(RFIP_BASE_ADDR + 0x09c,9,8,3);
#if 0
    uint32 RcTuningResults;
	uint32 read_data_rccal_done;
	uint32 rxabb_ldo_ref_pup_bak;
	uint32 rxabb_ldo_load_en_bak;
	/*back up psm status reg*/
	/* 1:½øÈëforce state×´Ì¬*/
	rxabb_ldo_ref_pup_bak=HW_READ_REG(0xacc01a0);
	rxabb_ldo_load_en_bak=HW_READ_REG(0xacc01a4);
	ROM_HW_WRITE_REG_BIT(0xacc0004,1,1,1);
	/* 2:´ò¿ªRXABB LDO¸øRC calibrationµÍÑ¹Ä£¿é¹©µç*/
	
	/*rxabb_ldo_ref_pup*/
	ROM_HW_WRITE_REG_BIT(0xacc01a0 ,9,9,1);
	/*rxabb_ldo_load_en*/
	ROM_HW_WRITE_REG_BIT(0xacc01a4 ,4,4,1);
	/*rxabb_ldo_pup*/
	ROM_HW_WRITE_REG_BIT(0xacc01a0 ,10,10,1);
	/*3:Ê¹ÄÜ24MÊ±ÖÓ */
	
	ROM_HW_WRITE_REG_BIT(0x16101024	 ,13,13,0);
	/*4:Æô¶¯RC calibration²¢°Ñ½á¹ûÐ´Èëµ½txabb/rxabbµÄµçÈÝÕóÁÐ¿ØÖÆ×ÖÖÐ*/
	
	/*rccali_pup=rsv_rw0<0>=1*/
	ROM_HW_WRITE_REG_BIT(0xacc02a4,0,0,1);
	/*rccali_rstn= rsv_rw0<10>=1*/
	ROM_HW_WRITE_REG_BIT(0xacc02a4,10,10,1);
	/*rccali_start=rsv_rw0<9>=1*/
	ROM_HW_WRITE_REG_BIT(0xacc02a4,9,9,1);
	/*rccali_start=rsv_rw0<9>=0*/
	ROM_HW_WRITE_REG_BIT(0xacc02a4,9,9,0);
	/*delay 20us*/
	ROM_lmac_Wait(20);
	
	/*Èç¹ûÎª1,¶ÁÈ¡Ð£×¼½á¹ûrccali_result<4:0> ( rsv_ro<11:7>=0acc017c<11:7>)*/
	#if 0
	read_data_rccal_done = ROM_HW_READ_REG_BIT(0xacc02a8,0,0);
	if (read_data_rccal_done){
		RcTuningResults = ROM_HW_READ_REG_BIT(0xacc02a8,5,1);
	//	RcTuningResults =((RcTuningResults&);//get the bit[5,1],rsh 1;
		/*tx_lpf_ccal_offset<4:0>=rccali_result<4:0>*/
		ROM_HW_WRITE_REG_BIT(0xacc00A4,13,9,RcTuningResults);
		/*tx_lpf_ccal_fo=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,9,9,1);
		/*tx_lpf_rc_update=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,5,5,1);
		/*rx_lpf_ccal_offset<4:0>=rccali_result<4:0>*/
		ROM_HW_WRITE_REG_BIT(0xacc00A4,8,4,RcTuningResults);		
		/*rx_lpf_ccal_fo=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,8,8,1);
		/*//rx_lpf_rc_update=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,4,4,1);
	}else
	#endif
	{
		/*tx_lpf_ccal_offset<4:0>=rccali_result<4:0>*/
		ROM_HW_WRITE_REG_BIT(0xacc00A4,13,9,0xf);
		/*tx_lpf_ccal_fo=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,9,9,1);
		/*tx_lpf_rc_update=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,5,5,1);
		/*rx_lpf_ccal_offset<4:0>=rccali_result<4:0>*/
		ROM_HW_WRITE_REG_BIT(0xacc00A4,8,4,0xf);		
		/*rx_lpf_ccal_fo=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,8,8,1);
		/*//rx_lpf_rc_update=1*/
		ROM_HW_WRITE_REG_BIT(0xacc009C,4,4,1);
	}
	
	/*rccalÄ£¿éÍê³ÉÖ®ºó½«RX_PSM¸´Î»£¬¹Ø±Õrccal*/
	
	/*rxabb_ldo_ref_pup*/
	HW_WRITE_REG(0xacc01a0,rxabb_ldo_ref_pup_bak);
	/*rxabb_ldo_load_en*/
	HW_WRITE_REG(0xacc01a4,rxabb_ldo_load_en_bak);
	/*rxabb_ldo_pup*/
	//ROM_HW_WRITE_REG_BIT(0xacc01a0,10,10,0);
	/*24MÊ±ÖÓ */
	ROM_HW_WRITE_REG_BIT(0x16101024,13,13,1);
	/*rccali_pup=rsv_rw0<0>=1*/
	ROM_HW_WRITE_REG_BIT(0xacc0180,0,0,0);
	
	ROM_HW_WRITE_REG_BIT(0xacc0004,1,1,0);
	/*rccali_rstn= rsv_rw0<10>=1 ??? is right */
	//ROM_HW_WRITE_REG_BIT(0xacc0180,10,10,0);
	ROM_lmac_Wait(10);
#endif
}

#if 0
/**************************************************************************
**
** NAME         PHY_RF_PLL_Init_ROMCALL
**
** PARAMETERS:	none
**
** RETURNS:     void
**
** DESCRIPTION
**
**************************************************************************/
PUBLIC void PHY_RF_PLL_Init_ROMCALL(void)
{
	u32 uRegValue;
	PHY_RF_SetReg_Table(Rf_RfPll_Table);
/*#if (CFG_DPLL_CLOCK==24)
	ROM_HW_WRITE_REG_BIT(0xacc00ac,31,0,0x12C22203);
	ROM_HW_WRITE_REG_BIT(0xacc00bc,31,0,0x000600CC);
#elif (CFG_DPLL_CLOCK==26)	
	ROM_HW_WRITE_REG_BIT(0xacc00ac,31,0,0x12C22203);
	ROM_HW_WRITE_REG_BIT(0xacc00bc,31,0,0x000600DC);
#elif (CFG_DPLL_CLOCK==27)	
	ROM_HW_WRITE_REG_BIT(0xacc00ac,31,0,0x12C22203);
	ROM_HW_WRITE_REG_BIT(0xacc00bc,31,0,0x000600DC);
#endif*/
	/*rfpll_ref_pup_fc_18,fast_charge*/
	ROM_HW_WRITE_REG_BIT(0xacc0288,0,0,1);
	ROM_lmac_Wait(5);
	/*rfpll_ref_pup_fc_18*/
	ROM_HW_WRITE_REG_BIT(0xacc0288,0,0,0);
	/*power up LDO*/
	/*rfpll_pup_cpldo*/
	ROM_HW_WRITE_REG_BIT(0xacc00AC,3,3,1); 
	/*rfpll_pup_digldo*/
	ROM_HW_WRITE_REG_BIT(0xacc00AC,4,4,1); 
	/*rfpll_pup_vcoldo*/
	ROM_HW_WRITE_REG_BIT(0xacc00B8,21,21,1); 
	/*rfpll_vco_fc,fast_charge*/
	ROM_HW_WRITE_REG_BIT(0xacc00AC,6,6,1);
	ROM_lmac_Wait(5);
	/*rfpll_vco_fc*/
	ROM_HW_WRITE_REG_BIT(0xacc00AC,6,6,0);	
}
#endif
static void PHY_RF_ConfigRFIP(void)
{
	if(initial_rf == 0){
		return;
	}
	PHY_RF_TX_Config();
	PHY_RF_RX_Config();
	PHY_RF_RxAdc_Init();
	PHY_RF_Abb_RcCal_Init();
	PHY_RF_PA_Reg();
	PHY_RF_Init_Reg();
	PHY_RF_Temp_Reg();

}
/***********************************************************************/
/***                           End Of File                           ***/
/***********************************************************************/

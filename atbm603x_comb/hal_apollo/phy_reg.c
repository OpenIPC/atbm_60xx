#include "phy_reg_data.h"
#include "mac80211/ieee80211_i.h"
#include "wsm.h"

static int initial_phy = 1;
module_param(initial_phy, int, 0644);
static int initial_rf = 1;
module_param(initial_rf, int, 0644);
static int initial_mac = 1;
module_param(initial_mac, int, 0644);



static DEFINE_MUTEX(phy_mutex);

struct wsm_phy_regval_sets{
	wsm_regval_bit *table;
	u32				table_size;
	u32				table_index;
	wsm_phy_init_global_para phy_params;
};

static struct wsm_phy_regval_sets *sets = NULL;
static void *phy_alloc_mem(u32 size)
{
	void *mem = NULL;
	u32 tries = 0;
	
	do{
		mem = atbm_kzalloc(size,GFP_KERNEL);
		
		if(mem)
			break;
		
		tries ++ ;
		
		if(tries > 5)
			break;
		
		msleep(100);
	}while(mem == NULL);

	return mem;
}

static void phy_free_mem(void *mem)
{
	atbm_kfree(mem);
}
static int phy_reg_table_set(struct wsm_phy_regval_sets *psets)
{
	int ret = -1;
	
	
	if(sets == NULL){
		sets = psets;
		ret = 0;
	}
	
	return ret;
}

static int phy_reg_table_free(void)
{
	sets = NULL;
	return 0;
}

static struct wsm_phy_regval_sets *phy_reg_table_get(void)
{
	BUG_ON(sets == NULL);
	return sets;
}

#include "phy_rf_init_8601_c1.c"

static void phy_reg_muxlock(void)
{
	mutex_lock(&phy_mutex);
}

static void phy_reg_muxunlock(void)
{
	mutex_unlock(&phy_mutex);
}

void ROM_HW_WRITE_REG_BIT(u32 addr, u8 end, u8 start, u32 data)
{
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
	
	BUG_ON(phy_sets == NULL);
	BUG_ON(phy_sets->table_index >= phy_sets->table_size);
	
	phy_sets->table[phy_sets->table_index].addr = __cpu_to_le32(addr);
	phy_sets->table[phy_sets->table_index].endBit = __cpu_to_le16(end);
	phy_sets->table[phy_sets->table_index].startBit = __cpu_to_le16(start);
	phy_sets->table[phy_sets->table_index].data = __cpu_to_le32(data);
	phy_sets->table_index++;
	/*
	g_PhyRegInitBuf[g_PhyRegInitIndex].addr = __cpu_to_le32(addr); 
	g_PhyRegInitBuf[g_PhyRegInitIndex].endBit = __cpu_to_le16(end);
	g_PhyRegInitBuf[g_PhyRegInitIndex].startBit = __cpu_to_le16(start);
	g_PhyRegInitBuf[g_PhyRegInitIndex].data = __cpu_to_le32(data);
	g_PhyRegInitIndex++;
	*/
}
void ROM_lmac_Wait(u32 us)
{
	ROM_HW_WRITE_REG_BIT(PHY_WAIT,0,0,us);
}

u16 *PHY_Init_ChooseAgcRegTable(void)
{
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
	
	if (GET_LMAC_ATE_MODE(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
		return (u16 *)PHY_INIT_AGC_REG_TABLE_1;
	} else { 
		return (u16 *)PHY_INIT_AGC_REG_TABLE_3;
	}

}

void PHY_Initial_Agc_Table(void)
{
	u32 reg_addr;
	u32 reg_idx;
	u16 *PHY_INIT_AGC_REG_TABLE = PHY_Init_ChooseAgcRegTable();
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
	
	reg_addr = 0xAC80800;
	for (reg_idx=0; reg_idx<40; reg_idx++)
	{
		PHY_BB_HW_WRITE_REG(reg_addr, PHY_INIT_AGC_REG_TABLE[reg_idx]);
		reg_addr = reg_addr + 0x4;
		PHY_BB_HW_WRITE_REG(reg_addr, PHY_INIT_AGC_REG_TABLE[reg_idx]);
		reg_addr = reg_addr + 0x4;
	}
	if (phy_sets->phy_params.rx_outer_lna_flag == 1)
	{
		reg_addr = 0xAC80800;
		for (reg_idx=0; reg_idx<20; reg_idx++)
		{
			if (phy_sets->phy_params.chipFeatureType == phy_sets->phy_params.richwaveOuterpaType) {
				PHY_BB_HW_WRITE_REG(reg_addr, 0x1A6D);//52dB
			} else { //RICHWAVE_OUTERPA_SW_TYPE
				PHY_BB_HW_WRITE_REG(reg_addr, 0x196E);//50dB
			}
			reg_addr = reg_addr + 0x4;	
		}
	}

}

void PHY_Initial_Memory_Table(u8 set_agc_reg)
{
	u8 reg_idx;
	u32 reg_addr;
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
	// ***********init tx memory table*************
	if(initial_phy == 0){
		return;
	}
#if 0
	ROM_PHY_UTL_WriteTable(PHY_INIT_MEMORY_TABLE);
#else
	PHY_BB_HW_WRITE_REG(0xAC389AC, 0x2);
	reg_addr = 0xACBD000;
	for (reg_idx=0; reg_idx<48; reg_idx++)
	{
		if (GET_LMAC_FPGA(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
			PHY_BB_HW_WRITE_REG(reg_addr,0x4000);
		} else{
			PHY_BB_HW_WRITE_REG(reg_addr,0x5a60e);
		}
		reg_addr = reg_addr + 0x4;
	}
	reg_addr = 0xacbd0c0;
	for (reg_idx=0; reg_idx<48; reg_idx++)
	{
		if (GET_LMAC_FPGA(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
			PHY_BB_HW_WRITE_REG(reg_addr,0x0);
		}else {
			PHY_BB_HW_WRITE_REG(reg_addr,0x9143c);
		}
		reg_addr = reg_addr + 0x4;
	}

	reg_addr = 0xACBD180;
	for (reg_idx=0; reg_idx<48; reg_idx++)
	{		
		PHY_BB_HW_WRITE_REG(reg_addr,0x7623c);
		reg_addr = reg_addr + 0x4;
	}
	PHY_BB_HW_WRITE_REG(0xAC389AC, 0x4);
	PHY_BB_HW_WRITE_REG(0xAC389AC, 0x5);
#endif
	
	// ***********init agc table*************
	if (set_agc_reg)
	{
		PHY_Initial_Agc_Table();
	}
	// *********end agc table*************

}

void PHY_Initial_RegBitTable(void)
{
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
	
	if (GET_LMAC_ATE_MODE(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
		/* params_gain_shift_c2_20M 	0218		3:0 	AC80218 7
		   params_gain_shift_c1_20M 	0218		7:4 	AC80218 8
		   params_phase_shift_c2_20M	0218		11:8	AC80218 7
		   params_phase_shift_c1_20M	0218		15:12	AC80218 8 */
		ROM_HW_WRITE_REG_BIT(0xAC80218, 	3,		0,		0x5 	);
		ROM_HW_WRITE_REG_BIT(0xAC80218, 	7,		4,		0x6 	);
		ROM_HW_WRITE_REG_BIT(0xAC80218, 	11, 	8,		0x5 	);
		ROM_HW_WRITE_REG_BIT(0xAC80218, 	15, 	12, 	0x6 	);
	}
	
	
	ROM_HW_WRITE_REG_BIT(0xac880e0, 	7,		7,		0x1 	); // /params_co_clock_lock

	if (GET_LMAC_MODEM_TX_RX_ETF(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
		ROM_HW_WRITE_REG_BIT(0xAC880a8, 	1,		1,		0x0 	); // /mmse smooth based to htsig smoothing
	} else {
		ROM_HW_WRITE_REG_BIT(0xAC880a8, 	1,		1,		0x1 	); // /ce force mmse smooth;
	}

	// tx iq imbalance and dc offset
//	ROM_HW_WRITE_REG_BIT(0xACB8914, 	9,		0,		0x0 	); // params_txEPSILON_20m
//	ROM_HW_WRITE_REG_BIT(0xACB8914, 	19, 	10, 	0x86	); // params_txPHI_20m,15degree, 15/360*512*2*pi=134
//	ROM_HW_WRITE_REG_BIT(0xACB8918, 	9,		0 , 	0		); // params_txEPSILON_40m
//	ROM_HW_WRITE_REG_BIT(0xACB8918, 	19, 	10, 	0x86	); // params_txPHI_40m
//	ROM_HW_WRITE_REG_BIT(0xACB891C, 	9,		0,		0		); // params_txEPSILON_20ul
//	ROM_HW_WRITE_REG_BIT(0xACB891C, 	19, 	10, 	0x86	); // params_txPHI_20ul
	ROM_HW_WRITE_REG_BIT(0xACB8920, 	23, 	0,		0		); // params_txDIGOFFSETI_20m
 // ROM_HW_WRITE_REG_BIT(0xACB8920, 	23, 	12, 	0		); // params_txDIGOFFSETQ_20m
 // ROM_HW_WRITE_REG_BIT(0xACB8924, 	23, 	0,		0		); // params_txDIGOFFSETI_40m
 // ROM_HW_WRITE_REG_BIT(0xACB8924, 	23, 	12, 	0		); // params_txDIGOFFSETQ_40m
 // ROM_HW_WRITE_REG_BIT(0xACB8928, 	23, 	0,		0		); // params_txDIGOFFSETI_20ul
 // ROM_HW_WRITE_REG_BIT(0xACB8928, 	23, 	12, 	0		); // params_txDIGOFFSETQ_20ul
//	ROM_HW_WRITE_REG_BIT(0xACB892C, 	11, 	0,		0x800	); // params_txNEGCLIP_20m
//	ROM_HW_WRITE_REG_BIT(0xACB892C, 	23, 	12, 	0x800	); // params_txNEGCLIP_40m
//	ROM_HW_WRITE_REG_BIT(0xACB8930, 	11, 	0,		0x800	); // params_txNEGCLIP_20ul
	// phy rx regsitors
	ROM_HW_WRITE_REG_BIT(0xac80180, 	5,		0,		0x16	); // params_num_frame_det_low, default is 18;
	ROM_HW_WRITE_REG_BIT(0xAC80434, 	18, 	11, 	0xaf	); // min_level_dBm

	ROM_HW_WRITE_REG_BIT(0xac80438, 	13, 	7,		0x1 	); // params_p_idx_high_gain
	ROM_HW_WRITE_REG_BIT(0xac80440, 	4,		0,		0x10	); // agc west
	ROM_HW_WRITE_REG_BIT(0xac80440, 	10, 	5,		0x18	); // 20M loop delay, 16 -> 18
	ROM_HW_WRITE_REG_BIT(0xac8044c, 	7,		0,		0x38	); // agc_pwr_hist
	ROM_HW_WRITE_REG_BIT(0xac80484, 	6,		0,		0x3f	); // params_agc_noise_update_max_gain
	ROM_HW_WRITE_REG_BIT(0xacb8ac8, 	14, 	8,		0x1a	); // digital_dc_calibration_minus_gain
	ROM_HW_WRITE_REG_BIT(0xac8048c, 	10, 	5,		0x1a	); // sat_loop_delay_20M
	ROM_HW_WRITE_REG_BIT(0xac8048c, 	16, 	11, 	0x1d	); // bit[16:11] is sat_loop_delay_40M;
	ROM_HW_WRITE_REG_BIT(0xac80cb0, 	16, 	0,		0x7e40	); // params_symbol_end_period[16:0]
	ROM_HW_WRITE_REG_BIT(0xac80cb0, 	25, 	17, 	0x10	); // params_symbol_end_period[25:17]
	ROM_HW_WRITE_REG_BIT(0xac80d24, 	1,		1,		0x1 	); // params_cca_interrupt_rst_state_en
	ROM_HW_WRITE_REG_BIT(0xac80d08, 	24, 	24, 	0x0 	); // cca watch dog enable
	ROM_HW_WRITE_REG_BIT(0xac80c90, 	9,		0,		0x10	); // delta_acc_noise_power
	ROM_HW_WRITE_REG_BIT(0xAC80444, 	31, 	0,		0x87f	); // params_agc_noise_update_min_gain[7:1];bit[0] is agc_delay_on
	ROM_HW_WRITE_REG_BIT(0xac80190, 	19, 	10, 	0x280	); // snd_signal_num_frame_det_low
	ROM_HW_WRITE_REG_BIT(0xac80294, 	17, 	8,		0x20	); // min_correlation_thr_40M
	ROM_HW_WRITE_REG_BIT(0xac80490, 	15, 	0,		0xf0f6	); // bit[7:0] is only_snd_have_signal_thr1;bit[15:8] is only_snd_have_signal_thr2
	ROM_HW_WRITE_REG_BIT(0xac80450, 	31, 	0,		0xe0a	); // params_delta_pwr_dB_thr[11��4]
	ROM_HW_WRITE_REG_BIT(0xac80498, 	9,		0,		0x2c3	); // bit[0] pri_and_snd_remove_dc_flag;bit[1] use_fit_gain_delta_pwr, bit[9:2]fit_gain_delta_pwr_dB_thr, b0 is -20dB
	ROM_HW_WRITE_REG_BIT(0xac80450, 	11, 	4,		0xb0	); // bit[11:4] saturated gain_delta_pwr_dB_thr, b0 is -20dB
	ROM_HW_WRITE_REG_BIT(0xac8043c, 	21, 	16, 	0x18	); // bit[21:16] loop_delay_40M;
//	ROM_HW_WRITE_REG_BIT(0xac8043c, 	6,		0,		0x3e	); // params_g_init_high
	ROM_HW_WRITE_REG_BIT(0xac80dac, 	8,		8,		0x1 	); // enable dsss power up logic
 // ROM_HW_WRITE_REG_BIT(0xac80d5c, 	5,		0,		0x1f	); // digital_snd_defer_thr_before_sync_low2�� 33dB , U(6, 0)
 // ROM_HW_WRITE_REG_BIT(0xac80d5c, 	11, 	6,		0x1c	); // digital_snd_defer_thr_before_sync_low1�� 30dB , U(6, 0)
 // ROM_HW_WRITE_REG_BIT(0xac80d58, 	5,		0,		0x1f	); // digital_snd_defer_thr_after_sync_low2�� 33dB , U(6, 0)
 // ROM_HW_WRITE_REG_BIT(0xac80d58, 	11, 	6,		0x1c	); // digital_snd_defer_thr_after_sync_low1
 // ROM_HW_WRITE_REG_BIT(0xac80d54, 	5,		0,		0x1f	); // digital_snd_defer_thr_before_sync_high2
 // ROM_HW_WRITE_REG_BIT(0xac80d54, 	11, 	6,		0x1c	); // digital_snd_defer_thr_before_sync_high1
 // ROM_HW_WRITE_REG_BIT(0xac80d50, 	5,		0,		0x1f	); // digital_snd_defer_thr_after_sync_high2
 // ROM_HW_WRITE_REG_BIT(0xac80d50, 	11, 	6,		0x1c	); // digital_snd_defer_thr_after_sync_high1
	ROM_HW_WRITE_REG_BIT(0xac80ca0, 	6,		0,		0x10	); // defer_thr_dB, U(7,0),-1*16 = -16dBm
 // ROM_HW_WRITE_REG_BIT(0xac80d44, 	24, 	19, 	0x1f	); // digital_defer_thr_dB, U(6,0)
 // ROM_HW_WRITE_REG_BIT(0xac80da4, 	10, 	3,		0x40	); // frame_detected digital thr dB, 4*4 = 16dB
 // ROM_HW_WRITE_REG_BIT(0xac80d48, 	8,		1,		0x40	); // frame_valid digital thr dB, 4*4 = 16dB
 // ROM_HW_WRITE_REG_BIT(0xac80d7c, 	18, 	13, 	0x24	); // digital_stop_update_noise_thr_dB, 24 means digital power dB is 36dB;
	ROM_HW_WRITE_REG_BIT(0xac80c94, 	19, 	13, 	0x10	); // stop_update_noise_thr_dB, 44 means antennal power dBm is -16dBm;
 // ROM_HW_WRITE_REG_BIT(0xac80c94, 	25, 	20, 	0x10	); // stop_update_noise_delta_thr_dB, 16dB
	ROM_HW_WRITE_REG_BIT(0xac90194, 	1,		0,		0x1 	); // bit[0]: 0: legacy 40M llr need combine; bit[1]: 0: mcs32 llr need combine;

	if (GET_LMAC_NORMAL_SIFS(__le32_to_cpu(phy_sets->phy_params.compileMacro))) { //0xf8->16us
		ROM_HW_WRITE_REG_BIT(0xac80dc8, 	8,		0,		0xf8	); // rx_phy_ready pulling down timer for stbc 40M short GI eco
	} else {
		ROM_HW_WRITE_REG_BIT(0xac80dc8, 	8,		0,		0x119	); // rx_phy_ready pulling down timer for stbc 40M short GI eco
	}

	ROM_HW_WRITE_REG_BIT(0xac80cb4, 	8,		0,		0x118	); // valid_ppdu pulling down timer for stbc 40M short GI eco
	ROM_HW_WRITE_REG_BIT(0xac80de0, 	1,		0,		0x0 	); // bit0 is legacy_carrier_lost_return_idle_flag; bit1 is carrier_lost_return_idle_flag
	ROM_HW_WRITE_REG_BIT(0xacc0140, 	27, 	25, 	0x4 	); // bit[25]:1->80M to 80M,0->else;bit[27:26]:0->320M to 80M;1->160M to 80M;2->120M to 80M
	ROM_HW_WRITE_REG_BIT(0xACB8008, 	10, 	0,		0x044	); // TxAnaOnDelay
	ROM_HW_WRITE_REG_BIT(0xACB8008, 	26, 	16, 	0x01f	); // TxAnaEndDelay
	ROM_HW_WRITE_REG_BIT(0xACB8004, 	0,		0,		0x0 	); // bandwidth sel.0:2.4G,1:5G
	ROM_HW_WRITE_REG_BIT(0xACB838C, 	2,		0,		0x4 	); // bit[2]:force enable,bit[1:0]: pta priority force value
	ROM_HW_WRITE_REG_BIT(0xAC80CAC, 	1,		1,		0x0 	); // noise_update_fast_enable
	ROM_HW_WRITE_REG_BIT(0xac8043c, 	15, 	15, 	0x0 	); // noise_update_gain enable
	ROM_HW_WRITE_REG_BIT(0xac80ca0, 	26, 	17, 	0x3ff	); // snd_defer_thr_dB_high; S(10, 2)
	ROM_HW_WRITE_REG_BIT(0xac80ca4, 	9,		0,		0x3ff	); // snd_defer_thr_dB_low; S(10, 2)
	// cca_vht_watch_dog			   //
	ROM_HW_WRITE_REG_BIT(0xac80d94, 	5,		0,		0x2 	); // cca watch dog, set 3f is enable; set 2 is wait_end_sig state watch dog enable;
	ROM_HW_WRITE_REG_BIT(0xac80da4, 	0,		0,		0x1 	); // params_cca_interrupt_rst_valid_ppdu_en
	//ROM_HW_WRITE_REG_BIT(0xac80d44, 	18, 	13, 	0x10	); // params_defer_delta_thr_dB
	ROM_HW_WRITE_REG_BIT(0xAC80D48, 	0,		0,		0x0 	); // params_snd_channel_busy_mode
 // ROM_HW_WRITE_REG_BIT(0xAC80D48, 	18, 	17, 	0x2 	); // params_channel_busy_mode
	ROM_HW_WRITE_REG_BIT(0xAC80D4C, 	13, 	0,		0x814	); // bit[6:0] is params_snd_delta_thr_dB_high,20dB;bit[13:7] is params_snd_delta_thr_dB_low, 16dB
 // ROM_HW_WRITE_REG_BIT(0xac80190, 	9,		0,		0x260	); // params_amp_det_threshold_low
	ROM_HW_WRITE_REG_BIT(0xac8017c, 	23, 	20, 	0x6 	); // params_num_frame_det_high
 // ROM_HW_WRITE_REG_BIT(0xac801cc, 	11, 	0,		0x6f	); // noise_power_threshold
 // ROM_HW_WRITE_REG_BIT(0xac80c98, 	9,		0,		0x6f	); // ofdm_noise_thr
 // ROM_HW_WRITE_REG_BIT(0xac80c98, 	19, 	10, 	0x6f	); // dsss_noise_thr	dsss noise threshold
	ROM_HW_WRITE_REG_BIT(0xac9011c, 	1,		1,		0x0 	); // exit_on_lsig_error
	ROM_HW_WRITE_REG_BIT(0xacd0054, 	0,		0,		0x0 	); // 1: use_new_dsss_cca_flag
	ROM_HW_WRITE_REG_BIT(0xac80dbc, 	0,		0,		0x0 	); // bit0: params_legacy_rx_data_power_up_return_idle_flag
									//
 // ROM_HW_WRITE_REG_BIT(0xac801cc, 	19, 	12, 	0x1f	); // bit[19:12] params_noise_block_len
	ROM_HW_WRITE_REG_BIT(0xac801cc, 	21, 	12, 	0x11f	); // bit[21:20] is pre_frame_noise_use
	ROM_HW_WRITE_REG_BIT(0xac80c8c, 	9,		0,		0xb4	); // bit[9:0]:params_time_slot_period
	ROM_HW_WRITE_REG_BIT(0xac80e1c, 	7,		7,		0x0 	); // params_dsss_sync_use_ofdm_sync_memory_flag
	ROM_HW_WRITE_REG_BIT(0xacb8900, 	3,		3,		0x1 	); // bypass 40M dc for rifs case;
	ROM_HW_WRITE_REG_BIT(0xacb8900, 	0,		0,		0x1 	); // params_precomp_enable
 // ROM_HW_WRITE_REG_BIT(0xacb89a4, 	9,		0,		(0x4 | (2<<10)) ); // params_precomp_enable
 // ROM_HW_WRITE_REG_BIT(0xacb89a4, 	19, 	10, 	0x2 	); // params_precomp_coeffs1 
	ROM_HW_WRITE_REG_BIT(0xacb89a4, 	19, 	0,		(0x4 | (2<<10)) );
	ROM_HW_WRITE_REG_BIT(0xacb89a4, 	29, 	20, 	 0x3f6	); // params_precomp_coeffs2
	ROM_HW_WRITE_REG_BIT(0xacb89a8, 	9,		0,		0x3e7	); // params_precomp_coeffs3
	ROM_HW_WRITE_REG_BIT(0xacb89a8, 	19, 	10, 	0x13e	); // params_precomp_coeffs4
	ROM_HW_WRITE_REG_BIT(0xac88220, 	4,		2,		0x2 	); // rnn_scale    
	ROM_HW_WRITE_REG_BIT(0xac80e20, 	0,		0,		0x0 	); // rifs on;

	if (GET_LMAC_TX_CFO_PPM_CORRECTION(__le32_to_cpu(phy_sets->phy_params.compileMacro)) || 
		GET_LMAC_CFO_DCXO_CORRECTION(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
		ROM_HW_WRITE_REG_BIT(0xac88220, 	1,		1,		0x1 	); // when ofdm, 1 -> cfo out of dpll, 0 -> cfo out of sync
		ROM_HW_WRITE_REG_BIT(0xAca806c, 	14, 	7,		0x30	);
	}

	// based channel busy/sync fail decide agc gain and sync reg;
	ROM_HW_WRITE_REG_BIT(0xac80e44, 	5,		0,		0x1a	); // bit3:use_sat_flag; bit2: snd_channel_busy_mode_1; bit[1:0]:channel_busy_mode; 
		// bit4:params_reset_cca_decide_agc_gain; bit5:params_enable_cca_decide_agc_gain;
	 // ROM_HW_WRITE_REG_BIT(0x0ac80e50,	31, 	0,		0x430c2620 );		// dsss reg
		/*	params_digital_snd_defer_threshold_after_sync_low2_1		0E3C		5:0 	AC80E3C 14
			params_digital_snd_defer_threshold_after_sync_low1_1		0E3C		11:6	AC80E3C f
			params_digital_snd_defer_threshold_before_sync_low2_1		0E3C		17:12	AC80E3C 14
			params_digital_snd_defer_threshold_before_sync_low1_1		0E3C		23:18	AC80E3C f
			params_digital_defer_thr_dB_1								0E3C		29:24	AC80E3C 14
			params_digital_snd_defer_threshold_after_sync_low2_2		0E40		5:0 	AC80E40 c
			params_digital_snd_defer_threshold_after_sync_low1_2		0E40		11:6	AC80E40 7
			params_digital_snd_defer_threshold_before_sync_low2_2		0E40		17:12	AC80E40 c
			params_digital_snd_defer_threshold_before_sync_low1_2		0E40		23:18	AC80E40 7
			params_digital_defer_thr_dB_2								0E40		29:24	AC80E40 c
		*/
	 /* ROM_HW_WRITE_REG_BIT(0xac80e3c, 	5,		0,		0x1f	); // digital_snd_defer_thr_before_sync_low2_1�� 33dB , U(6, 0)
		ROM_HW_WRITE_REG_BIT(0xac80e3c, 	11, 	6,		0x1c	); // digital_snd_defer_thr_before_sync_low1_1�� 30dB , U(6, 0)
		ROM_HW_WRITE_REG_BIT(0xac80e3c, 	17, 	12, 	0x1f	); // digital_snd_defer_thr_after_sync_low2_1�� 33dB , U(6, 0)
		ROM_HW_WRITE_REG_BIT(0xac80e3c, 	23, 	18, 	0x1c	); // digital_snd_defer_thr_after_sync_low1_1
		ROM_HW_WRITE_REG_BIT(0xac80e3c, 	29, 	24, 	0x1f	); // params_digital_defer_thr_dB_1

		ROM_HW_WRITE_REG_BIT(0xac80e40, 	5,		0,		0x18	); // digital_snd_defer_thr_before_sync_low2_2�� 33dB , U(6, 0) 
		ROM_HW_WRITE_REG_BIT(0xac80e40, 	11, 	6,		0x15	); // digital_snd_defer_thr_before_sync_low1_2�� 30dB , U(6, 0) 
		ROM_HW_WRITE_REG_BIT(0xac80e40, 	17, 	12, 	0x18	); // digital_snd_defer_thr_after_sync_low2_2�� 33dB , U(6, 0)
		ROM_HW_WRITE_REG_BIT(0xac80e40, 	23, 	18, 	0x15	); // digital_snd_defer_thr_after_sync_low1_2
		ROM_HW_WRITE_REG_BIT(0xac80e40, 	29, 	24, 	0x18	); // params_digital_defer_thr_dB_1
		*/

	/*
#if ADAPTIVE_TEST
		ROM_HW_WRITE_REG_BIT(0xac80d44, 	24, 	19, 	0x1a	); // digital_delta_thr_dB 26-45.5-60 = 
		ROM_HW_WRITE_REG_BIT(0xAC80D48, 	18, 	17, 	0x0 	); // params_channel_busy_mode
		ROM_HW_WRITE_REG_BIT(0xac80d48, 	8,		1,		0x10	); // digital_validate_sync_channel_busy_thr_dB
		ROM_HW_WRITE_REG_BIT(0xac80da4, 	10, 	3,		0x10	); // digital_frame_detected_channel_busy_thr_dB
		ROM_HW_WRITE_REG_BIT(0xac80dc4, 	7,		0,		0x10	); // digital_saturated_channel_busy_thr_dB
		ROM_HW_WRITE_REG_BIT(0xac80d50, 	5,		0,		0x20	); // digital_snd_defer_threshold_after_sync_high2
		ROM_HW_WRITE_REG_BIT(0xac80d50, 	11, 	6,		0x20	); // digital_snd_defer_threshold_after_sync_high1
		ROM_HW_WRITE_REG_BIT(0xac80d54, 	5,		0,		0x20	); // digital_snd_defer_threshold_before_sync_high2
		ROM_HW_WRITE_REG_BIT(0xac80d54, 	11, 	6,		0x20	); // digital_snd_defer_threshold_before_sync_high1
		ROM_HW_WRITE_REG_BIT(0xac80d5c, 	5,		0,		0x20	); // params_digital_snd_defer_threshold_before_sync_low2
		ROM_HW_WRITE_REG_BIT(0xac80d5c, 	11, 	6,		0x1e	); // params_digital_snd_defer_threshold_before_sync_low1
		ROM_HW_WRITE_REG_BIT(0xac80d58, 	5,		0,		0x20	); // digital_snd_defer_threshold_before_sync_low2
		ROM_HW_WRITE_REG_BIT(0xac80d58, 	11, 	6,		0x1e	); // digital_snd_defer_threshold_before_sync_low1
		ROM_HW_WRITE_REG_BIT(0xac80c94, 	25, 	20, 	0x3f	); // params_stop_update_noise_delta_thr_dB
		ROM_HW_WRITE_REG_BIT(0xac80d7c, 	18, 	13, 	0x3f	); // digital_stop_update_noise_thr_dB, 36dB

	#elif USE_OUTER_PA
		ROM_HW_WRITE_REG_BIT(0xac80d44, 	24, 	19, 	0x23	); // 0x1e);
		ROM_HW_WRITE_REG_BIT(0xAC80D48, 	18, 	17, 	0x2 	); // 0x0);
		ROM_HW_WRITE_REG_BIT(0xac80d48, 	8,		1,		0x60	); 
		ROM_HW_WRITE_REG_BIT(0xac80da4, 	10, 	3,		0x60	);
		ROM_HW_WRITE_REG_BIT(0xac80dc4, 	7,		0,		0x60	);
		ROM_HW_WRITE_REG_BIT(0xac80d50, 	5,		0,		0x28	);
		ROM_HW_WRITE_REG_BIT(0xac80d50, 	11, 	6,		0x25	);
		ROM_HW_WRITE_REG_BIT(0xac80d54, 	5,		0,		0x28	);
		ROM_HW_WRITE_REG_BIT(0xac80d54, 	11, 	6,		0x25	);
		ROM_HW_WRITE_REG_BIT(0xac80d5c, 	5,		0,		0x23	);
		ROM_HW_WRITE_REG_BIT(0xac80d5c, 	11, 	6,		0x20	);
		ROM_HW_WRITE_REG_BIT(0xac80d58, 	5,		0,		0x23	);
		ROM_HW_WRITE_REG_BIT(0xac80d58, 	11, 	6,		0x20	);
		ROM_HW_WRITE_REG_BIT(0xac80c94, 	25, 	20, 	0x1a	);

	#else
		ROM_HW_WRITE_REG_BIT(0xac80d44, 	24, 	19, 	0x21	); // 0x1e);
		ROM_HW_WRITE_REG_BIT(0xAC80D48, 	18, 	17, 	0x2 	); // 0x0);
		ROM_HW_WRITE_REG_BIT(0xac80d48, 	8,		1,		0x50	);
		ROM_HW_WRITE_REG_BIT(0xac80da4, 	10, 	3,		0x50	);
		ROM_HW_WRITE_REG_BIT(0xac80dc4, 	7,		0,		0x50	);
		ROM_HW_WRITE_REG_BIT(0xac80d50, 	5,		0,		0x26	);
		ROM_HW_WRITE_REG_BIT(0xac80d50, 	11, 	6,		0x23	);
		ROM_HW_WRITE_REG_BIT(0xac80d54, 	5,		0,		0x26	);
		ROM_HW_WRITE_REG_BIT(0xac80d54, 	11, 	6,		0x23	);
		ROM_HW_WRITE_REG_BIT(0xac80d5c, 	5,		0,		0x21	);
		ROM_HW_WRITE_REG_BIT(0xac80d5c, 	11, 	6,		0x1e	);
		ROM_HW_WRITE_REG_BIT(0xac80d58, 	5,		0,		0x21	);
		ROM_HW_WRITE_REG_BIT(0xac80d58, 	11, 	6,		0x1e	);
		ROM_HW_WRITE_REG_BIT(0xac80c94, 	25, 	20, 	0x16	); 
#endif
	*/

	// the follow registors all bits set 1, this means close clock gated 

	 /* ROM_HW_WRITE_REG_BIT(0xaca0108, 	4,		0,		0x1f	); // params_tsg_clk
		ROM_HW_WRITE_REG_BIT(0xac8814c, 	6,		0,		0x7f	); // params_ce_clk
		ROM_HW_WRITE_REG_BIT(0xac9017c, 	5,		0,		0x3f	); // params_rdc_clk
		ROM_HW_WRITE_REG_BIT(0xacb89b0, 	5,		0,		0x3f	); // params_bb_clk
		ROM_HW_WRITE_REG_BIT(0xac8023c, 	13, 	0,		0x3fff	); // params_sync_clk
		ROM_HW_WRITE_REG_BIT(0xac88144, 	2,		0,		0x7 	); // params_rdf_clk_enable
		ROM_HW_WRITE_REG_BIT(0xacd005c, 	12, 	0,		0x1fff	); //
	// close all soc clock gated
		ROM_HW_WRITE_REG_BIT(0x16100038,	31, 	0,		0xffffffff);
		ROM_HW_WRITE_REG_BIT(0x1610003c,	31, 	0,		0xffffffff);
		ROM_HW_WRITE_REG_BIT(0x16100040,	31, 	0,		0xffffffff);
		ROM_HW_WRITE_REG_BIT(0x16100044,	31, 	0,		0xffffffff); */

	// the follow registors means open clock gated 
	 /* ROM_HW_WRITE_REG_BIT(0xaca0108, 	4,		0,		0x0 	); // params_tsg_clk
		ROM_HW_WRITE_REG_BIT(0xac8814c, 	6,		0,		0x0 	); // params_ce_clk
		ROM_HW_WRITE_REG_BIT(0xac9017c, 	5,		0,		0x20	); // params_rdc_clk
		ROM_HW_WRITE_REG_BIT(0xacb89b0, 	5,		0,		0x0 	); // params_bb_clk
		ROM_HW_WRITE_REG_BIT(0xac8023c, 	13, 	0,		0x0 	); // params_sync_clk
		ROM_HW_WRITE_REG_BIT(0xac88144, 	2,		0,		0x0 	); // params_rdf_clk_enable
		ROM_HW_WRITE_REG_BIT(0xacd005c, 	12, 	0,		0x1800	); //
		ROM_HW_WRITE_REG_BIT(0x16100038,	31, 	0,		0x0 	);
		ROM_HW_WRITE_REG_BIT(0x1610003c,	31, 	0,		0x0 	);
		ROM_HW_WRITE_REG_BIT(0x16100040,	31, 	0,		0x0 	);
		ROM_HW_WRITE_REG_BIT(0x16100044,	31, 	0,		0x154	); */


		/* 补偿Rx 40M带内的不平坦特性，补偿之后40M性能会变好 */
		ROM_HW_WRITE_REG_BIT(0xacb8900, 	0,		0,		0x1 	);		
		ROM_HW_WRITE_REG_BIT(0xacb8900, 	11, 	8,		0x4 	); // ofdm_sync_initial_estimate_dc_window
		ROM_HW_WRITE_REG_BIT(0xacb8a70, 	12, 	12, 	0x0 	); // ofdm_sync_use_dc
		ROM_HW_WRITE_REG_BIT(0xacb89a4, 	9,		0,		0x4 	);
		ROM_HW_WRITE_REG_BIT(0xacb89a4, 	19, 	10, 	0x2 	);
		ROM_HW_WRITE_REG_BIT(0xacb89a4, 	29, 	20, 	0x3f6	);
		ROM_HW_WRITE_REG_BIT(0xacb89a8, 	9,		0,		0x3e7	);
		ROM_HW_WRITE_REG_BIT(0xacb89a8, 	19, 	10, 	0x13e	);
		ROM_HW_WRITE_REG_BIT(0xac800cc, 	13, 	8,		0x20	);
		ROM_HW_WRITE_REG_BIT(0xac800d0, 	5,		0,		0x10	);
	 // ROM_HW_WRITE_REG_BIT(0xac801cc, 	7,		0,		0x6f	); // noise_power_threshold, 6dB
	 // ROM_HW_WRITE_REG_BIT(0xac80c98, 	9,		0,		0x6f	); // noise_power_threshold, 6dB

#ifdef SDIO_BUS
	ROM_HW_WRITE_REG_BIT(0xac80190,9,0,0x260);
#else
	ROM_HW_WRITE_REG_BIT(0xac80190,9,0,0x220);
#endif
	ROM_HW_WRITE_REG_BIT(0xac80c98,19,10,0x1f);
	ROM_HW_WRITE_REG_BIT(0xac80c98,9,0,0x2f);
	ROM_HW_WRITE_REG_BIT(0xac801cc,11,0,0x2f);

	if (GET_LMAC_ATE_MODE(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
		ROM_HW_WRITE_REG_BIT(0xac80c98,9,0,0x1ff);
		ROM_HW_WRITE_REG_BIT(0xac80c88,1,1,0x0);
		ROM_HW_WRITE_REG_BIT(0xac801cc,17,16,0x0);
		ROM_HW_WRITE_REG_BIT(0xac80438,13,7,0x40);
	}

}

void ROM_PHY_UTL_WriteTable(const wsm_regval *pTable)
{
	u32 iRegAddr;
	u32 iRegVal;
	u32 const *p;
	
	p = (u32*)pTable;

	while (*p != PHY_EOT)
	{
		iRegAddr = *p++;
		iRegVal  = *p++; 
		PHY_BB_HW_WRITE_REG(iRegAddr, iRegVal);
	}

	return;	
}

void ROM_PHY_UTL_WriteReg_Bit_u16(const wsm_regval_bit* pTable)
{
	//uint32	uiRegValue=0;
	//uint32  regmask=0;
	wsm_regval_bit *pReg;
	
	
	pReg = (wsm_regval_bit*)pTable;

	while (pReg->addr != PHY_EOT)
	{
		/*
		uiRegValue=PHY_BB_HW_READ_REG(pReg->addr);
		regmask = ~((1<<pReg->startBit) -1);
		regmask &= ((1<<pReg->endBit) -1)|(1<<pReg->endBit);
		uiRegValue &= ~regmask;
		uiRegValue |= (pReg->data <<pReg->startBit)&regmask;
		PHY_BB_HW_WRITE_REG(pReg->addr,uiRegValue);
		*/
		ROM_HW_WRITE_REG_BIT(pReg->addr,pReg->endBit,pReg->startBit,pReg->data);
		pReg++;
	}
}

void PHY_11b_shape_filter(u8 modem_ramp_flag)
{
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
	if (modem_ramp_flag == 1)
	{// ramp coefs

		HW_WRITE_REG(0xacb0068, 0xd800); //BTDRAMPBK01:bit[19:10], bit[9:0]		
		HW_WRITE_REG(0xacb006c, 0x1b06c); //BTDRAMPBK23
		HW_WRITE_REG(0xacb0070, 0x1b06c); //BTDRAMPBK45
		HW_WRITE_REG(0xacb0074, 0x1b06c); //BTDRAMPBK67
		HW_WRITE_REG(0xacb0078, 0x480035); 
	}
	else
	{
		if (GET_LMAC_USE_NORMAL_RAMP_AND_DOWN(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
			HW_WRITE_REG(0xacb0068, 0xd800); //BTDRAMPBK01:bit[19:10], bit[9:0]
			HW_WRITE_REG(0xacb006c, 0x1b06c); //BTDRAMPBK23
			HW_WRITE_REG(0xacb0070, 0x1b06c); //BTDRAMPBK45
			HW_WRITE_REG(0xacb0074, 0x1b06c); //BTDRAMPBK67
			HW_WRITE_REG(0xacb0078, 0x480035); 
		} else {
			HW_WRITE_REG(0xacb0068, 0x1801); 
			HW_WRITE_REG(0xacb006c, 0x8010); 
			HW_WRITE_REG(0xacb0070, 0x14038); 
			HW_WRITE_REG(0xacb0074, 0x1b060); 
			HW_WRITE_REG(0xacb0078, 0x5800ff); // power on:0x58; power off: 0x ff;
		}
	
	}
	if (0)//(modem_shape_filter == 1)
	{
		// 11b tx shape filter	 ,default value 
		HW_WRITE_REG(0xACB0010, 0x3FE); 
		HW_WRITE_REG(0xACB0014, 0x3F7); 
		HW_WRITE_REG(0xACB0018, 0x3EC); 
		HW_WRITE_REG(0xACB001C, 0x3E8); 
		HW_WRITE_REG(0xACB0020, 0x002); 
		HW_WRITE_REG(0xACB0024, 0x04D); 
		HW_WRITE_REG(0xACB0028, 0x0BD); 
		HW_WRITE_REG(0xACB002C, 0x126); 
		HW_WRITE_REG(0xACB0030, 0x151); 
	}
	
	/*
	else
	{
		HW_WRITE_REG(0xACB0010, 0x3FE );
		HW_WRITE_REG(0xACB0014, 0x003 );
		HW_WRITE_REG(0xACB0018, 0x00f );
		HW_WRITE_REG(0xACB001C, 0x003 );
		HW_WRITE_REG(0xACB0020, 0x3d3 );
		HW_WRITE_REG(0xACB0024, 0x3cd );
		HW_WRITE_REG(0xACB0028, 0x51 );
		HW_WRITE_REG(0xACB002C, 0x12e );
		HW_WRITE_REG(0xACB0030, 0x19f );
	}*/
	
}

void PHY_Initial_Common_Reg(void)
{
	//u32 mac_version = 0;
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();

	if(initial_phy == 0){
		return;
	}
	
	ROM_PHY_UTL_WriteTable(PHY_INIT_COMMON_REG_TABLE);
	// ROM_PHY_UTL_WriteReg_Bit_u16(PHY_INIT_REG_BIT_TABLE);
	PHY_Initial_RegBitTable();

	if (GET_LMAC_MOD_PHY_ETF_SUPPORT(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
		PHY_11b_shape_filter(0);
	} else {
		PHY_11b_shape_filter(1);
	}
	
	{
		HW_WRITE_REG(0xac80458,0x101020); //  //byte[2:3]:ofdm_margin_40M1; byte[4:5]:dsss_margin_40M1; byte[0:1]:dsss_margin_20M
		HW_WRITE_REG(0xac80454,0x10200c); //8/4=2;
		HW_WRITE_REG(0xac8045c,0x1010);

		ROM_HW_WRITE_REG_BIT(0xAC80434,10,3,0xa5); //(165-256)/2=-91/2=-45.5
		ROM_HW_WRITE_REG_BIT(0xAC8043c,13,7,0x3d);
		HW_WRITE_REG(0xac80c88,0x33801FF);
	}

		/* PHY_Adaptive_Set_Config(); */ // Init in lmac.

	// ETF mode
	if (GET_LMAC_MOD_PHY_ETF_SUPPORT(__le32_to_cpu(phy_sets->phy_params.compileMacro)) == 1) {
		// the follow reg modify to support rifs case;
		//ROM_HW_WRITE_REG_BIT( 0xacd0060, 1, 0,  2); // phy auto test use mac crc;
		ROM_HW_WRITE_REG_BIT( 0xac80e20, 0, 0,	0); // rifs_open flag;
		ROM_HW_WRITE_REG_BIT( 0xac80c88, 26, 24,	0); // default is 3;
		ROM_HW_WRITE_REG_BIT( 0xac80c8c, 9, 0,	0x14);//params_time_slot_period,default is b4;
		ROM_HW_WRITE_REG_BIT( 0xac80d04, 7, 0,	0); //	params_rx_end_drop_thr_dB, default is fb;
		ROM_HW_WRITE_REG_BIT( 0xac90194, 3, 0,	0); //bit[0]: 0: legacy 40M llr need combine; bit[1]: 0: mcs32 llr need combine;
	} else {
		ROM_HW_WRITE_REG_BIT( 0xac90194, 3, 0,	0); //bit[0]: 0: legacy 40M llr need combine; bit[1]: 0: mcs32 llr need combine;
	}
	
	if (GET_LMAC_FPGA(__le32_to_cpu(phy_sets->phy_params.compileMacro)) == 1) {
		//ROM_HW_WRITE_REG_BIT(0xACB8914	,19,10, 	 0x0	  ); // tx phi 20m
		//ROM_HW_WRITE_REG_BIT(0xACB8918	,19,10, 	 0x0	  ); // tx phi 40m
		//ROM_HW_WRITE_REG_BIT(0xacb891c	,19,10, 	 0x0	  ); // // tx phi 20UL
		//HW_WRITE_REG(0xac80234,  0x0   );	//params_imb_gain[9:0], params_imb_phase[19:10]
		ROM_HW_WRITE_REG_BIT(0xacb8900	,2,0,	 0x0   ); // bit0 is precomp_enable;bit1 is bypass_freq_shift_40m; bit2 is bypass_freq_shift_80m
	

		ROM_HW_WRITE_REG_BIT(0xac8048c	,10,5,	 0x1e	   ); //sat_loop_delay_20M
		ROM_HW_WRITE_REG_BIT(0xac8048c	,16,11,  0x21	   ); //bit[16:11] is sat_loop_delay_40M;
		ROM_HW_WRITE_REG_BIT(0xac80440	,10,5,	 0x1a	   ); //20M loop delay,
		ROM_HW_WRITE_REG_BIT(0xac8043c	,21,16,  0x21	   ); //40M loop delay,
		HW_WRITE_REG(0xac80454,  0xc200c   ); //bit[16:23]: ofdm_margin_20M
		ROM_HW_WRITE_REG_BIT(0xac8019c	,6, 0,	0x5a   ); //params_det_saturate_offset

		ROM_HW_WRITE_REG_BIT(0xac8044c	,7,0,		 0x5a	   ); //agc_pwr_hist, CHIP IS 30  
		ROM_HW_WRITE_REG_BIT(0xac801cc	,11,0,		 0x3f); //sync_noise_thr
	}
}

void PHY_Initial_FEM_Reg(void)
{
	if(initial_phy == 0){
		return;
	}
	ROM_PHY_UTL_WriteReg_Bit_u16(PHY_INIT_FEM_TABLE);
}

void ROM_PHY_RF_TX_WriteToGainTbl(u8 u8PowerIdx, s16 DigGainValue_dB_multile10,u8 bw_40m_flag)
{

// Input: DigGainValue_dB_multile10: 10 means 1dB; 100 means 10dB;
// u8PowerIdx: 0~1: 11b rate; 2~8: 11g rate; 9: 65M 11n rate;

		
	u32 AddrOffset;
	s16 s16DigGainVal;
	u16 uiRegValue;

	s16DigGainVal = (DigGainValue_dB_multile10*4 + 5)/10 + DigGain_Table_0dB_Index;
	s16DigGainVal = max(min((s16)DigGain_Table_Max_Index, s16DigGainVal), 0);	
	uiRegValue = (ROM_ro_txdigital_gain_magic_table[s16DigGainVal]<<1);
		
	AddrOffset = (u8PowerIdx << 2);
	
	HW_WRITE_REG(0xAC389AC, 0x2);
	if (bw_40m_flag == 0)
	{
		//20M/20U/20L	
		ROM_HW_WRITE_REG_BIT(0xACBD000+AddrOffset,14,6,uiRegValue);
		ROM_HW_WRITE_REG_BIT(0xACBD040+AddrOffset,14,6,uiRegValue);
	}
	else		
	{
		//40M	
		ROM_HW_WRITE_REG_BIT(0xACBD080+AddrOffset,14,6,uiRegValue);
	}
	HW_WRITE_REG(0xAC389AC, 0x4);
	HW_WRITE_REG(0xAC389AC, 0x5);
}

static void PHY_Initial_DigScaler_Table(void)
{
	s16 DigGainValue;
	u8 index;
	
	if(initial_phy == 0){
		return;
	}
	for (index = 0; index < MAX_POWER_INDEX_ENTRIES; index ++)
	{
		DigGainValue = S8DigGainInitTbl_20M[index];
		ROM_PHY_RF_TX_WriteToGainTbl(index, DigGainValue, 0);
		DigGainValue = S8DigGainInitTbl_40M[index];
		ROM_PHY_RF_TX_WriteToGainTbl(index, DigGainValue, 1);
	}
}

static void ROM_PHY_RF_TX_WriteToPPATbl(u8 u8PowerIdx, u32 uiRegValue)
{
	u32 AddrOffset;
	
	AddrOffset = (u8PowerIdx << 2);
	HW_WRITE_REG(0xAC389AC, 0x2);
	//20M/20U/20L	
	//ROM_DBG_Printf("idx%d,0x%x=%x,%x\n",u8PowerIdx, uiRegValue, ROM_HW_READ_REG_BIT(0xACBD0c0+AddrOffset,9,0));
	ROM_HW_WRITE_REG_BIT(0xACBD0c0+AddrOffset,9,0,uiRegValue);
	ROM_HW_WRITE_REG_BIT(0xACBD100+AddrOffset,9,0,uiRegValue);
	ROM_HW_WRITE_REG_BIT(0xACBD140+AddrOffset,9,0,uiRegValue);
		
	HW_WRITE_REG(0xAC389AC, 0x4);
	HW_WRITE_REG(0xAC389AC, 0x5);
}

static void PHY_Initial_Ppa_Table(void)
{
    u32 uiRegValue;
	u8 	index;
	const u8  *u16PpaGainValTbl;	
	if(initial_phy == 0){
		return;
	}
	u16PpaGainValTbl = u16PpaGainValTbl_20M_40M;	

	for (index = 0; index < MAX_POWER_INDEX_ENTRIES; index ++)
	{
		uiRegValue = u16PpaGainValTbl[index];	
		ROM_PHY_RF_TX_WriteToPPATbl(index, uiRegValue);
	}		
	
}

void ROM_TX_PM_Setup_LongFilter_A1_OwnMac(void)
{
    u16 *pAddr;
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
    /* Setup Our Address 0 filter, Use Long filter 0 */
    pAddr = (u16*)&phy_sets->phy_params.stationId0;

    HW_WRITE_REG(PAC_RXF_LFILTER_A1_OWNMAC_0_REF_1, ((u32)__le16_to_cpu(pAddr[0])) | ((u32)__le16_to_cpu(pAddr[1]))<<16);
    HW_WRITE_REG(PAC_RXF_LFILTER_A1_OWNMAC_0_REF_2, (u32)__le16_to_cpu(pAddr[2]));
    HW_WRITE_REG(PAC_RXF_LFILTER_A1_OWNMAC_0_CTL, PAC_RXD_MAKE_LONG_FILTER_CTL(PAC_LF__USE_EXTRACTOR_A1,PAC_LF__MATCH_PATTERN_DEFAULT) );    /* mask for address match, ofs in frame */

	if (GET_LMAC_SUPPORT_MULTI_MAC_ADDR(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
	    /* Setup Our Address 1 filter, Use Long filter 2
	       The last byte of the MAC address can be changed dynamically
	     */
	    pAddr = (u16*)&phy_sets->phy_params.stationId1;

	    HW_WRITE_REG(PAC_RXF_LFILTER_A1_OWNMAC_1_REF_1, ((u32)__le16_to_cpu(pAddr[0])) | ((u32)__le16_to_cpu(pAddr[1]))<<16);
	    HW_WRITE_REG(PAC_RXF_LFILTER_A1_OWNMAC_1_REF_2, (u32)__le16_to_cpu(pAddr[2]));
	    HW_WRITE_REG(PAC_RXF_LFILTER_A1_OWNMAC_1_CTL, PAC_RXD_MAKE_LONG_FILTER_CTL(PAC_LF__USE_EXTRACTOR_A1,PAC_LF__MATCH_PATTERN_DEFAULT) );    /* mask for address match, ofs in frame */

		//interfaceID 1 mac addr , used by chiper model
		HW_WRITE_REG(SECOND_INFADDR_0_31, ((u32)__le16_to_cpu(pAddr[0])) | ((u32)__le16_to_cpu(pAddr[1]))<<16);	
		HW_WRITE_REG(SECOND_INFADDR_32_47, (u32)__le16_to_cpu(pAddr[2]));
	}

}

static void TX_PM_Setup_All_STA_Filters(void)
{
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
    // ----------------------------------------------------------------------------------
    //1. Setup all BYTE FILTERS

    // Control-ACK (Detect Ack frame)        // value to match = 1101 0100
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_ACK,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_CNTL_ACK, D11_SUB_TYPE_MASK, 0, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Control-CTS (Detect CTS frame)        // value to match = 1100 0100
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_CTS,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_CNTL_CTS, D11_SUB_TYPE_MASK, 0, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Control-RTS (Detect RTS frame)        // value to match = 1011 0100
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_RTS,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_CNTL_RTS, D11_SUB_TYPE_MASK, 0, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Control-CF-End (Detect CF_End frame)    // value to match = 111x 0100
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_CF_END,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_CNTL_CFEND, (D11_SUB_TYPE_MASK) & (~(1<<4)), 0, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Control-BAR (Detect BAR frame)        // value to match = 1000 0100
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_BAR,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_CNTL_BLKACK_REQ, D11_SUB_TYPE_MASK, 0, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Control-BA (Detect Block Ack frame)    // value to match = 1001 0100
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_BA,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_CNTL_BLKACK, D11_SUB_TYPE_MASK, 0, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Mgt-Beacon (Detect Beacon frame)        // value to match = 1000 0000
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_BEACON,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_MGMT_BCN, D11_SUB_TYPE_MASK, 0, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Data-QoS (Detect Qos Data frame)        // value to match = 1xxx 1000
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_QOSDATA,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_QDATA, D11_SUB_QTYPE_MASK, 0, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Poll-Bit (Detect Detect Poll bit in any frame)    // value to match = xx1x xxxx
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_POLL_BIT,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_CFPOLL_BIT, D11_CFPOLL_BIT, 0, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Null-Bit (Detect Null Data frame?)        // value to match = x1xx xxxx
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_NULL_BIT,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_NULL_BIT, D11_NULL_BIT, 0, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Data-Bit (Detect Data ?)                // value to match = x0xx xxxx
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_DATA_BIT,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_DATA_BIT, 0x40, 0, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Multicast (Detect Multicast)            // value to match = xxxx xxx1
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_MCAST,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(0x01, 0x01, 4, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // BA-NoAck-Bit (Detect BA-NoAck frame)        // value to match = xxxx xxx1
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_BA_CTRL_NO_ACK,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(0x01, 0x01, 16, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // NormalACKPolicy (Detect frames with normal ACK policy) // value to match = x00x xxxx    // ofs = 24 or 30 // mismatch
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_QOS_CTRL_NORMAL_ACK_MISMATCH,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_ACK_POLICY_NORMAL, D11_ACK_POLICY_MASK, 24, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Duration field low byte  (BSS_ClearNAV filter)        // value to match = 0000 0000
    //BSS-Clear-NAV (Duration=0, A1=BSSAddr - STA mode only) - Part-1
    //Since hw does not support ofs to begin with 0,1,2 so changed this filter.
    //Now using 1 long filter and 2 byte filters to perform Bss-Clear-Nav
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_DURATION_LOW_ZERO,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(0x00, 0xff, 2, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Duration field high byte (BSS_ClearNAV filter)        // value to match = 0000 0000
    //BSS-Clear-NAV (Duration=0, A1=BSSAddr - STA mode only) - Part-2
    //Since hw does not support ofs to begin with 0,1,2 so changed this filter.
    //Now using 1 long filter and 2 byte filters to perform Bss-Clear-Nav
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_DURATION_HIGH_ZERO,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(0x00, 0xff, 3, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Dedicated control wrapper matching. The default one is handled by the dynamic filter 1
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_CONTROL_WRAPPER,
      PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_SUB_CNTL_WRAPPER, D11_SUB_TYPE_MASK, 0, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Setup Byte filter to detect HT frames
    //  PhyRX vector enable, RX Mode is at offset 4 => 0x84 + No dynamic offset
    //  101: 802.11n OFDM long (mixed-mode) preamble
    //  100: 802.11n OFDM short (greenfield-mode) preamble
    //  so mask is 110 => 0x6 and value is 100 => 0x4
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HT_MODE,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(0x4, 0x6, 0x84, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Detect Order Bit
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_ORDER_BIT,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(0x80, 0x80, 0x01, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Setup Filter to detect TRQ bit of data frames
    // TRQ is Bit MSB1 of Byte 27 (Qos=2Bytes)
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HTC_DATA_TRQ,
        PAC_RXD_MAKE_BYTE_FILTER_VAL((1<<1), (1<<1), 0x1A , USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Setup Filter to detect NDP Announce bit of data frames
    // NDP is Bit MSB1 of Byte 30 (Qos=2Bytes)
    // Dynamic Filter to handle address 4
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HTC_DATA_NDP,
        PAC_RXD_MAKE_BYTE_FILTER_VAL((0<<0), (1<<0), 0x1D, USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Dynamic offset would be useful here but ctrl wrapper matching already used with different offsets

    // Setup Filter to detect TRQ bit
    // TRQ is Bit MSB1 of Byte 13
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HTC_CTRL_WRAPPER_TRQ,
        PAC_RXD_MAKE_BYTE_FILTER_VAL((1<<1), (1<<1), 0xc , DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Setup Filter to detect NDP Announce bit
    // NDP is Bit MSB1 of Byte 16
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HTC_CTRL_WRAPPER_NDP,
        PAC_RXD_MAKE_BYTE_FILTER_VAL((0<<0), (1<<0), 0xf, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Setup Filter to detect TRQ bit of mgmt frames
    // TRQ is Bit MSB1 of Byte 25
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HTC_MGMT_TRQ,
        PAC_RXD_MAKE_BYTE_FILTER_VAL((1<<1), (1<<1), 0x18 , DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Setup Filter to detect NDP Announce bit of data frames
    // NDP is Bit MSB1 of Byte 28
    // Dynamic Filter to handle address 4
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HTC_MGMT_NDP,
        PAC_RXD_MAKE_BYTE_FILTER_VAL((0<<0), (1<<0), 0x1B, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Mgt Frame (Detect Mgmt frame)
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_MGMT,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(D11_MGMT_TYPE, D11_TYPE_MASK, 0, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // Setup Byte filter to detect aggregation frames
    // PhyRX vector enable, RX Mode is at offset 4 => 0x84 + No dynamic offset
    // agg is bit 5
    // also check for HT
    //  so mask is 100110 => 0x26 and value is 100100 => 0x24
    HW_WRITE_REG(PAC_RXF_BYTE_FILTER_HT_AGGREGATION,
        PAC_RXD_MAKE_BYTE_FILTER_VAL(0x24, 0x26, 0x84, DONT_USE_DYNAMIC_FILTERS) );    //value_to_match, mask, ofs

    // ----------------------------------------------------------------------------------
    //2. Setup all LONG FILTERS

    //A1-Address (Match our address in A1 field of frame)
    ROM_TX_PM_Setup_LongFilter_A1_OwnMac();

    // ----------------------------------------------------------------------------------
    //3. Setup all Filter match or mismatch
    //ByteFilter-13 (Normal-Ack) and TSF_Comare are for doing mismatch
    //Also, we need mismatch for end of aggregation in condition B
    HW_WRITE_REG( PAC_RXD_CONDITION_INV,
        (PAC_RXD_BFILTER__QOS_CTRL_NORMAL_ACK_MISMATCH) );
    HW_WRITE_REG( PAC_RXD_CONDITIONB_INV,
        PAC_RXD_SP_FILTER__END_OF_AGGR_MISMATCH );

    // ----------------------------------------------------------------------------------
    //4. Setup dynamic offset filter

    //Note that Dynamic offset filter 1 will be given preference if both dynamic filters match

    // DS to DS filter. Meaningful for QoS filter only (for QoS Data only) // value to match =   11 1xxx 10
    //(b2 to b9) => b9_b8 = ToDs/FromDS = 11, b7_b4=subtype = 1xxx for "Qos Data Frame", b3_b2=type=10 for "Data frame"
    HW_WRITE_REG(PAC_RXF_DYNAMIC_OFFSET_0,
        PAC_RXD_MAKE_DYNAMIC_FILTER_VAL(
            ((D11_DS_DS>>8)<<6) | (D11_SUB_QDATA>>2),   // value_to_match
            (D11_FROM_DS_TO_DS_MASK<<6)   | (D11_SUB_QTYPE_MASK>>2),     // mask (shifted by 2 as hw is checking b2-b9 and not b0-b7)
            0,                                          // dynamic ofset for  extractors enabled for offset 0: for fields SeqCtl and before
            6,                                          // dynamic ofset for target byte filter and extractors enabled for offset 1: QOS field
                                                        // CW1250 PAS ECO bypasses it when taking dynamic offset for A2 
            ENABLE_DYNAMIC_FILTER));                    // enable dynamic filter

    // Control Wrapper frame // value to match =   xx 0111 01
    //(b2 to b9) => b9_b8 = ToDs/FromDS = xx (ignored), b7_b4=subtype = 0111 for "control wrapper frame", b3_b2=type=01 for "control frame"

    HW_WRITE_REG(PAC_RXF_DYNAMIC_OFFSET_1,
        PAC_RXD_MAKE_DYNAMIC_FILTER_VAL(
            (D11_SUB_CNTL_WRAPPER >> 2),                // value_to_match (FC = HT Control, shift by 2 to remove Protocol version.
            (D11_SUB_TYPE_MASK >> 2),                   // mask (shifted by 2 as hw is checking b2-b9 and not b0-b7)
            10,                                         // dynamic ofset for target byte filter and extractors enabled for offset 0 -
                                                        //                                   ACK,RTS in Carried FrameCtl after A1.
            6,                                          // dynamic ofset for  extractors enabled for offset 1: for A2, BAR, BA SSC
            ENABLE_DYNAMIC_FILTER));                    // enable dynamic filter

    // ----------------------------------------------------------------------------------
    //5. Setup Extractors
    // these are hardware defaults in 8601 latest RTL, but not in earlier 8601 FPGAs.
    // 2-byte extractors, must be set up for 8601 PAC hardware to function as designed
    HW_WRITE_REG(PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_0, 0x80000000); // FC (CTL), offset 0, dynamic offset 0
    HW_WRITE_REG(PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_1, 0x00020000); // DUR, offset 2, non-dynamic
    HW_WRITE_REG(PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_2, 0x00160000); // SeqCtl, offset 22, not in a control frame, non-dynamic
    HW_WRITE_REG(PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_3, 0xC0180000); // QosCtl, offset 24, dynamic offset 1
    HW_WRITE_REG(PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_4, 0xC0100000); // BAR, offset 16, dynamic offset 1
    HW_WRITE_REG(PAC_RXF_TWO_BYTE_EXTRACT_CONTROL_5, 0xC0120000); // BA SSC, offset 18, dynamic offset 1
    // 6-byte extractors, not used by hardware
    HW_WRITE_REG(PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_0, 0x00040000); // A1, non-dynamic
	if (GET_LMAC_ENABLE_A4_FORMAT_ECO(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
	    HW_WRITE_REG(PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_1, 0x800A0000); // A2, dynamic, HW always chooses offset 1
	} else {
	    HW_WRITE_REG(PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_1, 0x000A0000); // A2, dynamic, HW always chooses offset 1
	}
    HW_WRITE_REG(PAC_RXF_SIX_BYTE_EXTRACT_CONTROL_2, 0x00100000); // A3, not in a control frame, non-dynamic
}

static void TX_PM_Setup_All_STA_RX_Events(void)
{
	struct wsm_phy_regval_sets *phy_sets = phy_reg_table_get();
    /* It is OK to set up all RX events but not enable them */

    //Rx Conditions / Events (generate rx event N when all the filters given here match)

    // RxEvent 0: Match for AggregateEnd-Respond-BA for our MAC Address-0 event
	if (GET_LMAC_ASIC_1250_CUT_1(__le32_to_cpu(phy_sets->phy_params.compileMacro)) && 
		GET_LMAC_ENABLE_AGG_END_ECO(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
	    HW_WRITE_REG(PAC_RXD_CONDITION_AGGREND_RESPONDBA_MA_0,
	                    PAC_RXD_LFILTER__A1_OWNMAC_0 |
	                   PAC_RXD_SP_FILTER__END_OF_AGGR | PAC_RXD_SP_FILTER__NORMAL_ACK_IN_AGG );
	} else {
	    HW_WRITE_REG(PAC_RXD_CONDITION_AGGREND_RESPONDBA_MA_0,
	                   PAC_RXD_SP_FILTER__END_OF_AGGR | PAC_RXD_SP_FILTER__NORMAL_ACK_IN_AGG );
	}
     //HW_WRITE_REG(PAC_RXD_CONDITIONB_AGGREND_RESPONDBA_MA_0, 0 );

    // RxEvent 1: Match for AggregateEnd-Respond-BA for our MAC Address-1 event
	if (GET_LMAC_ASIC_1250_CUT_1(__le32_to_cpu(phy_sets->phy_params.compileMacro)) && 
		GET_LMAC_ENABLE_AGG_END_ECO(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
	    HW_WRITE_REG(PAC_RXD_CONDITION_AGGREND_RESPONDBA_MA_1,
	                    PAC_RXD_LFILTER__A1_OWNMAC_1 |
	                    PAC_RXD_SP_FILTER__END_OF_AGGR | PAC_RXD_SP_FILTER__NORMAL_ACK_IN_AGG );
	} else {
	    HW_WRITE_REG(PAC_RXD_CONDITION_AGGREND_RESPONDBA_MA_1,
	                    PAC_RXD_SP_FILTER__END_OF_AGGR | PAC_RXD_SP_FILTER__NORMAL_ACK_IN_AGG );
	}
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_AGGREND_RESPONDBA_MA_1, 0 );

    // RxEvent 2: Match for AggregateEnd event
    HW_WRITE_REG(PAC_RXD_CONDITION_AGGREND,
                    PAC_RXD_SP_FILTER__END_OF_AGGR);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_AGGREND, 0 );

    // RxEvent 3: Match for PHY error and RX overflow
    HW_WRITE_REG(PAC_RXD_CONDITION_FRAME_ABORT,
                    PAC_RXD_SP_FILTER__FRAME_ABORT);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_FRAME_ABORT, 0);

    // RxEvent 4: Match for CRC check fail event
    HW_WRITE_REG(PAC_RXD_CONDITION_CRC_ERROR,
                    PAC_RXD_SP_FILTER__CRC_ERROR);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_CRC_ERROR, 0);

    // RxEvent 6: Match for CTS Received event
    HW_WRITE_REG(PAC_RXD_CONDITION_CTS,
                    PAC_RXD_LFILTER__A1_OWNMAC | PAC_RXD_BFILTER__CTS);
    HW_WRITE_REG(PAC_RXD_CONDITIONB_CTS, 0);

    // RxEvent 7: Match for RTS Received for our MAC Address-0 event
    HW_WRITE_REG(PAC_RXD_CONDITION_RTS_MA_0,
                    PAC_RXD_LFILTER__A1_OWNMAC_0 | PAC_RXD_BFILTER__RTS );
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_RTS_MA_0, 0);

    // RxEvent 8: Match for RTS Received for our MAC Address-1 event
    HW_WRITE_REG(PAC_RXD_CONDITION_RTS_MA_1,
                    PAC_RXD_LFILTER__A1_OWNMAC_1 | PAC_RXD_BFILTER__RTS );
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_RTS_MA_1, 0);

    // RxEvent 9: Match for BAR-Immediate Received for our MAC Address-0 event
    HW_WRITE_REG(PAC_RXD_CONDITION_BAR_IMM_MA_0,
                    PAC_RXD_LFILTER__A1_OWNMAC_0 | PAC_RXD_BFILTER__BAR | PAC_RXD_SP_FILTER__TATID);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_BAR_IMM_MA_0, 0);

    // RxEvent 10: Match for BAR-Immediate Received for our MAC Address-1 event
    HW_WRITE_REG(PAC_RXD_CONDITION_BAR_IMM_MA_1,
                    PAC_RXD_LFILTER__A1_OWNMAC_1 | PAC_RXD_BFILTER__BAR | PAC_RXD_SP_FILTER__TATID);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_BAR_IMM_MA_1, 0);

    // RxEvent 11: Match for BAR-Delayed-No-Ack Received event
    HW_WRITE_REG(PAC_RXD_CONDITION_BAR_DELAYED_NOACK,
                    PAC_RXD_LFILTER__A1_OWNMAC | PAC_RXD_BFILTER__BAR | PAC_RXD_BFILTER__BA_CTRL_NO_ACK);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_BAR_DELAYED_NOACK, 0);

    // RxEvent 12: Match for BA Immediate-Received event
    HW_WRITE_REG(PAC_RXD_CONDITION_BA_IMM, PAC_RXD_LFILTER__A1_OWNMAC |
                                           PAC_RXD_BFILTER__BA );
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_BA_IMM, 0);

    // RxEvent 13: Match for BA-Delayed-NoAck Received event
    HW_WRITE_REG(PAC_RXD_CONDITION_BA_DELAYED_NOACK,
                    PAC_RXD_LFILTER__A1_OWNMAC | PAC_RXD_BFILTER__BA | PAC_RXD_BFILTER__BA_CTRL_NO_ACK);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_BA_DELAYED_NOACK, 0);

    // RxEvent 14: Match for BSS-Beacon Received event
    HW_WRITE_REG(PAC_RXD_CONDITION_BSS_BEACON,
                    PAC_RXD_LFILTER__A3_BSSID_IBSSONLY| PAC_RXD_BFILTER__BEACON);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_BSS_BEACON, 0);

    // RxEvent 15: Match for Multicast Pkt Received event
    HW_WRITE_REG(PAC_RXD_CONDITION_MULTICAST_PKT,
                    PAC_RXD_BFILTER__MCAST);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_MULTICAST_PKT, 0);

    // RxEvent 16: Match for Qos-Data (No Ack) Received event
    HW_WRITE_REG(PAC_RXD_CONDITION_QOSDATA_NOACK,
                    PAC_RXD_LFILTER__A1_OWNMAC | PAC_RXD_BFILTER__QOSDATA | PAC_RXD_BFILTER__QOS_CTRL_NORMAL_ACK_MISMATCH);
    HW_WRITE_REG(PAC_RXD_CONDITIONB_QOSDATA_NOACK, 0);

    // RxEvent 17: Match for Ctrl-Ack-Respond-Data event (Ack Recd, send next data)
    HW_WRITE_REG(PAC_RXD_CONDITION_ACK_RECD_SEND_NEXT_DATA,
                    PAC_RXD_LFILTER__A1_OWNMAC | PAC_RXD_BFILTER__ACK);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_ACK_RECD_SEND_NEXT_DATA, 0);

    // RxEvent 5,18: Spare

    // RxEvent 19: Match for Respond-ACK event (unicast pkt address to our MA-0 received, send ACK back)
    HW_WRITE_REG(PAC_RXD_CONDITION_UNICAST_PKT_RECD_SEND_ACK_MA_0,
                    PAC_RXD_LFILTER__A1_OWNMAC_0);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_UNICAST_PKT_RECD_SEND_ACK_MA_0, 0);

    // RxEvent 20: Match for Respond-ACK event (unicast pkt address to our MA-1 received, send ACK back)
    HW_WRITE_REG(PAC_RXD_CONDITION_UNICAST_PKT_RECD_SEND_ACK_MA_1,
                    PAC_RXD_LFILTER__A1_OWNMAC_1);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_UNICAST_PKT_RECD_SEND_ACK_MA_1, 0);


    // RxEvent 21: Match for Qos-Clear-NAV Received event (pkt not addressed to us received, and its of type QOS-Null_Data
    // See 802.11 standard 9.9.2.2.1, the standard says A1 has to be our BSS
    // But if we set NAV regardless of BSS, then we should reset NAV regardless of BSS
    HW_WRITE_REG(PAC_RXD_CONDITION_QOSDATA_CLEARNAV,
                    PAC_RXD_BFILTER__DURATION_LOW_ZERO | PAC_RXD_BFILTER__DURATION_HIGH_ZERO |
                    PAC_RXD_BFILTER__QOSDATA | PAC_RXD_BFILTER__NULL_BIT | PAC_RXD_BFILTER__POLL_BIT);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_QOSDATA_CLEARNAV, 0);

    // RxEvent 22: Match for CF-End-Clear-NAV Received event
    // See 802.11 standard 9.9.2.2.1, the standard says A1 has to be our BSS
    // But 9.3.2.2 contradict, it says, if received CF-End from any BSS, then reset NAV
    HW_WRITE_REG(PAC_RXD_CONDITION_CFEND_RECD_CLEARNAV,
                    PAC_RXD_BFILTER__CF_END);
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_CFEND_RECD_CLEARNAV, 0);

    // RxEvent 23: Load NAV for any packets not for us
	if (GET_LMAC_SW_SET_NAV(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
	    HW_WRITE_REG(PAC_RXD_CONDITION_NOT_FOR_US_UPDATENAV,PAC_RXD_BFILTER__CTS);
	   // HW_WRITE_REG(PAC_RXD_CONDITIONB_NOT_FOR_US_UPDATENAV, 0);
	} else if (GET_LMAC_STA_MONITOR(__le32_to_cpu(phy_sets->phy_params.compileMacro))) {
		HW_WRITE_REG(PAC_RXD_CONDITION_NOT_FOR_US_UPDATENAV, 0);
		HW_WRITE_REG(PAC_RXD_CONDITIONB_NOT_FOR_US_UPDATENAV,0);
	} else {	
	    HW_WRITE_REG(PAC_RXD_CONDITION_NOT_FOR_US_UPDATENAV, 0);
	}
    //HW_WRITE_REG(PAC_RXD_CONDITIONB_NOT_FOR_US_UPDATENAV, 0);

    HW_WRITE_REG( PAC_NTD_RX_EVENT_NAV_LOADA, 0);
    HW_WRITE_REG( PAC_NTD_RX_EVENT_NAV_LOADB, 0x02<<((PAC_NTD_STATUS__NOT_FOR_US_UPDATENAV-16)*2) ); /* 23 = catch nav */

    // RxEvent 24: Used for DUR table RX-event, generated even not selected for NTD
    // will be programmed dynamically with PAC_RXD_LFILTER__A1_OWNMAC_0 or PAC_RXD_LFILTER__A1_OWNMAC_1
    //HW_WRITE_REG(PAC_RXD_CONDITION_ALT_LINK, PAC_RXD_LFILTER__A1_OWNMAC_0);

    //HW_WRITE_REG(PAC_RXD_CONDITIONB_ALT_LINK, PAC_RXD_SP_FILTER__A2_FOR_ALT_LINK);

    // RxEvent 25: BAB - Initialize / Update SSN (Starting Seq No)
    //AK_CHK: Check comment
    //BAR with Normal Ack not possible in A-MPDU, so its not a BAR frame.
    //When we receive BAR, then event 4 ill also be fired!
    //Operation in BAB: When this event matches, BAB will update its SSN to the one provided with the received BAR frame
    HW_WRITE_REG(PAC_RXD_CONDITION_UPDATE_SSN_BAB,
                    PAC_RXD_SP_FILTER__TATID  | // For BAB - Matches TID in QosCtrl (or TID in BA-Ctrl of BAR/BA frame). Matches TA - AK_CHK?
                    PAC_RXD_LFILTER__A1_OWNMAC | PAC_RXD_BFILTER__BAR);
    HW_WRITE_REG(PAC_RXD_CONDITIONB_UPDATE_SSN_BAB,
                    0);

    // RxEvent 26: BAB - Update Ack bitmap in BAB
    //Catched Unicast QoS data frame directed to us.
    //Operation in BAB: When this event occurs, BAB checks the SeqNo of the frame and updates its bitmap in SRAM
    HW_WRITE_REG(PAC_RXD_CONDITION_UPDATE_ACK_BITMAP_BAB,
                    PAC_RXD_SP_FILTER__TATID |           // For BAB - Matches TID in QosCtrl of QosData frame (or TID in BA-Ctrl of BAR/BA frame). Matches TA - AK_CHK?
                    PAC_RXD_LFILTER__A1_OWNMAC | PAC_RXD_BFILTER__QOSDATA
                    ); // Match any Qos Data coming to us

   // HW_WRITE_REG(PAC_RXD_CONDITIONB_UPDATE_ACK_BITMAP_BAB,
   //                 PAC_RXD_BFILTER__HT_AGGREGATION);
    HW_WRITE_REG(PAC_RXD_CONDITIONB_UPDATE_ACK_BITMAP_BAB,
	  				 0);

    HW_WRITE_REG(PAC_RXD_CONDITION__ALT_RESP_RATE_HT_RTS,
                    PAC_RXD_BFILTER__RTS |   //  Match RTS received
                    PAC_RXD_BFILTER__HT_MODE); // Match HT Phy Modes
    //HW_WRITE_REG(PAC_RXD_CONDITIONB__ALT_RESP_RATE_HT_RTS, 0);

    HW_WRITE_REG(PAC_RXD_CONDITION__ALT_RESP_RATE_HTC_DATA_TRQ_NDP,
                    PAC_RXD_BFILTER__HT_MODE     | //  Match HT Phy modes
                    PAC_RXD_BFILTER__ORDER_BIT   | //  Match OrderBit=1
                    PAC_RXD_BFILTER__QOSDATA     | //  Match Data Frame
                    PAC_RXD_BFILTER__HTC_DATA_TRQ | //  Match TRQ=1
                    PAC_RXD_BFILTER__HTC_DATA_NDP); // Match NDP=0
    //HW_WRITE_REG(PAC_RXD_CONDITIONB__ALT_RESP_RATE_HTC_DATA_TRQ_NDP, 0);

    HW_WRITE_REG(PAC_RXD_CONDITION__ALT_RESP_RATE_HTC_CTRL_WRAPPER_TRQ_NDP,
                    PAC_RXD_BFILTER__HT_MODE     | //  Match HT Phy modes
                    PAC_RXD_BFILTER__ORDER_BIT   | //  Match OrderBit=1
                    PAC_RXD_BFILTER__CONTROL_WRAPPER | // Match Ctrl Wrapper
                    PAC_RXD_BFILTER__HTC_CTRL_WRAPPER_TRQ | //  Match TRQ=1
                    PAC_RXD_BFILTER__HTC_CTRL_WRAPPER_NDP); // Match NDP=0
    //HW_WRITE_REG(PAC_RXD_CONDITIONB__ALT_RESP_RATE_HTC_CTRL_WRAPPER_TRQ_NDP, 0);

    HW_WRITE_REG(PAC_RXD_CONDITION__ALT_RESP_RATE_HTC_MGMT_TRQ_NDP,
                    PAC_RXD_BFILTER__HT_MODE        |   //  Match HT Phy modes
                    PAC_RXD_BFILTER__ORDER_BIT );       //  Match OrderBit=1

    HW_WRITE_REG(PAC_RXD_CONDITIONB__ALT_RESP_RATE_HTC_MGMT_TRQ_NDP,
                    PAC_RXD_BFILTER__MGMT           | //  Match Mgmt Frame
                    PAC_RXD_BFILTER__HTC_MGMT_TRQ   | //  Match TRQ=1
                    PAC_RXD_BFILTER__HTC_MGMT_NDP); // Match NDP=0

    return;
    //====================//
    //This was removed during the last update
    /* Load NAV. Note we do not need to change this between CP and CFP, because
       TX is not programmed to be controlled by NAV during CFP
     */
    //HW_WRITE_REG(PAC_NTD_RX_EVT_NAV_LOADA, 0);
    //HW_WRITE_REG(PAC_NTD_RX_EVT_NAV_LOADB, 0x02<<((PAC_NTD_STATUS__UNRECOGNISEDRX_CATCHNAV-16)*2) ); /* AK: CHK */
}

void TX_PM_Init(void)
{
	if(initial_mac == 0){
		return;
	}
    /* First thing to do is setup the PM filters that we'll need */
    TX_PM_Setup_All_STA_Filters();

    /* SETUP The PM Event Table */
    TX_PM_Setup_All_STA_RX_Events();
}

wsm_regval_bit *send_buf_backup = NULL;
int send_buf_backup_count = 0;

void atbm_phy_init(struct atbm_common *hw_priv)
{
	int i;
	wsm_regval_bit *sendBuf;
	struct wsm_phy_regval_sets *phy_sets = NULL;

	if(!(hw_priv->wsm_caps.firmeareExCap & EX_CAPABILITIES_DRIVER_PHY_REG_INIT)){
		atbm_printk_init("not need phy init in driver\n");
		return;
	}
	phy_reg_muxlock();
	
	phy_sets = phy_alloc_mem(sizeof(struct wsm_phy_regval_sets) + 
							 sizeof(wsm_regval_bit) * ATBM_PHY_REG_INIT_BUF_MAX_SIZE);
	sendBuf  = phy_alloc_mem(sizeof(wsm_regval_bit) * (ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME + 1));
	
	BUG_ON(phy_sets == NULL);
	BUG_ON(sendBuf == NULL);
	
	phy_sets->table_index = 0;
	phy_sets->table_size  = ATBM_PHY_REG_INIT_BUF_MAX_SIZE;
	phy_sets->table       = (wsm_regval_bit *)(phy_sets + 1);
	
	atbm_printk_init("Start to phy reg init.");
	WARN_ON(wsm_phy_init_get_global_flag(hw_priv, &phy_sets->phy_params));
	atbm_printk_init("rfSubtypeDefine[%d][%d][%d]\n",phy_sets->phy_params.rfSubtypeDefine,
					 phy_sets->phy_params.aresAy,
					 phy_sets->phy_params.aresAzlc);
	atbm_printk_init("compileMacro(%x)\n",__le32_to_cpu( phy_sets->phy_params.compileMacro));
	BUG_ON(phy_reg_table_set(phy_sets));

	/* Initial Memory Table */
	PHY_Initial_Memory_Table(1);
	atbm_printk_init("PHY_Initial_Memory_Table index: %d\n", phy_sets->table_index);

	/* Initial Common Reg */
	PHY_Initial_Common_Reg();
	atbm_printk_init("PHY_Initial_Common_Regindex: %d\n", phy_sets->table_index);

	/*Init Digital scaler table*/
	PHY_Initial_DigScaler_Table();
	atbm_printk_init("PHY_Initial_DigScaler_Table: %d\n", phy_sets->table_index);

	/*Init Ppa Gain table*/
	PHY_Initial_Ppa_Table();
	atbm_printk_init("PHY_Initial_Ppa_Tableindex: %d\n", phy_sets->table_index);
	
	/* Initial FEM Reg */
	PHY_Initial_FEM_Reg();
	atbm_printk_init("PHY_Initial_FEM_Regindex: %d\n", phy_sets->table_index);

	TX_PM_Init();
	atbm_printk_init("TX_PM_Initindex: %d\n", phy_sets->table_index);

	/* Initial RFIP Reg*/
	PHY_RF_ConfigRFIP();
	atbm_printk_init("PHY_RF_ConfigRFIPindex: %d\n", phy_sets->table_index);
	
	/* final */
	ROM_HW_WRITE_REG_BIT(PHY_EOT, 0, 0, HB_EOT);

#ifdef ATBM_REG_COMPARE_WH
		send_buf_backup = phy_alloc_mem(sizeof(wsm_regval_bit) * (phy_sets->table_index));
		BUG_ON(send_buf_backup == NULL);
		memset(send_buf_backup, 0, sizeof(wsm_regval_bit) * (phy_sets->table_index));
		memcpy(send_buf_backup, phy_sets->table, sizeof(wsm_regval_bit) * (phy_sets->table_index));
		send_buf_backup_count = phy_sets->table_index;
#endif

	i = 0;
	while (i < phy_sets->table_index) {
		if (i + ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME < phy_sets->table_index) {
			memset(sendBuf, 0, sizeof(wsm_regval_bit) * (ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME + 1));
			memcpy(sendBuf, phy_sets->table + i, sizeof(wsm_regval_bit) * ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME);
			sendBuf[ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME].addr = __cpu_to_le32(PHY_EOT);
			WARN_ON(wsm_phy_write_reg_bit_u32(hw_priv, sendBuf, sizeof(wsm_regval_bit) * (ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME + 1)));
		} else {
			WARN_ON(wsm_phy_write_reg_bit_u32(hw_priv, phy_sets->table + i,
				sizeof(wsm_regval_bit) * (phy_sets->table_index - i)));
		}
		i += ATBM_PHY_REG_INIT_SEND_BUF_EACH_TIME;
	}
	phy_reg_table_free();
	
	phy_reg_muxunlock();
	
	phy_free_mem(phy_sets);
	phy_free_mem(sendBuf);
	return;
}

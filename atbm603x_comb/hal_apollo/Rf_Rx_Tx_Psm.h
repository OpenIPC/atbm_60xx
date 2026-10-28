#ifndef RF_RX_TX_PSM_H
#define RF_RX_TX_PSM_H
#define APOLLOE_SHORTGI_ECO_EN   (BIT(31)|BIT(30))
typedef struct rf_reg_s{
	u32 addr;
	u32 data;
}rf_reg;
typedef struct rf_reg_s_bit{
	u32 addr;
	u16 endBit;
	u16 startBit;
	u32 data;
}rf_reg_bit;
typedef struct rf_reg_s_bit_u16{
	u32 addr;
	u8 endBit;
	u8 startBit;
	u32 data;
}rf_reg_bit_u16;

//init other reg
const rf_reg Rf_init_value[]=
{
	{PHY_RFIP_TXRFOFFDELAY_ADDR, 		0x000000A0},
	{PHY_RFIP_TOP_CTRL_ADDR,			0x0},
	{PHY_RFIP_CTRL_ADDR,				0x104},
	{PHY_RFIP_RXCFG_ADDR,				0xc000},
	{PHY_RFIP_RXPSDELAY_ADDR,			0x000f001f},
	{PHY_RFIP_RXFONDELAY_ADDR,			0x0000000f},
	{PHY_RFIP_TXCALCFG_ADDR, 			0x40000},
	{PHY_RFIP_RXLO_PUP_DOWN_DELAY0,		0x0a0a0a0a},
	{PHY_RFIP_RXLO_PUP_DOWN_DELAY1,		0x000a0a0a},
	{PHY_RFIP_TXRXLDO_LQ_DELAY,			0x000f000f},
	{PHY_RFIP_TXPSDELAY_ADDR,			0x00300030},
	{PHY_RFIP_TXLO_PUP_DOWN_DELAY0,		0x0404041c}, //Acc0220
	{PHY_RFIP_TXLO_PUP_DOWN_DELAY1,		0x00060404},//Acc0224
	{PHY_RFIP_RXCFG2_ADDR,				0x20000000},
	{PHY_RFIP_RXLDOCFG_ADDR,			0x00000000},

	{PHY_RFIP_RXRF_REG1_ADDR,			0x740},
	{PHY_RFIP_INVALID,					0xffffffff}
};


//Tx Psm
const u16 Rf_Tx_Psm_addr_offset[]=
{
PHY_RFIP_TX_OFF0_PSM_OFFSET,
PHY_RFIP_TX_OFF1_PSM_OFFSET,
PHY_RFIP_TX_LDO_ON0_PSM_OFFSET,
PHY_RFIP_TX_LDO_ON1_PSM_OFFSET,
PHY_RFIP_TX_BIAS_ON0_PSM_OFFSET,
PHY_RFIP_TX_BIAS_ON1_PSM_OFFSET,
PHY_RFIP_TX_ANA_ON0_PSM_OFFSET,
PHY_RFIP_TX_ANA_ON1_PSM_OFFSET,
PHY_RFIP_TX_LDO_LQ0_PSM_OFFSET,
PHY_RFIP_TX_LDO_LQ1_PSM_OFFSET,
PHY_RFIP_TX_ACTIVE0_PSM_OFFSET,
PHY_RFIP_TX_ACTIVE1_PSM_OFFSET,
PHY_RFIP_TX_RF_OFF0_PSM_OFFSET,
PHY_RFIP_TX_RF_OFF1_PSM_OFFSET,
};
const u32 Rf_Tx_Psm_value_outerpa[]=
{
#if (RF_SUBTYPE_DEFINE == Ares_AZLC)
	0x00000821,
	0x00020811,
	0x00000821,
	0x00020811,
	0x00001905,
	0x00030a37,//ACC01EC
	0x00001bdd,
	0x0003ca27,
	0x00007bdf,
	0x0003c227,
	0x00007bdf,
	0x0003c227,
	0x00007bdf,
	0x0003c237
#else 
	//1_PSM reg bit[3] must be 0;
	0x00000821,
	0x00020801,
	0x00000821,
	0x00020801,
	0x00001905,
	(0x00030a27 | BIT(18)),//ACC01EC,BIT(18): psm decide auxadc clock;
	0x00001bdd,
	(0x0003ca37 | BIT(18)),
	0x00007bdf,
	(0x0003c237 | BIT(18)),
	0x00007bdf,
	(0x0003c237 | BIT(18)),
	0x00007bdf,
	0x0003c227
#endif
};
const u32  Rf_Tx_Psm_valueAresAZLC[] = {
	0x00000821, //PHY_RFIP_TX_OFF0_PSM,		              
	0x00020819, //PHY_RFIP_TX_OFF1_PSM,		              
	0x00000821, //PHY_RFIP_TX_LDO_ON0_PSM,	            
	0x00020819, //PHY_RFIP_TX_LDO_ON1_PSM,	            
	0x00001905, //PHY_RFIP_TX_BIAS_ON0_PSM,             
	0x00030a3f, //PHY_RFIP_TX_BIAS_ON1_PSM, /ACC01EC    
	0x00001bdd, //PHY_RFIP_TX_ANA_ON0_PSM,	            
	0x0003ca27, //PHY_RFIP_TX_ANA_ON1_PSM,	            
	0x00007bdf, //PHY_RFIP_TX_LDO_LQ0_PSM,	            
	0x0003c227, //PHY_RFIP_TX_LDO_LQ1_PSM,	            
	0x00007bdf, //PHY_RFIP_TX_ACTIVE0_PSM,	            
	0x0003c227, //PHY_RFIP_TX_ACTIVE1_PSM,	            
	0x00007bdf, //PHY_RFIP_TX_RF_OFF0_PSM,	            
	0x0003c237 //PHY_RFIP_TX_RF_OFF1_PSM,	   
};
const u32 Rf_Tx_Psm_valueAres[] = {
	//1_PSM reg bit[3] must be 0;	
	0x00000821,//PHY_RFIP_TX_OFF0_PSM,		           
	0x00020801,//PHY_RFIP_TX_OFF1_PSM,		           
	0x00000821,//PHY_RFIP_TX_LDO_ON0_PSM,	           
	0x00020801,//PHY_RFIP_TX_LDO_ON1_PSM,	           
	0x00001905,//PHY_RFIP_TX_BIAS_ON0_PSM,           
	(0x00030a27 | BIT(18)),//PHY_RFIP_TX_BIAS_ON1_PSM, /ACC01EC  
	0x00001bdd,//PHY_RFIP_TX_ANA_ON0_PSM,	           
	(0x0003ca3f | BIT(18)),//PHY_RFIP_TX_ANA_ON1_PSM,	  //ACC01EC,BIT(18): psm decide auxadc clock;         
	0x00007bdf,//PHY_RFIP_TX_LDO_LQ0_PSM,	           
	(0x0003c23f | BIT(18)),//PHY_RFIP_TX_LDO_LQ1_PSM,	           
	0x00007bdf,//PHY_RFIP_TX_ACTIVE0_PSM,	           
	(0x0003c23f | BIT(18)),//PHY_RFIP_TX_ACTIVE1_PSM,	           
	0x00007bdf,//PHY_RFIP_TX_RF_OFF0_PSM,	           
	0x0003c22f//PHY_RFIP_TX_RF_OFF1_PSM,	  
};
//TXPA config
const  rf_reg_bit_u16 Rf_TxPa_TableAresAy[] = {
	{PHY_RFIP_TXPA_REG0_ADDR,	2,	   0,	  7 	},	   
	{PHY_RFIP_TXPA_REG0_ADDR,	5,	   3,	  0 	},	   
	{PHY_RFIP_TXPA_REG0_ADDR,	8, 6, 3 	},
	{PHY_RFIP_TXPA_REG0_ADDR,	11, 9, 3   },	  
	{PHY_RFIP_TXPA_REG0_ADDR,	14, 12, 3  },	  
	{PHY_RFIP_TXPA_REG0_ADDR,	17, 15, 3  },	  
	{PHY_RFIP_TXPA_REG0_ADDR,	18, 18, 1  },	  
	{PHY_RFIP_TXPA_REG0_ADDR,	20, 20, 0  }, 
	{PHY_RFIP_TXPA_REG0_ADDR,	24, 21, 6  }, 
	{PHY_RFIP_TXPA_REG1_ADDR,	0,	   0,	  0 	},
	{PHY_RFIP_TXPA_REG1_ADDR,	5,	   1,	  0xb },	  
	{PHY_RFIP_TXPA_REG1_ADDR,	7,	   7,	  1 },	 
	{PHY_RFIP_TXPA_REG1_ADDR,	8,	   8,	  1 },	 
	{PHY_RFIP_TXPA_REG1_ADDR,	9,	   9,	  1 },	 
	{PHY_RFIP_TXPA_REG1_ADDR,	10,   10,	0 },   
	{PHY_RFIP_TXPA_REG1_ADDR,	13, 11, 4  },	  
	{PHY_RFIP_TXPA_REG2_ADDR,	0,	   0,	  0 	},
	{PHY_RFIP_TXPA_REG2_ADDR,	3,	   2,	  3 	},
	{PHY_RFIP_TXPA_REG2_ADDR,	8,	   6,	  5 	},
	{PHY_RFIP_TXPA_REG2_ADDR,	9,	   9,	  0 	},
	{PHY_RFIP_TXPA_REG2_ADDR,	13,   10,	0xd  },
	{PHY_RFIP_TXPA_REG2_ADDR,	16,   16,	1	  },
	{PHY_RFIP_TXPA_REG2_ADDR,	19,17,		0	  },
	{PHY_RFIP_TXPA_REG2_ADDR,	20,20,		0	  },
	{PHY_RFIP_TXPA_VSWR,		3,0,0x7},
	{PHY_RFIP_TXPA_VSWR,		7,4,0},
	{PHY_RFIP_TXPA_VSWR,		8,8,0},
	{PHY_RFIP_TXPA_VSWR,		12,9,0x8},
	{PHY_RFIP_TXPA_VSWR,		13,13,1},
	{PHY_RFIP_TXPA_VSWR,		14,14,1},
	{PHY_RFIP_TXCFG_ADDR,		0,0,0}, 
	{PHY_RFIP_INVALID,			31, 0,		0xffff}

};
const  rf_reg_bit_u16 Rf_TxPa_TableAresAZLC[]={
	{PHY_RFIP_TXPA_REG0_ADDR,	2,	0,	0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	5,	3,	0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	8, 6,	0	},
	{PHY_RFIP_TXPA_REG0_ADDR,	11, 9, 0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	14, 12, 0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	17, 15, 0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	18, 18, 0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	20, 20, 0	}, 
	{PHY_RFIP_TXPA_REG0_ADDR,	24, 21, 0xf	}, 
	{PHY_RFIP_TXPA_REG1_ADDR,	0,	0,	1	},
	{PHY_RFIP_TXPA_REG1_ADDR,	5,	1,	0xf },	
	{PHY_RFIP_TXPA_REG1_ADDR,	7,	7,	1 },	
	{PHY_RFIP_TXPA_REG1_ADDR,	8,	8,	1 },	
	{PHY_RFIP_TXPA_REG1_ADDR,	9,	9,	1 },	
	{PHY_RFIP_TXPA_REG1_ADDR,	10,	10,	0 },	
	{PHY_RFIP_TXPA_REG1_ADDR,	13, 11, 4	},	
	{PHY_RFIP_TXPA_REG2_ADDR,	0,	0,	1	},
	{PHY_RFIP_TXPA_REG2_ADDR,	3,	2,	3	},
	{PHY_RFIP_TXPA_REG2_ADDR,	8,	6,	0	},
	{PHY_RFIP_TXPA_REG2_ADDR,	9,	9,	0	},
	{PHY_RFIP_TXPA_REG2_ADDR,	13,	10,	0xd	},
	{PHY_RFIP_TXPA_REG2_ADDR,	16,	16,	1	},
	{PHY_RFIP_TXPA_REG2_ADDR,	19,17,	7	},
	{PHY_RFIP_TXPA_REG2_ADDR,	20,20,	0	},
	{PHY_RFIP_TXPA_VSWR,		3,0,0xf},
	{PHY_RFIP_TXPA_VSWR,		7,4,0xf},
	{PHY_RFIP_TXPA_VSWR,		8,8,1},
	{PHY_RFIP_TXCFG_ADDR,		0,0,0}, 	
	{PHY_RFIP_INVALID,			31, 0,	0xffff}
};

const  rf_reg_bit_u16 Rf_TxPa_TableAres[]={
	/*{PHY_RFIP_TXPA_REG0_ADDR,	2,	0,	0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	5,	3,	0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	8, 6,	0	},
	{PHY_RFIP_TXPA_REG0_ADDR,	11, 9, 0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	14, 12, 0	},	
	{PHY_RFIP_TXPA_REG0_ADDR,	17, 15, 0	},	*/
#if 0//(RF_SUBTYPE_DEFINE == Ares_C)
	{PHY_RFIP_TXPA_REG0_ADDR,	18, 0, 5	},	
	{PHY_RFIP_TXPA_REG1_ADDR,	13, 7, 0x77	},	
	{PHY_RFIP_TXCFG_ADDR,		13,8,0x30}, 	
#else
	{PHY_RFIP_TXPA_REG0_ADDR,	18, 0, 0	},
	{PHY_RFIP_TXPA_REG1_ADDR,	13, 7, 0x57	},	
#endif
	//{PHY_RFIP_TXPA_REG0_ADDR,	20, 20, 0	}, 
	{PHY_RFIP_TXPA_REG0_ADDR,	24, 20, 12	}, 
	//{PHY_RFIP_TXPA_REG1_ADDR,	0,	0,	0	},
	{PHY_RFIP_TXPA_REG1_ADDR,	5,	0,	22 },	
	/*{PHY_RFIP_TXPA_REG1_ADDR,	7,	7,	1 },	
	{PHY_RFIP_TXPA_REG1_ADDR,	8,	8,	1 },	
	{PHY_RFIP_TXPA_REG1_ADDR,	9,	9,	1 },	
	{PHY_RFIP_TXPA_REG1_ADDR,	10,	10,	0 },*/	
	
	{PHY_RFIP_TXPA_REG2_ADDR,	0,	0,	0	},
	{PHY_RFIP_TXPA_REG2_ADDR,	3,	2,	3	},
	//{PHY_RFIP_TXPA_REG2_ADDR,	8,	6,	5	},
	//{PHY_RFIP_TXPA_REG2_ADDR,	9,	9,	0	},
	{PHY_RFIP_TXPA_REG2_ADDR,	13,	6,	0xd5	},
	//{PHY_RFIP_TXPA_REG2_ADDR,	16,	16,	1	},
	//{PHY_RFIP_TXPA_REG2_ADDR,	19,17,	0	},
	{PHY_RFIP_TXPA_REG2_ADDR,	20,16,	1	},
	//{PHY_RFIP_TXPA_VSWR,		3,0,0x7},
	//{PHY_RFIP_TXPA_VSWR,		7,4,0},
	{PHY_RFIP_TXPA_VSWR,		8,0,0x7},
	{PHY_RFIP_TXCFG_ADDR,		0,0,0}, 	
	{PHY_RFIP_INVALID,			31, 0,	0xffff}
};
/**************************AthenaBx Start***************************************/
//Rx psm
const rf_reg Rf_Rx_Psm[]=
{	
	{PHY_RFIP_RXOFF0_PSM_ADDR,			0x00000641},
	{PHY_RFIP_RXOFF1_PSM_ADDR,			0x00000020},
	{PHY_RFIP_RX_BIAS_ON0_PSM,			0x00000f4b},
	{PHY_RFIP_RX_BIAS_ON1_PSM,			0x00000aaf},
	{PHY_RFIP_RX_ANA_ON0_PSM,			0x00000f7b},
	{PHY_RFIP_RX_ANA_ON1_PSM,			0x00001aaf},
	{PHY_RFIP_RX_LDO_LQ0_PSM,			0x00000f7f},
	{PHY_RFIP_RX_LDO_LQ1_PSM,			0x000019bf},
	{PHY_RFIP_RX_IDLE0_PSM,				0x00000f7f},
	{PHY_RFIP_RX_IDLE1_PSM,				0x000019bf},
	{PHY_RFIP_RX_ACTIVE0_PSM,			0x00000f7f},
	{PHY_RFIP_RX_ACTIVE1_PSM,			0x000019bf},
	{PHY_RFIP_RX_FAST_ON0_PSM,			0x00000f7f},
	{PHY_RFIP_RX_FAST_ON1_PSM,			0x000019bf},
	{PHY_RFIP_INVALID,					0xffffffff}
};

const  rf_reg_bit_u16 Rf_RxAdc_Table[]=
{
	{0xacc013C, 16, 15, 3}, //rxadc_ibcomp_i[1:0]
	//{0xacc029c, 0 , 0 , 0}, //rxadc_gain_manual_en
	{0xacc029c, 2 , 0 , 0}, //rxadc_gain_manual[2:1];rxadc_gain_manual_en[0]
	{0xacc013c, 19, 19, 0}, //rxadc_syncclk_freq_sel 0 for 120Ms£¬output 240Mhz,1 for 80Ms and 160Ms
	{0xacc013c, 8 , 8 , 0}, //rxadc_clk_shift_enable 0 for 120Ms,1 for 80Ms and 160Ms
	{0xacc0184, 0 , 0 , 0}, //dpll_adcclk_sel  0 for 480M 1 for 640M
	{PHY_RFIP_INVALID,31,0,0xffff}
};
const  rf_reg Rf_RfPll_Table40[]=
{
	{0xacc0114,0x00000000},
	{0xacc0288,0x0030C200},
	{0xacc00B0,0x20454000},
	{0xacc00B8,0x00100010},
	{0xacc00BC,0x00060154},
	{0xacc00AC,0x12C20003},
	{0xacc00B4,0x05A73333},
	{0xacc0120,0x00000000},
	{0xacc0290,0x0000000C},		
	{PHY_RFIP_INVALID,0xffffffff} 
};
const  rf_reg Rf_RfPll_Table24[]=
{
	{0xacc0114,0x00000004},
#if 0
	{0xacc0288,0x30c208},
#else	
	{0xacc0288,0x0030C230},
#endif
	{0xacc00B0,0x20454000},
	{0xacc00B8,0x00100010},
	{0xacc00BC,0x000600CC},
	{0xacc00AC,0x12CA2003},
	{0xacc00B4,0x05A73333},
	{0xacc0120,0x00000000},
	{0xacc0290,0x0000000C},
	{PHY_RFIP_INVALID,0xffffffff}
};
const rf_reg Rf_RfPll_Table16[]=
{
	{0xacc0114,0x00000004},
	{0xacc0288,0x0030C230},
	{0xacc00B0,0x20454000},
	{0xacc00B8,0x00100010},
	//Acc00bc [9:2]//rfpll_calfreq_N1
	{0xacc00BC,0x00060098},//N1
	{0xacc00AC,0x12CA2003},
	{0xacc00B4,0x05A73333},
	{0xacc0120,0x00000000},
	{0xacc0290,0x0000000C},
	{0x16101020,0x78},
	{PHY_RFIP_INVALID,0xffffffff}
};
//static rfPll reg
const  rf_reg Rf_RfPll_Table[]=
{
#if (CFG_DPLL_CLOCK==40)		
		{0xacc0114,0x00000000},
		{0xacc0288,0x0030C200},
		{0xacc00B0,0x20454000},
		{0xacc00B8,0x00100010},
		{0xacc00BC,0x00060154},
		{0xacc00AC,0x12C20003},
		{0xacc00B4,0x05A73333},
		{0xacc0120,0x00000000},
		{0xacc0290,0x0000000C},		
		{PHY_RFIP_INVALID,0xffffffff} 
#elif(CFG_DPLL_CLOCK==24)   
		{0xacc0114,0x00000004},
#if 0
		{0xacc0288,0x30c208},
#else	
		{0xacc0288,0x0030C230},
#endif
		{0xacc00B0,0x20454000},
		{0xacc00B8,0x00100010},
		{0xacc00BC,0x000600CC},
		{0xacc00AC,0x12CA2003},
		{0xacc00B4,0x05A73333},
		{0xacc0120,0x00000000},
		{0xacc0290,0x0000000C},
		{PHY_RFIP_INVALID,0xffffffff}
#elif(CFG_DPLL_CLOCK==16)  
		{0xacc0114,0x00000004},
		{0xacc0288,0x0030C230},
		{0xacc00B0,0x20454000},
		{0xacc00B8,0x00100010},
		//Acc00bc [9:2]//rfpll_calfreq_N1
		{0xacc00BC,0x00060098},//N1
		{0xacc00AC,0x12CA2003},
		{0xacc00B4,0x05A73333},
		{0xacc0120,0x00000000},
		{0xacc0290,0x0000000C},
		{0x16101020,0x78},
		{PHY_RFIP_INVALID,0xffffffff}
#endif
};

u16 Rf_Channel_Table[]=
{	
2412 ,//ch1
2417 ,//ch2
2422 ,//ch3
2427 ,//ch4
2432 ,//ch5
2437 ,//ch6
2442 ,//ch7
2447 ,//ch8
2452 ,//ch9
2457 ,//ch10
2462 ,//ch11
2467 ,//ch12
2472 ,//ch13
2484  //ch14
};
		
u16 Rf_Special_Channel_Table[]=
{
2380 ,//ch23	
2308 ,//ch24
2320 ,//ch25	
2332 ,//ch26
2400 ,//ch27
2480 ,//ch28
2548 ,//ch29
2560 ,//ch30
2572 ,//ch31
2321 ,//ch32
2401 ,//ch33
2481, //ch34
2561, // ch35
};


/*
const NORELOC DTCM_CONST rf_reg_bit_Exvalue ROM_ro_Rf_Channel_TableAddr[]=
{
	{0xacc0114,0},
	{0xacc00AC,28},
	{0xacc00B0,22},
	{0xacc00B0,16},
	{0xacc00B0,18},
	{0xacc00B0,21},
	{0xacc00B0,20},
	{0xacc00AC,5},
	{0xacc0288,2},
	{0xacc0288,1},
	{0xacc00AC,1},
	{0xacc0290,7},
	{0xacc0120,25}
};
*/

const  u16 Rf_Channel_TableCfg[]=
{
#if (CFG_DPLL_CLOCK==40) // = (N1*freq_MHz*1.5/2/40); N1 is 85(dec);
	0xF04,	
	0xF0C,	
	0xF14,	
	0xF1C,	
	0xF24,	
	0xF2C,	
	0xF34,	
	0xF3C,	
	0xF44,	
	0xF4C,	
	0xF54,	
	0xF5C,	
	0xF64,	
	0xF77,	
	//0xED1,
#elif(CFG_DPLL_CLOCK==27)//27M
	0xE65,
	0xE6D,
	0xE74,
	0xE7C,
	0xE84,
	0xE8B,
	0xE93,
	0xE9A,
	0xEA2,
	0xEAA,
	0xEB1,
	0xEB9,
	0xEC1,
	0xED3,
#elif(CFG_DPLL_CLOCK==26)//26M
	0xEF3,
	0xEFB,
	0xF03,
	0xF0B,
	0xF12,
	0xF1A,
	0xF22,
	0xF2A,
	0xF32,
	0xF3A,
	0xF42,
	0xF4A,
	0xF52,
	0xF65,
#elif(CFG_DPLL_CLOCK==24)//24M  = (N1*freq_MHz*1.5/2/24); N1 is 51(dec);
	0xF04,
	0xF0C,
	0xF14,
	0xF1C,
	0xF24,
	0xF2C,
	0xF34,
	0xF3C,
	0xF44,
	0xF4C,
	0xF54,
	0xF5C,
	0xF64,
	0xF77,
#elif(CFG_DPLL_CLOCK==16)//24M
	0x10C8,
	0x10D1,
	0x10DA,
	0x10E3,
	0x10EC,
	0x10F5,
	0x10FE,
	0x1107,
	0x1110,
	0x1119,
	0x1121,
	0x112A,
	0x1133,
	0x1149,
	0x108F,
#elif(CFG_DPLL_CLOCK==12)//12M = (N1*freq_MHz*1.5/2/12); N1 is 48(dec);
	0x1C44,
	0x1C53,
	0x1C62,
	0x1C71,
	0x1C80,
	0x1C8F,
	0x1C9E,
	0x1CAD,
	0x1CBC,
	0x1CCB,
	0x1CDA,
	0x1CE9,
	0x1CF8,
	0x1D1C,
#else 
#error
#endif
};
/**************************AthenaBx End***************************************/

#endif


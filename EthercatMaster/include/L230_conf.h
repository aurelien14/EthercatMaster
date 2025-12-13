#pragma once
#include "soem/soem.h"


#define L230_TIME_OUT_PROCESS_DATA_DEF	1000	//100ms
#define L230_TIME_OUT_PROCESS_DATA		5000	//500ms, 0: watchdog désactivé


#pragma pack(push, 1)

typedef struct
{
	// RxPDO 0x1600 - L230_DO_Byte0
	union {
		uint8 L230_DO_Byte0;
		struct {
			uint8 X15_Out0 : 1;
			uint8 X12_Out1 : 1;
			uint8 X13_Out2 : 1;
			uint8 X3_Out3 : 1;
			uint8 X4_Out4 : 1;
			uint8 X14_Out5 : 1;
			uint8 X1_Out6 : 1;
			uint8 padding : 1;
		} L230_DO_Byte0_bits;
	};

	// RxPDO 0x1601 - L230_DO_Byte1
	union {
		uint8 L230_DO_Byte1;
		struct {
			uint8 X11_Out0 : 1;
			uint8 X10_Out1 : 1;
			uint8 X5_Out2 : 1;
			uint8 X6_Out3 : 1;
			uint8 X7_Out4 : 1;
			uint8 X8_Out5 : 1;
			uint8 X9_Out6 : 1;
			uint8 X8a_Out7 : 1;
		} L230_DO_Byte1_bits;
	};

	// RxPDO 0x1602 - X21_CPU_Pt1
	uint32 X21_CPU_Pt1_Ctrl;
	// RxPDO 0x1603 - X21_CPU_Pt2
	uint32 X21_CPU_Pt2_Ctrl;
	// RxPDO 0x1604 - X21_CPU_VC1
	uint32 X23_CPU_VC1_Ctrl;
} L230_RX_PDO_t;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct
{
	// TxPDO 0x1A00 - L230_DI_Byte0
	struct {
		uint8 X30_2_In0 : 1;	//0x6080
		uint8 X30_4_In1 : 1;	//0x6090
		uint8 X30_6_In2 : 1;	//0x60A0
		uint8 X30_8_In3 : 1;	//0x60B0
		uint8 X30_11_In4 : 1;	//0x60C0
		uint8 X30_13_In5 : 1;	//0x60D0
		uint8 X30_19_In6 : 1;	//0x60E0
		uint8 X30_21_In7 : 1;	//0x60F0
	} L230_DI_Byte0;

	// TxPDO 0x1A01 - L230_DO_Byte1
	struct {
		uint8 X55_In0 : 1;
		uint8 padding : 7;
	} L230_DI_Byte1;

	// TxPDO 0x1A02 - X21_CPU_Pt1 
	struct {
		float X21_CPU_Pt1_Value;	//0x63A0 (value in ohm)
		uint32 X21_CPU_Pt1_State;	//0x63B0 (Bit 0 Error PT100)
	} X21_CPU_Pt1;


	// TxPDO 0x1A03 - X21_CPU_Pt2
	struct {
		float X21_CPU_Pt2_Value;	//0x63C0 (value in ohm)
		uint32 X21_CPU_Pt2_State;	//0x63D0 (Bit 0 Error PT100)
	} X21_CPU_Pt2;

	// TxPDO 0x1A04 - X21_CPU_VC1 
	struct {
		float X23_CPU_VC1_Value;	//0x63C0 (value in V/mA)
		uint32 X23_CPU_VC1_State;	//0x69C0 (Bit 0 Error U/I)
	} X21_CPU_VC1;


	// TxPDO 0x1A05 - L230_DO_Byte1
	struct {
		uint32 ProductCode;
		uint32 SerialNumber;
		uint32 HW_Version;
		uint32 FW_Version;
	} slaveinfo;

	struct {
		uint32 L230_SerialNumber;
		uint32 L230_HW_Version;
		uint32 L230_FW_Version;
		uint32 L230_State;
	} L230Info;
} L230_TX_PDO_t;
#pragma pack(pop)




#define L230_OUTPUT(slave)  ((L230_RX_PDO_t*) (slave)->pdo.rx_buf[atomic_load(&(slave)->pdo.active_rx_app)])
#define L230_INPUT(slave)  ((L230_TX_PDO_t*) (slave)->pdo.tx_buf[atomic_load(&(slave)->pdo.active_rx_app)])



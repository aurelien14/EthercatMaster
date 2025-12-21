#pragma once
#include <stdint.h>

static_assert(sizeof(float) == 4, "float must be 32-bit");

#define L230_TIME_OUT_PROCESS_DATA_DEF	1000	//100ms
#define L230_TIME_OUT_PROCESS_DATA		5000	//500ms, 0: watchdog désactivé

#define X15_out (1<< 0)
#define X12_out (1<< 1)
#define X13_out (1<< 2)
#define X3_out  (1<< 3)
#define X4_out  (1<< 4)
#define X14_out (1<< 5)
#define X1_out  (1<< 6)
#define X11_out (1<< 0)
#define X10_out (1<< 1)
#define X5_out  (1<< 2)
#define X6_out  (1<< 3)
#define X7_out  (1<< 4)
#define X8_out  (1<< 5)
#define X9_out  (1<< 6)
#define X8a_out (1<< 7)

#define X30_2_in   (1<< 0)
#define X30_4_in   (1<< 1)
#define X30_6_in   (1<< 2)
#define X30_8_in   (1<< 3)
#define X30_11_in  (1<< 4)
#define X30_13_in  (1<< 5)
#define X30_19_in  (1<< 6)
#define X30_21_in  (1<< 7)
#define X55_in     (1<< 0)

#define L230_BYTE0_OFFSET 0
#define L230_BYTE1_OFFSET 1


#pragma pack(push, 1)

typedef struct
{
	// RxPDO 0x1600 - L230_DO_Byte0
	union {
		uint8_t L230_DO_Byte0;
		struct {
			uint8_t X15_Out0 : 1;
			uint8_t X12_Out1 : 1;
			uint8_t X13_Out2 : 1;
			uint8_t X3_Out3 : 1;
			uint8_t X4_Out4 : 1;
			uint8_t X14_Out5 : 1;
			uint8_t X1_Out6 : 1;
			uint8_t padding : 1;
		} L230_DO_Byte0_bits;
	};

	// RxPDO 0x1601 - L230_DO_Byte1
	union {
		uint8_t L230_DO_Byte1;
		struct {
			uint8_t X11_Out0 : 1;
			uint8_t X10_Out1 : 1;
			uint8_t X5_Out2 : 1;
			uint8_t X6_Out3 : 1;
			uint8_t X7_Out4 : 1;
			uint8_t X8_Out5 : 1;
			uint8_t X9_Out6 : 1;
			uint8_t X8a_Out7 : 1;
		} L230_DO_Byte1_bits;
	};

	// RxPDO 0x1602 - X21_CPU_Pt1
	uint32_t X21_CPU_Pt1_Ctrl;
	// RxPDO 0x1603 - X21_CPU_Pt2
	uint32_t X21_CPU_Pt2_Ctrl;
	// RxPDO 0x1604 - X21_CPU_VC1
	uint32_t X23_CPU_VC1_Ctrl;
} L230_RX_PDO_t;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct
{
	// TxPDO 0x1A00 - L230_DI_Byte0
	struct {
		uint8_t X30_2_In0 : 1;	//0x6080
		uint8_t X30_4_In1 : 1;	//0x6090
		uint8_t X30_6_In2 : 1;	//0x60A0
		uint8_t X30_8_In3 : 1;	//0x60B0
		uint8_t X30_11_In4 : 1;	//0x60C0
		uint8_t X30_13_In5 : 1;	//0x60D0
		uint8_t X30_19_In6 : 1;	//0x60E0
		uint8_t X30_21_In7 : 1;	//0x60F0
	} L230_DI_Byte0;

	// TxPDO 0x1A01 - L230_DO_Byte1
	struct {
		uint8_t X55_In0 : 1;
		uint8_t padding : 7;
	} L230_DI_Byte1;

	// TxPDO 0x1A02 - X21_CPU_Pt1 
	struct {
		float Pt_Value;	//0x63A0 (value in ohm)
		uint32_t Pt_State;	//0x63B0 (Bit 0 Error PT100)
	} X21_CPU_Pt1;


	// TxPDO 0x1A03 - X21_CPU_Pt2
	struct {
		float Pt_Value;	//0x63C0 (value in ohm)
		uint32_t Pt_State;	//0x63D0 (Bit 0 Error PT100)
	} X22_CPU_Pt2;

	// TxPDO 0x1A04 - X21_CPU_VC1 
	struct {
		float VC_Value;	//0x63C0 (value in V/mA)
		uint32_t VC_State;	//0x69C0 (Bit 0 Error U/I)
	} X23_CPU_VC1;


	// TxPDO 0x1A05 - L230_DO_Byte1
	struct {
		uint32_t ProductCode;
		uint32_t SerialNumber;
		uint32_t HW_Version;
		uint32_t FW_Version;
	} slaveinfo;

	struct {
		uint32_t L230_SerialNumber;
		uint32_t L230_HW_Version;
		uint32_t L230_FW_Version;
		uint32_t L230_State;
	} L230Info;

} L230_TX_PDO_t;
#pragma pack(pop)



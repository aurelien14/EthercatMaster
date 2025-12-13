#include "plc_functions.h"
#include "ethercat_slaves.h"
#include "L230_conf.h"





typedef enum DATA_TYPE {
	PLC_BOOL, PLC_BYTE, PLC_WORD, PLC_DWORD, PLC_REAL, PLC_STRING
};

typedef enum DATA_TYPE2 {
	IO, PV
};

typedef struct IOTag {
	const char name[32];
	DATA_TYPE type;
	DATA_TYPE2 type2;
	void* ptr;
}IOTag_t;

typedef struct Conf_IO_ethercat {
	IOTag_t X15;
	IOTag_t X13;
	IOTag_t X12;
	IOTag_t X11;
	IOTag_t X10;
	IOTag_t X9;
	IOTag_t X8;
	IOTag_t X8a;
	IOTag_t X7;
	IOTag_t X6;
	IOTag_t X5;
	IOTag_t X4;
	IOTag_t X3;
	IOTag_t X1;
	IOTag_t AV_CPU_Pt_X21;
	IOTag_t AV_CPU_Pt_X22;
} Conf_IO_ethercat_t;



typedef struct MyPLCApp_conf {
	L230_TX_PDO_t* L230_inputs;
	L230_RX_PDO_t* L230_outputs;

	//Conf_IO_ethercat IO_Conf;

	PLC_timer_t timer1;
	PLC_timer_t timer2;
}MyPLCApp_conf_t;

void task_app1(PLCTask_t* self);
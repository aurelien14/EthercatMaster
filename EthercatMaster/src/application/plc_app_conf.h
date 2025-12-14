#include "plc_functions.h"
#include "L230_conf.h"
#include "plc_io.h"



typedef struct Conf_IO_ethercat {
	IOTag_t X15;
	IOTag_t X14;
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
	IOTag_t DI0;
	IOTag_t DI1;
	IOTag_t DI2;
	IOTag_t DI3;
	IOTag_t DI4;
	IOTag_t DI5;
	IOTag_t DI6;
	IOTag_t DI7;
	IOTag_t X55;
	IOTag_t X21_CPU_Pt1;
	IOTag_t X22_CPU_Pt2;
	IOTag_t AV_CPU_Pt_X21;
	IOTag_t AV_CPU_Pt_X22;
	IOTag_t AV_CPU_VC_X23;
} Conf_IO_ethercat_t;



typedef struct MyPLCApp_conf {
	L230_TX_PDO_t* L230_inputs;
	L230_RX_PDO_t* L230_outputs;

	Conf_IO_ethercat_t IO_Conf;

	PLC_timer_t timer1;
	PLC_timer_t timer2;
} MyPLCApp_conf_t;


void task_app1(PLCTask_t* self);
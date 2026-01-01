#include "config/system_config.h"
#include "backend/ethercat/ethercat_desc.h"
#include "devices/L230/L230_device_desc.h"
#include "devices/L230/L230_ethercat_pdo.h"
//#include "devices/DAIO16/daio16_device_desc.h"
//#include "devices/DriveX/drive_modbus_desc.h"
//#include "core/backend/backend.h"
#include "core/tags/tags_desc.h"

/*--------------------------------------------------*/
/*			D R I V E R S    C O N F				*/
/*--------------------------------------------------*/
static const BackendConfig_t system_backends[] =
{
	{
		.name = "ec0",
		.driver_desc = &ETHERCAT_DRIVER_DESC,
		.ethercat = {
			.ifname = "\\Device\\NPF_{FB092E67-7CA1-4E8F-966D-AC090D396487}",
			.cycle_us = 10000,
			.io_map_size = 4096,
			.has_dc_clock = false,
		}
	},
};

/*--------------------------------------------------*/
/*			D E V I C E S    C O N F				*/
/*--------------------------------------------------*/
static const DeviceConfig_t system_devices[] = {
	{
		.device_name = "DEVICE_1",
		.plc_address = 0,
		.device_desc = &L230_DEVICE_DESC,
		.backend_name = "ec0",
		.ethercat = {
			.expected_position = 1,
		}
	},

#if 0
	{
		.device_name = "ExtIO1",
		.device_desc = &DAIO16_DEVICE_DESC,
		.backend = BACKEND_ETHERCAT,
		.ethercat = {
			.expected_position = 2
		}
	},
	{
		.device_name = "Drive1",
		.device_desc = &DRIVE_X_MODBUS_DESC,
		.backend = BACKEND_MODBUS,
		.modbus = {
			.slave_id = 5
		}
	}
#endif

};


/*--------------------------------------------------*/
/*			P L C  T A S K    C O N F				*/
/*--------------------------------------------------*/
extern int plc_task1_run(void* ctx);
static const PLC_TaskConfig_t PLC_Task_conf[] = {
	{
		.name = "FAST",
		.period_ms = 100,
		.run = plc_task1_run,
		.policy = PLC_TASK_SKIP_ON_FAULT,
	},
};



/*--------------------------------------------------*/
/*			P L C  T A G S   C O N F				*/
/*--------------------------------------------------*/
static const PLC_Variables_Config_t PLC_Tags_desc_arr[] = {
	{//0
		.name = "AV_CPU_Pt_X21",
		.dtype = PLC_REAL,
		.vtype = PLC_IN,
		.device_addr = 0,
		.offset = L230_IN(X21_CPU_Pt1.Pt_Value),
	},
	{//1
		.name = "AV_CPU_Pt_X22",
		.dtype = PLC_REAL,
		.vtype = PLC_IN,
		.device_addr = 0,
		.offset = L230_IN(X22_CPU_Pt2.Pt_Value),
	},
	{//2
		.name = "X15_1",
		.dtype = PLC_BOOL,
		.vtype = PLC_OUT,
		.device_addr = 0,
		.offset = L230_OUT(L230_DO_Byte0),
		.bit = X15_BIT
	},
	{//3
		.name = "X12_1",
		.dtype = PLC_BOOL,
		.vtype = PLC_OUT,
		.device_addr = 0,
		.offset = L230_OUT(L230_DO_Byte0),
		.bit = X12_BIT
	},
	{//4
		.name = "X13_1",
		.dtype = PLC_BOOL,
		.vtype = PLC_OUT,
		.device_addr = 0,
		.offset = L230_OUT(L230_DO_Byte0),
		.bit = X13_BIT
	},
	{//5
		.name = "X1_1",
		.dtype = PLC_BOOL,
		.vtype = PLC_OUT,
		.device_addr = 0,
		.offset = L230_OUT(L230_DO_Byte0),
		.bit = X1_BIT
	},
	{
		.name = "AV_CPC_HP",
		.dtype = PLC_REAL,
		.vtype = PLC_HMI,
	},
	{
		.name = "SP_Temperature",
		.dtype = PLC_REAL,
		.vtype = PLC_HMI,
		.initial_value.real_value = 23.0f,
	},
};

#define X15	2
#define X12	3
#define X13	4

/*--------------------------------------------------*/
/*			G L O B A L  P L C    C O N F			*/
/*--------------------------------------------------*/
const PLCSystemConfig_t PLC_SYSTEM_CONFIG = {
	.backends = (BackendConfig_t*)system_backends,
	.backend_count = sizeof(system_backends) / sizeof(system_backends[0]),

	.devices = (DeviceConfig_t*)system_devices,
	.device_count = sizeof(system_devices) / sizeof(system_devices[0]),

	.PLC_Task_conf = PLC_Task_conf,
	.plc_task_count = sizeof(PLC_Task_conf) / sizeof(PLC_Task_conf[0]),

	.plc_variables = PLC_Tags_desc_arr,
	.plc_variables_count = sizeof(PLC_Tags_desc_arr) / sizeof(PLC_Tags_desc_arr[0])
};
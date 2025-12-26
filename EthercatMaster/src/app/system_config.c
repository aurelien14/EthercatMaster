#include "config/system_config.h"
#include "devices/L230/L230_device_desc.h"
#include "devices/L230/L230_ethercat_pdo.h"
//#include "devices/DAIO16/daio16_device_desc.h"
//#include "devices/DriveX/drive_modbus_desc.h"
#include "core/backend/backend.h"
#include "core/plc/tags_desc.h"

/*--------------------------------------------------*/
/*			D R I V E R S    C O N F				*/
/*--------------------------------------------------*/
static const BackendConfig_t system_backends[] =
{
	{
		.type = PROTO_ETHERCAT,
		.name = "ec0",
		.ethercat = {
			.ifname = "\\Device\\NPF_{FB092E67-7CA1-4E8F-966D-AC090D396487}",
			.cycle_us = 1000,
			.io_map_size = 4096,
		}
	},
};

/*--------------------------------------------------*/
/*			D E V I C E S    C O N F				*/
/*--------------------------------------------------*/
static const DeviceConfig_t system_devices[] = {
	{
		.device_name = "L230_1",
		.device_desc = &L230_DEVICE_DESC,
		.backend_name = "ec0",
		.ethercat = {
			.expected_position = 1
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
/*			P L C  T A G S   C O N F				*/
/*--------------------------------------------------*/
static const PLC_TagDesc_Config_t PLC_Tags_desc_arr[] = {
	{
		.name = "AV_CPU_Pt_X21",
		.dtype = PLC_REAL,
		.vtype = PLC_IN,
		.device_name = "L230_1",
		.offset = L230_IN(X21_CPU_Pt1.Pt_Value),
	},
	{
		.name = "AV_CPU_Pt_X22",
		.dtype = PLC_REAL,
		.vtype = PLC_IN,
		.device_name = "L230_1",
		.offset = L230_IN(X22_CPU_Pt2.Pt_Value),
	},
	{
		.name = "X15_1",
		.dtype = PLC_BOOL,
		.vtype = PLC_OUT,
		.device_name = "L230_1",
		.offset = L230_OUT(L230_DO_Byte0),
		.bit = X15_BIT
	},
	{
		.name = "X12_1",
		.dtype = PLC_BOOL,
		.vtype = PLC_OUT,
		.device_name = "L230_1",
		.offset = L230_OUT(L230_DO_Byte0),
		.bit = X12_BIT
	},
	{
		.name = "AV_CPC_HP",
		.dtype = PLC_REAL,
		.vtype = PLC_HMI,
	},
	{
		.name = "SP_Temperature",
		.dtype = PLC_REAL,
		.vtype = PLC_HMI
	},
};




/*--------------------------------------------------*/
/*			G L O B A L  P L C    C O N F			*/
/*--------------------------------------------------*/
const PLCSystemConfig_t PLC_SYSTEM_CONFIG = {
	.backends = (BackendConfig_t*)system_backends,
	.backend_count = sizeof(system_backends) / sizeof(system_backends[0]),

	.devices = (DeviceConfig_t*)system_devices,
	.device_count = sizeof(system_devices) / sizeof(system_devices[0]),

	.plc_tags_desc = PLC_Tags_desc_arr,
	.plc_tags_count = sizeof(PLC_Tags_desc_arr) / sizeof(PLC_Tags_desc_arr[0])
};
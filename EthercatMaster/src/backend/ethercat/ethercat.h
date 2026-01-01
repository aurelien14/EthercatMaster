#pragma once
#include "config/config.h"
//#include "config/system_config.h"
#include "core/backend/backend.h"
#include "ethercat_device.h"
#include <soem/soem.h>



typedef struct Ethercat_Stats {
	uint32_t cycle_time_us;
	uint32_t min_cycle_time_us;
	uint32_t max_cycle_time_us;
	uint32_t jitter_us;
	uint32_t min_jitter_us;
	uint32_t max_jitter_us;
	uint64_t total_cycles;
} Ethercat_Stats_t;

typedef struct {
	uint8_t	in_op;			// 0/1
	uint8_t	fault_latched;		// 0/1
	uint16_t ethercat_state;		// EC_STATE_*

	uint32_t wkc;
	uint32_t expected_wkc;
	uint32_t dowkccheck;

	uint32_t slavecount;
	uint32_t lost_count;		// combien islost
	uint32_t not_op_count;		// combien state != OP

	uint64_t total_cycles;

#if defined(ETHERCAT_JITTER_CALC)
	uint32_t jitter_us;
	uint32_t max_jitter_us;
#endif
} EtherCAT_Status_t;



typedef struct EtherCAT_Driver {
	BackendDriver_t base;

	ecx_contextt ctx;
	uint8_t* iomap;
	size_t iomap_size;
	uint8_t has_dc_clock;

	EtherCAT_Device_t* slaves[ECAT_MAX_SLAVES];
	size_t slave_count;

	//ethcat state
	EtherCAT_Status_t status;
	atomic_i32_t in_op;			// 0/1
	atomic_i32_t fault_latched;		// 0/1
	atomic_i32_t ethercat_state;		// EC_STATE_*
	uint32_t counter_fault;

	uint32_t wkc;
	uint32_t expected_wkc;
	uint32_t dowkccheck;
	atomic_i32_t reset_req;     // 0/1 (mis à 1 par HMI/PLC)


	//Double buffuring
	atomic_i32_t active_out_buffer_idx;
	atomic_i32_t active_in_buffer_idx;
	atomic_i32_t rt_out_buffer_idx;

	//thread ethercat
	atomic_i32_t running;
	atomic_i32_t soem_lock;
	uint32_t thread_cycle_us; //temps du cycle thread ethercat en microsecondes
	OSAL_THREAD_HANDLE rt_thread;
	OSAL_THREAD_HANDLE watchdog_thread;
	LARGE_INTEGER qpc_freq;
	LARGE_INTEGER next_deadline;

	Ethercat_Stats_t stats;
} EtherCAT_Driver_t;



BackendDriver_t* EtherCAT_Driver_Create(const BackendConfig_t* config, int instance_index);
void EtherCAT_Driver_Destroy(BackendDriver_t* b);


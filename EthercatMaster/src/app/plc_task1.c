#include "core/scheduler/task.h"
#include "plc_tasks.h"
#include "core/runtime/runtime.h"
#include "backend/ethercat/ethercat.h"
#include "devices/L230/L230_ethercat_pdo.h"

int plc_task1_run(void* ctx) {
	Runtime_t* r = (Runtime_t*)ctx;

	//if(r.)

	// Exemple de tâche PLC qui effectue une opération simple
	PLC_Task_t* task = (PLC_Task_t*)ctx;
	static int counter = 0;

	EtherCAT_Driver_t* ecat = (EtherCAT_Driver_t * )r->backends[0];
	EtherCAT_SlaveRuntime_t* ecat_r0 = ecat->slaves[0];
	L230_RX_PDO_t* L230_OUTPUT = ecat_r0->rx_pdo;
	L230_OUTPUT->L230_DO_Byte0_bits.X15_Out0 = 1;


	counter++;
	return 0; // Retourner 0 pour indiquer le succès
}
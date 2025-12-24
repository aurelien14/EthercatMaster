#include "core/scheduler/task.h"
#include "plc_tasks.h"

int plc_task1_run(void* ctx) {
	// Exemple de tâche PLC qui effectue une opération simple
	PLC_Task_t* task = (PLC_Task_t*)ctx;
	static int counter = 0;
	counter++;
	return 0; // Retourner 0 pour indiquer le succès
}
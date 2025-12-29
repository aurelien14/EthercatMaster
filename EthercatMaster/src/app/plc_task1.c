#include "core/scheduler/task.h"
#include "plc_tasks.h"
#include "core/runtime/runtime.h"
#include "backend/ethercat/ethercat.h"
#include "devices/L230/L230_ethercat_pdo.h"
#include "core/plc/tags.h"

int plc_task1_run(void* ctx) {
	Runtime_t* r = (Runtime_t*)ctx;
	static int counter = 0;
	static int value = 0;
	// Exemple de tâche PLC qui effectue une opération simple
	PLC_Task_t* task = (PLC_Task_t*)ctx;
	PLC_Tag_t* x15 = &r->tags[2];
	if (counter > 10) {
		value = !value;
		plc_tag_write(x15, value);
		counter = 0;
		printf("Value=%d\n", value);
		fflush(stdout);
	}
		counter++;
	
	return 0; // Retourner 0 pour indiquer le succès
}
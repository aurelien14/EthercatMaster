#include "core/scheduler/task.h"
#include "core/runtime/runtime.h"
#include <stdio.h>

int plc_task1_run(void* ctx) {
	Runtime_t* r = (Runtime_t*)ctx;
	static int counter = 0;
	static boolean value = 0;
	// Exemple de tâche PLC qui effectue une opération simple
	PLC_Task_t* task = (PLC_Task_t*)ctx;
	PLC_Variable_t* x15 = &r->tags[2];
	if (counter > 10) {
		value = !value;
		plc_tag_write(x15, &value);
		counter = 0;
		printf("Value=%d\n", value);
		fflush(stdout);
	}
		counter++;
	
	return 0; // Retourner 0 pour indiquer le succès
}
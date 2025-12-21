#include <stdio.h>
#include "config/plc_config.h"
#include "core/runtime/runtime.h"

int main(int argc, char* argv[]) {
	Runtime_t* runtime = create_runtime();
	if (runtime == NULL) {
		return -1;
	}

	int ret = runtime_init(runtime, &plc_system_config);
	if (ret < 0) {
		return -1;
	}

	runtime_start();

	return 0;
}
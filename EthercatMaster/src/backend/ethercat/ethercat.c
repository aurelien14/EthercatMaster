#include "ethercat.h"
#include "ethercat_slave_runtime.h"
#include <stdlib.h>
#include <stdio.h>

static void print_available_adapters(void) {
	ec_adaptert* adapter = NULL;
	ec_adaptert* head = NULL;

	printf("	Available adapters:\n");
	head = adapter = ec_find_adapters();
	while (adapter != NULL)
	{
		printf("		- %s  (%s)\n", adapter->name, adapter->desc);
		adapter = adapter->next;
	}
	ec_free_adapters(head);
}


static int ethercat_init(BackendDriver_t* b)
{
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;

	if (ecx_config_init(&d->ctx) <= 0)
		return -1;

	if (d->ctx.slavecount != d->slave_count)
		return -1;

	for (size_t i = 0; i < d->slave_count; i++) {

		Device_t* dev = b->devices[i];

		const DeviceProtocolDesc_t* pdesc =
			device_find_protocol_desc(dev->desc, PROTO_ETHERCAT);

		const EtherCAT_DeviceDesc_t* ecat_desc =
			(const EtherCAT_DeviceDesc_t*)pdesc->hw_desc;

		ec_slavet* s = &d->ctx.slavelist[i + 1];

		if (s->eep_man != ecat_desc->vendor_id ||
			s->eep_id != ecat_desc->product_code)
			return -1;

		if (ecat_desc->config_hook)
			ecat_desc->config_hook(i + 1);

		// liaison PDO
		...
	}

	return 0;
}


static int ethercat_process(BackendDriver_t* b) {
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;
	ecx_send_processdata(&d->ctx);
	ecx_receive_processdata(&d->ctx, EC_TIMEOUTRET);
	return 0;
}

static void ethercat_destroy(BackendDriver_t* b) {

	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;
	free(d->iomap);
	for (int i = 0; i < d->slave_count; i++) {
		free(d->slaves[i]);
	}
	free(d);
}

static BackendDriverOps_t ops = {
	.init = ethercat_init,
	.process = ethercat_process,
	.destroy = ethercat_destroy
};


EtherCAT_Driver_t* EtherCAT_Driver_Create(const char* ifname) {
	EtherCAT_Driver_t* d = calloc(1, sizeof(*d));
	if (d == NULL) {
		return NULL;
	}
	d->base.name = "EtherCAT";
	d->base.ops = &ops;
	d->base.is_initialized = 0;
	d->base.ref_count = 0;

	if (!ecx_init(&d->ctx, ifname)) {
		printf("[%s] Failed to initialize EtherCAT context on interface %s\n", d->base.name, ifname);
		print_available_adapters();
		free(d);
		return NULL;
	}
	return d;
}

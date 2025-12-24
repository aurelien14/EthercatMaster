#include "ethercat.h"

#include "ethercat_device_desc.h"
#include "ethercat_thread.h"
#include "config/config.h"
#include <stdlib.h>
#include <stdio.h>


int ethercat_transition_safeop(EtherCAT_Driver_t* ec)
{
	printf("[ECAT] Passage en SAFE-OP...\n");

	ec->ctx.slavelist[0].state = EC_STATE_SAFE_OP;
	ecx_writestate(&ec->ctx, 0);

	int chk = 200;
	do {
		ecx_statecheck(&ec->ctx, 0, EC_STATE_SAFE_OP, 50000);
	} while (chk-- &&
		(ec->ctx.slavelist[0].state != EC_STATE_SAFE_OP));

	if (ec->ctx.slavelist[0].state != EC_STATE_SAFE_OP) {
		printf("[ECAT] ERREUR SAFE-OP\n");
		for (int i = 1; i <= ec->ctx.slavecount; i++) {
			if (ec->ctx.slavelist[i].state != EC_STATE_SAFE_OP) {
				printf("  Slave %d bloqué en 0x%02x\n",
					i, ec->ctx.slavelist[i].state);
			}
		}
		return -1;
	}

	printf("[ECAT] Tous les esclaves en SAFE-OP\n");
	return 0;
}


int ethercat_transition_op(EtherCAT_Driver_t* ec)
{
	printf("[ECAT] Mise à zéro des sorties...\n");

	memset(ec->iomap, 0, ec->iomap_size);

	for (int i = 0; i < 3; i++) {
		ecx_send_processdata(&ec->ctx);
		ecx_receive_processdata(&ec->ctx, EC_TIMEOUTRET);
		Sleep(2);
	}

	printf("[ECAT] Passage en OPERATIONAL...\n");

	ec->ctx.slavelist[0].state = EC_STATE_OPERATIONAL;
	ecx_writestate(&ec->ctx, 0);

	int chk = 200;
	do {
		ecx_send_processdata(&ec->ctx);
		ecx_receive_processdata(&ec->ctx, EC_TIMEOUTRET);
		ecx_statecheck(&ec->ctx, 0,
			EC_STATE_OPERATIONAL, 50000);
	} while (chk-- &&
		(ec->ctx.slavelist[0].state != EC_STATE_OPERATIONAL));

	if (ec->ctx.slavelist[0].state != EC_STATE_OPERATIONAL) {
		printf("[ECAT] ERREUR OPERATIONAL\n");
		for (int i = 1; i <= ec->ctx.slavecount; i++) {
			if (ec->ctx.slavelist[i].state != EC_STATE_OPERATIONAL) {
				printf("  Slave %d bloqué en 0x%02x\n",
					i, ec->ctx.slavelist[i].state);
			}
		}
		return -1;
	}

	printf("[ECAT] Tous les esclaves en OPERATIONAL\n");
	return 0;
}


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


static int ethercat_init(BackendDriver_t* b) {
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;

	if (!QueryPerformanceFrequency(&d->qpc_freq)) {
		printf("[ECAT] QueryPerformanceFrequency failed\n");
		return -1;
	}

	if (d->qpc_freq.QuadPart == 0) {
		printf("[ECAT] Invalid QPC frequency\n");
		return -1;
	}

	printf("[ECAT] QPC freq = %lld Hz\n", d->qpc_freq.QuadPart);

	if (ecx_config_init(&d->ctx) <= 0) {
		printf("[ECAT] No slaves found\n");
		return -1;
	}
	
	d->iomap = calloc(1, d->iomap_size);
	if (!d->iomap) {
		printf("[ECAT] Unable to allocate IOmap\n");
		return -1;
	}

	printf("[ECAT] %d slaves detected\n", d->ctx.slavecount);
	return 0;
}


static int ethercat_start(BackendDriver_t* b)
{
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;

	//ethercat_transition_safeop(d); //fait dans ethercat_finalize_mapping
	ethercat_transition_op(d);

	ethercat_start_thread(d);
	return 0;
}

static int ethercat_stop(BackendDriver_t* b)
{
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;
	ethercat_stop_thread(d);

	d->ctx.slavelist[0].state = EC_STATE_SAFE_OP;
	ecx_writestate(&d->ctx, 0);
	ecx_statecheck(&d->ctx, 0, EC_STATE_SAFE_OP, 200000);

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
	for (size_t i = 0; i < d->slave_count; i++)
		free(d->slaves[i]);
	free(d);
}


static const BackendDriverOps_t ops = {
	.init = ethercat_init,
	.start = ethercat_start,
	.stop = ethercat_stop,
	.process = ethercat_process,
	.destroy = ethercat_destroy
};


EtherCAT_Driver_t* EtherCAT_Driver_Create(EtherCAT_config_t *config, int ethercat_index)
{
	EtherCAT_Driver_t* d = calloc(1, sizeof(*d));
	if (!d)
		return NULL;
	snprintf(d->base.name, sizeof(d->base.name), "ec%d", ethercat_index);
	d->base.protocol = PROTO_ETHERCAT;
	d->base.ops = &ops;
	d->thread_cycle_us = config->cycle_us;
	d->iomap_size = config->io_map_size;

	if (!ecx_init(&d->ctx, config->ifname)) {
		free(d);
		return NULL;
	}
	return d;
}


int ethercat_finalize_mapping(EtherCAT_Driver_t* ec) {
	int size = ecx_config_map_group(&ec->ctx, ec->iomap, 0);

	if (size == 0) {
		printf("[CONFIG] erreur pendant le mapping");
		return -1;
	}
	if (size > ec->iomap_size) {
		printf("[CONFIG] tableau IOmap trop petit, taille:%llu, requière:%d", (unsigned long long)ec->iomap_size, size);
		return -1;
	}

	for (int i = 0; i < ec->slave_count; i++) {
		EtherCAT_SlaveRuntime_t* ecat_rt = ec->slaves[i];
		ec_slavet* sl = &ec->ctx.slavelist[i + 1];
		ecat_rt->tx_pdo = sl->inputs;
		ecat_rt->rx_pdo = sl->outputs;
	}

	ecx_configdc(&ec->ctx);

	ethercat_transition_safeop(ec);
	return 0;
}


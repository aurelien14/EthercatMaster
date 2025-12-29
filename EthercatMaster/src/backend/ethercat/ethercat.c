#include "ethercat.h"
#include "ethercat_device_desc.h"
#include "ethercat_thread.h"
#include "config/system_config.h"
#include "core/system/memalloc.h"
#include <stdio.h>
#include "core/plateform/plateform.h"


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

	printf("Available adapters:\n");
	head = adapter = ec_find_adapters();
	while (adapter != NULL)
	{
		printf("	- %s  (%s)\n", adapter->name, adapter->desc);
		adapter = adapter->next;
	}
	ec_free_adapters(head);
}


static void ethercat_stats_init(EtherCAT_Driver_t* d)
{
	d->stats.cycle_time_us = d->thread_cycle_us;
	d->stats.min_cycle_time_us = UINT32_MAX;
	d->stats.max_cycle_time_us = 0;

	d->stats.min_jitter_us = UINT32_MAX;
	d->stats.max_jitter_us = 0;

	d->stats.jitter_us = 0;
	d->stats.total_cycles = 0;
}

static void ethercat_create_device_buffers() {

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

	ethercat_stats_init(d);

	if (ecx_config_init(&d->ctx) <= 0) {
		printf("[ECAT] No slaves found\n");
		return -1;
	}
	
	d->iomap = CALLOC(1, d->iomap_size);
	if (!d->iomap) {
		printf("[ECAT] Unable to allocate IOmap\n");
		return -1;
	}

	printf("[ECAT] %d slaves detected\n", d->ctx.slavecount);
	return 0;
}


static int ethercat_finalize_mapping(BackendDriver_t* drv) {
	sleep(1000);
	EtherCAT_Driver_t* ec = (EtherCAT_Driver_t*)drv;
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
		EtherCAT_Device_t* ec_dev = ec->slaves[i];
		int pos = ec_dev->slave_index;

		ec_slavet* sl = &ec->ctx.slavelist[pos];

		// On mémorise où sont les données dans l'IOmap de SOEM
		ec_dev->soem_inputs = sl->inputs;
		ec_dev->soem_outputs = sl->outputs;

		// On récupère les tailles (SOEM les remplit après config_map)
		ec_dev->rx_size = sl->Ibytes;
		ec_dev->tx_size = sl->Obytes;

		// ALLOCATION DU DOUBLE BUFFERING
		if (ec_dev->rx_size > 0) {
			ec_dev->rx_buffers[0] = CALLOC(1, ec_dev->rx_size);
			ec_dev->rx_buffers[1] = CALLOC(1, ec_dev->rx_size);
			atomic_store_i32(&ec_dev->active_rx_idx, 0);
		}
		if (ec_dev->tx_size > 0) {
			ec_dev->tx_buffers[0] = CALLOC(1, ec_dev->tx_size);
			ec_dev->tx_buffers[1] = CALLOC(1, ec_dev->tx_size);
			atomic_store_i32(&ec_dev->active_tx_idx, 0);
		}
	}

	if(ec->has_dc_clock) ecx_configdc(&ec->ctx);

	ethercat_transition_safeop(ec);
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
	//ecx_send_processdata(&d->ctx);
	//ecx_receive_processdata(&d->ctx, EC_TIMEOUTRET);
	return 0;
}


static int ethercat_bind_device(BackendDriver_t* d, Device_t* dev, const DeviceConfig_t* dev_conf)
{
	EtherCAT_Driver_t* ed = (EtherCAT_Driver_t*)d;

	ecx_contextt* ecx = &ed->ctx;
	int pos = dev_conf->ethercat.expected_position;
	if (pos > ecx->slavecount) {//ecat slave start à index 1
		printf("[ECAT] error in configuration ethercat position %d, slave count: %d\n", pos, ecx->slavecount);
		return -1;
	}

	ec_slavet* sl = &ed->ctx.slavelist[pos];

	EtherCAT_Device_t* ec_dev = (EtherCAT_Device_t*)dev;

	const DeviceDesc_t* d_desc = (DeviceDesc_t*)dev->desc;
	const EtherCAT_DeviceDesc_t* hw_desc = (EtherCAT_DeviceDesc_t*)d_desc->hw_desc;

	/* Vérification identité */
	if (sl->eep_man != hw_desc->product_code ||
		sl->eep_id != hw_desc->vendor_id) {
		printf("[ECAT] mismatch at pos %d\n", pos);
		return -1;
	}

	if(hw_desc->config_hook)
		sl->PO2SOconfig = hw_desc->config_hook;

	ec_dev->slave_index = pos;
	ed->slaves[ed->slave_count++] = ec_dev;

	return 0;
}


static const BackendDriverOps_t ops = {
	.init = ethercat_init,
	.bind = ethercat_bind_device,
	.finalize = ethercat_finalize_mapping,
	.start = ethercat_start,
	.stop = ethercat_stop,
	.process = ethercat_process,
};



BackendDriver_t* EtherCAT_Driver_Create(const BackendConfig_t* config, int instance_index)
{
	EtherCAT_Driver_t* d = CALLOC(1, sizeof(EtherCAT_Driver_t));
	if (!d)
		return NULL;

	// Configuration de base
	BackendDriver_t* base = &d->base;
	base->ops = &ops;
	snprintf(base->system_name, sizeof(base->system_name), "ec%d", instance_index);
	d->thread_cycle_us = config->ethercat.cycle_us;
	d->iomap_size = config->ethercat.io_map_size;
	d->has_dc_clock = config->ethercat.has_dc_clock;

	// Initialisation SOEM EtherCAT
	if (!ecx_init(&d->ctx, config->ethercat.ifname)) {
		print_available_adapters();
		FREE(d);
		return NULL;
	}

	return base;
}


void EtherCAT_Driver_Destroy(BackendDriver_t* b) {
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;
	for (int i = 0; i < d->slave_count; i++) {
		EtherCAT_Device_t* dev = d->slaves[i];
		FREE(dev->rx_buffers[0]);
		FREE(dev->rx_buffers[1]);
		FREE(dev->tx_buffers[0]);
		FREE(dev->tx_buffers[1]);
	}
	FREE(d->iomap);
	FREE(d);
}





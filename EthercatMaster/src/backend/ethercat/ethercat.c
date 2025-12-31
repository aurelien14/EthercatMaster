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


static int soem_config_callback(ecx_contextt* ctx, uint16_t slave) {
	// 1. Trouver le device correspondant à cet index d'esclave
	/*EtherCAT_Device_t* dev = find_device_by_slave_index(slave);

	// 2. Appeler le hook du descripteur
	if (dev->desc->config_hook) {
		return dev->desc->config_hook(dev->master, slave);
	}*/
	return 1;
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


static void ethercat_buffer_cleanup(EtherCAT_Device_t* dev) {
	if (!dev) return;

	for (int i = 0; i < 2; i++) {
		if (dev->out_buffers[i]) {
			FREE(dev->out_buffers[i]);
			dev->out_buffers[i] = NULL;
		}
		if (dev->in_buffers[i]) {
			FREE(dev->in_buffers[i]);
			dev->in_buffers[i] = NULL;
		}
	}
}


static int ethercat_buffers_init(EtherCAT_Device_t* bdev, size_t in_size, size_t out_size) {
	if (!bdev) return -1;

	// 1. Sécurité : Initialisation forcée à NULL de tous les pointeurs
	// Cela évite de tenter un FREE sur une valeur aléatoire en cas d'erreur
	for (int i = 0; i < 2; i++) {
		bdev->out_buffers[i] = NULL;
		bdev->in_buffers[i] = NULL;
	}

	bdev->out_size = out_size;
	bdev->in_size = in_size;

	// 2. Allocation des buffers de sorties vu du Device (Sorties)
	if (out_size > 0) {
		bdev->out_buffers[0] = CALLOC(1, out_size);
		bdev->out_buffers[1] = CALLOC(1, out_size);

		if (!bdev->out_buffers[0] || !bdev->out_buffers[1]) {
			goto allocation_failed;
		}
	}

	// 3. Allocation des buffers d'entrées vu du Device (Entrées)
	if (in_size > 0) {
		bdev->in_buffers[0] = CALLOC(1, in_size);
		bdev->in_buffers[1] = CALLOC(1, in_size);

		if (!bdev->in_buffers[0] || !bdev->in_buffers[1]) {
			goto allocation_failed;
		}
	}

	return 0; // Succès

allocation_failed:
	ethercat_buffer_cleanup(bdev);
	return -1;
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

	//d->ctx.userdata = MALLOC(sizeof(uint16_t));

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
		if (sl->Ibytes != ec_dev->tx_pdo_size || sl->Obytes != ec_dev->rx_pdo_size) {
			printf("[CONFIG] size mismatch slave %d: Ibytes=%d in_size=%llu, Obytes=%d out_size=%llu\n",
				pos, sl->Ibytes, ec_dev->in_size, sl->Obytes, ec_dev->out_size);
			return -1;
		}

		if (ethercat_buffers_init(ec_dev, sl->Ibytes, sl->Obytes))
			return -1;
	}

	atomic_store_i32(&ec->active_in_buffer_idx, 0);
	atomic_store_i32(&ec->active_out_buffer_idx, 0);
	atomic_store_i32(&ec->rt_out_buffer_idx, 0);


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
	//CoE, mailbox...
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

	ec_dev->rx_pdo_size = hw_desc->rx_pdo_size;
	ec_dev->tx_pdo_size = hw_desc->tx_pdo_size;

	/* Vérification identité */
	if (sl->eep_man != hw_desc->product_code ||
		sl->eep_id != hw_desc->vendor_id) {
		printf("[ECAT] mismatch at pos %d\n", pos);
		return -1;
	}


	if (hw_desc->config_hook) {
		sl->PO2SOconfig = hw_desc->config_hook;
		//uint16_t* ecx_user = (uint16_t*)ecx->userdata;
		//ecx_user[pos] = hw_desc->watchdog;
	}

	ec_dev->slave_index = pos;
	ed->slaves[ed->slave_count++] = ec_dev;
	dev->driver = (BackendDriver_t*)ed;

	return 0;
}


static void ethercat_sync_buffers(BackendDriver_t* b)
{
	EtherCAT_Driver_t* d = (EtherCAT_Driver_t*)b;

	int plc_idx = atomic_load_i32(&d->active_out_buffer_idx);
	int next_plc = (plc_idx == 0) ? 1 : 0;

	// 1) Commit : RT enverra ce que le PLC vient d'écrire
	atomic_store_i32(&d->rt_out_buffer_idx, plc_idx);

	// 2) Rémanence (optionnel mais recommandé si PLC n'écrit pas tout)
	for (int i = 0; i < d->slave_count; i++) {
		EtherCAT_Device_t* dev = d->slaves[i];
		if (dev->out_size > 0) {
			memcpy(dev->out_buffers[next_plc], dev->out_buffers[plc_idx], dev->out_size);
		}
	}

	// 3) Publication : PLC écrira dans next_plc au prochain tick
	atomic_store_i32(&d->active_out_buffer_idx, next_plc);
}


static uint8_t* ethercat_device_get_input_ptr(EtherCAT_Driver_t* drv, EtherCAT_Device_t* dev)
{
	int idx = atomic_load_i32(&drv->active_in_buffer_idx);
	return dev->in_buffers[idx];
}


static uint8_t* ethercat_device_get_output_ptr(EtherCAT_Driver_t* drv, EtherCAT_Device_t* dev)
{
	int idx = atomic_load_i32(&drv->active_out_buffer_idx);
	return dev->out_buffers[idx];
}


static const BackendDriverOps_t ops = {
	.init = ethercat_init,
	.bind = ethercat_bind_device,
	.finalize = ethercat_finalize_mapping,
	.start = ethercat_start,
	.stop = ethercat_stop,
	.process = ethercat_process,
	.sync = ethercat_sync_buffers,
	.get_input_data = ethercat_device_get_input_ptr,
	.get_output_data = ethercat_device_get_output_ptr
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
	//FREE(d->ctx.userdata);
	ecx_close(&d->ctx);
	for (int i = 0; i < d->slave_count; i++) {
		EtherCAT_Device_t* dev = d->slaves[i];
		ethercat_buffer_cleanup(&dev->base);
	}
	FREE(d->iomap);
	FREE(d);
}




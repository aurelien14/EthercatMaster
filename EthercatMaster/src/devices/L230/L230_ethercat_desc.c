#include "l230_ethercat_desc.h"
#include "l230_ethercat_pdo.h"
#include "backend/ethercat/ethercat_slave_desc.h"
#include <soem/soem.h>
#include <stdio.h>

static int config_hook_EL230(ecx_contextt* ctx, uint16_t slave)
{
	ec_slavet* pslave = &ctx->slavelist[slave];

	pslave->SM[2].StartAddr = 0x1100;
	pslave->SM[2].SMflags = 0x64;
	pslave->SM[3].StartAddr = 0x1300;
	pslave->SM[3].SMflags = 0x20;

	uint16_t new_timeout = L230_TIME_OUT_PROCESS_DATA;
	ecx_FPWRw(&ctx->port, pslave->configadr, 0x420,
		new_timeout, EC_TIMEOUTRET);

	return 1;
}

const EtherCAT_SlaveDesc_t L230_ECAT_DESC = {
	.vendor_id = 0x3213335,
	.product_code = 0x47535953,
	.rx_pdo_size = sizeof(L230_RX_PDO_t),
	.tx_pdo_size = sizeof(L230_TX_PDO_t),
	.config_hook = config_hook_EL230
};

#include "core/backend/backend_desc.h"
#include "ethercat.h"

/* Description device générique */
const BackendDesc_t ETHERCAT_DRIVER_DESC = {
	.name = "EtherCAT",
	.create = EtherCAT_Driver_Create,
	.destroy = EtherCAT_Driver_Destroy
};
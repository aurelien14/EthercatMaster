#include "ethercat_device.h"
#include "ethercat_device_desc.h"
#include "config/system_config.h"
#include "core/plateform/plateform.h"
#include "core/backend/backend.h"

int ethercat_device_init(EtherCAT_Device_t* ecat_dev, DeviceConfig_t* cfg) {
    if (!ecat_dev || !cfg) return -1;
    const EtherCAT_DeviceDesc_t* ecat_desc = (const EtherCAT_DeviceDesc_t*)cfg->device_desc->hw_desc;

    int res = buffered_device_init(&ecat_dev->base,
        ecat_desc->tx_pdo_size,
        ecat_desc->rx_pdo_size);
    if (res != 0) return res;


    ecat_dev->out_size = ecat_desc->rx_pdo_size;
    ecat_dev->in_size = ecat_desc->tx_pdo_size;


    ecat_dev->soem_inputs = NULL;
    ecat_dev->soem_outputs = NULL;

    return 0;
}





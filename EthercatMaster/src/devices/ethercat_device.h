#pragma once
#include <stdint.h>
#include "core/device/device.h"
#include "config/config.h"

#if defined ECAT_DOUBLE_BUFFURING
#include "core/plateform/plateform.h"
#endif

typedef struct {
    Device_t base;
    uint16_t slave_index;

    uint8_t* soem_outputs;
    uint8_t* soem_inputs;

    // Deux buffers pour les entrées (RX)
    uint8_t* rx_buffers[2];
    atomic_i32_t active_rx_idx; // 0 ou 1 : l'index que le PLC peut lire

    // Deux buffers pour les sorties (TX)
    uint8_t* tx_buffers[2];
    atomic_i32_t active_tx_idx; // 0 ou 1 : l'index que le PLC doit remplir

    size_t rx_size;
    size_t tx_size;

} EtherCAT_Device_t;

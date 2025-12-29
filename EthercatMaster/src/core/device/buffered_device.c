#include "buffered_device.h"
#include "core/backend/backend.h"
#include "core/system/memalloc.h"
#include "core/plateform/plateform.h"

int buffered_device_init(BufferedDevice_t* bdev, size_t rx_sz, size_t tx_sz) {
    if (!bdev) return -1;

    // 1. Sécurité : Initialisation forcée à NULL de tous les pointeurs
    // Cela évite de tenter un FREE sur une valeur aléatoire en cas d'erreur
    for (int i = 0; i < 2; i++) {
        bdev->rx_buffers[i] = NULL;
        bdev->tx_buffers[i] = NULL;
    }
    
    bdev->rx_size = rx_sz;
    bdev->tx_size = tx_sz;

    // 2. Allocation des buffers de Réception (Entrées)
    if (rx_sz > 0) {
        bdev->rx_buffers[0] = calloc(1, rx_sz);
        bdev->rx_buffers[1] = calloc(1, rx_sz);

        if (!bdev->rx_buffers[0] || !bdev->rx_buffers[1]) {
            goto allocation_failed;
        }
    }

    // 3. Allocation des buffers de Transmission (Sorties)
    if (tx_sz > 0) {
        bdev->tx_buffers[0] = calloc(1, tx_sz);
        bdev->tx_buffers[1] = calloc(1, tx_sz);

        if (!bdev->tx_buffers[0] || !bdev->tx_buffers[1]) {
            goto allocation_failed;
        }
    }

    return 0; // Succès

allocation_failed:
    buffered_device_cleanup(bdev);
    return -1;
}


void buffered_device_cleanup(BufferedDevice_t* bdev) {
    if (!bdev) return;

    for (int i = 0; i < 2; i++) {
        if (bdev->rx_buffers[i]) {
            FREE(bdev->rx_buffers[i]);
            bdev->rx_buffers[i] = NULL;
        }
        if (bdev->tx_buffers[i]) {
            FREE(bdev->tx_buffers[i]);
            bdev->tx_buffers[i] = NULL;
        }
    }
}


void buffered_device_swap_rx(Device_t* dev) {
    int current = atomic_load_i32(&dev->driver->active_rx_idx);
    atomic_store_i32(&dev->driver->active_rx_idx, current == 0 ? 1 : 0);
}


uint8_t* device_get_input_ptr(Device_t* dev) {
    if (!dev || !dev->driver) return NULL;

    BufferedDevice_t* bdev = (BufferedDevice_t*)dev;
    int32_t idx = atomic_load_i32(&dev->driver->active_tx_idx);
    return bdev->tx_buffers[idx];
}


uint8_t* device_get_output_ptr(Device_t* dev) {
    if (!dev || !dev->driver) return NULL;

    BufferedDevice_t* bdev = (BufferedDevice_t*)dev;
    int idx = atomic_load_i32(&dev->driver->active_rx_idx);
    return bdev->tx_buffers[idx];
}
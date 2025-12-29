#include "buffered_device.h"
#include "core/backend/backend.h"
#include "core/system/memalloc.h"
#include "core/plateform/plateform.h"


int buffered_device_init(BufferedDevice_t* bdev, size_t in_size, size_t out_size) {
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
        bdev->out_buffers[0] = calloc(1, out_size);
        bdev->out_buffers[1] = calloc(1, out_size);

        if (!bdev->out_buffers[0] || !bdev->out_buffers[1]) {
            goto allocation_failed;
        }
    }

    // 3. Allocation des buffers d'entrées vu du Device (Entrées)
    if (in_size > 0) {
        bdev->in_buffers[0] = calloc(1, in_size);
        bdev->in_buffers[1] = calloc(1, in_size);

        if (!bdev->in_buffers[0] || !bdev->in_buffers[1]) {
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
        if (bdev->out_buffers[i]) {
            FREE(bdev->out_buffers[i]);
            bdev->out_buffers[i] = NULL;
        }
        if (bdev->in_buffers[i]) {
            FREE(bdev->in_buffers[i]);
            bdev->in_buffers[i] = NULL;
        }
    }
}


void buffered_device_swap_rx(BufferedDevice_t* dev) {
    //int current = atomic_load_i32(&dev->base.driver->active_rx_idx);
    //atomic_store_i32(&dev->base.driver->active_rx_idx, current == 0 ? 1 : 0);
}

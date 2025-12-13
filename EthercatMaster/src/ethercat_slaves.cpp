#include "ethercat_slaves.h"

void ecat_slave_init(EcatSlave* slave, EcatSlaveInfo* info) {
    slave->slave_info = info;

    // Initialiser le triple buffer
    atomic_store(&slave->pdo.active_rx_app, 0);
    atomic_store(&slave->pdo.active_tx_rt, 0);
    atomic_store(&slave->pdo.rx_dirty, false);
}


/*
void ecat_slave_iomap(EcatSlave* ecat_slave, ecx_contextt* ctx) {
    // Mapper les PDO vers notre structure
    ecat_slave->pdo.rx_iomap = (RX_PDO_t*)ctx->slavelist[ecat_slave->slave_index].outputs;
    ecat_slave->pdo.tx_iomap = (TX_PDO_t*)ctx->slavelist[ecat_slave->slave_index].inputs;
#if 0
    // Initialiser le double buffer
    atomic_store(&ecat_slave->pdo.active_buf, 0);
#else
    // Initialiser le triple buffer
    atomic_store(&ecat_slave->pdo.active_rx_app, 0);
    atomic_store(&ecat_slave->pdo.active_tx_rt, 0);
    atomic_store(&ecat_slave->pdo.rx_dirty, false);
#endif

    memset(&ecat_slave->pdo.rx_buf, 0, sizeof(ecat_slave->pdo.rx_buf));
    memset(&ecat_slave->pdo.tx_buf, 0, sizeof(ecat_slave->pdo.tx_buf));
}
*/

void ecat_slave_iomap(EcatSlave* slave, ecx_contextt* ctx)
{
    Slave_PDO_t* pdo = &slave->pdo;
    const EcatSlaveInfo* info = slave->slave_info;

    pdo->rx_size = info->rx_pdo_size;
    pdo->tx_size = info->tx_pdo_size;

    printf("ecat_slave_iomap tx_size=%d, rx_size=%d\n", pdo->rx_size, pdo->tx_size);

    for (int i = 0; i < 3; i++) {
        pdo->rx_buf[i] = calloc(1, pdo->rx_size);
        pdo->tx_buf[i] = calloc(1, pdo->tx_size);
    }

    pdo->rx_iomap = ctx->slavelist[slave->slave_index].outputs;
    pdo->tx_iomap = ctx->slavelist[slave->slave_index].inputs;
}

void ecat_slave_clean(EcatSlave* slave) {
    Slave_PDO_t* pdo = &slave->pdo;
    for (int i = 0; i < 3; i++) {
        free(pdo->rx_buf[i]);
        free(pdo->tx_buf[i]);
        pdo->rx_buf[i] = NULL;
        pdo->tx_buf[i] = NULL;
    }

    pdo->rx_size = 0;
    pdo->tx_size = 0;
    pdo->rx_iomap = NULL;
    pdo->tx_iomap = NULL;
    atomic_store(&pdo->active_rx_app, 0);
    atomic_store(&pdo->active_tx_rt, 0);
    atomic_store(&pdo->rx_dirty, false);
}
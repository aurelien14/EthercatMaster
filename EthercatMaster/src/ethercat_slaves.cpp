#include "ethercat_slaves.h"

void ecat_slave_init(EcatSlave* slave, EcatSlaveInfo* info)
{
    slave->slave_info = info;

    Slave_PDO_t* pdo = &slave->pdo;
    Slave_PDO_RX_t* pdo_rx = &pdo->PDO_rx;
    Slave_PDO_TX_t* pdo_tx = &pdo->PDO_tx;

    // RX : RT lit rx_buf[0] au démarrage
    atomic_store(&pdo_rx->active_rx_rt, 0);

    // TX : APP lira tx_buf[1]
    //atomic_store(&pdo_rx->active_tx_app, 1);

    atomic_store(&pdo_rx->dirty, false);
}


void ecat_slave_iomap(EcatSlave* slave, ecx_contextt* ctx)
{
    Slave_PDO_t* pdo = &slave->pdo;
    Slave_PDO_RX_t* pdo_rx = &pdo->PDO_rx;
    Slave_PDO_TX_t* pdo_tx = &pdo->PDO_tx;

    const EcatSlaveInfo* info = slave->slave_info;

    pdo_rx->size = info->rx_pdo_size;
    //pdo->tx_size = info->tx_pdo_size;

    for (int i = 0; i < 3; i++) {
        pdo_rx->buf[i] = calloc(1, pdo_rx->size);
        //pdo->tx_buf[i] = calloc(1, pdo->tx_size);
    }

    printf("    Esclave %d rx buffers iomap à 0x%p\n", slave->slave_index, pdo_rx->buf);
    printf("    Esclave %d tx buffers iomap à 0x%p\n", slave->slave_index, pdo_tx->iomap);

    // IOmap SOEM (RT only)
    pdo_rx->iomap = ctx->slavelist[slave->slave_index].outputs;
    pdo_tx->iomap = ctx->slavelist[slave->slave_index].inputs;
}


void ecat_slave_clean(EcatSlave* slave)
{
    Slave_PDO_t* pdo = &slave->pdo;
    Slave_PDO_RX_t* pdo_rx = &pdo->PDO_rx;
    Slave_PDO_TX_t* pdo_tx = &pdo->PDO_tx;

    // S'assurer que les threads ont arrêté avant
    atomic_store(&pdo_rx->dirty, false);
    atomic_store(&pdo_rx->active_rx_rt, 0);
    //atomic_store(&pdo->active_tx_app, 0);

    for (int i = 0; i < 3; i++) {
        free(pdo_rx->buf[i]);
        //free(pdo->tx_buf[i]);
        pdo_rx->buf[i] = NULL;
        //pdo->tx_buf[i] = NULL;
    }

    pdo_rx->size = 0;
    //pdo->tx_size = 0;
    pdo_rx->iomap = NULL;
    pdo_rx->iomap = NULL;

    atomic_store(&pdo_rx->active_rx_rt, 0);
    atomic_store(&pdo_rx->dirty, false);
    //atomic_store(&pdo->active_tx_app, 1);
    
}

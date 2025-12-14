#pragma once

#include "soem/soem.h"
#include <stdatomic.h>

#define TRIPLE_BUFFER_ON_RX

/***********************************************************
* Slave_PDO_t;
* **********************************************************/

#if defined TRIPLE_BUFFER_ON_RX
typedef struct {
    void*       buf[3];      // Buffers RX typés dynamiquement
    size_t      size;
    atomic_int  active_rx_rt;     // index RX consommé par RT
    atomic_bool dirty;
    void*       iomap;
} Slave_PDO_RX_t;


typedef struct {
    void* iomap;
} Slave_PDO_TX_t;


typedef struct {
    Slave_PDO_RX_t PDO_rx;
    Slave_PDO_TX_t PDO_tx;

} Slave_PDO_t;

#else
typedef struct
{
    /* --- Double buffer APPLICATION --- */
    void* rx_buf[3];      // Buffers RX typés dynamiquement
    void* tx_buf[3];      // Buffers TX typés dynamiquement

    size_t rx_size;
    size_t tx_size;

    atomic_int  active_rx_rt;     // index RX consommé par RT
    atomic_int  active_tx_app;    // index TX lu par APP

    atomic_bool rx_dirty;

    /* --- IOmap (temps réel EtherCAT uniquement) --- */
    void* rx_iomap;
    void* tx_iomap;

} Slave_PDO_t;
#endif



/***********************************************************
* hook fonction pointer
* **********************************************************/
typedef int (*config_slave_hook)(ecx_contextt* ctx, uint16 slave);

/***********************************************************
* EcatSlave
* **********************************************************/
typedef struct {
	const uint32 id;
	const uint32 man;

	size_t rx_pdo_size;
#if !defined TRIPLE_BUFFER_ON_RX
	size_t tx_pdo_size;
#endif

	config_slave_hook config_hook;

	const char* name;
} EcatSlaveInfo;


typedef struct {
	const EcatSlaveInfo* slave_info;
	uint32 slave_index;
	Slave_PDO_t pdo;
} EcatSlave;


/***********************************************************
* functions ethercat slave
* **********************************************************/
void ecat_slave_init(EcatSlave* board, EcatSlaveInfo* info);
void ecat_slave_iomap(EcatSlave* board, ecx_contextt* ctx);
void ecat_slave_clean(EcatSlave* slave);

/***********************************************************
* helper functions ethercat slave buffers
* **********************************************************/
#if NEW_BUFF3 //erreur lors de la libération des buffer

static inline void ecat_get_app_buffers(Slave_PDO_t* pdo,
    void** tx_data,
    void** rx_data)
{
    int tx_idx = atomic_load_explicit(&pdo->active_tx_app, memory_order_acquire);

    *tx_data = pdo->tx_buf[tx_idx];  // lecture entrées
    *rx_data = pdo->rx_buf[2];       // écriture sorties
}


static inline void ecat_commit_app_outputs(Slave_PDO_t* pdo)
{
    int old = atomic_exchange_explicit(&pdo->active_rx_rt, 2, memory_order_acq_rel);

    // recycler ancien buffer RT comme buffer APP
    pdo->rx_buf[2] = pdo->rx_buf[old];

    atomic_store_explicit(&pdo->rx_dirty, true, memory_order_release);
}


static inline void ecat_copy_outputs_app_to_rt(Slave_PDO_t* pdo)
{
    if (atomic_exchange_explicit(&pdo->rx_dirty, false, memory_order_acq_rel)) {

        int idx = atomic_load_explicit(&pdo->active_rx_rt, memory_order_acquire);

        memcpy(pdo->rx_iomap,
            pdo->rx_buf[idx],
            pdo->rx_size);
    }
}

#elif defined TRIPLE_BUFFER_ON_RX
// Appelé par le thread APP pour obtenir les buffers de travail
static inline void ecat_get_app_buffers(Slave_PDO_t* pdo,
    void** tx_data,
    void** rx_data) {
    // Lecture directe des entrées : zéro copy
    *tx_data = pdo->PDO_tx.iomap;

    // Écriture sur le buffer PLC (triple buffering)
    *rx_data = pdo->PDO_rx.buf[2];
}

// Appelé par le thread APP après avoir modifié les sorties
static inline void ecat_commit_app_outputs(Slave_PDO_t* pdo) {
    // Copier du buffer PLC (2) vers le buffer de transit (1)
    memcpy(pdo->PDO_rx.buf[1], pdo->PDO_rx.buf[2], pdo->PDO_rx.size);

    // Marquer comme dirty pour que le RT copie au prochain cycle
    atomic_store(&pdo->PDO_rx.dirty, true);
}

// Appelé par le thread RT avant envoi - uniquement si dirty
static inline void ecat_copy_outputs_app_to_rt(Slave_PDO_t* pdo) {
    if (atomic_load(&pdo->PDO_rx.dirty)) {
        memcpy(pdo->PDO_rx.iomap, pdo->PDO_rx.buf[1], pdo->PDO_rx.size);
        atomic_store(&pdo->PDO_rx.dirty, false);
    }
}


#else

static inline void ecat_swap_buffers_rt_to_app(Slave_PDO_t* pdo) {
    // Buffer 0 = toujours utilisé par RT pour lire l'IOmap
    // On copie dans buffer 1 pour l'application
    memcpy(pdo->tx_buf[1], pdo->tx_iomap, pdo->tx_size);

    // Signaler à l'APP que de nouvelles données sont disponibles
    atomic_store(&pdo->active_tx_app, 1);
}


// Appelé par le thread APP pour obtenir les buffers de travail
static inline void ecat_get_app_buffers(Slave_PDO_t* pdo,
    void** tx_data,
    void** rx_data) {
    // Lire les dernières entrées (du RT)
    int rx_index = atomic_load(&pdo->active_tx_app);
    *tx_data = pdo->tx_buf[rx_index];

    // Écrire dans le buffer 2 (dédié à l'APP)
    *rx_data = pdo->rx_buf[2];
}

// Appelé par le thread APP après avoir modifié les sorties
static inline void ecat_commit_app_outputs(Slave_PDO_t* pdo) {
    // Copier du buffer APP (2) vers le buffer de transit (1)
    memcpy(pdo->rx_buf[1], pdo->rx_buf[2], pdo->rx_size);

    // Marquer comme "dirty" pour que le RT copie au prochain cycle
    atomic_store(&pdo->rx_dirty, true);
}

// Appelé par le thread RT avant envoi - SEULEMENT si changement
static inline void ecat_copy_outputs_app_to_rt(Slave_PDO_t* pdo) {
    // Vérifier si l'APP a écrit de nouvelles données
    if (atomic_load(&pdo->rx_dirty)) {
        // Copier du buffer de transit (1) vers l'IOmap (0)
        memcpy(pdo->rx_iomap, pdo->rx_buf[1], pdo->rx_size);

        // Acquitter
        atomic_store(&pdo->rx_dirty, false);
    }
    // Sinon, on ne fait RIEN - l'IOmap garde ses valeurs
}

#endif

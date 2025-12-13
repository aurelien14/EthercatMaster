#pragma once

#include "soem/soem.h"
#include <stdatomic.h>

/***********************************************************
* Slave_PDO_t;
* **********************************************************/
typedef struct
{
	/* --- Double buffer APPLICATION --- */
	void* rx_buf[3];      // Buffers RX typés dynamiquement
	void* tx_buf[3];      // Buffers TX typés dynamiquement

	size_t rx_size;
	size_t tx_size;

	atomic_int  active_rx_app;
	atomic_int  active_tx_rt;
	atomic_bool rx_dirty;

	/* --- IOmap (temps réel EtherCAT uniquement) --- */
	void* rx_iomap;
	void* tx_iomap;

} Slave_PDO_t;


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
	size_t tx_pdo_size;

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
static inline void ecat_swap_buffers_rt_to_app(Slave_PDO_t* pdo) {
    // Buffer 0 = toujours utilisé par RT pour lire l'IOmap
    // On copie dans buffer 1 pour l'application
    memcpy(pdo->tx_buf[1], pdo->tx_iomap, pdo->tx_size);

    // Signaler à l'APP que de nouvelles données sont disponibles
    atomic_store(&pdo->active_rx_app, 1);
}

// Appelé par le thread APP pour obtenir les buffers de travail
static inline void ecat_get_app_buffers(Slave_PDO_t* pdo,
    void** tx_data,
    void** rx_data) {
    // Lire les dernières entrées (du RT)
    int rx_index = atomic_load(&pdo->active_rx_app);
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

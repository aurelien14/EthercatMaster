#pragma once


#include "soem/soem.h"
#include "plc_task_manager.h"
#include "ethercat_slaves.h"


#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000LL
#endif

#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000LL
#endif


#define USEC_PER_SEC_FLOAT 1000000.0f


//configuration du système ethercat

//controle si plc task n'est pas bloqué, si bloqué, met ethercat en safe-op
#define WATCHDOG_PLC_TASK

//block thread ecat_app_thread
#define BLOCK_ON_SYSTEM_STATE

//calcul du jitter pour info
#define JITTER_CALC

//temps cycle thread real time ethercat
#define ECAT_RT_THREAD_CYCLE_US 2000    //2ms

//temps cycle ordonnanceur taches PLC
#define SCHEDULER_PLC_TASK_CYCLE_US 10000   //10ms


// ============================================================================
// STRUCTURE GLOBALE DU SYSTÈME
// ============================================================================

typedef ec_state EcatSystemState;

typedef struct {
    // Contexte EtherCAT
    ecx_contextt ecx_context;
    char IOmap[4096];

    // Slaves
    EcatSlave* slaves;
    int slave_count;

    // État du système
    atomic_int system_state;
    atomic_bool running;

    // Threads
    HANDLE thread_rt;
    HANDLE thread_app;
    HANDLE thread_watchdog;
    HANDLE thread_tcp_server;

    // Gestionnaire de taches
    PLCTaskManager_t* plc_task_manager;

    // Timing
    LARGE_INTEGER frequency;
    LARGE_INTEGER cycle_start;
    uint64_t cycle_time_us;

    // Watchdog
    atomic_uint_fast64_t watchdog_counter;

    // Statistiques
    atomic_uint_fast64_t cycle_count;
    atomic_uint_fast64_t errors_count;

    atomic_uint max_jitter_us;
    atomic_int64_t last_jitter_us;
    atomic_uint jitter_overflow_count;

    // *** NOUVEAU : horodatage RT pour watchdog ***
    atomic_uint_fast64_t last_cycle_ticks;

} EcatSystem;



// ============================================================================
// FONCTIONS D'AIDE
// ============================================================================
static inline EcatSystemState ecat_get_systeme_state(EcatSystem* sys) {
    return (EcatSystemState)atomic_load(&sys->system_state);
}

// ============================================================================
// FONCTIONS D'INITIALISATION
// ============================================================================

// Fonctions d'initialisation et transitions d'état
int ecat_system_init(EcatSystem* sys, const char* ifname, int cycle_time_us);
int ecat_system_add_slave(EcatSystem* sys, EcatSlaveInfo* info);
int ecat_system_config(EcatSystem* sys);
int ecat_system_transition_to_safeop(EcatSystem* sys);
int ecat_system_transition_to_op(EcatSystem* sys);
int ecat_system_start(EcatSystem* sys);
void ecat_system_stop(EcatSystem* sys);

const char* ecat_state_to_string(uint16_t state);


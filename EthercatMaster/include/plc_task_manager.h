#pragma once

#include "soem/soem.h"
#include "ethercat_slaves.h"
#include <stdatomic.h>

// ============================================================================
// TASK MANAGER - Définitions (à inclure depuis plc_task_manager.h)
// ============================================================================
// Forward declaration
typedef struct PLCTask_t PLCTask_t;
typedef void (*PLCTaskFunction)(PLCTask_t* self);

typedef enum {
    TASK_PRIORITY_HIGH = 0,
    TASK_PRIORITY_NORMAL = 1,
    TASK_PRIORITY_LOW = 2
} PLCTaskPriority;

typedef enum {
    TASK_STATE_DISABLED = 0,
    TASK_STATE_ENABLED = 1,
    TASK_STATE_ERROR = 2
} PLCTaskState;

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

// typedef existants supposés : PLCTaskFunction, PLCTaskPriority, PLCTaskState, EcatSlave

typedef struct PLCTask_t {
    // Configuration
    const char* name;
    PLCTaskFunction function;
    uint32_t cycle_time_ms;         // cycle désiré en ms


    PLCTaskPriority priority;
    PLCTaskState state;

    // Contexte système (lecture seule pour la tâche)
    EcatSlave* slaves;              // pointeur si besoin
    int slave_count;

    // Contexte temporel (internal: utiliser QPC ticks pour toute la logique)
    int64_t last_execution_ticks;   // QPC ticks (utiliser QPC partout)
    uint64_t timestamp_ms;          // utile pour affichage; mis à jour à l'exécution
    float cycle_time_s;             // delta réel (s) mis à jour

    // État (accessible)
    bool init;                      // true après 1ère exécution
    uint64_t execution_count;
    void* user_data;
    void* arg;

    // Stats (auto)
    uint32_t	execution_time_us;	// ✅ Moyenne glissante
    uint32_t	max_execution_us;
    uint32_t	overrun_count;
} PLCTask_t;


typedef struct {
    PLCTask_t *tasks;
    int task_count;
    int max_tasks;
    uint64_t total_cycles;

    uint32_t scheduler_time_us;

    // WATCHDOG
    atomic_int_fast64_t last_cycle_ticks; // QPC ticks du dernier cycle complet (atomic, utilisé par monitor/watchdog)
    uint32_t watchdog_timeout_ms;         // Timeout configuré (ex: 1000ms)

    // ESCLAVES ETHERCAT (pointé depuis EcatSystem aussi)
    EcatSlave* slaves;
    int slave_count;

    // Fréquence QPC (copie locale pour calculs rapides)
    int64_t qpc_frequency;                // = QueryPerformanceFrequency().QuadPart

    // Diagnostics globaux (facultatifs)
    atomic_uint max_task_exec_us;         // max exec observé parmi les tâches (option)

    // Pré-calcul ticks → µs / ms pour éviter divisions en RT
    uint64_t tick_to_us;
    uint64_t tick_to_ms;
} PLCTaskManager_t;


// Fonctions du Task Manager (implémentation dans un .c séparé)
//void plc_task_manager_init(PLCTaskManager_t* tm, int max_tasks);
void plc_task_manager_init(PLCTaskManager_t* tm, int max_tasks, EcatSlave* slaves, int slave_count);
void task_manager_sort(PLCTaskManager_t* tm);
int plc_task_manager_add(PLCTaskManager_t* tm, const char* name, PLCTaskFunction function,
    uint32_t cycle_time_ms, PLCTaskPriority priority, void* arg);
void plc_task_manager_run(PLCTaskManager_t* tm);
void plc_task_manager_print_stats(PLCTaskManager_t* tm);
void plc_task_manager_cleanup(PLCTaskManager_t* tm);
void* task_alloc_userdata(PLCTask_t* task, size_t size);







static inline int64_t ticks_to_us(int64_t ticks, int64_t qpc_freq) {
    return (ticks * 1000000LL) / qpc_freq;
}
static inline int64_t ticks_to_ms(int64_t ticks, int64_t qpc_freq) {
    return (ticks * 1000LL) / qpc_freq;
}
static inline int64_t us_to_ticks(int64_t us, int64_t qpc_freq) {
    return (us * qpc_freq) / 1000000LL;
}

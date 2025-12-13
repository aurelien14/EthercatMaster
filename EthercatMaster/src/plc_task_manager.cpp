
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "plc_task_manager.h"
//

void plc_task_manager_init(PLCTaskManager_t* tm, int max_tasks, EcatSlave* slaves, int slave_count)
{
    LARGE_INTEGER fq;
    QueryPerformanceFrequency(&fq);

    tm->tasks = (PLCTask_t*)calloc(max_tasks, sizeof(PLCTask_t));

    // ============================================================
    // CONFIGURATION GÉNÉRALE
    // ============================================================
    tm->qpc_frequency = fq.QuadPart;

    tm->task_count = 0;
    tm->max_tasks = max_tasks;
    tm->total_cycles = 0;
    tm->scheduler_time_us = 0;

    atomic_store(&tm->last_cycle_ticks, 0);
    tm->watchdog_timeout_ms = 1000;

    // ============================================================
    // CONTEXTE ETHERCAT
    // ============================================================
    tm->slaves = slaves;
    tm->slave_count = slave_count;

    // ============================================================
    // INITIALISATION DES TÂCHES
    // ⚠️ tm->tasks DOIT DÉJÀ ÊTRE ALLOUÉ AVANT
    // ============================================================
    for (int i = 0; i < max_tasks; ++i) {
        PLCTask_t* task = &tm->tasks[i];

        task->state = TASK_STATE_DISABLED;
        task->init = false;

        task->execution_count = 0;
        task->execution_time_us = 0;
        task->max_execution_us = 0;
        task->overrun_count = 0;

        task->last_execution_ticks = 0;
        task->timestamp_ms = 0;
        task->cycle_time_s = 0.0f;

        task->user_data = NULL;
        task->arg = NULL;
    }
}

#if 0
//void plc_task_manager_init(PLCTaskManager_t* tm, int max_tasks) {
void plc_task_manager_init(PLCTaskManager_t * tm, int max_tasks, EcatSlave * slaves, int slave_count) {
    tm->tasks = (PLCTask_t*)calloc(max_tasks, sizeof(PLCTask_t));
    tm->task_count = 0;
    tm->max_tasks = max_tasks;
    tm->total_cycles = 0;
    tm->scheduler_time_us = 0;

    //Initialiser le watchdog
    atomic_store(&tm->last_cycle_ticks, 0);
    tm->watchdog_timeout_ms = 1000;  // 1000ms par défaut (ajustable)

    tm->slaves = slaves;
    tm->slave_count = slave_count;
}
#endif

// Trier les tâches par priorité (appelé une fois après ajout de toutes les tâches)
void task_manager_sort(PLCTaskManager_t* tm) {
    if (tm->task_count <= 1) return;

    printf("[TASK] Tri par priorité...\n");

    // Bubble sort avec memcpy
    for (int i = 0; i < tm->task_count - 1; i++) {
        for (int j = 0; j < tm->task_count - i - 1; j++) {
            if (tm->tasks[j].priority > tm->tasks[j + 1].priority) {
                PLCTask_t temp;
                memcpy(&temp, &tm->tasks[j], sizeof(PLCTask_t));
                memcpy(&tm->tasks[j], &tm->tasks[j + 1], sizeof(PLCTask_t));
                memcpy(&tm->tasks[j + 1], &temp, sizeof(PLCTask_t));
            }
        }
    }

    printf("[TASK] Ordre d'exécution:\n");
    for (int i = 0; i < tm->task_count; i++) {
        printf("  %d. %s (prio=%d, cycle=%ums)\n",
            i + 1,
            tm->tasks[i].name,
            tm->tasks[i].priority,
            tm->tasks[i].cycle_time_ms);
    }
}

int plc_task_manager_add(PLCTaskManager_t* tm,
    const char* name,
    PLCTaskFunction function,
    uint32_t cycle_time_ms,
    PLCTaskPriority priority, void *arg)
{
    if (tm->task_count >= tm->max_tasks) {
        printf("[TASK] Erreur: Nombre max de tâches atteint (%d)\n", tm->max_tasks);
        return -1;
    }

    int id = tm->task_count;
    PLCTask_t* task = &tm->tasks[id];

    // ============================================================
    // CONFIGURATION
    // ============================================================
    task->name = name;
    task->function = function;
    task->cycle_time_ms = cycle_time_ms;
    task->priority = priority;

    // Contexte système
    task->slaves = tm->slaves;
    task->slave_count = tm->slave_count;

    // ============================================================
    // ÉTAT
    // ============================================================
    task->state = TASK_STATE_ENABLED;
    task->init = false;					// ✅ Pas encore exécutée
    task->execution_count = 0;
    task->user_data = NULL;
    task->arg = arg;

    // ============================================================
    // TEMPS & CONTEXTE
    // ============================================================
    task->timestamp_ms = 0;
    task->cycle_time_s = 0.0f;
    task->last_execution_ticks = 0;		// ✅ QPC propre

    // ============================================================
    // STATISTIQUES
    // ============================================================
    task->execution_time_us = 0;
    task->max_execution_us = 0;
    task->overrun_count = 0;


    tm->task_count++;

    printf("[TASK] Ajoutée: '%s' (cycle=%u ms, prio=%d)\n",
        name, cycle_time_ms, priority);

    return id;
}

#if 0
int plc_task_manager_add(PLCTaskManager_t* tm,
    const char* name,
    PLCTaskFunction function,
    uint32_t cycle_time_ms,
    PLCTaskPriority priority) {

    if (tm->task_count >= tm->max_tasks) {
        printf("[TASK] Erreur: Nombre max de tâches atteint (%d)\n", tm->max_tasks);
        return -1;
    }

    int id = tm->task_count;
    PLCTask_t* task = &tm->tasks[id];

    //configuration
     // Contexte système (sera mis à jour par le scheduler)
    task->timestamp_ms = 0;
    //task->cycle_time_s = 0.0f;
    task->cycle_time_ms = cycle_time_ms;
    task->function = function;
    task->priority = priority;
    task->name = name;

    //TODO: au lieu de slaves, mettre pdo ou une structure pour récupérer tous les pdo des esclaves
    task->slaves = tm->slaves;
    task->slave_count = tm->slave_count;

    // État
    task->state = TASK_STATE_ENABLED;
    task->init = false;          // ✅ FALSE au démarrage
    task->execution_count = 0;

    // Données privées
    task->user_data = NULL;

    // Runtime
    task->last_execution_ms = 0;
    task->execution_time_us = 0;
    task->max_execution_us = 0;
    task->overrun_count = 0;

    tm->task_count++;

    printf("[TASK] Ajoutée: '%s' (cycle=%ums, prio=%d)\n",
        name, cycle_time_ms, priority);

    return id;
}
#endif

void plc_task_manager_enable(PLCTaskManager_t* tm, int task_id) {
    if (task_id >= 0 && task_id < tm->task_count) {
        tm->tasks[task_id].state = TASK_STATE_ENABLED;
    }
}

void plc_task_manager_disable(PLCTaskManager_t* tm, int task_id) {
    if (task_id >= 0 && task_id < tm->task_count) {
        tm->tasks[task_id].state = TASK_STATE_DISABLED;
    }
}

#define PLC_EXEC_AVG_WINDOW 16

void plc_task_manager_run(PLCTaskManager_t* tm)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    tm->total_cycles++;

    // ============================================================
    // EXÉCUTION DES TÂCHES
    // ============================================================
    for (int i = 0; i < tm->task_count; i++) {
        PLCTask_t* task = &tm->tasks[i];

        if (task->state != TASK_STATE_ENABLED)
            continue;

        // ========================================================
        // PREMIÈRE INITIALISATION
        // ========================================================
        if (!task->init) {
            task->last_execution_ticks = now.QuadPart;
            task->init = true;
            continue;
        }

        // ========================================================
        // CALCUL DU TEMPS ÉCOULÉ
        // ========================================================
        int64_t elapsed_ticks = now.QuadPart - task->last_execution_ticks;
        uint64_t elapsed_ms = (elapsed_ticks * 1000ULL) / tm->qpc_frequency;

        if (elapsed_ms < task->cycle_time_ms)
            continue;

        // ========================================================
        // CONTEXTE AVANT EXÉCUTION
        // ========================================================
        task->timestamp_ms = (now.QuadPart * 1000ULL) / tm->qpc_frequency;
        task->cycle_time_s = (float)elapsed_ms / 1000.0f;

        // ========================================================
        // MESURE TEMPS D’EXÉCUTION
        // ========================================================
        LARGE_INTEGER task_start, task_end;
        QueryPerformanceCounter(&task_start);

        task->function(task);

        QueryPerformanceCounter(&task_end);

        int64_t exec_ticks = task_end.QuadPart - task_start.QuadPart;
        uint32_t exec_us = (uint32_t)((exec_ticks * 1000000ULL) / tm->qpc_frequency);

        // ========================================================
        // MISE À JOUR DES STATS
        // ========================================================
        task->last_execution_ticks = now.QuadPart;
        task->execution_count++;

        // ✅ MOYENNE GLISSANTE (EMA)
        if (task->execution_time_us == 0)
            task->execution_time_us = exec_us;
        else
            task->execution_time_us =
            (task->execution_time_us * (PLC_EXEC_AVG_WINDOW - 1) + exec_us)
            / PLC_EXEC_AVG_WINDOW;

        // ✅ MAX
        if (exec_us > task->max_execution_us)
            task->max_execution_us = exec_us;

        // ✅ OVERRUN
        if (exec_us > (task->cycle_time_ms * 1000ULL)) {
            task->overrun_count++;
            printf("[TASK] ⚠️  Overrun '%s' : %u µs (cycle %u ms)\n",
                task->name, exec_us, task->cycle_time_ms);
        }


    }

    // ============================================================
    // WATCHDOG — FIN DE CYCLE VALIDE
    // ============================================================
    atomic_store(&tm->last_cycle_ticks, now.QuadPart);
}



void plc_task_manager_print_stats(PLCTaskManager_t* tm) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                        STATISTIQUES DES TÂCHES                           ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Nom          │  Cycle  │ Exec  │  Avg   │  Max   │ Overrun │ État        ║\n");
    printf("╠══════════════╪═════════╪═══════╪════════╪════════╪═════════╪═════════════╣\n");

    for (int i = 0; i < tm->task_count; i++) {
        PLCTask_t* task = &tm->tasks[i];

        const char* state_str = "?";
        switch (task->state) {
        case TASK_STATE_ENABLED:  state_str = "ENABLED "; break;
        case TASK_STATE_DISABLED: state_str = "DISABLED"; break;
        case TASK_STATE_ERROR:    state_str = "ERROR   "; break;
        }

        printf("║ %-12s │ %4ums  │ %5llu │ %4uµs │ %4uµs │ %7u │ %s    ║\n",
            task->name,
            task->cycle_time_ms,
            task->execution_count,
            task->execution_time_us,   // ✅ Moyenne glissante
            task->max_execution_us,
            task->overrun_count,
            state_str);
    }

    printf("╚══════════════════════════════════════════════════════════════════════════╝\n");
    printf("  Scheduler overhead: %u µs/cycle\n", tm->scheduler_time_us);
    printf("\n");
}


void plc_task_manager_cleanup(PLCTaskManager_t* tm) {
    // Libérer les user_data de chaque tâche
    for (int i = 0; i < tm->task_count; i++) {
        if (tm->tasks[i].user_data) {
            free(tm->tasks[i].user_data);
            tm->tasks[i].user_data = NULL;
        }
    }

    if (tm->tasks) {
        free(tm->tasks);
        tm->tasks = NULL;
    }
    tm->task_count = 0;
}

void* task_alloc_userdata(PLCTask_t* task, size_t size) {
    if (task->user_data) {
        free(task->user_data);
    }
    task->user_data = calloc(1, size);
    return task->user_data;
}
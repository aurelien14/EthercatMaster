
#include "ecat_system.h"
#include "plc_tcp_server.h"
#include <windows.h>
#include <process.h>

// ============================================================================
// THREADS
// ============================================================================

// Thread 1: RT EtherCAT - PRIORITÉ MAXIMALE
unsigned __stdcall ecat_rt_thread(void* arg);

// Thread 2: Logique applicative - PRIORITÉ HAUTE
unsigned __stdcall ecat_app_thread(void* arg);

// Thread 3: Watchdog - PRIORITÉ NORMALE
unsigned __stdcall ecat_watchdog_thread(void* arg);




static inline void short_spin_wait(const LARGE_INTEGER* freq, int64_t remaining_us)
{
    // Busy-spin pour la précision (très court)
    if (remaining_us <= 0) return;
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    int64_t target = t0.QuadPart + (remaining_us * freq->QuadPart) / 1000000LL;
    do { QueryPerformanceCounter(&t1); } while (t1.QuadPart < target);
}

unsigned __stdcall ecat_rt_thread(void* arg)
{
    EcatSystem* sys = (EcatSystem*)arg;
    if (!sys) return 1;

    LARGE_INTEGER now;
    LARGE_INTEGER next_cycle;
    LARGE_INTEGER qpf;
    QueryPerformanceFrequency(&qpf);

    // sécurité : utiliser sys->frequency si initialisé, sinon qpf
    LARGE_INTEGER freq = (sys->frequency.QuadPart != 0) ? sys->frequency : qpf;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);

    // Pin + priorité
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    DWORD_PTR core_count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    DWORD_PTR mask = 1ULL << (core_count - 1);
    SetThreadAffinityMask(GetCurrentThread(), mask);

    QueryPerformanceCounter(&next_cycle);

    // cycle_time en ticks QPC
    int64_t cycle_time_ticks = ((int64_t)sys->cycle_time_us * freq.QuadPart) / 1000000LL;

    // Watchdog / stats initiales
    atomic_store(&sys->max_jitter_us, 0);
    atomic_store(&sys->last_jitter_us, 0);
    atomic_store(&sys->jitter_overflow_count, 0);

    bool watchdog_ok = true;
    bool watchdog_prev = true;
    uint64_t watchdog_errors = 0;
    uint64_t last_watchdog_check = 0;

    while (atomic_load(&sys->running)) {

        // avancée prévue
        next_cycle.QuadPart += cycle_time_ticks;

        // ========== attente hybride ==========
        while (1) {
            QueryPerformanceCounter(&now);
            int64_t remaining_ticks = next_cycle.QuadPart - now.QuadPart;
            int64_t remaining_us = (remaining_ticks * 1000000LL) / freq.QuadPart;

            if (remaining_us <= 0)
                break;

            // coarse sleep si >> 2ms
            if (remaining_us > 2000) {
                // garder une marge
                Sleep((DWORD)((remaining_us - 1500) / 1000));
                continue;
            }

            // si entre 1000us et 2000us, Sleep(1) est acceptable
            if (remaining_us > 1000) {
                Sleep(1);
                continue;
            }

            // pour < 1000us, on yield brièvement si > 200us, sinon busy spin
            if (remaining_us > 200) {
                // yield court - SwitchToThread est plus prévisible que Sleep(0)
                SwitchToThread();
                continue;
            }

            // busy spin pour précision finale (<200us)
            short_spin_wait(&freq, remaining_us);
            break;
        }

#if defined JITTER_CALC
        // ========== calcul du jitter ==========
        QueryPerformanceCounter(&now);
        int64_t jitter_ticks = now.QuadPart - next_cycle.QuadPart;
        int64_t jitter_us = (jitter_ticks * 1000000LL) / freq.QuadPart;

        // Mise à jour atomique des stats
        atomic_store(&sys->last_jitter_us, (uint32_t)(jitter_us < 0 ? 0 : jitter_us));
        if (jitter_us > (int64_t)sys->cycle_time_us)
            atomic_fetch_add(&sys->jitter_overflow_count, 1);

        if (jitter_us > 0) {
            uint32_t cur_max = atomic_load(&sys->max_jitter_us);
            if ((uint32_t)jitter_us > cur_max)
                atomic_store(&sys->max_jitter_us, (uint32_t)jitter_us);
        }

        // update last cycle tick (pour watchdog du PLC app thread)
        atomic_store(&sys->last_cycle_ticks, (uint64_t)now.QuadPart);
#endif

#if defined WATCHDOG_PLC_TASK
        // ========== Watchdog toutes les ~10ms ==========
        if ((now.QuadPart - last_watchdog_check) > freq.QuadPart / 100) {
            last_watchdog_check = now.QuadPart;

            if (sys->plc_task_manager) {
                uint64_t last_app_cycle = atomic_load(&sys->plc_task_manager->last_cycle_ticks);

                if (last_app_cycle == 0) {
                    watchdog_ok = true;
                }
                else {
                    int64_t elapsed_ticks = (int64_t)now.QuadPart - (int64_t)last_app_cycle;
                    int64_t elapsed_ms = (elapsed_ticks * 1000LL) / freq.QuadPart;
                    watchdog_ok = (elapsed_ms <= sys->plc_task_manager->watchdog_timeout_ms);
                }

                if (watchdog_ok != watchdog_prev) {
                    if (!watchdog_ok) watchdog_errors++;
                    watchdog_prev = watchdog_ok;
                }
            }
        }
#endif

        // ========== Cycle EtherCAT ==========
#if defined WATCHDOG_PLC_TASK
        if (watchdog_ok) {
#endif
            // Copier sorties si nécessaire (non bloquant, buffers pré-alloués)
            for (uint32_t i = 0; i < sys->slave_count; ++i) {
                if (sys->slaves[i].pdo.PDO_rx.dirty)
                    ecat_copy_outputs_app_to_rt(&sys->slaves[i].pdo);
            }

            // Envoi / réception processe data (SOEM)
            ecx_send_processdata(&sys->ecx_context);
            ecx_receive_processdata(&sys->ecx_context, EC_TIMEOUTRET);

            // swap buffers RT->APP
#if !defined TRIPLE_BUFFER_ON_RX
            for (uint32_t i = 0; i < sys->slave_count; ++i)
                ecat_swap_buffers_rt_to_app(&sys->slaves[i].pdo);
#endif

            atomic_fetch_add(&sys->cycle_count, 1);
#if defined WATCHDOG_PLC_TASK
        }
        else {
            atomic_fetch_add(&sys->errors_count, 1);
        }
#endif

        // ========== Gestion du retard massif ==========
        // si on est déjà en retard de plusieurs cycles, réaligner next_cycle
        QueryPerformanceCounter(&now);
        int64_t late_ticks = now.QuadPart - next_cycle.QuadPart;
        if (late_ticks > (5 * cycle_time_ticks)) {
            // on saute des cycles pour réaligner (évite backlog infinie)
            int64_t cycles_to_skip = late_ticks / cycle_time_ticks;
            next_cycle.QuadPart += cycles_to_skip * cycle_time_ticks;
#if defined JITTER_CALC
            // Optionnel : incrémenter compteur d'overrun pour diagnostique
            atomic_fetch_add(&sys->jitter_overflow_count, (uint32_t)cycles_to_skip);
#endif
        }
    }

    return 0;
}


#if defined WATCHDOG_PLC_TASK //moins performant
unsigned __stdcall ecat_rt_thread_old2(void* arg)
{
    EcatSystem* sys = (EcatSystem*)arg;

    LARGE_INTEGER cycle_start;
    LARGE_INTEGER next_cycle;
    LARGE_INTEGER now;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    DWORD_PTR core_count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    DWORD_PTR mask = 1ULL << (core_count - 1);
    SetThreadAffinityMask(GetCurrentThread(), mask);

    QueryPerformanceCounter(&cycle_start);
    next_cycle = cycle_start;

    int64_t cycle_time_ticks =
        ((int64_t)sys->cycle_time_us * sys->frequency.QuadPart) / 1000000LL;

    // ============================
    // WATCHDOG
    // ============================
    bool watchdog_ok = true;
    bool watchdog_prev = true;

    uint64_t watchdog_errors = 0;
    uint64_t last_watchdog_check = 0;

    // ============================
    // INIT JITTER
    // ============================
    atomic_store(&sys->max_jitter_us, 0);
    atomic_store(&sys->last_jitter_us, 0);
    atomic_store(&sys->jitter_overflow_count, 0);

    // ============================
    // BOUCLE TEMPS RÉEL
    // ============================
    while (atomic_load(&sys->running)) {

        // Calcul du prochain cycle
        next_cycle.QuadPart += cycle_time_ticks;

        // ============================
        // ATTENTE HYBRIDE (TIMER + SPIN)
        // ============================
        while (1) {
            QueryPerformanceCounter(&now);

            int64_t remaining_ticks = next_cycle.QuadPart - now.QuadPart;
            int64_t remaining_us =
                (remaining_ticks * 1000000LL) / sys->frequency.QuadPart;

            if (remaining_us <= 0)
                break;

            if (remaining_us > 500)
                Sleep(1);
            else if (remaining_us > 100)
                Sleep(0);
        }

        // ============================
        // CALCUL DU JITTER
        // ============================
        QueryPerformanceCounter(&now);

        int64_t jitter_ticks = now.QuadPart - next_cycle.QuadPart;
        int64_t jitter_us =
            (jitter_ticks * 1000000LL) / sys->frequency.QuadPart;

        // Stockage valeurs jitter
        atomic_store(&sys->last_jitter_us, jitter_us);

        if (jitter_us > (int64_t)sys->cycle_time_us)
            atomic_fetch_add(&sys->jitter_overflow_count, 1);

        if (jitter_us > 0) {
            uint32_t current_max = atomic_load(&sys->max_jitter_us);
            if ((uint32_t)jitter_us > current_max)
                atomic_store(&sys->max_jitter_us, (uint32_t)jitter_us);
        }

        // ============================
        // WATCHDOG (toutes les 10 ms)
        // ============================
        if ((now.QuadPart - last_watchdog_check) >
            sys->frequency.QuadPart / 100) {

            last_watchdog_check = now.QuadPart;

            if (sys->plc_task_manager) {

                uint64_t last_app_cycle =
                    atomic_load(&sys->plc_task_manager->last_cycle_ticks);

                if (last_app_cycle == 0) {
                    watchdog_ok = true;
                }
                else {
                    int64_t elapsed_ticks =
                        now.QuadPart - (int64_t)last_app_cycle;

                    int64_t elapsed_ms =
                        (elapsed_ticks * 1000LL) / sys->frequency.QuadPart;

                    watchdog_ok =
                        (elapsed_ms <= sys->plc_task_manager->watchdog_timeout_ms);
                }

                if (watchdog_ok != watchdog_prev) {
                    if (!watchdog_ok)
                        watchdog_errors++;

                    watchdog_prev = watchdog_ok;
                }
            }
        }

        // ============================
        // CYCLE ETHERCAT
        // ============================
        if (watchdog_ok) {

            for (uint32_t i = 0; i < sys->slave_count; i++) {
                if (sys->slaves[i].pdo.PDO_rx.dirty)
                    ecat_copy_outputs_app_to_rt(&sys->slaves[i].pdo);
            }

            ecx_send_processdata(&sys->ecx_context);
            ecx_receive_processdata(&sys->ecx_context, EC_TIMEOUTRET);
#if !defined TRIPLE_BUFFER_ON_RX
            for (uint32_t i = 0; i < sys->slave_count; i++) {
                ecat_swap_buffers_rt_to_app(&sys->slaves[i].pdo);
            }
#endif
            atomic_fetch_add(&sys->cycle_count, 1);
        }
        else {
            atomic_fetch_add(&sys->errors_count, 1);
        }
    }

    return 0;
}


#else

unsigned __stdcall ecat_rt_thread_old(void* arg) {
    EcatSystem* sys = (EcatSystem*)arg;
    LARGE_INTEGER cycle_start, cycle_end;
    uint64_t elapsed_us, sleep_us;

    // Priorité temps critique
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // Affinité CPU (fixer sur le dernier cœur)
    DWORD_PTR mask = 1ULL << (GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) - 1);
    SetThreadAffinityMask(GetCurrentThread(), mask);

    printf("[RT] Thread démarré, cycle = %llu us\n", sys->cycle_time_us);

    while (atomic_load(&sys->running)) {
        QueryPerformanceCounter(&cycle_start);

        // 1. Copier les outputs de l'application vers l'IOmap
        for (int i = 0; i < sys->slave_count; i++) {
            ecat_copy_outputs_app_to_rt(&sys->slaves[i].pdo);
        }

        // 2. Envoyer les données aux esclaves
        ecx_send_processdata(&sys->ecx_context);

        // 3. Attendre la réception (avec timeout)
        ecx_receive_processdata(&sys->ecx_context, EC_TIMEOUTRET);

        // 4. Copier les inputs de l'IOmap vers les buffers application
        for (int i = 0; i < sys->slave_count; i++) {
            ecat_swap_buffers_rt_to_app(&sys->slaves[i].pdo);
        }

        atomic_fetch_add(&sys->cycle_count, 1);

        // 5. Timing et attente du prochain cycle
        QueryPerformanceCounter(&cycle_end);
        elapsed_us = ((cycle_end.QuadPart - cycle_start.QuadPart) * 1000000LL) / sys->frequency.QuadPart;
        sleep_us = sys->cycle_time_us - elapsed_us;

        // Mesure du jitter
        if (elapsed_us > sys->cycle_time_us) {
            uint32_t jitter = (uint32_t)(elapsed_us - sys->cycle_time_us);
            uint32_t current_max = atomic_load(&sys->max_jitter_us);
            if (jitter > current_max) {
                atomic_store(&sys->max_jitter_us, jitter);
            }
        }

        // Attente active pour les dernières microsecondes (plus précis)
        if (sleep_us > 100) {
            LARGE_INTEGER wait_until;
            wait_until.QuadPart = cycle_start.QuadPart +
                (sys->cycle_time_us * sys->frequency.QuadPart) / 1000000LL;

            while (1) {
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                if (now.QuadPart >= wait_until.QuadPart) break;

                // Sleep pour la majorité du temps
                int64_t remaining_us = ((wait_until.QuadPart - now.QuadPart) * 1000000LL) / sys->frequency.QuadPart;
                if (remaining_us > 1000) {
                    Sleep(1);
                }
                else if (remaining_us > 100) {
                    Sleep(0); // Yield
                }
                // Sinon busy-wait
            }
        }
    }

    printf("[RT] Thread arrêté\n");
    return 0;
}
#endif


// ============================================================================
// SCHEDULER PLC TASK THREAD
// ============================================================================
unsigned __stdcall ecat_app_thread(void* arg) {
    EcatSystem* sys = (EcatSystem*)arg;
    PLCTaskManager_t* tm = sys->plc_task_manager;

    LARGE_INTEGER cycle_start, cycle_end;
    uint64_t app_cycle_time_us = SCHEDULER_PLC_TASK_CYCLE_US;  // 1 ms cycle de base

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    printf("[APP] Thread PLC avec Task Manager démarré\n");

    // Statistiques et blocage EtherCAT
    LARGE_INTEGER last_stats_qpc = { 0 };
    bool was_blocked = false;
    uint32_t blocked_cycles = 0;

    while (atomic_load(&sys->running)) {
        QueryPerformanceCounter(&cycle_start);

#if defined BLOCK_ON_SYSTEM_STATE
        EcatSystemState state = ecat_get_systeme_state(sys);

        if (state != EC_STATE_OPERATIONAL) {
            if (!was_blocked) {
                printf("\n[APP] ⏸️  BLOQUÉ: EtherCAT non-opérationnel (état: %d)\n", state);
                was_blocked = true;
                blocked_cycles = 0;
            }
            blocked_cycles++;
            if (blocked_cycles % 10000 == 0)
                printf("[APP] ⏳ Toujours bloqué... (%u cycles)\n", blocked_cycles);
            Sleep(1);
            continue;
        }
        if (was_blocked) {
            printf("\n[APP] ▶️  REPRISE: EtherCAT opérationnel après %u cycles bloqués\n", blocked_cycles);
            was_blocked = false;
        }
#endif

        // Exécuter les tâches PLC
        plc_task_manager_run(tm);

        //mise à jour desrtination du triple buffering
        //TODO: voir si on peut optimiser, ne pas executer si pas de modif dans write slave
        for (int i = 0; i < tm->slave_count; i++) {
            ecat_commit_app_outputs(&tm->slaves[i].pdo);
        }

        // ====================================================
        // Stats toutes les 10 secondes
        // ====================================================
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (last_stats_qpc.QuadPart == 0)
            last_stats_qpc = now;

        uint64_t elapsed_ms = ((now.QuadPart - last_stats_qpc.QuadPart) * 1000ULL) / sys->frequency.QuadPart;
        if (elapsed_ms >= 10000) {  // 10 s
            //plc_task_manager_print_stats(tm);
            last_stats_qpc = now;
        }

        // ====================================================
        // Attente du prochain cycle
        // ====================================================
        QueryPerformanceCounter(&cycle_end);
        int64_t elapsed_us = ((cycle_end.QuadPart - cycle_start.QuadPart) * 1000000LL) / sys->frequency.QuadPart;
        int64_t sleep_us = app_cycle_time_us - elapsed_us;

        if (sleep_us > 0) {
            Sleep((DWORD)(sleep_us / 1000));  // Sleep en ms
        }
    }

    printf("[APP] Thread PLC arrêté\n");
    return 0;
}


/*****************************************************
*   ecat_watchdog_thread
*****************************************************/
unsigned __stdcall ecat_watchdog_thread(void* arg) {
    EcatSystem* sys = (EcatSystem*)arg;
    uint64_t last_cycle = 0;
    uint16 prev_state = 0;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    printf("[WD] Thread démarré\n");

    while (atomic_load(&sys->running)) {
        // Vérifier que le cycle RT progresse
        uint64_t current_cycle = atomic_load(&sys->cycle_count);
        if (current_cycle == last_cycle) {
            printf("[WD] ALERTE: Thread RT bloqué!\n");
            atomic_fetch_add(&sys->errors_count, 1);
        }
        last_cycle = current_cycle;

        // Vérifier l'état des esclaves
        int16_t state = ecx_readstate(&sys->ecx_context);
        for (int i = 1; i <= sys->ecx_context.slavecount; i++) {
            if (sys->ecx_context.slavelist[i].state != EC_STATE_OPERATIONAL) {
                atomic_store(&sys->system_state, state);

                printf("[WD] Esclave %d en erreur, état: 0x%02x\n",
                    i, sys->ecx_context.slavelist[i].state);

                // Tentative de récupération
                sys->ecx_context.slavelist[i].state = EC_STATE_OPERATIONAL;
                ecx_writestate(&sys->ecx_context, i);
            }
        }

        /*if (prev_state != state) {
            atomic_store(&sys->system_state, state);
            printf("[WD] Changement d'état de Ethercat, state=0x%02x\n", state);
            prev_state = state;
        }*/
        
#if defined JITTER_CALC
        // Afficher les statistiques
        uint32_t jitter = atomic_load(&sys->max_jitter_us);
        uint32_t jitter_overflow = atomic_load(&sys->jitter_overflow_count);
        if (jitter > 100 || jitter_overflow > 0) {
            printf("[WD] Jitter max: %u us, overflow: %d\n", jitter, jitter_overflow);
        }
#endif

        Sleep(1000); // Vérification chaque seconde
    }

    printf("[WD] Thread arrêté\n");
    return 0;
}

// ============================================================================
// INITIALISATION DU SYSTÈME
// ============================================================================

int ecat_system_init(EcatSystem* sys, const char* ifname, int cycle_time_us) {
    memset(sys, 0, sizeof(EcatSystem));

    sys->cycle_time_us = cycle_time_us;
    atomic_store(&sys->system_state, EC_STATE_INIT);

    atomic_store(&sys->running, 0);

    // Initialiser le timer haute résolution
    QueryPerformanceFrequency(&sys->frequency);

    // Initialiser SOEM
    if (ecx_init(&sys->ecx_context, ifname) <= 0) {
        printf("Erreur: Impossible d'initialiser l'interface %s\n", ifname);
        return -1;
    }

    printf("EtherCAT initialisé sur %s\n", ifname);
    return 0;
}



int ecat_system_add_slave(EcatSystem* sys, EcatSlaveInfo* info)
{
    if (!info) return -1;

    // Réallouer le tableau (croissance simple)
    int new_count = sys->slave_count + 1;
    EcatSlave* new_list = (EcatSlave*)realloc(sys->slaves, new_count * sizeof(EcatSlave));
    if (!new_list) return -1;

    sys->slaves = new_list;

    // Init struct
    EcatSlave* s = &sys->slaves[sys->slave_count];
    memset(s, 0, sizeof(*s));
    ecat_slave_init(s, info); // cf. implémentation ci-dessous

    sys->slave_count = new_count;
    return 0;
}



int ecat_system_config(EcatSystem* sys) {
    int wkc;

    printf("[CONFIG] Recherche et configuration des esclaves...\n");

    // 1. Rechercher les esclaves sur le bus
    wkc = ecx_config_init(&sys->ecx_context);
    if (wkc <= 0) {
        printf("[CONFIG] ERREUR: Aucun esclave trouvé!\n");
        return -1;
    }
    printf("[CONFIG] %d esclave(s) trouvé(s)\n", wkc);



    // 2. Configurer les esclaves spécifiques
    for (int i = 1; i <= sys->ecx_context.slavecount; i++) {
        ec_slavet* slave = &sys->ecx_context.slavelist[i];
        printf("[CONFIG] Esclave %d: %s\n", i, slave->name);
        printf("         Man: 0x%08x Prod: 0x%08x Rev: 0x%08x\n",
            slave->eep_man, slave->eep_id, slave->eep_rev);

        // Chercher dans notre liste de slaves configurés
        for (int j = 0; j < sys->slave_count; j++) {
            EcatSlave* ecat_slave = &sys->slaves[j];
            if (ecat_slave->slave_info->man == slave->eep_man &&
                ecat_slave->slave_info->id == slave->eep_id) {

                ecat_slave->slave_index = i;

                
                // Appeler le hook de configuration si présent
                if (ecat_slave->slave_info->config_hook) {
                    ecat_slave->slave_info->config_hook(&sys->ecx_context, i);
                }

                printf("         => Mappé sur EcatSlave[%d]\n", j);
                break;
            }
        }
    }

    Sleep(1000); // 100 Hz par exemple

    // 3. Mapper les PDO (Process Data Objects)
    int size_iommap = ecx_config_map_group(&sys->ecx_context, sys->IOmap, 0);
    if (size_iommap == 0) {
        printf("[CONFIG] erreur pendant le mapping");
        return -1;
    }
    if (size_iommap > sizeof(sys->IOmap)) {
        printf("[CONFIG] tableau IOmap trop petit, taille:%d, requière:%d", sizeof(sys->IOmap), size_iommap);
        return -1;
    }

    printf("[CONFIG] IOmap configuré, taille utilisée: %d bytes\n",
        sys->ecx_context.grouplist[0].Obytes + sys->ecx_context.grouplist[0].Ibytes);

    for (int j = 0; j < sys->slave_count; j++) {
        EcatSlave* ecat_slave = &sys->slaves[j];
        ecat_slave_iomap(ecat_slave, &sys->ecx_context);
    }
    

    // 4. Configurer les Distributed Clocks (DC) si nécessaire
    // ecx_configdc(&sys->ecx_context);

    atomic_store(&sys->system_state, EC_STATE_PRE_OP); //ECAT_STATE_PREOP
    printf("[CONFIG] Configuration terminée - État: PRE-OPERATIONAL\n");
    return 0;
}


int ecat_system_transition_to_safeop(EcatSystem* sys) {

    printf("[SAFEOP] Passage en SAFE-OPERATIONAL...\n");

    // Demander SAFE-OP pour tous les esclaves
    sys->ecx_context.slavelist[0].state = EC_STATE_SAFE_OP;
    ecx_writestate(&sys->ecx_context, 0);

    // Attendre que tous les esclaves atteignent SAFE-OP
    int chk = 200;  // Timeout en nombre de cycles (200 * 50ms = 10s)
    do {
        ecx_statecheck(&sys->ecx_context, 0, EC_STATE_SAFE_OP, 50000);
    } while (chk-- && (sys->ecx_context.slavelist[0].state != EC_STATE_SAFE_OP));

    if (sys->ecx_context.slavelist[0].state != EC_STATE_SAFE_OP) {
        printf("[SAFEOP] ERREUR: Impossible d'atteindre SAFE-OP\n");
        for (int i = 1; i <= sys->ecx_context.slavecount; i++) {
            if (sys->ecx_context.slavelist[i].state != EC_STATE_SAFE_OP) {
                printf("         Esclave %d bloqué en état: 0x%02x\n",
                    i, sys->ecx_context.slavelist[i].state);
            }
        }
        return -1;
    }

    atomic_store(&sys->system_state, EC_STATE_SAFE_OP);
    printf("[SAFEOP] Tous les esclaves en SAFE-OPERATIONAL\n");
    return 0;
}

int ecat_system_transition_to_op(EcatSystem* sys) {
    printf("[OP] Mise à zéro des sorties...\n");

    // IMPORTANT: Mettre les sorties à zéro avant de passer en OP
    // Cela évite des mouvements intempestifs au démarrage
    for (int i = 0; i < sys->slave_count; i++) {
        memset(sys->slaves[i].pdo.PDO_rx.iomap, 0, sys->slaves[i].pdo.PDO_rx.size);
    }

    // Envoyer les données avec sorties à zéro (3 fois pour être sûr)
    for (int i = 0; i < 3; i++) {
        ecx_send_processdata(&sys->ecx_context);
        ecx_receive_processdata(&sys->ecx_context, EC_TIMEOUTRET);
        Sleep(2);
    }

    printf("[OP] Passage en OPERATIONAL...\n");

    // Demander OPERATIONAL pour tous les esclaves
    sys->ecx_context.slavelist[0].state = EC_STATE_OPERATIONAL;
    ecx_writestate(&sys->ecx_context, 0);

    // Attendre que tous les esclaves atteignent OP
    int chk = 200;
    do {
        ecx_send_processdata(&sys->ecx_context);
        ecx_receive_processdata(&sys->ecx_context, EC_TIMEOUTRET);
        ecx_statecheck(&sys->ecx_context, 0, EC_STATE_OPERATIONAL, 50000);
    } while (chk-- && (sys->ecx_context.slavelist[0].state != EC_STATE_OPERATIONAL));

    if (sys->ecx_context.slavelist[0].state != EC_STATE_OPERATIONAL) {
        printf("[OP] ERREUR: Impossible d'atteindre OPERATIONAL\n");
        for (int i = 1; i <= sys->ecx_context.slavecount; i++) {
            if (sys->ecx_context.slavelist[i].state != EC_STATE_OPERATIONAL) {
                printf("    Esclave %d bloqué en état: 0x%02x\n",
                    i, sys->ecx_context.slavelist[i].state);
            }
        }
        return -1;
    }

    atomic_store(&sys->system_state, EC_STATE_OPERATIONAL);
    printf("[OP] Tous les esclaves en OPERATIONAL - Système prêt!\n");
    return 0;
}

int ecat_system_start(EcatSystem* sys) {
    // Séquence complète de démarrage EtherCAT

    // 1. Configuration et mapping des esclaves
    if (ecat_system_config(sys) < 0) {
        return -1;
    }

    // 2. Passage en SAFE-OPERATIONAL
    if (ecat_system_transition_to_safeop(sys) < 0) {
        return -1;
    }

    // 3. Passage en OPERATIONAL (avec sorties à zéro)
    if (ecat_system_transition_to_op(sys) < 0) {
        return -1;
    }

    // 4 . Demarrer le server tcp
    sys->thread_tcp_server = (HANDLE)_beginthreadex(NULL, 0, plc_tcp_server_thread, sys, 0, NULL);

    // 4. Démarrer les threads de traitement
    atomic_store(&sys->running, 1);

    sys->thread_rt = (HANDLE)_beginthreadex(NULL, 0, ecat_rt_thread, sys, 0, NULL);
    sys->thread_app = (HANDLE)_beginthreadex(NULL, 0, ecat_app_thread, sys, 0, NULL);
    sys->thread_watchdog = (HANDLE)_beginthreadex(NULL, 0, ecat_watchdog_thread, sys, 0, NULL);



    if (!sys->thread_rt || !sys->thread_app || !sys->thread_watchdog) {
        printf("Erreur: Impossible de créer les threads\n");
        atomic_store(&sys->running, 0);
        return -1;
    }

    printf("[START] Système démarré avec succès!\n");
    printf("        - Thread RT: cycle %llu us\n", sys->cycle_time_us);
    printf("        - Thread APP: actif\n");
    printf("        - Thread Watchdog: actif\n");

    return 0;
}

void ecat_system_stop(EcatSystem* sys) {
    printf("Arrêt du système...\n");
    atomic_store(&sys->running, 0);

    // Attendre les threads
    HANDLE threads[] = { sys->thread_rt, sys->thread_app, sys->thread_watchdog };
    WaitForMultipleObjects(3, threads, TRUE, 5000);

    CloseHandle(sys->thread_rt);
    CloseHandle(sys->thread_app);
    CloseHandle(sys->thread_watchdog);

    printf("Système arrêté\n");
}


const char* ecat_state_to_string(uint16_t state) {
    switch (state) {
    case EC_STATE_NONE:         return "NONE";
    case EC_STATE_INIT:         return "INIT";
    case EC_STATE_PRE_OP:       return "PRE-OP";
    case EC_STATE_BOOT:         return "BOOT";
    case EC_STATE_SAFE_OP:      return "SAFE-OP";
    case EC_STATE_OPERATIONAL:  return "OP";
    case EC_STATE_ERROR:        return "ERROR";
    default:                    return "UNKNOWN";
    }
}


#if 0


// Cette fonction peut être appelée depuis le thread APP
void application_logic_example(EcatSlave* l230_slave) {
    TX_PDO_t* inputs;   // Données venant des esclaves
    RX_PDO_t* outputs;  // Données vers les esclaves

    // Récupérer les pointeurs thread-safe
    ecat_get_app_buffers(&l230_slave->pdo, &inputs, &outputs);

    // -----------------------------------------------
    // Exemple 1: Copier une entrée vers une sortie
    // -----------------------------------------------
    outputs->L230_DO_Byte0_bits.X15_Out0 = inputs->L230_DI_Byte0.X30_2_In0;

    // -----------------------------------------------
    // Exemple 2: Logique combinatoire
    // -----------------------------------------------
    if (inputs->L230_DI_Byte0.X30_4_In1 && inputs->L230_DI_Byte0.X30_6_In2) {
        outputs->L230_DO_Byte0_bits.X12_Out1 = 1;
    }
    else {
        outputs->L230_DO_Byte0_bits.X12_Out1 = 0;
    }

    // -----------------------------------------------
    // Exemple 3: Lecture de température PT100
    // -----------------------------------------------
    if (!(inputs->X21_CPU_Pt1.X21_CPU_Pt1_State & 0x01)) {
        float resistance_ohm = inputs->X21_CPU_Pt1.X21_CPU_Pt1_Value;

        // Conversion approximative Ohm -> °C pour PT100
        // R(T) ≈ R0(1 + α*T) avec R0=100Ω, α≈0.00385
        float temp_celsius = (resistance_ohm - 100.0f) / 0.385f;

        // Régulation simple: ventilation si > 30°C
        if (temp_celsius > 30.0f) {
            outputs->L230_DO_Byte0_bits.X3_Out3 = 1; // Activer ventilateur
        }
        else if (temp_celsius < 28.0f) {
            outputs->L230_DO_Byte0_bits.X3_Out3 = 0; // Désactiver (hystérésis)
        }
    }
    else {
        // Erreur capteur: mettre les sorties en sécurité
        outputs->L230_DO_Byte0_bits.X3_Out3 = 0;
    }

    // -----------------------------------------------
    // Exemple 4: Machine d'états
    // -----------------------------------------------
    static enum { EC_IDLE, EC_RUNNING, EC_ERROR_S } state = EC_IDLE;
    static int counter = 0;

    switch (state) {
    case EC_IDLE:
        if (inputs->L230_DI_Byte0.X30_8_In3) { // Bouton START
            state = EC_RUNNING;
            counter = 0;
        }
        break;

    case EC_RUNNING:
        outputs->L230_DO_Byte0_bits.X4_Out4 = 1; // LED RUN
        counter++;

        if (inputs->L230_DI_Byte0.X30_11_In4) { // Bouton STOP
            state = EC_IDLE;
            outputs->L230_DO_Byte0_bits.X4_Out4 = 0;
        }

        if (counter > 1000) {
            state = EC_ERROR_S;
        }
        break;

    case EC_ERROR_S:
        outputs->L230_DO_Byte0_bits.X14_Out5 = 1; // LED ERROR
        if (inputs->L230_DI_Byte0.X30_13_In5) { // Bouton RESET
            state = EC_IDLE;
            outputs->L230_DO_Byte0_bits.X14_Out5 = 0;
        }
        break;
    }

    // -----------------------------------------------
    // Exemple 5: Configuration module analogique
    // -----------------------------------------------
    // Le module X23_CPU_VC1 peut être configuré en tension ou courant
    outputs->X23_CPU_VC1_Ctrl = 0x00000001; // Exemple: Mode tension
}

#endif
#include "ecat_system.h"
#include <stdio.h>
#include <signal.h>
#include "application/plc_app_conf.h"
#include "http_server/civetweb.h"

//Global PLC configuration
extern MyPLCApp_conf PLCApp_conf;


//Serveur HTPP
const char* options[] = {
    "listening_ports", "8080",
    "document_root", "www",   // <--- dossier des pages
    0
};



//interface ethercat
static const char* IFACE_NAME = "\\Device\\NPF_{FB092E67-7CA1-4E8F-966D-AC090D396487}"; //usb
//static const char* IFACE_NAME = "\\Device\\NPF_{1EAEBB34-4CE7-4036-8CE8-0D17BF36C88B}"; //intern


// Variable globale pour l'arrêt propre
static EcatSystem* g_system = NULL;

// Handler pour Ctrl+C
void signal_handler(int signum) {
    if (g_system) {
        printf("\n[MAIN] Signal d'arrêt reçu (Ctrl+C)\n");
        ecat_system_stop(g_system);
    }
}

int main(int argc, char* argv[]) {
    EcatSystem system;
    g_system = &system;
    PLCTaskManager_t plc_task_manager;

    printf("==============================================\n");
    printf("  Programme EtherCAT avec SOEM - Windows 10\n");
    printf("==============================================\n\n");

    // Installer le handler de signal
    signal(SIGINT, signal_handler);


    // =========================================================================
    // ETAPE 1: initialiser le server http
    // =========================================================================
    struct mg_context *http_ctx = mg_start(NULL, NULL, options);
    if (http_ctx == NULL) {
        printf("[MAIN] Erreur dans l'initialisation du server http\n");
    }


    // =========================================================================
    // ÉTAPE 1: Initialisation du système EtherCAT
    // =========================================================================

    const char* ifname = IFACE_NAME; // Votre interface réseau
    int cycle_time_us = ECAT_RT_THREAD_CYCLE_US; // 10ms = 1000Hz

    if (argc > 1) {
        ifname = argv[1];
    }
    if (argc > 2) {
        cycle_time_us = atoi(argv[2]);
    }

    printf("[MAIN] Initialisation sur interface: %s\n", ifname);
    printf("[MAIN] Temps de cycle: %d us\n", cycle_time_us);


    if (ecat_system_init(&system, ifname, cycle_time_us) < 0) {
        printf("[MAIN] ERREUR: Échec de l'initialisation\n");
        return -1;
    }

    // =========================================================================
    // ÉTAPE 2: Ajouter les esclaves attendus
    // =========================================================================

    // Allouer la mémoire pour vos esclaves
    extern EcatSlaveInfo L230_info;
    if (ecat_system_add_slave(&system, &L230_info)) {
        printf("[MAIN] ERREUR: Allocation mémoire\n");
        ecx_close(&system.ecx_context);
        return -1;
    }

    // Initialiser votre esclave L230
    ecat_slave_init(&system.slaves[0], &L230_info);
    printf("[MAIN] Esclave L230 ajouté à la configuration\n");

    // =========================================================================
    // ÉTAPE 3: Initialiser le gestionnaire de tâches
    // =========================================================================
    printf("\n[MAIN] Initialisation du gestionnaire de tâches...\n");
    plc_task_manager_init(&plc_task_manager, 10, system.slaves, system.slave_count);  // Max 10 tâches
    system.plc_task_manager = &plc_task_manager;

    //ajoute la tache plc
    //MyPLCApp_conf = { 0 };
    plc_task_manager_add(&plc_task_manager, "tache 1", task_app1, 100, TASK_PRIORITY_NORMAL, &PLCApp_conf);

    //TODO: trié les taches à l'ajout et enlever l'appel task_manager_sort
    task_manager_sort(&plc_task_manager);
    

    // =========================================================================
    // ÉTAPE 4: Initialiser le gestionnaire de tâches
    // =========================================================================
    extern void init_plc_map_io(MyPLCApp_conf_t * plc_conf, EcatSlave * slaves, int slave_count);
    init_plc_map_io(&PLCApp_conf, system.slaves, system.slave_count);

    // =========================================================================
    // ÉTAPE 5: Démarrage du système (config + safe-op + op + threads)
    // =========================================================================

    printf("\n[MAIN] Démarrage du système EtherCAT...\n");

    if (ecat_system_start(&system) < 0) {
        printf("[MAIN] ERREUR: Échec du démarrage\n");
        free(system.slaves);
        ecx_close(&system.ecx_context);
        return -1;
    }


    // =========================================================================
    // ÉTAPE 6: Boucle principale - Surveillance
    // =========================================================================

    printf("\n[MAIN] Système en fonctionnement - Appuyez sur Ctrl+C pour arrêter\n\n");


    while (atomic_load(&system.running)) {
        Sleep(5000); // Toutes les 5 secondes
        
        int state = ecat_get_systeme_state(&system);
        printf("state = 0x%02x\n", state);
        if(state == EC_STATE_OPERATIONAL)
            plc_task_manager_print_stats(&plc_task_manager);


#if STAT
        // Afficher les statistiques
        uint64_t cycles = atomic_load(&system.cycle_count);
        uint64_t errors = atomic_load(&system.errors_count);
        uint32_t jitter = atomic_load(&system.max_jitter_us);

        printf("[STATS] Cycles: %llu | Erreurs: %llu | Jitter max: %u us | État: ",
            cycles, errors, jitter);

        switch (atomic_load(&system.system_state)) {
        case ECAT_STATE_OPERATIONAL: printf("OPERATIONAL"); break;
        case ECAT_STATE_SAFEOP: printf("SAFE-OP"); break;
        case ECAT_STATE_PREOP: printf("PRE-OP"); break;
        case ECAT_STATE_ERROR: printf("ERROR"); break;
        default: printf("UNKNOWN");
        }
        printf("\n");

        // Exemple: Afficher une donnée de votre esclave L230
        EcatSlave* l230 = &system.slaves[0];
        TX_PDO_t* inputs;
        RX_PDO_t* outputs;
        ecat_get_app_buffers(&l230->pdo, &inputs, &outputs);

        printf("[L230]  Température Pt1: %.2f Ω", inputs->X21_CPU_Pt1.X21_CPU_Pt1_Value);
        if (inputs->X21_CPU_Pt1.X21_CPU_Pt1_State & 0x01) {
            printf(" (ERREUR)");
        }
        printf("\n");

        printf("[L230]  Input X30_2: %d | Output X15: %d\n",
            inputs->L230_DI_Byte0.X30_2_In0,
            outputs->L230_DO_Byte0_bits.X15_Out0);

        printf("\n");

#endif
        // Reset du jitter max après affichage
        atomic_store(&system.max_jitter_us, 0);

    }



    // =========================================================================
    // ÉTAPE 7: Arrêt propre
    // =========================================================================

    printf("\n[MAIN] Arrêt du système EtherCAT...\n");

    // Mettre les sorties à zéro avant l'arrêt
    for (int i = 0; i < system.slave_count; i++) {
        memset(system.slaves[i].pdo.PDO_rx.iomap, 0, system.slaves[i].pdo.PDO_rx.size);
    }

    // Envoyer les sorties à zéro
    for (int i = 0; i < 10; i++) {
        ecx_send_processdata(&system.ecx_context);
        ecx_receive_processdata(&system.ecx_context, EC_TIMEOUTRET);
        Sleep(2);
    }

    // Passage en INIT
    system.ecx_context.slavelist[0].state = EC_STATE_INIT;
    ecx_writestate(&system.ecx_context, 0);

    // Nettoyage task manager
    plc_task_manager_cleanup(&plc_task_manager);

    
    //nettoyage des pdo des esclaves
    //TODO: erreur, slave null, à voir
    for (int i = 0; i < system.slave_count; i++) {
        ecat_slave_clean(&system.slaves[i]);
    }

    // Fermeture
    free(system.slaves);
    ecx_close(&system.ecx_context);

    printf("[MAIN] Programme terminé proprement\n");

    return 0;
}
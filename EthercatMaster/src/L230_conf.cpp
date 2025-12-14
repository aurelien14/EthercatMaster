#include "L230_conf.h"
#include "ecat_system.h"
#include "ethercat_slaves.h"

int config_EL230(ecx_contextt* ctx, uint16 slave);

EcatSlaveInfo L230_info = {
    .id = 0x3213335,
    .man = 0x47535953,
    
    .rx_pdo_size = sizeof(L230_RX_PDO_t),
#if !defined TRIPLE_BUFFER_ON_RX
    .tx_pdo_size = sizeof(L230_TX_PDO_t),
#endif

    .config_hook = config_EL230,
    .name = "L230",
};

EcatSlaveInfo L400_info = {
    .id = 0x0,
    .man = 0xff,
    .config_hook = config_EL230
};



int config_EL230(ecx_contextt* ctx, uint16 slave) {
    ec_slavet* pslave = &ctx->slavelist[slave];
    pslave->SM[2].StartAddr = 0x1100;
    pslave->SM[2].SMflags = 0x64;
    pslave->SM[3].StartAddr = 0x1300;
    pslave->SM[3].SMflags = 0x20;

    uint16_t new_timeout = L230_TIME_OUT_PROCESS_DATA; // microsecondes ou unité attendue par le slave
    int ret = ecx_FPWRw(&ctx->port, pslave->configadr, 0x420, new_timeout, EC_TIMEOUTRET);
    if (ret <= 0) {
        printf("Erreur écriture FPWR\n");
    }
    
    /*
    uint16_t v = ecx_FPRDw(&ctx->port, pslave->configadr, 0x420, new_timeout);
    if (v == new_timeout) {
        printf("Succes ecriture registre 0x%04x valeur 0x%04x", 0x420, v);
    }
    else {
        printf("Erreur écriture FPWR\n");
    }*/


    return 1;
}


extern EcatSlaveInfo L230_info;
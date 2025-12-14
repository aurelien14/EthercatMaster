#include "plc_app_conf.h"


MyPLCApp_conf_t PLCApp_conf;




//TODO: enelevr slave et utiliser base address des buffers rx et PDO_tx.iomap
void map_IOtags_to_PDO(Conf_IO_ethercat_t* io, Slave_PDO_t* pdo) {
    set_iotag(&io->X15, "X15", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte0), 0, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X12, "X12", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte0), 1, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X13, "X13", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte0), 2, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X3, "X3", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte0), 3, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X4, "X4", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte0), 4, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X14, "X14", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte0), 5, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X1, "X1", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte0), 6, PLC_BOOL, SL_OUTPUT);

    set_iotag(&io->X11, "X11", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 0, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X10, "X10", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 1, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X5, "X5", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 2, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X6, "X6", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 3, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X7, "X7", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 4, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X8, "X8", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 5, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X9, "X9", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 6, PLC_BOOL, SL_OUTPUT);
    set_iotag(&io->X8a, "X8a", pdo, offsetof(L230_RX_PDO_t, L230_DO_Byte1), 7, PLC_BOOL, SL_OUTPUT);



    set_iotag(&io->DI0, "DI0", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 0, PLC_BOOL, SL_INPUT);
    set_iotag(&io->DI1, "DI1", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 1, PLC_BOOL, SL_INPUT);
    set_iotag(&io->DI2, "DI2", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 2, PLC_BOOL, SL_INPUT);
    set_iotag(&io->DI3, "DI3", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 3, PLC_BOOL, SL_INPUT);
    set_iotag(&io->DI4, "DI4", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 4, PLC_BOOL, SL_INPUT);
    set_iotag(&io->DI5, "DI5", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 5, PLC_BOOL, SL_INPUT);
    set_iotag(&io->DI6, "DI6", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 6, PLC_BOOL, SL_INPUT);
    set_iotag(&io->DI7, "DI7", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte0), 7, PLC_BOOL, SL_INPUT);
    set_iotag(&io->X55, "X55", pdo, offsetof(L230_TX_PDO_t, L230_DI_Byte1), 0, PLC_BOOL, SL_INPUT);

    set_iotag(&io->AV_CPU_Pt_X21, "AV_CPU_Pt_X21", pdo, offsetof(L230_TX_PDO_t, X21_CPU_Pt1.Pt_Value), 0, PLC_REAL, SL_INPUT);
    set_iotag(&io->AV_CPU_Pt_X22, "AV_CPU_Pt_X22", pdo, offsetof(L230_TX_PDO_t, X22_CPU_Pt2.Pt_Value), 0, PLC_REAL, SL_INPUT);
    set_iotag(&io->AV_CPU_VC_X23, "AV_CPU_VC_X23", pdo, offsetof(L230_TX_PDO_t, X23_CPU_VC1.VC_Value), 0, PLC_REAL, SL_INPUT);
}


void init_plc_map_io(MyPLCApp_conf_t* plc_conf, EcatSlave* slaves, int slave_count) {
    Conf_IO_ethercat* IO_Conf = &plc_conf->IO_Conf;
    for (int i = 0; i < slave_count; i++) {
        map_IOtags_to_PDO(IO_Conf, &slaves[i].pdo);
    }
}




void task_app1(PLCTask_t* self) {
	MyPLCApp_conf_t *plc_conf = (MyPLCApp_conf_t*)self->arg;
    const Conf_IO_ethercat* io = &plc_conf->IO_Conf;

	//get_slave_pdo_buffer(&self->slaves[0].pdo, plc_conf);


    if (self->slave_count == 0) return;

    static bool t_on_off_500 = true;
    static int heartbeat;

    if (!self->init) {
        printf("TASK %s initialised\n", self->name);
        plc_conf->timer1 = { 0 };
        plc_conf->timer2 = { 0 };
        t_on_off_500 = true;
        heartbeat = 0;
    }

    L230_TX_PDO_t* inputs;
    L230_RX_PDO_t* outputs;



    if (t_on_off_500) {
        if (plc_ton(self, &plc_conf->timer1, t_on_off_500, 500)) {
            plc_write(&io->X12, 1);
            //outputs->L230_DO_Byte0_bits.X12_Out1 = 1;
            //outputs->L230_DO_Byte1_bits.X6_Out3 = 1;
            t_on_off_500 = 0;
            plc_conf->timer2.output = true;
        }
    }
    else {
        if (!plc_tof(self, &plc_conf->timer2, t_on_off_500, 500)) {
            t_on_off_500 = 1;
            plc_write(&io->X12, 0);
            //outputs->L230_DO_Byte0_bits.X12_Out1 = 0;
            //outputs->L230_DO_Byte1_bits.X6_Out3 = 0;
        }
    }


	
}
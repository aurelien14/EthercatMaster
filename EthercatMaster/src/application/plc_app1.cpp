#include "plc_app_conf.h"
#include "plc_task_manager.h"



static inline void get_slave_pdo_buffer(Slave_PDO_t *pdo, MyPLCApp_conf_t* MyPLCApp_conf) {
	ecat_get_app_buffers(pdo,
		(void**)&MyPLCApp_conf->L230_inputs,
		(void**)&MyPLCApp_conf->L230_outputs);
}


void init_plc(MyPLCApp_conf_t* plc_conf) {
    //Conf_IO_ethercat *io_conf = &plc_conf->IO_Conf;
    //io_conf->X15 = { "X15", PLC_BOOL, IO, NULL};
}


void task_app1(PLCTask_t* self) {
	MyPLCApp_conf_t *plc_conf = (MyPLCApp_conf_t*)self->arg;

	get_slave_pdo_buffer(&self->slaves[0].pdo, plc_conf);


    //if (self->slave_count == 0) return;
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

    ecat_get_app_buffers(&self->slaves[0].pdo,
        (void**)&inputs,
        (void**)&outputs);

    int a = 0;
    for (int i = 0; i < 1000000; i++) {
        a = a * 10;
    }

    if (t_on_off_500) {
        if (plc_ton(self, &plc_conf->timer1, t_on_off_500, 500)) {
            outputs->L230_DO_Byte0_bits.X12_Out1 = 1;
            outputs->L230_DO_Byte1_bits.X6_Out3 = 1;
            t_on_off_500 = 0;
            plc_conf->timer2.output = true;
        }
    }
    else {
        if (!plc_tof(self, &plc_conf->timer2, t_on_off_500, 500)) {
            t_on_off_500 = 1;
            outputs->L230_DO_Byte0_bits.X12_Out1 = 0;
            outputs->L230_DO_Byte1_bits.X6_Out3 = 0;
        }
    }

    /*heartbeat++;
    outputs->L230_DO_Byte0_bits.X15_Out0 = heartbeat & 0x01;*/
    if (!outputs->L230_DO_Byte0_bits.X15_Out0) {
        outputs->L230_DO_Byte0_bits.X15_Out0 = 1;
    }

    // Lire température
    if (!(inputs->X21_CPU_Pt1.X21_CPU_Pt1_State & 0x01)) {
        float resistance = inputs->X21_CPU_Pt1.X21_CPU_Pt1_Value;
        float temp_celsius = (resistance - 100.0f) / 0.385f;

        // PID simple (à compléter avec votre logique)
        float setpoint = 50.0f;
        if (temp_celsius < setpoint - 2.0f) {
            //outputs->L230_DO_Byte0_bits.X3_Out3 = 1;  // Chauffage ON
        }
        else if (temp_celsius > setpoint + 2.0f) {
            //outputs->L230_DO_Byte0_bits.X3_Out3 = 0;  // Chauffage OFF
        }
    }

	ecat_commit_app_outputs(&self->slaves[0].pdo);
}
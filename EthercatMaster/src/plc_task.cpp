
#include "plc_functions.h"
#include "ethercat_slaves.h"
#include "L230_conf.h"

// =========================================================================
// EXEMPLE DE LOGIQUE APPLICATIVE PERSONNALISÉE
// =========================================================================
static PLC_timer timer1;
static PLC_timer timer2;

void task1(PLCTask_t* self) {
    //if (self->slave_count == 0) return;
    static bool t_on_off_500 = true;
    static int heartbeat;

    if (!self->init) {
        printf("TASK %s initialised\n", self->name);
        timer1 = { 0 };
        timer2 = { 0 };
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


    //printf("task1 tx_size=%d, rx_size=%d\n", self->slaves[0].pdo.rx_size, self->slaves[0].pdo.tx_size);

    if (t_on_off_500) {
        if (plc_ton(self, &timer1, t_on_off_500, 500)) {
            outputs->L230_DO_Byte1_bits.X7_Out4 = 1;
            outputs->L230_DO_Byte1_bits.X6_Out3 = 1;
            t_on_off_500 = 0;
            timer2.output = true;
        }
    }
    else {
        if (!plc_tof(self, &timer2, t_on_off_500, 500)) {
            t_on_off_500 = 1;
            outputs->L230_DO_Byte1_bits.X7_Out4 = 0;
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

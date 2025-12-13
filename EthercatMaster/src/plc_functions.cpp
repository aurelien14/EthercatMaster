#include <stdint.h>
#include "plc_functions.h"

// ============================================================================
// FONCTIONS AUXILIAIRES PLC
// ============================================================================

// Temporisation ON (TON) - démarre quand input=TRUE
bool plc_ton(PLCTask_t *self, PLC_timer *t, bool input, uint32_t preset_ms) {
    uint64_t now = self->timestamp_ms;

    if (input) {
        if (!t->active) {
            t->active = true;
            t->start_time_ms = now;
            t->preset_ms = preset_ms;
            t->output = false;
        }
        else {
            uint64_t elapsed = now - t->start_time_ms;
            if (elapsed >= t->preset_ms) {
                t->output = true;
                t->active = false;
            }
        }
    }
    else {
        t->active = false;
        t->output = false;
    }

    return t->output;
}


// Temporisation OFF (TOF) - maintient output pendant preset après input=FALSE
bool plc_tof(PLCTask_t* self, PLC_timer* t, bool input, uint32_t preset_ms) {
    uint64_t now = self->timestamp_ms;

    if (!input && t->output) {
        if (!t->active) {
            t->active = true;
            t->start_time_ms = now;
            t->preset_ms = preset_ms;
        }
        else {
            uint64_t elapsed = now - t->start_time_ms;
            if (elapsed >= t->preset_ms) {
                t->output = false;
                t->active = false;
            }
        }
    }
    else if (input) {
        t->active = false;
        t->output = true;
    }

    return t->output;
}

#if 0
// Compteur UP (CTU)
static inline bool plc_ctu(PLC_Context_t* plc, int counter_id, bool count_up, bool reset, int32_t preset) {
    auto* c = &plc->counters[counter_id];
    static bool last_count_up[16] = { false };

    if (reset) {
        c->count = 0;
        c->output = false;
    }
    else if (count_up && !last_count_up[counter_id]) {
        c->count++;
        if (c->count >= preset) {
            c->output = true;
        }
    }

    last_count_up[counter_id] = count_up;
    c->preset = preset;
    return c->output;
}
#endif
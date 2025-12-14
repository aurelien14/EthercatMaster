#include <stdint.h>
#include <math.h>
#include "plc_functions.h"

// ============================================================================
// FONCTIONS AUXILIAIRES PLC
// ============================================================================

// Temporisation ON (TON) - démarre quand input=TRUE
boolean plc_ton(PLCTask_t *self, PLC_timer_t *t, boolean input, uint32_t preset_ms) {
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
boolean plc_tof(PLCTask_t* self, PLC_timer_t* t, boolean input, uint32_t preset_ms) {
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

//rampe en x/s
int ramp(PLCTask_t* self, boolean input, uint32_t slope) {
    uint64_t now = self->timestamp_ms;
    return 0;
}

/**
 * Convertit la résistance d'une PT100 en température (°C)
 *
 * Utilise l'équation de Callendar-Van Dusen pour PT100
 * R(T) = R0 * (1 + A*T + B*T² + C*(T-100)*T³)
 *
 * Pour T >= 0°C : C = 0
 * Pour T < 0°C : on utilise l'équation complète
 *
 * Constantes standards IEC 60751 :
 * R0 = 100 Ω à 0°C
 * A = 3.9083 × 10⁻³
 * B = -5.775 × 10⁻⁷
 * C = -4.183 × 10⁻¹² (pour T < 0°C)
 *
 * @param resistance Résistance mesurée en ohms
 * @return Température en degrés Celsius, ou NAN si hors plage
 */
float32 pt100_resistance_to_temperature(float32 resistance) {
    // Constantes IEC 60751 pour PT100
    const float32 R0 = 100.0f;        // Résistance à 0°C
    const float32 A = 3.9083e-3f;
    const float32 B = -5.775e-7f;
    const float32 C = -4.183e-12f;    // Utilisé seulement pour T < 0°C

    // Vérification de la plage valide (environ -200°C à 850°C)
    if (resistance < 18.52f || resistance > 390.0f) {
        return NAN;  // Hors plage
    }

    // Calcul pour T >= 0°C (équation simplifiée)
    // R = R0 * (1 + A*T + B*T²)
    // Résolution de l'équation du second degré: B*T² + A*T + (1 - R/R0) = 0

    if (resistance >= R0) {
        // Température positive
        float32 discriminant = A * A - 4.0f * B * (1.0f - resistance / R0);

        if (discriminant < 0) {
            return NAN;
        }

        float32 temp = (-A + sqrtf(discriminant)) / (2.0f * B);
        return temp;
    }
    else {
        // Température négative - utilise une approximation itérative
        // ou l'équation complète de Callendar-Van Dusen
        float32 temp = (resistance / R0 - 1.0f) / A;  // Première approximation

        // Itération de Newton-Raphson pour affiner
        for (int i = 0; i < 5; i++) {
            float32 R_calc = R0 * (1.0f + A * temp + B * temp * temp +
                C * (temp - 100.0f) * temp * temp * temp);
            float32 dR_dT = R0 * (A + 2.0f * B * temp +
                C * (4.0f * temp * temp * temp - 300.0f * temp * temp));
            temp = temp - (R_calc - resistance) / dR_dT;
        }

        return temp;
    }
}

/**
 * Version simplifiée utilisant une approximation linéaire
 * Précision : ±2°C environ entre -50°C et 200°C
 * Plus rapide mais moins précise
 */
float32 pt100_resistance_to_temperature_simple(float32 resistance) {
    const float32 R0 = 100.0f;
    const float32 alpha = 0.00385f;  // Coefficient de température moyen

    // T ≈ (R - R0) / (R0 * alpha)
    return (resistance - R0) / (R0 * alpha);
}
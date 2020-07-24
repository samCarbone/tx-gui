#include "altitudecontroller.h"

AltitudeController::AltitudeController(QObject *parent) : QObject(parent)
{



}

void AltitudeController::altitudeReceived(RangingData_t alt)
{
    measurement = alt;

    if(!state_set) {
        state_est.pos_z = alt.range_mm;
        state_est.vel_z = 0;
        state_est.time = alt.time_meas_ms;
        state_set = true;
        return;
    }

    // Propagate
    state_est.pos_z = state_est.pos_z + state_est.vel_z*(alt.time_meas_ms - state_est.time);
    state_est.vel_z = state_est.vel_z;

    // Measurement update

}


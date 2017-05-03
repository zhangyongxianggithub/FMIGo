#ifndef MODELEXCHANGE_H
#define MODELEXCHANGE_H

#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include <stdbool.h>
#include "gsl-interface.h"
typedef struct TimeLoop
{
    fmi2_real_t t_safe, dt_new, t_crossed, t_end;
} TimeLoop;
typedef struct Backup
{
    double t;
    double h;
    double *dydt;
    fmi2_real_t* ei;
    fmi2_real_t* ei_b;
    fmi2_real_t* x;
    unsigned long failed_steps;
    fmi2_event_info_t eventInfo;
}Backup;
typedef struct fmu_parameters{
    int nx;
    int ni;

    double t_ok;
    double t_past;
    int count;                    /* number of function evaluations */

    bool stateEvent;
}fmu_parameters;

void setFMUstate();
Backup* getBackup();
Backup* getTempBackup();
fmi2_import_t** getFMU();
void restoreStates(cgsl_simulation *sim, Backup *backup);
void storeStates(cgsl_simulation *sim, Backup *backup);
void runIteration(cgsl_simulation *sim, double t, double dt, Backup *backup);
void prepare(cgsl_simulation *sim, enum cgsl_integrator_ids integrator);
#endif

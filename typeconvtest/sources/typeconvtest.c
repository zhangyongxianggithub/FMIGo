#include "modelDescription.h"

#define SIMULATION_INIT typeconvtest_init

#include "fmuTemplate.h"

static void typeconvtest_init(state_t *s) {
    s->md.r_in = s->md.r_out = s->md.r0;
    s->md.i_in = s->md.i_out = s->md.i0;
    s->md.b_in = s->md.b_out = s->md.b0;
    //s->md.s_in = s->md.s_out = s->md.s0;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    s->md.r_out = s->md.r_in;
    s->md.i_out = s->md.i_in;
    s->md.b_out = s->md.b_in;
    //s->md.s_out = s->md.s_in;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

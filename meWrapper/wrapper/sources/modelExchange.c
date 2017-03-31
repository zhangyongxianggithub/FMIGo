#include "modelExchange.h"
#include "math.h"
#ifndef max
#define max(a,b) ((a>b) ? a : b)
#define min(a,b) ((a<b) ? a : b)
#endif
/* //#define get_storage m_baseMaster->get_storage */
/* /\** get_p */
/*  *  Extracts the parameters from the model */
/*  * */
/*  *  @param m Model */
/*  *\/ */
inline fmu_parameters* get_p(fmu_model* m){
    return (fmu_parameters*)(m->model->get_model_parameters(m->model));
}

bool past_event(fmi2Real* a, fmi2Real* b, int i){
    for(;i>0;--i){
        if(signbit( a[i] ) != signbit( b[i] ))
            return true;
    }
    return false;
}

/* /\** fmu_function */
/*  *  function needed by cgsl_simulation to get and set the current */
/*  *  states and derivatives */
/*  * */
/*  *  @param t Input time */
/*  *  @param x Input states vector */
/*  *  @param dxdt Output derivatives */
/*  *  @param params Contains model specific parameters */
/*  *\/ */
static int fmu_function(double t, const double x[], double dxdt[], void* params)
{
    // make local variables
    fmu_parameters* p = (fmu_parameters*)params;

    ++p->count; /* count function evaluations */
    if(p->stateEvent)return GSL_SUCCESS;

    fmi2SetTime(wrapper.m_fmi2Instance,t);
    fmi2SetContinuousStates(wrapper.m_fmi2Instance,x,p->nx);

    fmi2GetDerivatives(wrapper.m_fmi2Instance,dxdt,p->nx);

    if(p->ni){
        fmi2GetEventIndicators(wrapper.m_fmi2Instance,p->ei,p->ni);
        if(past_event(p->ei,p->ei_backup,p->ni)){
            p->stateEvent = true;
            p->t_past = t;
            return GSL_SUCCESS;
        } else{
            p->stateEvent = false;
            p->t_ok = t;
        }
    }

    return GSL_SUCCESS;
}


/* /\** allocate Memory */
/*  *  Allocates memory needed by the fmu_model */
/*  * */
/*  *  @param m The fmu_model */
/*  *  @param clients Vector with clients */
/*  *\/ */
void allocateMemory(fmu_model *m){
    m->model = (cgsl_model*)calloc(1,sizeof(cgsl_model));
    m->model->n_variables = fmi2_import_get_number_of_continuous_states(wrapper.m_fmi2Instance);;
    if(m->model->n_variables == 0){
        fprintf(stderr,"ModelExchangeStepper nothing to integrate\n");
        exit(0);
    }

    m_p.nx = m->model->n_variables;
    m_p.ni = fmi2_import_get_number_of_event_indicators(wrapper.m_fmi2Instance);;
    m_p.ei             = (fmi2Real*)calloc(m_p.ni, sizeof(fmi2_real_t));
    m_p.ei_backup      = (fmi2Real*)calloc(m_p.ni, sizeof(fmi2_real_t));
    m->model->x        = (double*)calloc(m->model->n_variables, sizeof(double));
    m->model->x_backup = (double*)calloc(m->model->n_variables, sizeof(double));
    m_p.backup.dydt    = (double*)calloc(m->model->n_variables, sizeof(double));

    if(!m->model->x || !m->model->x_backup || !m_p.backup.dydt){
        //freeFMUModel(m);
        perror("WeakMaster:ModelExchange:allocateMemory ERROR -  could not allocate memory");
        exit(1);
    }
}

/* /\** init_fmu_model */
/*  *  Setup all parameters and function pointers needed by fmu_model */
/*  * */
/*  *  @param m The fmu_model we are working on */
/*  *  @param client A vector with clients */
/*  *\/ */

void init_fmu_model(fmu_model *m){
    allocateMemory(m);
    m->model->parameters = (void*)&m_p;
    m->model->get_model_parameters = get_model_parameters;
    fmu_parameters* p = get_p(m);

    p->t_ok = 0;
    p->t_past = 0;
    p->stateEvent = false;
    p->count = 0;

    p->backup.t = 0;
    p->backup.h = 0;

    //m->model->function = fmu_function;
    m->model->jacobian = NULL;
    m->model->post_step = NULL;
    m->model->pre_step = NULL;
    m->model->free = cgsl_model_default_free;//freeFMUModel;

    //fmi2_status_t status = fmi2GetContinuousStates(wrapper.m_fmi2Instance, m->model->x, m->model->n_variables);

    memcpy(m->model->x_backup,m->model->x,m->model->n_variables);
}


/* /\** prepare() */
/*  *  Setup everything */
/*  *\/ */
void prepare() {
    fprintf(stderr,"prepare \n");
    init_fmu_model(&m_model);
    // set up a gsl_simulation for each client
    fmu_parameters* p = get_p(&m_model);

    //m_sim = (cgsl_simulation *)malloc(sizeof(cgsl_simulation));
    m_sim = cgsl_init_simulation(m_model.model,
                                 rk8pd, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                 1e-10,
                                 0,
                                 0,
                                 0, NULL
                                 );
#ifdef USE_GPL
#endif
}

/* /\** restoreStates() */
/*  *  Restores all values needed by the simulations to restart */
/*  *  before the event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void restoreStates(cgsl_simulation *sim){
    fmu_parameters* p = get_p((fmu_model*)sim->model);
    //restore previous states


    memcpy(sim->model->x,sim->model->x_backup,sim->model->n_variables);

    memcpy(sim->i.evolution->dydt_out, p->backup.dydt,
           sim->model->n_variables * sizeof(p->backup.dydt[0]));

    sim->i.evolution->failed_steps = p->backup.failed_steps;
    sim->t = p->backup.t;
    sim->h = p->backup.h;
}

/* /\** storeStates() */
/*  *  Stores all values needed by the simulations to restart */
/*  *  from a state before an event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void storeStates(cgsl_simulation *sim){
    fmu_parameters* p = get_p((fmu_model*)sim->model);
    memcpy(sim->model->x_backup, sim->model->x, sim->model->n_variables);

    p->backup.failed_steps = sim->i.evolution->failed_steps;
    p->backup.t = sim->t;
    p->backup.h = sim->h;

    memcpy(p->backup.dydt, sim->i.evolution->dydt_out,
           sim->model->n_variables * sizeof(p->backup.dydt[0]));
}

/* /\** hasStateEvent() */
/*  *  Retrieve stateEvent status */
/*  *  Returns true if at least one simulation crossed an event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
bool hasStateEvent(cgsl_simulation *sim){
    return get_p((fmu_model*)sim->model)->stateEvent;
}

/* /\** getGoldenNewTime() */
/*  *  Calculates a time step which brings solution closer to the event */
/*  *  Uses the golden ratio to get t_crossed and t_safe to converge */
/*  *  to the event time */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void getGoldenNewTime(cgsl_simulation *sim){
    // golden ratio
    double phi = (1 + sqrt(5)) / 2;
    /* passed solution, need to reduce tEnd */
    if(hasStateEvent(sim)){
        getSafeAndCrossed();
        restoreStates(sim);
        timeLoop.dt_new = (timeLoop.t_crossed - sim->t) / phi;
    } else { // havent passed solution, increase step
        storeStates(sim);
        timeLoop.t_safe = max(timeLoop.t_safe, sim->t);
        timeLoop.dt_new = timeLoop.t_crossed - sim->t - (timeLoop.t_crossed - timeLoop.t_safe) / phi;
    }
}

/* /\** step() */
/*  *  Run cgsl_step_to the simulation */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void step(cgsl_simulation *sim){
    fmu_parameters *p;
    p = get_p((fmu_model*)sim->model);
    p->stateEvent = false;
    p->t_past = max(p->t_past, sim->t + timeLoop.dt_new);
    p->t_ok = sim->t;

    cgsl_step_to(&sim, sim->t, timeLoop.dt_new);
}

/* /\** stepToEvent() */
/*  *  To be runned when an event is crossed. */
/*  *  Finds the event and returns a state immediately after the event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void stepToEvent(cgsl_simulation *sim){
    double tol = 1e-9;
    while(!hasStateEvent(sim) &&(
          /*!(m_baseMaster->get_storage().absmin(STORAGE::indicators) < tol || */timeLoop.dt_new < tol)){
        getGoldenNewTime(sim);
        step(sim);
        if(timeLoop.dt_new == 0){
            fprintf(stderr,"stepToEvent: dt == 0, abort\n");
            exit(1);
        }
        if(hasStateEvent(sim)){
            // step back to where event occured
            restoreStates(sim);
            getSafeAndCrossed();
            timeLoop.dt_new = timeLoop.t_safe - sim->t;
            step(sim);
            if(!hasStateEvent(sim)) storeStates(sim);
            else{
                fprintf(stderr,"stepToEvent: failed at stepping to safe time, aborting\n");
                exit(1);
            }
            timeLoop.dt_new = timeLoop.t_crossed - sim->t;
            step(sim);
            if(!hasStateEvent(sim)){
                fprintf(stderr,"stepToEvent: failed at stepping to event \n");
                exit(1);
            }
        }
    }
}

/* /\** newDiscreteStates() */
/*  *  Should be used where a new discrete state ends and another begins. */
/*  *  Store the current state of the simulation */
/*  *\/ */
void newDiscreteStates(){
    fmu_parameters* p = get_p((fmu_model*)&m_sim.model);
    // start at a new state
    //p->FMIGO_ME_ENTER_EVENT_MODE(m_clients);
    //fmi2EnterEventMode(wrapper.m_fmi2Instance);
    // todo loop until newDiscreteStatesNeeded == false

    p->eventInfo.newDiscreteStatesNeeded = true;
    while(p->eventInfo.newDiscreteStatesNeeded){
        //   fmi2NewDiscreteStates(wrapper.m_fmi2Instance,&p->eventInfo);
        if(p->eventInfo.terminateSimulation){
                fprintf(stderr,"modelExchange.c: terminated simulation\n");
                exit(1);
        }
    }

    fmi2EnterContinuousTimeMode(wrapper.m_fmi2Instance);

    fmi2GetEventIndicators(wrapper.m_fmi2Instance,p->ei,p->ni);

    // store the current state of all running FMUs
    storeStates(&m_sim);
}

/* /\** getSafeAndCrossed() */
/*  *  Extracts safe and crossed time found by fmu_function */
/*  *\/ */
void getSafeAndCrossed(){
    fmu_parameters *p = get_p((fmu_model*)m_sim.model);
    timeLoop.t_safe    = p->t_ok;//max( timeLoop.t_safe,    t_ok);
    timeLoop.t_crossed = p->t_past;//min( timeLoop.t_crossed, t_past);
}

/* /\** safeTimeStep() */
/*  *  Make sure we take small first step when we're at on event */
/*  *  @param sim The simulation */
/*  *\/ */
void safeTimeStep(cgsl_simulation *sim){
    // if sims has a state event do not step to far
    if(hasStateEvent(sim)){
        double absmin = 0;//m_baseMaster->get_storage().absmin(STORAGE::indicators);
        timeLoop.dt_new = sim->h * (absmin > 0 ? absmin:0.00001);
    }else
        timeLoop.dt_new = timeLoop.t_end - sim->t;
}

/* /\** getSafeTime() */
/*  * */
/*  *  @param clients Vector with clients */
/*  *  @param t The current time */
/*  *  @param dt Timestep, input and output */
/*  *\/ */
void getSafeTime(double t, double *dt){
    fmu_parameters* p = get_p((fmu_model*)&m_sim.model);
    if(p->eventInfo.nextEventTimeDefined)
        *dt = min(*dt, t - p->eventInfo.nextEventTime);
}

/* /\** runIteration() */
/*  *  @param t The current time */
/*  *  @param dt The timestep to be taken */
/*  *\/ */
void runIteration(double t, double dt) {
    timeLoop.t_safe = t;
    timeLoop.t_end = t + dt;
    timeLoop.dt_new = dt;
    getSafeTime(t, &timeLoop.dt_new);
    newDiscreteStates();
    int iter = 2;
    while( timeLoop.t_safe < timeLoop.t_end ){
        step(&m_sim);
        if (hasStateEvent(&m_sim)){
            getSafeAndCrossed();

            // restore and step to before the event
            restoreStates(&m_sim);
            timeLoop.dt_new = timeLoop.t_safe - m_sim.t;
            step(&m_sim);

            // store and step to the event
            if(!hasStateEvent(&m_sim)) storeStates(&m_sim);
            timeLoop.dt_new = timeLoop.t_crossed - m_sim.t;
            step(&m_sim);

            // step closer to the event location
            if(hasStateEvent(&m_sim))
                stepToEvent(&m_sim);

        }
        else {
            timeLoop.t_safe = m_sim.t;
            timeLoop.t_crossed = timeLoop.t_end;
        }

        safeTimeStep(&m_sim);
        if(hasStateEvent(&m_sim))
            newDiscreteStates();
        storeStates(&m_sim);
    }
}

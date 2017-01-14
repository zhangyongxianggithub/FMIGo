/*
 * WeakMasters.h
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#ifndef WEAKMASTERS_H_
#define WEAKMASTERS_H_
#include "master/BaseMaster.h"
#include "master/WeakConnection.h"
#include "common/common.h"
#include <fmitcp/serialize.h>
#include <FMI2/fmi2_types.h>
#include "common/gsl_interface.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

using namespace fmitcp::serialize;

namespace fmitcp_master {

//aka parallel stepper
class JacobiMaster : public BaseMaster {
public:
    JacobiMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections) :
            BaseMaster(clients, weakConnections) {
        fprintf(stderr, "JacobiMaster\n");
    }

    void prepare() {
    }

    void runIteration(double t, double dt) {
        //get connection outputs
        for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            it->first->sendGetX(it->second);
        }
        wait();

        //print values
        /*for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            int i = 0;
            for (auto it2 = it->first->m_getRealValues.begin(); it2 != it->first->m_getRealValues.end(); it2++, i++) {
                fprintf(stderr, "%i real VR %i = %f\n", it->first->getId(), it->second[i], *it2);
            }
        }*/

        //set connection inputs, pipeline with do_step()
        const InputRefsValuesType refValues = getInputWeakRefsAndValues(m_weakConnections);

        // redirect outputs to inputs
        for (auto it = refValues.begin(); it != refValues.end(); it++) {
            it->first->sendSetX(it->second);
        }

        // why not wait here?? wait()

        sendWait(m_clients, fmi2_import_do_step(0, 0, t, dt, true));

    }
};

//aka serial stepper
class GaussSeidelMaster : public BaseMaster {
    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
public:
    GaussSeidelMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, std::vector<int> stepOrder) :
        BaseMaster(clients,weakConnections), stepOrder(stepOrder) {
        fprintf(stderr, "GSMaster\n");
    }

    void prepare() {
        for (size_t x = 0; x < m_weakConnections.size(); x++) {
            WeakConnection wc = m_weakConnections[x];
            clientGetXs[wc.to][wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
        }
    }

    void runIteration(double t, double dt) {
        //fprintf(stderr, "\n\n");
        for (int o : stepOrder) {
            FMIClient *client = m_clients[o];

            for (auto it = clientGetXs[client].begin(); it != clientGetXs[client].end(); it++) {
                it->first->sendGetX(it->second);
            }
            wait();
            const SendSetXType refValues = getInputWeakRefsAndValues(m_weakConnections, client);
            client->sendSetX(refValues);
            sendWait(client, fmi2_import_do_step(0, 0, t, dt, true));
        }
        //fprintf(stderr, "\n\n");
    }
};

//aka serial stepper
class ModelExchangeStepper : public BaseMaster {
    typedef struct TimeLoop
    {
        fmi2_real_t t_start, t_safe, t_new, t_crossed, t_end;
    } TimeLoop;
    typedef struct backup
    {
        std::vector<double> x;
        double t;
        unsigned long failed_steps;

        FILE* result_file;
        long size_of_file;

    } backup;
    typedef struct fmu_parameters{

        std::vector<double> z;        /* state event indicators */
        int count;                    /* number of function evaluations */
        int nz;                       /* number of event indicators, from XML file */
        int nx;                       /* number of states, from XML file */
        int loggingOn;                /* loggingOn == true => logging is enabled */
        char separator;               /* not used */
        char* resultFile;             /* path and file name of where to store result */
        fmi2_string_t* categories;       /* only used in setDebugLogging */
        int nCategories;              /* only used in setDebugLogging */

        BaseMaster* baseMaster;       /* BaseMaster object pointer */
        FMIClient* client;            /* FMIClient object pointer */

      //fmi2_real_t t_start, t_end;//t_safe, t_new, t_crossed;
        fmi2_event_info_t event;

        bool stateEvent;
        backup m_backup;
    }fmu_parameters;
    struct fmu_model{
      cgsl_model model;
    };
    vector<cgsl_simulation*> m_sims;
    vector<WeakConnection> m_weakConnections;
    enum INTEGRATORTYPE m_integratorType;
    double m_tolerance;
    TimeLoop timeLoop;

    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
 public:
 ModelExchangeStepper(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, double relativeTolerance, enum INTEGRATORTYPE integratorType  ) :
    BaseMaster(clients,weakConnections), m_tolerance(relativeTolerance), m_integratorType(integratorType)
    {
      fprintf(stderr, "ModelExchangeStepper\n");
    }

    /** getMedelResultPath
     *  extracts the filename of the path without extension,
     *  creates a directory to place the result file and
     *
     *  @param fmuPath
     */
    char* getModelResultPath(const char* fmuPath)
    {
        size_t n = strlen(fmuPath);
        char* p = (char*)malloc((1+n) * sizeof(char));

        // copy path to p, tp for destruction only.
        void* tp = memcpy(p, fmuPath, n);
        *(p+n) = '\0';

        // pointer to be
        char* name = p;

        // find file name of path 'path'
        while((p = strchr(p, '/')) != NULL)
            name = ++p;

        // remove file extension
        if((p = strchr(name, '.')) != NULL)
            *p = '\0';

        // allocate memory for resultFile
        size_t s = strlen(name);
        char* resultFile = (char*)calloc(s + 10, sizeof(char));

        // create the resultFile
        sprintf(resultFile, "data/");

        // if there is no directory, create it
        mkdir(resultFile, 0700);

        // add path and resultFile
        sprintf(resultFile, "%s%s.mat", resultFile, name);

        // free copy of path
        free(tp);
        return resultFile;
    }

    /** fmu_function
     *  function needed by cgsl_simulation to get and set the current
     *  states and derivatives
     *
     *  @param t Input time
     *  @param x Input states vector
     *  @param dxdt Output derivatives
     *  @param params Contains model specific parameters
     */
    static int fmu_function(double t, const double x[], double dxdt[], void* params)
    {
        // make local variables
        fmu_parameters* p = (fmu_parameters*)params;
        //        fmi2Component c = p->c; // instance of the fmu
        int nx = p->nx;
        /*should make sure we are in continuous state */

        ++p->count; /* count function evaluations */
        p->baseMaster->send(p->client, fmi2_import_set_time(0,0,t));
        p->baseMaster->send(p->client, fmi2_import_set_continuous_states(0,0,x,nx));
        p->baseMaster->send(p->client, fmi2_import_get_derivatives(0,0,nx));

        p->baseMaster->wait();

        common::extract_vector(dxdt, p->client->m_getDerivatives);

        // maybe just send all three and then wait?
        return GSL_SUCCESS;
    }

    /** getParameters
     *  Extracts the parameters from the model
     *
     *  @param m Model
     */
    fmu_parameters* getParameters(cgsl_model* m){
        return (fmu_parameters*)(m->parameters);
    }
    fmu_parameters* getParameters(fmu_model* m){
        return (fmu_parameters*)(m->model.parameters);
    }

    /** freeFMUModel
     *  Free all memory allocated by allacateMemory(fmu_model* m)
     *
     *  @param m Pointer to a fmu_model
     */
    void freeFMUModel(fmu_model* m){
        if(m == NULL) return;

        fmu_parameters* p = getParameters(m);
        if( p != NULL)             return;
        if( p->resultFile != NULL) free(p->resultFile);
        if( m->model.x != NULL)    free(m->model.x);

        p->m_backup.x.clear();
        free(p);
        free(m);
    }
    void freeSim(void){
      fmu_parameters *p;
      while(m_sims.size()){
        freeFMUModel((fmu_model *)m_sims[0]->model);
        free(m_sims[0]);
        m_sims.pop_back();
      }
    }

    /** allocate Memory
     *  Allocates memory needed by the fmu_model
     *
     *  @param m Pointer to a fmu_model
     */
    void allocateMemory(fmu_model* m){
        fmu_parameters* p = getParameters(m);
        m->model.x    = (double*)calloc(p->nx, sizeof(double));
        p->m_backup.x.reserve(p->nx);

        if((!m->model.x || p->m_backup.x.size() == 0)) {
            freeFMUModel(m);
            perror("WeakMaster:ModelExchange:allocateMemory ERROR -  could not allocate memory");
            exit(1);
        }
    }

    /** init_fmu_model
     *  Creates the fmu_model and sets the parameters
     *
     *  @param client A pointer to a fmi client
     */
    cgsl_model* init_fmu_model(FMIClient* client){
        fmu_parameters* p = (fmu_parameters*)malloc(sizeof(fmu_parameters));
        fmu_model* m = (fmu_model*)malloc(sizeof(fmu_model));

        m->model.parameters = (void*)p;
        p->client = client;
        // TODO one result file for each fmu
        p->resultFile = getModelResultPath("resultFile");
        if ( ( p->m_backup.result_file = fopen(p->resultFile, "w+") ) == NULL){
          printf("can't open file: %s\n", p->resultFile);
          exit(1);
        }
        p->baseMaster = this;
        p->nx = p->client->getNumContinuousStates();
        p->nz = p->client->getNumEventIndicators();
        m->model.n_variables = p->nx;

      allocateMemory(m);

        allocateMemory(m);

        m->model.function = fmu_function;
        m->model.jacobian = NULL;
        m->model.free = NULL;//freeFMUModel;
        sendWait(p->client, fmi2_import_get_continuous_states(0,0,p->nx));
        common::extract_vector(m->model.x, p->client->m_getContinuousStates);
        return(cgsl_model*)m;
    }

    void prepare() {
#ifdef USE_GPL
        /* This is the step control which determines tolerances. */
        cgsl_step_control_parameters step_control;
        step_control.eps_rel = 1e-6;
        step_control.eps_abs = 1e-6;
        step_control.id = step_control_y_new;
        step_control.start = 1e-8;

        for(FMIClient* client: m_clients)
          {
            cgsl_model* cgsl = init_fmu_model(client);
            fmu_parameters* p = getParameters(cgsl);
            cgsl_simulation* sim = (cgsl_simulation *)malloc(sizeof(cgsl_simulation));
            *sim = cgsl_init_simulation(cgsl,  /* model */
                                                       rk8pd, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                                       1,     /* write to file: YES! */
                                                       p->m_backup.result_file,
                                                       step_control);
            m_sims.push_back(sim);
          }

#endif
    }

    /** restoreStates
     *  restores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sims Vector of all simulations
     */
    void restoreStates(std::vector<cgsl_simulation*> *sims){
        for(auto sim : *sims) {
            fmu_parameters* p = getParameters(sim->model);
            for(int i = 0; i < p->m_backup.x.size(); i++)
                sim->model->x[i] = p->m_backup.x[i];

            sim->i.evolution->failed_steps = p->m_backup.failed_steps;
            sim->t = p->m_backup.t;

            // reset position in the result file
            fseek(p->m_backup.result_file, p->m_backup.size_of_file, SEEK_SET);

            // truncate the result file to have the previous result file size
            int trunc = ftruncate(fileno(p->m_backup.result_file), p->m_backup.size_of_file);
            if(0 > trunc )
                perror("storeStates: ftruncate");
        }
    }

    /** storeStates
     *  stores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sims Vector of all simulations
     */
    void storeStates(std::vector<cgsl_simulation*> *sims){
        for(auto sim : *sims){
            fmu_parameters* p = getParameters(sim->model);

            p->baseMaster->sendWait(p->client, fmi2_import_get_continuous_states(0,0,p->nx));
            common::extract_vector(p->m_backup.x, p->client->m_getContinuousStates);
            // if statement not needed if timestep is choosen more carefully
            if(timeLoop.t_start == sim->t && p->nz > 0){
                p->baseMaster->sendWait(p->client, fmi2_import_get_event_indicators(0,0, p->nz));
                common::extract_vector(p->z, p->client->m_getEventIndicators);
            }
            p->m_backup.failed_steps = sim->i.evolution->failed_steps;
            p->m_backup.t = sim->t;
            p->m_backup.size_of_file = ftell(p->m_backup.result_file);
      }
    }


    /** resetIntegratorTimeVariables:
     *  restore values of t_
     *
     *  @param t_safe A known safe time
     *  @param t_new A new time to try to reach
     */
    void resetIntegratorTimeVariables(double t_new)
    {
        // all simulations should be at the same time
        timeLoop.t_safe = m_sims[0]->t;
        timeLoop.t_crossed = timeLoop.t_end;
        timeLoop.t_new = t_new;//min(sim->h, max(max((sim->t - prevTimeEvent)/500,0),t_end));
    }


    /** getStateEvent: Tries to retrieve event indicators, if successful the signbit of all
     *  event indicators z are compaired with event indicators in p->z
     *
     *  @param sims Vector of all simulations
     */
    bool getStateEvent(std::vector<cgsl_simulation*> sims){
        bool ret = false;
        for(auto sim: sims) {
            fmu_parameters* p = getParameters(sim->model);
            p->stateEvent = 0;
            if(p->nz > 0){
                double z[p->nz];
                p->baseMaster->sendWait(p->client, fmi2_import_get_event_indicators(0,0,p->nz));

                /* compare signbit of previous state and the current */
                common::extract_vector(z, p->client->m_getEventIndicators);
                for(int i = 0; i < p->nz; i++){
                  printf("zzz = %f, %f \n", z[i], p->z[i]);
                    if(signbit(z[i]) != signbit(p->z[i])) {
                        p->stateEvent = 1;
                        ret = true;
                    }
                }
            }
        }
        return ret;
    }

    /** getSafeTime:
     *  caluclates a "safe" time, uses the golden ratio to get
     *  t_crossed and t_safe to converge towards same value
     *
     *  @param sim A cgsl simulation
     */
    double getSafeTime(cgsl_simulation *sim){

        // golden ratio
        double phi = (1 + sqrt(5)) / 2;

        fmu_parameters* p = getParameters(sim->model);
        /* passed solution, need to reduce tEnd */
        if(p->stateEvent) {
            timeLoop.t_crossed = timeLoop.t_new;
            timeLoop.t_new = timeLoop.t_safe + (timeLoop.t_crossed - timeLoop.t_safe) / phi;

        } else { // havent passed solution, increase step
            timeLoop.t_safe = timeLoop.t_new;
            timeLoop.t_new = timeLoop.t_crossed - (timeLoop.t_crossed - timeLoop.t_safe) / phi;
        }

        return timeLoop.t_new;
    }

    /** hasStateEvent:
     ** returns true if at least one simulation has an event
     *
     *  @param sims Vector of all simulations
     */
    bool hasStateEvent(std::vector<cgsl_simulation*> sims){
      fmu_parameters* p;
      for(auto sim:sims){
        p = getParameters(sim->model);
        if(p->stateEvent)
          return true;
      }
      return false;

    }

    /** step
     *  run cgsl_step_to on all simulations
     *
     *  @param sims Vector of all simulations
     */
    void step(std::vector<cgsl_simulation*> *sims){
        for(auto sim: *sims)
            cgsl_step_to(sim, sim->t, getSafeTime(sim));
    }

    /** reduceSims
     *  Forget about all simulations that will not reach the event
     *  during current time step.
     *
     *  @param sims Vector of all simulations
     */
    void reduceSims(std::vector<cgsl_simulation*> *sims){
      fmu_parameters* p;
        for(int it = 0; it < sims->size();it++){
          p = getParameters((*sims)[it]->model);
          if(!p->stateEvent)
            sims->erase(sims->begin() + it--);
        }
    }

    /** findEventTime
     *  if there is an event, find the event and return
     *  the time at where the time event occured
     *
     *  @param sims Vector of all simulations
     *  @return Returns the time immediatly after the event
     */
    double findEventTime(std::vector<cgsl_simulation*> sims){
        restoreStates(&sims);
        double tol = 1e-6;
        fmu_parameters* p;

        //remove all simulations that doesn't have an event
        reduceSims(&sims);

        while(timeLoop.t_crossed - timeLoop.t_safe > tol) {
            step(&sims);

            getStateEvent(sims);
            if(hasStateEvent(sims)){
                restoreStates(&sims);
                reduceSims(&sims);
            }else
                storeStates(&sims);
        }
        return timeLoop.t_crossed;
    }

    /** reachedEnd
     *  @param t The current time
     *  @param pt Previuous time
     *  @param pc Count of how many times the different t - pt < toleranc
     */
    int reachedEnd(double t, double* pt, int* pc){
      double tol = 1e-6;
      if( t - *pt < tol ) *pc++;
      else *pc = 0;
      *pt = t;

      return *pc == 20 ? 1:0;
    }

    /** newDiscreteStatesStart
     *  Should be used where a new discrete state ends
     *  and another begins. Resets the loop variables
     *  and store all states of the simulation
     *
     *  @param t The current time
     *  @param t_new New next time
     */
    void newDiscreteStatesStart(double t_new){
        // set start values for the time integration
        resetIntegratorTimeVariables(t_new);
        // start at a new state
        sendWait(m_clients, fmi2_import_enter_event_mode(0,0));
        // todo loop until newDiscreteStatesNeeded == false
        sendWait(m_clients, fmi2_import_new_discrete_states(0,0));
        sendWait(m_clients, fmi2_import_enter_continuous_time_mode(0,0));

        // store the current state of all running FMUs
        storeStates(&m_sims);
    }

    void printStates(void){
      fmu_parameters* p;

      for(auto sim: m_sims){
        p = getParameters(sim->model);
      sendWait(p->client,fmi2_import_get_continuous_states(0,0,p->nx));
      }
      for(auto sim: m_sims){
        p = getParameters(sim->model);
        std::vector<double> rep;

        common::extract_vector(rep, p->client->m_getContinuousStates);
        printf("\nstates\n");
        for(auto val:rep)
          printf("%f ",val);
        printf("\n");
      }
    }
    void runIteration(double t, double dt) {
        timeLoop.t_start = t;
        timeLoop.t_end = t + dt;
        double prevTimeEvent = t;
        int prevTimeCount = 0;

        newDiscreteStatesStart(timeLoop.t_end);
        printStates();

        while( timeLoop.t_safe < timeLoop.t_end ){
            step(&m_sims);
            if( getStateEvent(m_sims) ){
                timeLoop.t_new = findEventTime(m_sims);
                step(&m_sims);
            }

            newDiscreteStatesStart(timeLoop.t_end);

            printStates();
            if(reachedEnd(timeLoop.t_new, &prevTimeEvent, &prevTimeCount)) return;
        }

    }
};
}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */

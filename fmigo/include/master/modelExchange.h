#ifndef MODELEXCHANGE_H
#define MODELEXCHANGE_H
#include "master/BaseMaster.h"
#include "master/WeakConnection.h"
#include "common/common.h"
#include <fmitcp/serialize.h>
#include "gsl-interface.h"
using namespace fmitcp_master;
using namespace fmitcp;
namespace model_exchange{
typedef struct TimeLoop
{
    fmi2_real_t t_safe, dt_new, t_crossed, t_end;
} TimeLoop;
struct Backup
{
    double t;
    double h;
    double *dydt;
    unsigned long failed_steps;
};
struct fmu_parameters{

    double t_ok;
    double t_past;
    int count;                    /* number of function evaluations */

    BaseMaster* baseMaster;       /* BaseMaster object pointer */
    std::vector<FMIClient*> clients;            /* FMIClient vector */
    std::vector<int> intv;            /* FMIClient vector */
    //std::vector<WeakConnection> weakConnections;

    bool stateEvent;
    Backup backup;
    bool sim_started;
};
struct fmu_model{
    cgsl_model *model;
};
class ModelExchangeStepper {
cgsl_simulation m_sim;
fmu_parameters m_p;
BaseMaster* m_baseMaster;
fmu_model m_model;
TimeLoop timeLoop;
std::vector<WeakConnection> m_weakConnections;
std::vector<FMIClient*> m_clients;

 public:
    ModelExchangeStepper(std::vector<FMIClient*> clients, std::vector<WeakConnection> weakConnections,BaseMaster* baseMaster);
    ModelExchangeStepper(){}
    ~ModelExchangeStepper();

    void runIteration(double t, double dt);
    void prepare();

 private:

    /** allocate Memory
     *  Allocates memory needed by the fmu_model
     *
     *  @param m Pointer to a fmu_model
     */
    void allocateMemory(fmu_model &m, const std::vector<FMIClient*> &clients);

    static void *get_model_parameters(const cgsl_model *m){
        return m->parameters;
    }

    /** init_fmu_model
     *  Creates the fmu_model and sets the parameters
     *
     *  @param client A pointer to a fmi client
     */
    void init_fmu_model(fmu_model &m,  const std::vector<FMIClient*> &clients);

#define STATIC_GET_CLIENT_OFFSET(name)                                  \
   p->baseMaster->get_storage().get_offset(client->getId(), STORAGE::name)
#define STATIC_SET_(name, name2, data)                                       \
    p->baseMaster->send(client, fmi2_import_set_##name##_##name2(     \
                                                       data + STATIC_GET_CLIENT_OFFSET(name2), \
                                                       client->getNumContinuousStates()));
#define STATIC_GET_(name)                                               \
    p->baseMaster->send(client, fmi2_import_get_##name((int)client->getNumContinuousStates()))

    /** restoreStates
     *  restores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sim The simulation
     */
    void restoreStates(cgsl_simulation &sim);

    /** storeStates
     *  stores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sim The simulation
     */
    void storeStates(cgsl_simulation &sim);

    /** hasStateEvent:
     ** returns true if at least one simulation has an event
     *
     *  @param sim The simulation
     */
    bool hasStateEvent(cgsl_simulation &sim);

    /** getSafeTime:
     *  caluclates a "safe" time, uses the golden ratio to get
     *  t_crossed and t_safe to converge towards same value
     *
     *  @param sim A cgsl simulation
     */
    void getGoldenNewTime(cgsl_simulation &sim);

    /** step
     *  run cgsl_step_to on all simulations
     *
     *  @param sim The simulation
     */
    void step(cgsl_simulation &sim);

    /** stepToEvent
     *  if there is an event, find the event and return
     *  the time at where the time event occured
     *
     *  @param sim The simulation
     *  @return Returns the time immediatly after the event
     */
    void stepToEvent(cgsl_simulation &sim);

    /** newDiscreteStates
     *  Should be used where a new discrete state ends
     *  and another begins. Resets the loop variables
     *  and store all states of the simulation
     *
     *  @param t The current time
     *  @param t_new New next time
     */
    void newDiscreteStates();

    void printStates(void);

    void getSafeAndCrossed();

    void safeTimeStep(cgsl_simulation &sim);

    void getSafeTime(const std::vector<FMIClient*> clients, double &dt);
};
}
#endif

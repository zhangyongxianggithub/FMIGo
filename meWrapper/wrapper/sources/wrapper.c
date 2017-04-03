//TODO: add some kind of flag that switches this one between a clutch and a gearbox, to reduce the amount of code needed
#define SIMULATION_INIT wrapper_init
#include <unistd.h>
#include <fmilib.h>
//#include "modelDescription_me.h"
#include "modelDescription.h"
#include "fmuTemplate.h"
/* typedef struct wrapper{ */
/*     fmi_import_context_t* m_context; */
/*     fmi_version_enu_t m_version; */
/*     fmi2_import_t* FMU; */
/*     //fmi2_callback_functions_t m_fmi2CallbackFunctions; */
/*     fmi2CallbackFunctions m_fmi2CallbackFunctions; */
/*     fmi2_import_variable_list_t *m_fmi2Variables, *m_fmi2Outputs; */
/*     jm_callbacks m_jmCallbacks; */
/* }Wrapper; */

void wrapper_init(state_t *s);
#define NEW_DOSTEP //to get noSetFMUStatePriorToCurrentPoint
static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
fprintf(stderr,"do step run iteration\n");
runIteration(currentCommunicationPoint,communicationStepSize);
}


//gcc -g clutch.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){
    return 0;
}
#else

#include "fmuTemplate_impl.h"

#endif

//extern "C"{
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
    fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}
//typedef void (fmi2ComponentEnvironment,    fmi2String,        fmi2Status,          fmi2String,          fmi2String, ...);
void testLogger(fmi2ComponentEnvironment c, fmi2String module, fmi2Status log_level, fmi2String message, fmi2String s, ...) {
    fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}


void wrapper_init(state_t *s)  {
    fprintf(stderr,"Init Wrapper\n");
    char *m_fmuLocation;
    char *m_resourcePath;
    char* m_fmuPath;
    char* dir;
    const char* m_instanceName;
    fmi2_import_t** FMU = getfmi2Instance();
    jm_callbacks m_jmCallbacks;
    fmi_version_enu_t m_version;
    fmi_import_context_t* m_context;

    m_jmCallbacks.malloc = malloc;
    m_jmCallbacks.calloc = calloc;
    m_jmCallbacks.realloc = realloc;
    m_jmCallbacks.free = free;
    m_jmCallbacks.logger = jmCallbacksLogger;
    m_jmCallbacks.log_level = 0;
    m_jmCallbacks.context = 0;
    m_fmuPath = (char*)calloc(1024,sizeof(char));

    m_jmCallbacks.logger(NULL,"modulename",0,"jm_string");
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        exit(1);

    //strcpy(m_fmuPath, dir);
    strcpy(m_fmuPath,cwd);
    strcat(m_fmuPath, "/");
    strcat(m_fmuPath, s->md.fmu);

    if (!(dir = fmi_import_mk_temp_dir(&m_jmCallbacks, NULL, "wrapper_"))) {
        fprintf(stderr, "fmi_import_mk_temp_dir() failed\n");
        exit(1);
    }

    m_context = fmi_import_allocate_context(&m_jmCallbacks);
    // unzip the real fmu
    m_version = fmi_import_get_fmi_version(m_context, m_fmuPath, dir);
    fprintf(stderr,"%s\n",m_fmuPath);
    fprintf(stderr,"wrapper: got version %d\n",m_version);

    if ((m_version <= fmi_version_unknown_enu) || (m_version >= fmi_version_unsupported_enu)) {

        fmi_import_free_context(m_context);
        fmi_import_rmdir(&m_jmCallbacks, dir);
        exit(1);
    }
    if (m_version == fmi_version_2_0_enu) { // FMI 2.0
        // parse the xml file
        *FMU = fmi2_import_parse_xml(m_context, dir, 0);
        if(!FMU) {
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&m_jmCallbacks, dir);
            return;
        }

        // check FMU kind
        fmi2_fmu_kind_enu_t fmuType = fmi2_import_get_fmu_kind(*FMU);
        if(fmuType != fmi2_fmu_kind_me) {
            fprintf(stderr,"Wrapper only supports model exchange\n");
            fmi2_import_free(*FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&m_jmCallbacks, dir);
            return;
        }
        // FMI callback functions
        const fmi2_callback_functions_t m_fmi2CallbackFunctions = {fmi2_log_forwarding, calloc, free, 0, 0};

        // Load the binary (dll/so)
        jm_status_enu_t status = fmi2_import_create_dllfmu(*FMU, fmuType, &m_fmi2CallbackFunctions);
        if (status == jm_status_error) {
            fmi2_import_free(*FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&m_jmCallbacks, dir);
            return;
        }
        m_instanceName = fmi2_import_get_model_name(*FMU);
        {
            //m_fmuLocation = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, m_fmuPath);
        }
        fprintf(stderr,"have instancename %s\n",m_instanceName);

        {
            char *temp = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, dir);
            m_resourcePath = (char*)calloc(strlen(temp)+11,sizeof(char));
            strcpy(m_resourcePath,temp);
            strcat(m_resourcePath,"/resources");
            m_jmCallbacks.free(temp);
        }
#ifndef WIN32
        //prepare HDF5
        //getHDF5Info();
#endif
    } else {
        // todo add FMI 1.0 later on.
        fmi_import_free_context(m_context);
        fmi_import_rmdir(&m_jmCallbacks, dir);
        return;
    }
    free(dir);
    free(m_resourcePath);
    free(m_fmuPath);

    fmi2Status status = fmi2_import_instantiate(*FMU , m_instanceName, fmi2_fmu_kind_me, m_resourcePath, 0);
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: instatiate faild\n");
        exit(1);
    }

    //setup_experiment
    status = fmi2_import_setup_experiment(*FMU, true, 1e-6, 0, false, 0) ;
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: setup Experiment faild\n");
        exit(1);
    }

    status = fmi2_import_enter_initialization_mode(*FMU) ;
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: enter initialization mode faild\n");
        exit(1);
    }
    prepare();
    status = fmi2_import_exit_initialization_mode(*FMU);
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: exit initialization mode faild\n");
        exit(1);
    }
}

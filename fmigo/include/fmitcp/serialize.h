#ifndef FMITCP_SERIALIZE_H
#define FMITCP_SERIALIZE_H

#include <string>
#include "fmitcp.pb.h"

namespace fmitcp {
    namespace serialize {
        template<typename T> std::string pack(fmitcp_proto::fmitcp_message_Type type, T &req) {
          uint16_t t = type;
          uint8_t bytes[2] = {(uint8_t)t, (uint8_t)(t>>8)};
          return std::string(reinterpret_cast<char*>(bytes), 2) + req.SerializeAsString();
        }

        // FMI functions follow. These should correspond to FMILibrary functions.

        // =========== FMI 2.0 (CS) Co-Simulation functions ===========
        std::string fmi2_import_set_real_input_derivatives(std::vector<int> valueRefs, std::vector<int> orders, std::vector<double> values);
        std::string fmi2_import_get_real_output_derivatives(std::vector<int> valueRefs, std::vector<int> orders);
        std::string fmi2_import_cancel_step();
        std::string fmi2_import_do_step(double currentCommunicationPoint,
                                        double communicationStepSize,
                                        bool newStep);
        std::string fmi2_import_get_status        (fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_real_status   (fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_integer_status(fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_boolean_status(fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_string_status (fmitcp_proto::fmi2_status_kind_t s);

        // =========== FMI 2.0 (ME) Model Exchange functions ===========
        std::string fmi2_import_enter_event_mode();
        std::string fmi2_import_new_discrete_states();
        std::string fmi2_import_enter_continuous_time_mode();
        std::string fmi2_import_completed_integrator_step();
        std::string fmi2_import_set_time(double time);
        std::string fmi2_import_set_continuous_states(const double* x, int nx);
        std::string fmi2_import_get_event_indicators(int nz);
        std::string fmi2_import_get_continuous_states(int nx);
        std::string fmi2_import_get_derivatives(int nDerivatives);
        std::string fmi2_import_get_nominal_continuous_states(int nx);

        // ========= FMI 2.0 CS & ME COMMON FUNCTIONS ============
        std::string fmi2_import_get_version();
        std::string fmi2_import_set_debug_logging(bool loggingOn, std::vector<std::string> categories);
        std::string fmi2_import_instantiate();               //calls fmi2_import_instantiate2() with visible=false (backward compatibility)
        std::string fmi2_import_instantiate2(bool visible);
        std::string fmi2_import_setup_experiment(bool toleranceDefined, double tolerance, double startTime,
            bool stopTimeDefined, double stopTime);
        std::string fmi2_import_enter_initialization_mode();
        std::string fmi2_import_exit_initialization_mode();
        std::string fmi2_import_terminate();
        std::string fmi2_import_reset();
        std::string fmi2_import_free_instance();
        std::string fmi2_import_set_real   (const std::vector<int>& valueRefs, const std::vector<double>& values);
        std::string fmi2_import_set_integer(const std::vector<int>& valueRefs, const std::vector<int>& values);
        std::string fmi2_import_set_boolean(const std::vector<int>& valueRefs, const std::vector<bool>& values);
        std::string fmi2_import_set_string (const std::vector<int>& valueRefs, const std::vector<std::string>& values);

        //C is vector or set
        template<typename R, typename C> std::string collection_to_req(fmitcp_proto::fmitcp_message_Type type, const C& refs) {
          R req;
          for (int vr : refs) {
            req.add_valuereferences(vr);
          }
          return pack(type, req);
        }

        template<typename C> std::string fmi2_import_get_real(const C& valueRefs) {
          return collection_to_req<fmitcp_proto::fmi2_import_get_real_req>(fmitcp_proto::type_fmi2_import_get_real_req, valueRefs);
        }

        template<typename C> std::string fmi2_import_get_integer(const C& valueRefs) {
          return collection_to_req<fmitcp_proto::fmi2_import_get_integer_req>(fmitcp_proto::type_fmi2_import_get_integer_req, valueRefs);
        }

        template<typename C> std::string fmi2_import_get_boolean(const C& valueRefs) {
          return collection_to_req<fmitcp_proto::fmi2_import_get_boolean_req>(fmitcp_proto::type_fmi2_import_get_boolean_req, valueRefs);
        }

        template<typename C> std::string fmi2_import_get_string(const C& valueRefs) {
          return collection_to_req<fmitcp_proto::fmi2_import_get_string_req> (fmitcp_proto::type_fmi2_import_get_string_req,  valueRefs);
        }

        std::string fmi2_import_get_fmu_state();
        std::string fmi2_import_set_fmu_state(int stateId);
        std::string fmi2_import_free_fmu_state(int stateId);
        std::string fmi2_import_set_free_last_fmu_state();
        std::string fmi2_import_serialized_fmu_state_size();
        std::string fmi2_import_serialize_fmu_state();
        std::string fmi2_import_de_serialize_fmu_state();
        std::string fmi2_import_get_directional_derivative(const std::vector<int>& v_ref, const std::vector<int>& z_ref, const std::vector<double>& dv);

        // ========= NETWORK SPECIFIC FUNCTIONS ============
        std::string get_xml();
    }
}

#endif //FMITCP_SERIALIZE_H

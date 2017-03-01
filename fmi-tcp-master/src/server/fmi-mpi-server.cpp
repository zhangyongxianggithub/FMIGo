#include "common/mpi_tools.h"
#include "server/FMIServer.h"

using namespace std;
using namespace fmitcp;

#include "parse_server_args.cpp"

int main(int argc, char *argv[]) {
    MPI_Init(NULL, NULL);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    //parse arguments
    bool debugLogging = false;
    jm_log_level_enu_t log_level = jm_log_level_error;
    string fmuPath = "";
    string hdf5Filename;

    parse_server_args(argc, argv, &fmuPath, &hdf5Filename, &debugLogging, &log_level);

    FMIServer server(fmuPath, debugLogging, log_level, hdf5Filename);

    for (;;) {
        int rank, tag;
        std::string recv_str = mpi_recv_string(MPI_ANY_SOURCE, &rank, &tag);

        //shutdown command?
        if (tag == 1) {
            break;
        }

        //let Server handle packet, send reply back to master
        std::string str = server.clientData(recv_str.c_str(), recv_str.length());
        if (str.length() > 0) {
          MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, rank, tag, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();

    return 0;
}

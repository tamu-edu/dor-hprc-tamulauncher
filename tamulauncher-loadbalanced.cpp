#include "mpi.h"
  
#include <iostream>
#include <string>
#include <fstream>

#include "run_command_type.hpp"
#include "commands_type.hpp"
#include "master_type.hpp"
#include "worker_type.hpp"
#include "logger_type.hpp"
#include "empty_logger_type.hpp"

using namespace std;

int main(int argc, char* argv[]) {
  // initialize MPI
  MPI_Init(&argc,&argv);
  
  int task_id ;
  MPI_Comm_rank(MPI_COMM_WORLD,&task_id);

  if (task_id ==0) {
    // read the arguments 
    // possible flags: 
    //    --chunk-size
    //    --rerun-killed-commands
    //    --disable-load-balancing
    // NOTE: last argument has to be commands file

    bool rerun_signaled =false;
    bool rerun_exited =false;
    bool disable_load_balancing=false;
    bool force_rerun=false;
    int chunksize = 1;
    // first get the name of the commands  
    string filename(argv[argc-1]);

    // now iterate over all the arguments
    int arg_count=1;
    while (arg_count <argc-1) {
      string next_command(argv[arg_count]);
      if (next_command == "--chunk-size") {
	++arg_count;
	chunksize= atoi(argv[arg_count]);
      } else if (next_command == "--rerun-killed-commands") {
	rerun_signaled =true;
      } else if (next_command == "--rerun-exited-commands") {
        rerun_exited =true;
      } else if (next_command == "--disable-load-balancing") {
        disable_load_balancing =true;
      } else if (next_command == "--force-rerun") {
        force_rerun =true;
      } else {
	std::cout << "unrecognized flag: " << next_command << std::endl;
      }
      ++arg_count;
    }

    // create the .tamulauncher.(processed | signaled | ...) files
    base_logger_type::setup_logs();

    commands_type commands(filename,rerun_signaled);

    
    // this is the master, so create master object and start it
    logger_type lg(0);
    master_type master(commands,lg,chunksize);
    master.start();
    
  } else {

    // this is worker, so create worker object and start it
    empty_logger_type lg;
    worker_type worker(lg);
    worker.start();
  
  }
  // any task (master or worker) will wait here 
  MPI_Finalize ( );
  
}




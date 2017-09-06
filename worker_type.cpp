#include "mpi.h"
  
#include "worker_type.hpp"

using namespace std;

bool worker_type::receive_and_pack() {
  bool received = false;
  int num_commands=0;

  MPI_Status status; 
  // receive the number of commands this task will handle
  MPI_Recv(&num_commands,1 ,MPI_INT,0,0,MPI_COMM_WORLD,&status);
  
  if (num_commands > 0) {
    received = true;

    // receive array containing lenghts per command
    int command_indices[num_commands];
    MPI_Recv(&command_indices,num_commands,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    // receive array containing lenghts per command
    int command_lenghts[num_commands];
    MPI_Recv(&command_lenghts,num_commands,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    
    // receive the packed string, to get the total size of string compute
    // sum of command lenghts
    int total =0; for (int i=0;i<num_commands;++i) total += command_lenghts[i];
    
    char buff[total];
    MPI_Recv(&buff,total,MPI_CHAR,0,0,MPI_COMM_WORLD,&status);
    
    // next step is to unpack the command string
    string command_string = buff;
    
    int index = 0;
    for (int i = 0; i<num_commands;++i) {
      run_command_type run_command(command_string.substr(index,command_lenghts[i]),command_indices[i]);
      commands.push_back(run_command);
      index += command_lenghts[i];
    }
  }

  return received;
}


void worker_type::unpack_and_send() {


  int command_indices[commands.size()];
  int return_codes[commands.size()];
  double runtimes[commands.size()];

  int counter=0;
  for (vector<run_command_type>::iterator it=commands.begin();it!=commands.end();++it,++counter) {
    run_command_type& run_command = *it;
    command_indices[counter]=run_command.get_index();
    return_codes[counter]=run_command.get_return_code();
    runtimes[counter]=run_command.get_runtime();;
  }

  int return_codes_size = commands.size();
  MPI_Send(&return_codes_size,1,MPI_INT,0,0,MPI_COMM_WORLD);


  // if the master does the logging, need to send the additional information to the master
  // otherwise will just do the logging on the worker
  if (get_log_type() == MASTER_LOGS) {
    MPI_Send(&command_indices,return_codes_size,MPI_INT,0,0,MPI_COMM_WORLD);
    MPI_Send(&return_codes,return_codes_size,MPI_INT,0,0,MPI_COMM_WORLD);
    MPI_Send(&runtimes,return_codes_size,MPI_DOUBLE,0,0,MPI_COMM_WORLD);
  } else{
    // nothing else to send, just write to the log
    logger.write_log(commands);
  }
}

worker_type::worker_type(base_logger_type& lg, int logtype) :
  base_master_worker_type(logtype),
  logger(lg) {

  MPI_Comm_rank(MPI_COMM_WORLD,&task_id);
}

void worker_type::start() {
  
  int return_codes_size =0;
  bool received = true;
  
  if (get_log_type() == WORKER_LOGS) {
    logger.open(task_id);
  }
  while (received) {

    // receive the commands

    commands.clear();
    received = receive_and_pack();
    
    // if this worker received any elements process them and send results back

    if (received) {
      // execute the commands
      for (vector<run_command_type>::iterator it=commands.begin();it!=commands.end();++it) {
	run_command_type& run_command = *it;
	run_command.execute(task_id);
      }
      
      // send results back
      unpack_and_send();
    }
  }

  if (get_log_type() == WORKER_LOGS) {
    logger.close();
  }
}
   





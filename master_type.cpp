#include "mpi.h"
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <iostream>  

#include "master_type.hpp"


#include <stdio.h>

using namespace std;

int master_type::receive_and_log() {
  MPI_Status status;
  int return_codes_size=0;
  int myr = MPI_Recv(&return_codes_size,1 ,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
  int sender = status.MPI_SOURCE;

  int command_indices[return_codes_size];
  int return_codes[return_codes_size];
  double runtimes[return_codes_size];


  // if the master does the logging the worker will send the addtional information
  // need to write the logs. Other wise, nothing else needs to be done
  if (get_log_type() == MASTER_LOGS) {
    MPI_Recv(&command_indices,return_codes_size ,MPI_INT,sender,0,MPI_COMM_WORLD,&status);
    MPI_Recv(&return_codes,return_codes_size ,MPI_INT,sender,0,MPI_COMM_WORLD,&status);
    MPI_Recv(&runtimes,return_codes_size ,MPI_DOUBLE,sender,0,MPI_COMM_WORLD,&status);

    // pack it into run_command_type elements, need it for logging
    vector<run_command_type> received_commands;
    for (int i=0;i<return_codes_size;++i) {
      run_command_type& rc = commands.get_command(command_indices[i]);
      printf("master: sent command_index=%d, actual_command_index=%d\n",command_indices[i],rc.get_index());
      rc.set_runtime(runtimes[i]);
      rc.set_return_code(return_codes[i]);
      received_commands.push_back(rc);
    }
    
    // write the info from the received commands to the logs
    logger.write_log(received_commands);
  }

  return sender;
}

bool master_type::pack_and_send(int destination) {

  int counter = 0;

  if (!commands.has_next()) {
    MPI_Send(&counter,1,MPI_INT,destination,0,MPI_COMM_WORLD);
    return false;
  }


  // if this point is reached there should be at least one command left
  int commands_index_list[chunksize];
  string buffer = "";
  int line_lenghts[chunksize];
  // compute the real chunksize

  while (counter < chunksize && commands.has_next()) {
    
    run_command_type& cmd=commands.get_command();
    commands_index_list[counter]=cmd.get_index();
    line_lenghts[counter] = cmd.get_command_string().length();
    buffer.append(cmd.get_command_string());
    
    commands.proceed_next();
    ++counter;
  }
  

  // 2) send the number of elements this task will handle
  MPI_Send(&counter,1,MPI_INT,destination,0,MPI_COMM_WORLD);

  // send the index for every command
  MPI_Send(&commands_index_list,counter,MPI_INT,destination,0,MPI_COMM_WORLD);
  // 3) send array containing lengths of every command
  MPI_Send(&line_lenghts,counter,MPI_INT,destination,0,MPI_COMM_WORLD);
  // 4) send packed string
  MPI_Send((char *)buffer.c_str(), buffer.size(), MPI_CHAR, destination, 0, MPI_COMM_WORLD);

  return true;
}



master_type::master_type(commands_type& cmds, base_logger_type& l, int csize,int logtype) : 
  base_master_worker_type(logtype),
  commands(cmds), logger(l) {
  
  chunksize = csize;
  MPI_Comm_size(MPI_COMM_WORLD,&num_tasks);
}

void master_type::start() {

  
  if (get_log_type() == MASTER_LOGS) {
    logger.open(0);
  }
  // initial round: send commands to tasks, tasks that receive 0 elements know
  // they don't need to process anything and will not send info back
  int tasks_busy = num_tasks-1;
  for (int task_counter=1;task_counter<num_tasks;++task_counter) {
    bool commands_sent=pack_and_send(task_counter);
    if (commands_sent == false) {
      --tasks_busy;
    }
  }
    

  // keep receiving results from all tasks and send new commands until 
  // all tasks are aware there is nothing left.
  while (tasks_busy > 0) {
    int sender=receive_and_log();
    bool commands_sent= pack_and_send(sender);
    if (commands_sent == false) {
      --tasks_busy;
    }

  }
  if (get_log_type() == MASTER_LOGS) {
    logger.close();
  }

}




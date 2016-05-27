#include "mpi.h"
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <iostream>  

#include "master_type.hpp"


#include <stdio.h>

using namespace std;



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



master_type::master_type(commands_type& cmds, int csize) : commands(cmds) {
  chunksize = csize;
  MPI_Comm_size(MPI_COMM_WORLD,&num_tasks);
}

void master_type::start() {

  // open files to append processed and signaled commands
  ofstream processed_file(commands.get_processed_file_name(),std::ios_base::app);
  ofstream signaled_file(commands.get_signaled_file_name(),std::ios_base::app);
  
  
  // quick check if we need to do anything at all
  if (!commands.has_next()) {
    cout << "\n\n===========================================================\n";
    cout << "All commands have been processed.\n" << 
      "If you think this is a mistake and/or want to redo your run\n"<< 
      "remove file .tamulauncher.processed and run again\n";
    cout << "===========================================================\n\n\n";
  }

  ofstream log_file("tamulauncher.log",std::ios_base::app);
  
  
  int tasks_busy = num_tasks-1;
  for (int task_counter=1;task_counter<num_tasks;++task_counter) {
    bool commands_sent=pack_and_send(task_counter);
    if (commands_sent == false) {
      --tasks_busy;
    }
  }
    

  MPI_Status status;
  int return_codes_size=0;
  // keep sending commands until all tasks are aware there is nothing left.
  while (tasks_busy > 0) {

    int myr = MPI_Recv(&return_codes_size,1 ,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
    int sender = status.MPI_SOURCE;
    
    int command_indices[return_codes_size];
    int return_codes[return_codes_size];
    double runtimes[return_codes_size];
    
    MPI_Recv(&command_indices,return_codes_size ,MPI_INT,sender,0,MPI_COMM_WORLD,&status);
    MPI_Recv(&return_codes,return_codes_size ,MPI_INT,sender,0,MPI_COMM_WORLD,&status);
    MPI_Recv(&runtimes,return_codes_size ,MPI_DOUBLE,sender,0,MPI_COMM_WORLD,&status);
    // get a copy of the processed commands
    
    bool commands_sent= pack_and_send(sender);
    
    if (commands_sent == false) {
      --tasks_busy;
    }
    
    for (int pcount=0;pcount<return_codes_size;++pcount) {
      int ret_signal = return_codes[pcount];
      if (WIFSIGNALED(ret_signal)) {
	if (WTERMSIG(ret_signal) == SIGUSR2) {
	  // Special case, if this happens job was most likely killed by LSF because wall time ran out.
	  // In that case command should definitely not be added to list of processed commands. 
	} else {
	  signaled_file << command_indices[pcount] <<"\n";
	  signaled_file.flush();
	}
      } else {
	processed_file << command_indices[pcount] <<"\n";
	processed_file.flush();
      }
      
      log_file << command_indices[pcount] << " :: " << commands.get_command(command_indices[pcount]).get_command_string() << 
	" :: time spent: " << runtimes[pcount] << " :: return code: " << return_codes[pcount] << "\n";
      log_file.flush();
    }
  }

}




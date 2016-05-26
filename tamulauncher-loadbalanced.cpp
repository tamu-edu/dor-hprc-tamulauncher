#include "mpi.h"
  
#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <signal.h>
#include <sys/wait.h>

#include "string.h"
#include <dlfcn.h>


#include "run_command_type.hpp"
#include "commands_type.hpp"

using namespace std;



bool pack_and_send(commands_type& commands,
		   int chunksize, 
		   int destination) {


  int counter = 0;
  bool commands_sent = true;

  if (!commands.has_next()) {
    MPI_Send(&counter,1,MPI_INT,destination,0,MPI_COMM_WORLD);
    return false;
  }


  // if this point is reached there should be at least one element left
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

bool receive_and_unpack(vector<run_command_type>& commands) {

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


int main(int argc, char* argv[]) {
  // initialize MPI
  MPI_Init(&argc,&argv);

  
  int num_tasks, task_id ;
  MPI_Comm_size(MPI_COMM_WORLD,&num_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&task_id);

  if (task_id ==0) {

    // possible flags: 
    //    "--chunk-size"
    // NOTE: last argument has to be commands file

    bool rerun_signaled =false;
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
      } else {
	std::cout << "unrecognized flag: " << next_command << std::endl;
      }
      ++arg_count;
    }

    commands_type commands(filename,rerun_signaled);
    

    // open files to append processed and signaled commands
    ofstream processed_file(".tamulauncher.processed",std::ios_base::app);
    ofstream signaled_file(".tamulauncher.signaled",std::ios_base::app);


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
      bool commands_sent=pack_and_send(commands, chunksize, task_counter);
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
      
      bool commands_sent= pack_and_send(commands, chunksize, sender);
      
      if (commands_sent == false) {
	--tasks_busy;
      }
      
      
      for (int pcount=0;pcount<return_codes_size;++pcount) {
	int ret_signal = return_codes[pcount];
	cout << "RET CODE for command " << command_indices[pcount] << " = " << ret_signal << "\n";
	if (WIFSIGNALED(ret_signal)) {
	  if (WTERMSIG(ret_signal) == SIGUSR2) {
	  cout <<"SIGUSR2: " << commands.get_command(command_indices[pcount]).get_command_string() << "\n";
	  // Special case, if this happens job was most likely killed by LSF because wall time ran out.
	  // In that case command should definitely not be added to list of processed commands. 
	  } else {
	    cout << "SIGNALED\n";
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
    
  } else {

    vector<run_command_type> commands;
    int return_codes_size =0;
    bool received = true;
    while (received) {
      commands.clear();
      received = receive_and_unpack(commands);
      int command_indices[commands.size()];
      int return_codes[commands.size()];
      double runtimes[commands.size()];
      if (received) {
	int counter=0;
	for (vector<run_command_type>::iterator it=commands.begin();it!=commands.end();++it,++counter) {
	  run_command_type& run_command = *it;
	  run_command.execute(task_id);
	  int rc = run_command.get_return_code();
	  double rt = run_command.get_runtime();
	  command_indices[counter]=run_command.get_index();
	  return_codes[counter]=rc;
	  runtimes[counter]=rt;
	}
	return_codes_size = commands.size();
	MPI_Send(&return_codes_size,1,MPI_INT,0,0,MPI_COMM_WORLD);

	MPI_Send(&command_indices,return_codes_size,MPI_INT,0,0,MPI_COMM_WORLD);
	MPI_Send(&return_codes,return_codes_size,MPI_INT,0,0,MPI_COMM_WORLD);
	MPI_Send(&runtimes,return_codes_size,MPI_DOUBLE,0,0,MPI_COMM_WORLD);
      }
    }
  }   
  
  
  MPI_Finalize ( );
  
}




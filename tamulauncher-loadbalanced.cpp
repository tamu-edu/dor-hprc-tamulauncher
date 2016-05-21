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


using namespace std;

class run_command_type {

private:

  int command_index;
  string command;
  int return_code;
  double run_time;

public:

  run_command_type(string s,int index) {
    command_index=index;
    command = s;
  }

  double get_runtime() {return run_time;}
 
  int get_return_code() {return return_code;}

  int get_command_index() {return command_index;}

  void execute() {
    // tokenize
    string buf("");

    int task_id; 
    MPI_Comm_rank(MPI_COMM_WORLD,&task_id);
    string task_id_string = std::to_string(task_id);
    string command_index_string=std::to_string(command_index);
    buf.append("export TAMULAUNCHER_TASK_ID=");
    buf.append(task_id_string);
    buf.append("; export TAMULAUNCHER_COMMAND_ID=");
    buf.append(command_index_string);
    buf.append("; ");
    buf.append(command);


    double tstart = MPI::Wtime();
    this->return_code = system(buf.c_str());
    double tend = MPI::Wtime();
    this->run_time = (tend-tstart);
 
  }

};




int pack_and_send(vector<string>& commands, int commands_index,
		  vector<int>& processed, int* processed_index,
		  int chunksize, int destination) {


  int counter = 0;
  int num_commands = commands.size();
  int num_processed = processed.size();

  if (commands_index == -1) {
    MPI_Send(&counter,1,MPI_INT,destination,0,MPI_COMM_WORLD);
    return commands_index;
  }

  int commands_index_list[chunksize];
  string buffer = "";
  int line_lenghts[chunksize];
  // compute the real chunksize

  while (counter < chunksize && 
	 commands_index < num_commands) {

    // skip all the elements that are already processed
    while (*processed_index<num_processed &&
	   commands_index == processed[*processed_index] &&
	   commands_index < num_commands) {
      ++commands_index;
      ++(*processed_index);
    }

    if (commands_index < num_commands) {
      commands_index_list[counter]=commands_index;
      line_lenghts[counter] = commands[commands_index].length();
      buffer.append(commands[commands_index]);
      
      ++commands_index;
      ++counter;
    }
  }
  
  // 2) send the number of elements this task will handle
  MPI_Send(&counter,1,MPI_INT,destination,0,MPI_COMM_WORLD);

  if (counter > 0) {
    // send the index for every command
    MPI_Send(&commands_index_list,counter,MPI_INT,destination,0,MPI_COMM_WORLD);
    // 3) send array containing lengths of every command
    MPI_Send(&line_lenghts,counter,MPI_INT,destination,0,MPI_COMM_WORLD);
    // 4) send packed string
    MPI_Send((char *)buffer.c_str(), buffer.size(), MPI_CHAR, destination, 0, MPI_COMM_WORLD);
  } else {
    commands_index=-1;
  }
  return commands_index;
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
      } else {
	std::cout << "unrecognized flag: " << next_command << std::endl;
      }
      ++arg_count;
    }

    //printf("starting tamulauncher with chunk size: %d", chunksize);

    // read all the commands from file
    vector<string> commands;

    // since we will start counting from 1 instead of 0, set the first element
    // to a dummy empty string
    //string dummy("");
    //commands.push_back(dummy);

    int commands_index=0;

    ifstream in;    
    in.open(filename.c_str());  
    string line;
    getline(in,line);
    while (! in.eof()) {
      if ( ! line.empty()) {
	commands.push_back(line);
      }
      getline(in,line);
    }

    // read the commands that have been processed
    vector<int> processed_commands;
    ifstream processed_file_read(".tamulauncher.processed");

    if (processed_file_read.peek() == std::ifstream::traits_type::eof()) {
      // file is empty
    } else {
      int next_processed;
      while (! processed_file_read.eof()) {
	next_processed=-1;
	processed_file_read >> next_processed;
	if (next_processed != -1) 
	  processed_commands.push_back(next_processed);
      }
    }
    processed_file_read.close();
    ofstream processed_file(".tamulauncher.processed",std::ios_base::app);
    int processed_commands_index=0;

    // if #commands equals #processed_commands set commands_index to -1
    // no need for any computation.
    if (commands.size() == processed_commands.size()) {
      cout << "\n\n===========================================================\n";
      cout << "All commands have been processed.\n" << 
	"If you think this is a mistake and/or want to redo your run\n"<< 
	"remove file .tamulauncher.processed and run again\n";
      cout << "===========================================================\n\n\n";
      commands_index=-1;
    }
    
    // make sure the processed elements are sorted
    std::sort (processed_commands.begin(), processed_commands.end());

    ofstream log_file("tamulauncher.log",std::ios_base::app);


    int tasks_busy = num_tasks-1;
    for (int task_counter=1;task_counter<num_tasks;++task_counter) {
      commands_index = pack_and_send(commands, commands_index,
				     processed_commands, &processed_commands_index,
				     chunksize, task_counter);

      if (commands_index == -1) {
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
 
      commands_index = pack_and_send(commands, commands_index,
				     processed_commands, &processed_commands_index,
				     chunksize, sender);
      
      if (commands_index == -1) {
        --tasks_busy;
      }
      

      for (int pcount=0;pcount<return_codes_size;++pcount) {
	int ret_signal = return_codes[pcount];
	if (WIFSIGNALED(ret_signal) && WTERMSIG(ret_signal) == SIGUSR2) {
	  // process was killed with USR2, most likely because got killed by LSF
	  // will decide later what to do. Definitely should not be added to list
	  // of processed commands.
	} else {
	  processed_file << command_indices[pcount] <<"\n";
	  processed_file.flush();
	}

	log_file << command_indices[pcount] << " :: " << commands[command_indices[pcount]] << " :: time spent: " << runtimes[pcount] << 
	  " :: return code: " << return_codes[pcount] << "\n";
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
	  run_command.execute();
	  int rc = run_command.get_return_code();
	  double rt = run_command.get_runtime();
	  command_indices[counter]=run_command.get_command_index();
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




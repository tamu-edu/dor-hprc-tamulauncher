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

  string command;
  int return_code;
  double run_time;

public:

  run_command_type(string& s) {
    command = s;
    return_code=-1;
    run_time=0.0;
  }

  double get_runtime() {return run_time;}
 
  int get_return_code() {return return_code;}

  void execute(int add_task_id) {
    // tokenize
    string buf(command);
    
    if (add_task_id > 0) {
      int task_id; 
      MPI_Comm_rank(MPI_COMM_WORLD,&task_id);
      string task_id_string = std::to_string(task_id);
      buf.append(" ");
      buf.append(task_id_string);
    }
    

    double tstart = MPI::Wtime();
    this->return_code = system(buf.c_str());
    double tend = MPI::Wtime();
    this->run_time = (tend-tstart);
 
  }
};

class task_info_type {

private:

  vector<int> ind;
  
public:

  void add_indice(int i) {ind.push_back(i);}

  void clear_indices() { ind.clear();}

  const vector<int>& indices() {return ind;}
};




int pack_and_send(vector<string>& commands, int commands_index,
		  vector<int>& processed, int* processed_index,
		  int chunksize, int destination, 
		  task_info_type& task_info) {


  int counter = 0;
  int num_commands = commands.size();
  int num_processed = processed.size();

  if (commands_index == -1) {
    MPI_Send(&counter,1,MPI_INT,destination,0,MPI_COMM_WORLD);
    return commands_index;
  }

  string buffer = "";
  int line_lenghts[chunksize];
  // compute the real chunksize

  //std::cout << "\nsending next command to task " << destination << "\n";
  //std::cout<< "------------------------------------\n";
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
      line_lenghts[counter] = commands[commands_index].length();
      buffer.append(commands[commands_index]);
      task_info.add_indice(commands_index);
      
      ++commands_index;
      ++counter;
    }
  }
  
  // 2) send the number of elements this task will handle
  MPI_Send(&counter,1,MPI_INT,destination,0,MPI_COMM_WORLD);

  if (counter > 0) {
    // 3) send array containing lengths of every command
    MPI_Send(&line_lenghts,counter,MPI_INT,destination,0,MPI_COMM_WORLD);
    // 4) send packed string
    MPI_Send((char *)buffer.c_str(), buffer.size(), MPI_CHAR, destination, 0, MPI_COMM_WORLD);
  } else {
    commands_index=-1;
  }
  return commands_index;
}

bool receive_and_unpack(vector<string>& commands) {

  bool received = false;
  int num_commands=0;
  MPI_Status status;

  // receive the number of commands this task will handle
  MPI_Recv(&num_commands,1 ,MPI_INT,0,0,MPI_COMM_WORLD,&status);

  if (num_commands > 0) {
    received = true;
    // receive array containing lenghts per command
    int command_lenghts[num_commands];
    MPI_Recv(&command_lenghts,num_commands,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    
    // receive the packed string, to get the total size of string compute
    // sum of command lenghts
    int total =0; for (int i=0;i<num_commands;++i) total += command_lenghts[i];
    
    char buf[total];
    MPI_Recv(&buf,total,MPI_CHAR,0,0,MPI_COMM_WORLD,&status);
    
    // next step is to unpack the command string
    string command_string = buf;
    
    int index = 0;
    for (int i = 0; i<num_commands;++i) {
      commands.push_back(command_string.substr(index,command_lenghts[i]));
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
    //    "--add-taskid" 
    //    "--chunk-size"
    // NOTE: last argument has to be commands file
    
    int add_task_id = 0;

    int chunksize = 1;
    // first get the name of the commands  
    string filename(argv[argc-1]);

    // now iterate over all the arguments
    int arg_count=1;
    while (arg_count <argc-1) {
      string next_command(argv[arg_count]);
      if (next_command == "--add-taskid") {
	add_task_id=1;
      } else if (next_command == "--chunk-size") {
	++arg_count;
	chunksize= atoi(argv[arg_count]);
      } else {
	std::cout << "unrecognized flag: " << next_command << std::endl;
      }
      ++arg_count;
    }
    

    // read all the commands from file
    vector<string> commands;
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
      cout << "All commands have been processed.\n" << 
	"If you think this is a mistake and/or want to redo your run\n"<< 
	"remove file .tamulauncher.processed and run again\n";
      commands_index=-1;
    }
    
    // make sure the processed elements are sorted
    std::sort (processed_commands.begin(), processed_commands.end());

    ofstream log_file("tamulauncher.log",std::ios_base::app);

    vector<task_info_type> task_info(num_tasks);
    // send info to all tasks, no matter if no commands left

    MPI_Bcast(&add_task_id,1, MPI_INT, 0, MPI_COMM_WORLD);

    int tasks_busy = num_tasks-1;
    for (int task_counter=1;task_counter<num_tasks;++task_counter) {
      task_info[task_counter].clear_indices();
      commands_index = pack_and_send(commands, commands_index,
				     processed_commands, &processed_commands_index,
				     chunksize, task_counter,task_info[task_counter]);

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
      int return_codes[return_codes_size];
      double runtimes[return_codes_size];
      MPI_Recv(&return_codes,return_codes_size ,MPI_INT,sender,0,MPI_COMM_WORLD,&status);
      MPI_Recv(&runtimes,return_codes_size ,MPI_DOUBLE,sender,0,MPI_COMM_WORLD,&status);
      // get a copy of the processed commands
      vector<int> processed(task_info[sender].indices());

      task_info[sender].clear_indices();
      commands_index = pack_and_send(commands, commands_index,
				     processed_commands, &processed_commands_index,
				     chunksize, sender,task_info[sender]);
      
      if (commands_index == -1) {
        --tasks_busy;
      }
      
      if (processed.size() != return_codes_size) {
	printf("size of processed elements: %d, size of return codes: %d\n",processed.size(),return_codes_size);
      }
      for (int pcount=0;pcount<return_codes_size;++pcount) {
        const int next = processed[pcount];
        processed_file << next <<"\n";
        processed_file.flush();
	log_file << commands[next] << " :: time spent: " << runtimes[pcount] << 
	  " :: return code: " << return_codes[pcount] << "\n";
	log_file.flush();
      }

    }


  } else {

    // first thing to do is receive broadcast indicating if task_id should be 
    // added to every command
    int add_task_id;
    MPI_Bcast(&add_task_id,1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<string> commands;
    int return_codes_size =0;
    bool received = true;
    while (received) {
      commands.clear();
	//printf("task %d calling received_unpack\n",task_id);
      received = receive_and_unpack(commands);
      //printf("task %d received %d elements\n",task_id,commands.size());
      int return_codes[commands.size()];
      double runtimes[commands.size()];
      if (received) {
	int counter=0;
	for (vector<string>::iterator it=commands.begin();it!=commands.end();++it,++counter) {
	  run_command_type run_command(*it);
	  run_command.execute(add_task_id);
	  int rc = run_command.get_return_code();
	  double rt = run_command.get_runtime();
	  return_codes[counter]=rc;
	  runtimes[counter]=rt;
	}
	return_codes_size = commands.size();
	//printf("[%d] task calling send\n",task_id);
	MPI_Send(&return_codes_size,1,MPI_INT,0,0,MPI_COMM_WORLD);
	MPI_Send(&return_codes,return_codes_size,MPI_INT,0,0,MPI_COMM_WORLD);
	MPI_Send(&runtimes,return_codes_size,MPI_DOUBLE,0,0,MPI_COMM_WORLD);
	//printf("[%d] task finished calling send\n",task_id);
      }
    }
  }   
  
  
  MPI_Finalize ( );
  
}




#include "omp.h"

#include <unistd.h>  
#include <iostream>
#include <string>
#include <fstream>

#include <vector>
#include <algorithm>
#include <iterator>

#include <stdio.h>

#include "run_command_type.hpp"
#include "commands_type.hpp"
#include "logger_type.hpp"

int get_tasks_per_node(string& node, string& dirname) {
  // assumes there is a file named node_list in directory tamulauncher
  // first line should be name of node, next line should be number of tasks.
  // Continues for every node.

  bool done = false;
  string next_host;
  int next_num=0;
  int val=-1;
  string nlistname = dirname+"/node_list";
  std::ifstream node_list_file(nlistname);
  while (!done && !node_list_file.eof()) {
    node_list_file >>  next_host;
    node_list_file >> next_num;
    if (node.compare(next_host) == 0) {
      val=next_num;
      done=true;
    }
  }
  return val;

}
  
  
int main(int argc, char** argv) {

  int task_id = -1;
  string task_id_string;
  int num_tasks_per_node = 0;
  string dirname;

  // iterate over all the arguments
  int arg_count=1;
  while (arg_count <argc-1) {
    string next_command(argv[arg_count]);
    if (next_command == "--tasks_per_node") {
      ++arg_count;
      num_tasks_per_node= atoi(argv[arg_count]);
    }  else if (next_command == "--task_id") {
      ++arg_count;
      task_id_string=argv[arg_count];
      task_id= atoi(argv[arg_count]);
    } else if (next_command == "--dirname") {
      ++arg_count;
      dirname= argv[arg_count];
    }else {
      std::cout << "unrecognized flag: " << next_command << std::endl;
    }
    ++arg_count;
  }

  string filename=dirname+"/todo."+task_id_string;
  commands_type commands(filename);

  commands.read();
  if  (commands.num_commands() > 0) {
    
    // create logger, need to get the hostname for that
    char th[100];
    gethostname(th,100);
    std::string hostname(th);
    
    logger_type logger(hostname,dirname);
    
    // open the logger
    logger.open();
    
    
    // in case --tasks-per-node was set we will need to check how many tasks we
    // actually requested in the job file. If there are less tasks requested in the
    // job file than --tasks-per-node, need to adjust. NOTE, this should only be an
    // issue when #tasks cannot be divided by #nodes. With SLURM the --tasks-per-node
    // is not needed, not sure how to deal with it. TODO? 
    int num=get_tasks_per_node(hostname,dirname);
    if (num_tasks_per_node == 0) {
      num_tasks_per_node=num;
    } else if (num_tasks_per_node > num){
      printf("... WARNING: tamulauncer --commands-per-node = %d but only %d cores per node requested in batch job. Adjusting commands per node.\n",
	     num_tasks_per_node,num);
      num_tasks_per_node=num;
    }
    
    int global_command_index=num_tasks_per_node;
    int num_commands = commands.num_commands();
#pragma omp parallel num_threads(num_tasks_per_node)
    {
      
      int local_index= omp_get_thread_num();
      
      while (local_index < num_commands) {
	run_command_type& next_command = commands.command_at(local_index);
	next_command.execute();
	logger.write_log(next_command);
	
	// get next index to process
#pragma omp atomic capture
	local_index=global_command_index++;
      }
      
      // when this point is reached, the thread has no commands to process
      // anymore. For now just release the resource and wait for the 
      // threads to finish 
      
    }
    
  } else {
    std::cout << "\n\n===========================================================\n";
    std::cout << "All commands have been processed.\n" <<
      "If you think this is a mistake and/or want to redo your run\n"<<
      "run tamulauncher with --norestart option and run again\n";
    std::cout << "===========================================================\n\n\n";
  }
  
}




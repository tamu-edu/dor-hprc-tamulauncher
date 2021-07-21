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

  int num_tasks_per_node = 0;
  string dirname;
  string releasescript = "";

  // iterate over all the arguments
  int arg_count=1;
  while (arg_count <argc-1) {
    string next_command(argv[arg_count]);
    if (next_command == "--tasks_per_node") {
      ++arg_count;
      num_tasks_per_node= atoi(argv[arg_count]);
    } else if (next_command == "--dirname") {
      ++arg_count;
      dirname= argv[arg_count];
    } else if (next_command == "--releasescript") { 
      ++arg_count;
      releasescript= argv[arg_count];
    }else {
      std::cout << "unrecognized flag: " << next_command << std::endl;
    }
    ++arg_count;
  }


  char th[100];
  gethostname(th,100);
  std::string hostname(th);
  
  // TODO for now check for -lpar is hardcoded here. Need to move
  // it to different library function, will be cleaner
  size_t pos = hostname.find("-lpar");
  if (pos != std::string::npos) {
     hostname.erase(pos);
  }
  
  string filename=dirname+"/todo."+hostname;
  commands_type commands(filename);

  commands.read();

  //  printf("Task %s read in %d commands.\n",hostname.c_str(),commands.num_commands());
     
  // create logger, need to get the hostname for that
  logger_type logger(hostname,dirname);
  
  // open the logger
  logger.open();
  
  
  // in case --tasks-per-node was set we will need to check how many tasks we
  // actually requested in the job file. If there are less tasks requested in the
  // job file than --tasks-per-node, need to adjust. NOTE, this should only be an
  // issue when #tasks cannot be divided by #nodes. With SLURM the --tasks-per-node
  // is not needed, not sure how to deal with it. TODO? 
  int total_cores_per_node=get_tasks_per_node(hostname,dirname);
  if (num_tasks_per_node == 0) {
    num_tasks_per_node=total_cores_per_node;
  } else if (num_tasks_per_node > total_cores_per_node){
    printf("... WARNING: tamulauncer --commands-per-node = %d but only %d cores per node requested in batch job. Adjusting commands per node.\n",
	   num_tasks_per_node,total_cores_per_node);
    num_tasks_per_node=total_cores_per_node;
  }
  
  int global_command_index=num_tasks_per_node;
  int num_commands = commands.num_commands();
  int num_threads_left=0;

#pragma omp parallel num_threads(num_tasks_per_node)
  {
    
    num_threads_left=num_tasks_per_node;
    int local_index= omp_get_thread_num();
    int task_id=omp_get_thread_num();
    
    while (local_index < num_commands) {
      run_command_type& next_command = commands.command_at(local_index);
      next_command.execute();
      logger.write_log(next_command);
      
      // get next index to process
      //#pragma omp atomic capture
#pragma omp critical(update)
      {
	
	local_index=global_command_index++;
	//std::cout << "thread= "<<task_id<<",  local_index="<<local_index<<",   num_commands="<<num_commands<< std::endl;
      }
    }    
    
    if ( ! releasescript.empty() ) {
#pragma omp critical (release)
      {
	--num_threads_left;
	if (num_threads_left > 0) {
	  string release_string = releasescript + "  " + hostname + " " + dirname + " " + std::to_string(task_id) +
	    " " + std::to_string(num_tasks_per_node) + " " + std::to_string(total_cores_per_node);
	  system(release_string.c_str());
	}
      }
    }
  }
  
  if ( ! releasescript.empty() ) {
    // all commands have been finished, call the release script one more time
    string release_string = releasescript + "  "  + hostname + " " + dirname + " -1 " +
      " " + std::to_string(num_tasks_per_node) + " " + std::to_string(total_cores_per_node);
    system(release_string.c_str());
  }
  
    return 0;
}
  



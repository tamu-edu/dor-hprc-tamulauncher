#include "mpi.h"
#include "omp.h"

#include <unistd.h>  
#include <iostream>
#include <string>
#include <fstream>

#include <vector>
#include <algorithm>
#include <iterator>


#include "run_command_type.hpp"
#include "commands_type.hpp"
//#include "master_type.hpp"
//#include "worker_type.hpp"
#include "logger_type.hpp"

int get_tasks_per_node(string& node) {
  // assumes there is a file named node_list in directory tamulauncher
  // first line should be name of node, next line should be number of tasks.
  // Continues for every node.

  bool done = false;
  string next_host;
  int next_num=0;
  int val=-1;
  std::ifstream node_list_file(".tamulauncher-log/node_list");
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

void create_todo_vector(std::vector<unsigned int>& todo_vector) {
  
  // aggregate all the individual done files
  // create log.dummy file, will prevent cat command from issueing error
  system("cat .tamulauncher-log/log.* >> .tamulauncher-log/done");
  system("rm -f .tamulauncher-log/log.*");
  
  // create temporary sorted indices file, first element will be total #commands
  system("wc -l .tamulauncher-log/commands | cut -d' ' -f1 > .tamulauncher-log/done.indices");
  system("cut -d':' -f1 .tamulauncher-log/done | sort -nu >> .tamulauncher-log/done.indices");
  // read in the indices file 
  std::ifstream indices_file(".tamulauncher-log/done.indices");
  // read the first line, this is the total number of commands

  //  indices_file.open(".tamulauncher-log/done.indices");
  unsigned int total_commands;
  indices_file >> total_commands;
  std::vector<unsigned int> done_vector;
  unsigned int next_index=-1;
  while (! indices_file.eof()) {
    indices_file >>  next_index;
    if (next_index != -1) {
      done_vector.push_back(next_index);
    }
    next_index=-1;
  }
  
  indices_file.close();
  
  // create new todo file
  
  std::vector<unsigned int>::iterator done_it = done_vector.begin();
  std::vector<unsigned int>::iterator done_end = done_vector.end();

  // iterate over all all command indices, if not in done vector, add
  // to todo list.
  for (unsigned int i=0;i<total_commands;++i) {
    if ( done_it == done_end || i < *done_it ) {
      // index is less than next done index: add to todo list
      todo_vector.push_back(i);
    } else if (i  == *done_it) {
      // index matching next done index: don't add to todo
      // we are done with current done index so move to next done index
	++done_it;
    } else {
      // in therory this branch should never be reached
      // however the indices file will contain one duplicate
      // (last item in the vector) because of a new line in the indices file
      // to fix it jsut increast the iterator
      ++done_it;
    }
  
  }  
  // don't forget to remvoe the temp indices file
  system("rm -f .tamulauncher-log/done.indices");
    
}
  
  
int main(int argc, char** argv) {
  // initialize MPI
  MPI_Init(&argc,&argv);
  
  int mpi_task_id ;
  MPI_Comm_rank(MPI_COMM_WORLD,&mpi_task_id);

  int num_mpi_tasks;
  MPI_Comm_size(MPI_COMM_WORLD,&num_mpi_tasks);


  int num_tasks_per_node = 0; //TODO FIX

  // name of the commands file
  string filename(argv[argc-1]);


  commands_type commands(filename);


  // vector to store indices of commands that need to be processed 
  std::vector<unsigned int> todo_vec;



  if (mpi_task_id ==0) {
    // first need to read the arguments 
    // possible flags: 
    //    --tasks-per-node
    // NOTE: last argument has to be commands file
    
    
    
    // now iterate over all the arguments
    int arg_count=1;
    while (arg_count <argc-1) {
      string next_command(argv[arg_count]);
      if (next_command == "--tasks-per-node") {
	++arg_count;
	num_tasks_per_node= atoi(argv[arg_count]);
      } else {
	std::cout << "unrecognized flag: " << next_command << std::endl;
      }
      ++arg_count;
    }
    
    // create the todo vector
    create_todo_vector(todo_vec);

    
    if (todo_vec.size() == 0) {
      std::cout << "\n\n===========================================================\n";
      std::cout << "All commands have been processed.\n" <<
	"If you think this is a mistake and/or want to redo your run\n"<<
	"remove directory .tamulauncher-log and run again\n";
      std::cout << "===========================================================\n\n\n";
    }
    
    // send the options to all the mpi tasks
    int options[1];
    options[0]=num_tasks_per_node;
    MPI_Bcast(&options,1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // master broadcasts all indices and every mpi task will compute 
    // its own subset. Easiest solution
    
    // sending is easy since STL standard guarantees vectors are in
    // contiguous block of memory
    unsigned int size = todo_vec.size();
    MPI_Bcast(&size,1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    if (size > 0) {
      MPI_Bcast(&todo_vec[0],size , MPI_UNSIGNED, 0, MPI_COMM_WORLD);


      // this task will only process the first subset
      // every element will process at least (int)size/#tasks
      // if there is a remainder task 0 will definitely process 1 
      // additional element.
      int base = size / num_mpi_tasks;
      int remainder = size % num_mpi_tasks;
      int tot = base + (remainder > 0 ? 1 : 0);
      // since task 0 will only process the irst tot elements, can just resize and
      // and rest of the elements will be cut.
      todo_vec.resize(tot);

    }
    
  } else {
    
    int options[1];
    MPI_Bcast(&options,1, MPI_INT, 0, MPI_COMM_WORLD);
    num_tasks_per_node = options[0];

    // all the other tasks will receive the todo vector
    int size;
    MPI_Bcast(&size,1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    if (size > 0) {
      // cannot just assign to vector, no guarantee will be correct
      // declare array and store in there, after that copy to vector
      unsigned int temp_todo[size];
      MPI_Bcast(&temp_todo[0],size , MPI_UNSIGNED, 0, MPI_COMM_WORLD);

      // every element will process at least (int)size/#tasks
      // if task id > remainder will definitely process 1 additional  
      
      int base = size / num_mpi_tasks;
      int remainder = size % num_mpi_tasks;
      int tot = base + (mpi_task_id < remainder ? 1 : 0);
      int start_index = mpi_task_id * base + (mpi_task_id < remainder ? mpi_task_id : remainder);
      
      // copy the subset to todo_vec
      todo_vec.resize(tot);
      std::copy_n(&temp_todo[start_index],tot,todo_vec.begin());

    } 

  }


  // only continue if todo vector is not empty

  if (todo_vec.size() > 0) {
    // every mpi_task will read in all the commands, wi
    commands.read();

    // create logger, need to get the hostname for that
    char th[100];
    gethostname(th,100);
    std::string hostname(th);

    logger_type logger(hostname);

    // open the logger
    logger.open();


    // make sure the number of tasks is set correctly
    // mostly in case where #cores can not be divided by cores/node
    // in that case last node will have a few less cores.
    int num = get_tasks_per_node(hostname);
    if (num < num_tasks_per_node) {
      num_tasks_per_node=num;
    }

    std::cout << "numthreads(" << num_tasks_per_node << ")\n";
#pragma omp parallel num_threads(num_tasks_per_node)
    {
      
      // first we will add all the commands as tasks
#pragma omp single nowait
      {
	for (std::vector<unsigned int>::iterator i = todo_vec.begin(); i != todo_vec.end();++i) {
#pragma omp task
	  {
	    run_command_type& next_command = commands.command_at(*i);
	    next_command.execute();
	    logger.write_log(next_command);
	  }
	}
      }
      // all the commands have been added to the list
      
      
      
#pragma omp taskwait
      
    }
  }

  std::cout << "task " << mpi_task_id << " finished\n";

  MPI_Finalize ( );
  
}




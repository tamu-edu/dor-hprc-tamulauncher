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


using namespace std;


int main(int argc, char* argv[]) {
  
  // initialize MPI
  MPI::Init ( argc, argv );

  // gets the task id
  int task_id = MPI::COMM_WORLD.Get_rank ( );

  // total number of tasks
  int num_tasks = MPI::COMM_WORLD.Get_size ( );


  string workdir(argv[1]);
  string jobid(argv[2]);

  int num_commands;
  vector<string> commands;
  
  
  char numstr[4]; // limit max task id to 9999
  sprintf(numstr, "%d", task_id);

  // create all the file names
  string todo_file_name = workdir;
  string done_file_name = workdir;
  string log_file_name = workdir;

  todo_file_name += "/todo-";
  todo_file_name += numstr;
  done_file_name += "/done-";
  done_file_name += numstr;
  log_file_name += "/log-";
  log_file_name += numstr;
  log_file_name += ".";
  log_file_name += jobid;
  
  if (task_id ==0) {

    // store all commands from file in here
    vector<string> commands_string;

    // open the input file
    string input_file_name = workdir;
    input_file_name += "/todo";
    ifstream input_file;
    input_file.open(input_file_name.c_str());

    string line;
    getline(input_file,line);
    while (! input_file.eof()) {
      if ( ! line.empty())
        commands_string.push_back(line);
      getline(input_file,line);
    }

    // compute quotient and remainder for total num commands divided by num tasks
    // used to decide how many commands every tasks gets
    std::div_t dv = std::div(commands_string.size(),num_tasks);
    int quotient = dv.quot; int remainder = dv.rem;

    //cout << "#commands: " << commands_string.size() << ",   quotient: " << quotient << ", remainder: " << remainder << endl;
    int command_counter = 0;
    
    string task_todo_base = "tamulauncher/todo-";

    // start distributing over the tasks
    for (int task_counter=0;task_counter<num_tasks;task_counter++) {
      
      char numstr[4]; // limit max task id to 9999
      sprintf(numstr, "%d", task_counter);
      // create the name of the commands file for this task
      string task_todo_file_name = workdir;
      task_todo_file_name += "/todo-";
      task_todo_file_name += numstr;
      
      cout << "task_todo: " << task_todo_file_name << endl;
      
      // create the todo file
      ofstream task_todo_file;
      task_todo_file.open(task_todo_file_name);
      
      // compute the number of commands task 0 will excecute
      int task_num_commands = ( task_counter  < remainder ? quotient+1 : quotient);
      
      cout << "task: " << task_counter << "  : #commands " << task_num_commands << endl;
      for (int task_num_commands_counter = 0;
	   task_num_commands_counter < task_num_commands; 
	   task_num_commands_counter++) {
	
	//cout << command_counter<< ": " << commands_string[command_counter] << endl;
	task_todo_file << commands_string[command_counter++] << endl;

      }
      task_todo_file.close();
      
      if (task_counter > 0) {
	// send dummy message, so tasks knows it can start processing
	int mydummy=0;
	MPI::COMM_WORLD.Send(&mydummy,1,MPI_INT,task_counter,0);
      }
    }	
  } else {
    
    // since the commands are written to file, the only thing other
    // tasks have to do is wait until task 0 has finished writing
    // this tasks commands. Task 0 will send a dummy message to the 
    // task when finished 
    // receive the number of commands this task will handle
    int mydummy;
    MPI::COMM_WORLD.Recv(&mydummy,1 ,MPI_INT,0,0);
  }

  
  
  /*
    every task (including task 0 will read its part of the commands
    from file. It will also read the done file in.
    After that every task will start processing the commands in
    the list that are not in the done list.
    
    For now we assume that at restart the same number of task will
    be used. The current version will also not do any load balancing.
    Part of todo.
  */

  // read the commands
  ifstream todo_file;
  todo_file.open(todo_file_name.c_str());
  //cout << "todo file: " << todo_file_name << endl;

  string line;
  getline(todo_file,line);
    while (! todo_file.eof()) {
    if ( ! line.empty())
      commands.push_back(line);
    getline(todo_file,line);
     }
  num_commands=commands.size();

  ofstream done_file;
  done_file.open(done_file_name);  

  // open the output file 
  ofstream log_file;
  log_file.open(log_file_name.c_str());

  /*
    Every task will start iterating over its commands and execute them
  */
  for (int command_counter=0;command_counter<num_commands;command_counter++) {
    string command = commands[command_counter];         
    
    // execute the command, for now standard out/err is not captured 
    double tstart = MPI::Wtime();
    int ret = system(command.c_str());
    double tend = MPI::Wtime();
    
    string msg = "[FAILED]";
    string additional_msg = "";
    if (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
      additional_msg="(SIGINT or SIGQUIT detected)";
    } else if (WCOREDUMP(ret)) {
      additional_msg="(Core dump)";
    } else if (ret != 0) {
      additional_msg="(non-zero exit status)";
    } else {
      msg = "[SUCCESS]";
    }

    // print info
    log_file << "Command: " << command << "\t" << msg << " " << additional_msg <<  
      "\t time spent: (" << (tend-tstart) << " seconds)" << endl;

    // don't forget to update the done file
    done_file << command << endl;
  }
  
  // open the output file
  log_file.close();
  done_file.close();

  MPI::Finalize ( );

  return 0;    
}




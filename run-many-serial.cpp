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

// signal handler will send SIGINT to the current 
// process and to all its children to make sure the
// command that is being executed gets kill too
// (normally system() comand ignores SIGNINT)
void signal_handler(int signum) {
  //  pid_t pid = getpid();
  //std::cout << "Task " << MPI::COMM_WORLD.Get_rank() << " (pid "<< pid << ") received SIGINT. Sending SIGNINT to its children" << endl;
  //kill(0-pid,SIGKILL);
  //sleep(10);
  //exit(signum);
}


int main(int argc, char* argv[]) {
  
  // initialize MPI
  MPI::Init ( argc, argv );

  // get the task id
  int task_id = MPI::COMM_WORLD.Get_rank ( );

  // total number of tasks
  int num_tasks = MPI::COMM_WORLD.Get_size ( );


  string workdir(argv[1]);
  string jobid(argv[2]);

  // will store the commands this task will work on
  int num_commands;
  vector<string> commands;
  
  
  char numstr[5]; // limit max task id to 99999
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
    if (input_file.is_open()) {
      getline(input_file,line);
      while (! input_file.eof()) {
	if ( ! line.empty())
	  commands_string.push_back(line);
	getline(input_file,line);
      }
    } else {
      printf("task %d: warning, cannot open todo file\n",task_id);
    }

    // compute quotient and remainder for total num commands divided by num tasks
    // used to decide how many commands every tasks gets
    std::div_t dv = std::div(commands_string.size(),num_tasks);
    int quotient = dv.quot; int remainder = dv.rem;

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
      
      // create the todo file
      ofstream task_todo_file;
      task_todo_file.open(task_todo_file_name);
      
      // compute the number of commands task 0 will excecute
      int task_num_commands = ( task_counter  < remainder ? quotient+1 : quotient);

      cout << "TASK: " << task_counter << "  : #COMMANDS " << task_num_commands << endl;
      for (int task_num_commands_counter = 0;
	   task_num_commands_counter < task_num_commands; 
	   task_num_commands_counter++) {
	
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


  //setup the signal handler, needed to kill system() calls
  //signal(SIGINT, signal_handler);
  //signal(SIGQUIT,signal_handler);
  //signal(SIGUSR2,signal_handler);

  // read the commands
  ifstream todo_file;
  todo_file.open(todo_file_name.c_str());

  string line;
  if (todo_file.is_open()) {
    getline(todo_file,line);
    while (! todo_file.eof()) {
      if ( ! line.empty())
	commands.push_back(line);
      getline(todo_file,line);
    }
    num_commands=commands.size();
  } else {
    printf("task %d: warning: cannot open todo file\n",task_id);
  }

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
    
    string msg = "";

    // check if the command executed correctly, make sure
    // it wasn't ckilled by the system
    if (WIFSIGNALED(ret)) {
      // command was interrupted by system
      if (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT) {
	msg="[FAILED] (SIGINT or SIGQUIT detected)";
      } else if (WCOREDUMP(ret)) {
	msg="[FAILED] (core dump detected)";
      } else {
        msg="[FAILED] (unknown system signal received)";
      }
    } else if (WIFSTOPPED(ret)) { 
      // command was stopped, not sure when that happens
      msg="[FAILED] (command was stopped)";
    } else if (WIFEXITED(ret)){
      // command exited correctly
      // can be clean exit or non zero exit
      if (WEXITSTATUS(ret) == EXIT_SUCCESS)
	msg = "[DONE] (command finished succesfully; EXIT_SUCCESS)";
      else
	msg = "[EXIT] (command finished with NON ZERO exit status)";
      // no matter if command had non zero exit status
      // need to update done_file
      done_file << command << endl;
    }
    
    // print info
    log_file << "Command: " << command << "\t" << msg <<  
      "\t time spent: (" << (tend-tstart) << " seconds)" << endl;

  }
  
  // open the output file
  log_file.close();
  done_file.close();

  MPI::Finalize ( );

  // sleep for 1 seconds. This is done to REDUCE chance
  // the program returns with EXIT_SUCCESS  during the kill phase
  // i.e. kill process is underway but kill to this executable
  // not finished
  // sleep(3);

  return 21;    
}




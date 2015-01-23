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
  // argv[1] --> filename with commands
  // argv[2] --> log filename base
  
  // initialize MPI
  MPI::Init ( argc, argv );

  // gets the task id
  int task_id = MPI::COMM_WORLD.Get_rank ( );

  // total number of tasks
  int num_tasks = MPI::COMM_WORLD.Get_size ( );

  string filename(argv[1]);
  string logfile(argv[2]);

  int num_commands;
  vector<string> commands;
  
  // MPI task 0 will read the commands file and distribute to the other tasks
  if (task_id ==0) {
    
    /*
      task 0 will do two things:
      1) read the input file containing all the commands
      2) distribute the commands among all the tasks
     */


    // step 1: read all the commands from file
    vector<string> commands_string;

    ifstream in;    
    in.open(filename.c_str());  
    string line;
    getline(in,line);
    while (! in.eof()) {
      if ( ! line.empty()) 
	commands_string.push_back(line);
      getline(in,line);
    }

    
    /* 
       distribute the command over the tasks. This is done as follows:
       1) pack all the commands for the task into one long string
       2) send the number of elements this task will handle
       3) send array containing lengths of every command
       4) send packed string
    */


    // compute quotient and remainder for total num commands divided by num tasks
    // used to decide how many commands every tasks gets
    std::div_t dv = std::div(commands_string.size(),num_tasks);
    int quotient = dv.quot; int remainder = dv.rem;

    // compute the number of commands task 0 will excecute
    num_commands = ( task_id  < remainder ? quotient+1 : quotient);

    // task 0 will handle command_strings[0 - num_commands-1], so no need to send these elements.
    // Start sending from  command_strings[num_commands]
    int line_counter = num_commands;


    // start distributing over the tasks
    for (int task_counter=1;task_counter<num_tasks;++task_counter) {

      // compute the number of elements that task i will handle
      int task_num_commands = ( task_counter<remainder ? quotient+1 : quotient);
 
     // 1) pack all the commands in one big char vector
      string buffer = "";
      int line_lenghts[task_num_commands];
      for ( int counter = 0 ; counter < task_num_commands; ++counter, ++line_counter) {
	line_lenghts[counter] = commands_string[line_counter].length();
	buffer.append(commands_string[line_counter]);
      }


      // 2) send the number of elements this task will handle
      MPI::COMM_WORLD.Send(&task_num_commands,1,MPI_INT,task_counter,0);
      // 3) send array containing lengths of every command
      MPI::COMM_WORLD.Send(&line_lenghts,task_num_commands,MPI_INT,task_counter,0);
      // 4) send packed string
      MPI::COMM_WORLD.Send(buffer.c_str(), buffer.size(), MPI_CHAR, task_counter, 0);
      

      // only thing left to do is fill the command vector for task 0
      for (int count = 0; count < num_commands; ++count) {
	commands.push_back(commands_string[count]);
      }
    }
    
  } else {
    // all other tasks will recieve there parts

    // receive the number of commands this task will handle 
    MPI::COMM_WORLD.Recv(&num_commands,1 ,MPI_INT,0,0);

    // receive array containing lenghts per command
    int command_lenghts[num_commands];
    MPI::COMM_WORLD.Recv(&command_lenghts,num_commands,MPI_INT,0,0);

    // receive the packed string, to get the total size of string compute 
    // sum of command lenghts
    int total =0; for (int i=0;i<num_commands;++i) total += command_lenghts[i]; 
  
    char buf[total];
    MPI::COMM_WORLD.Recv(&buf,total,MPI_CHAR,0,0);

    // next step is to unpack the command string
    string command_string = buf;

    int index = 0;
    for (int i = 0; i<num_commands;++i) {
      commands.push_back(command_string.substr(index,command_lenghts[i]));
      index += command_lenghts[i];
    }
  }
  

  /*
    Every task will start processing the commands it was assigned.
    For logging purposes every task will write execution time for
    every command, and possible failures to it's own file
  */
  
  char numstr[4]; // limit max task id to 9999
  sprintf(numstr, "%d", task_id);
  logfile += numstr;

  // open the output file 
  ofstream out_file;
  out_file.open(logfile.c_str());
  

  for (int command_counter=0;command_counter<num_commands;++command_counter) {
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
    out_file << "Command: " << command << "\t" << msg << " " << additional_msg <<  
      "\t time spent: (" << (tend-tstart) << " seconds)" << endl;
  }
  
  // open the output file
  out_file.close();

  MPI::Finalize ( );
    
}




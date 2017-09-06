
#include <iostream>
#include <fstream>

#include "commands_type.hpp"
#include "run_command_type.hpp"

using namespace std;

void 
commands_type::read_commands() {
  ifstream in;
  in.open(commands_file_name.c_str());
  string line;
  getline(in,line);
  while (! in.eof()) {
    if ( ! line.empty()) {
      run_command_type cmd(line,commands.size());
      commands.push_back(cmd);
    }
    getline(in,line);
  }
}




void 
commands_type::read_processed() {
  // read the commands that have been processed
  ifstream processed_file_read(processed_file_name.c_str());
  if (processed_file_read.peek() == std::ifstream::traits_type::eof()) {
    // file is empty, nothing to do
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
}


void 
commands_type::init() {
  // determine which commands should be executed. Will enable/disable commands
  // depending if they are in the processed and signaled list. 
  // order is important. Can be the case that in one run a command got a signal
  // but in the next run it ran fine. So signaled should be processed first.
  // in general; signaled --> non-zero --> zero-exit

  // by default all commands will be enabled

  enabled_commands.resize(commands.size());
  fill(enabled_commands.begin(),enabled_commands.end(),true);

  // enable/disable all commands in signal list, depending if kill commands need to be rerun
  for (vector<int>::iterator it=signaled_commands.begin(); it != signaled_commands.end(); ++it) {
    enabled_commands[(*it)]=rerun_signaled;
  }

  // enable/disable all commands in exited list
  for (vector<int>::iterator it=exited_commands.begin(); it != exited_commands.end(); ++it) {
    enabled_commands[(*it)]=rerun_exited;
    printf("exited: command_index=%d, enable=%d",(*it),rerun_exited);
  }
  
  // disable all commands that have been processed
  for (vector<int>::iterator it=processed_commands.begin(); it != processed_commands.end(); ++it) {
    enabled_commands[(*it)]=false;
  }


  // disable all commands that start with a #
  // # will override commands in signaled list.
  for (int count=0;count<commands.size();++count) {
    const string& cmd = commands[count].get_command_string();
    if (cmd.length() > 0 && cmd.at(0) == '#') {
      
      enabled_commands[count]=false;
    }
  }


  // remove the disabled commands from the list
  all_commands=commands;
  commands.clear();
  for (int i=0;i<enabled_commands.size();++i) {
    if (enabled_commands[i]) {
      commands.push_back(all_commands[i]);
    }
  }
  // set commands_index to first enabled command
  proceed_next();
}


void
commands_type::read_exited() {
  // read the commands that have been processed
  ifstream exited_file_read(exited_file_name.c_str());
  if (exited_file_read.peek() == std::ifstream::traits_type::eof()) {
    // file is empty, nothing to do                                                                                                                         
  } else {
    int next_exited;
    while (! exited_file_read.eof()) {
      next_exited=-1;
      exited_file_read >> next_exited;
      if (next_exited != -1)
        exited_commands.push_back(next_exited);
    }
  }
  printf("NUMBER of EXITED COMMANDS: %d\n", exited_commands.size());
  exited_file_read.close();
}



// read the file with signaled commands
void 
commands_type::read_signaled() {
  ifstream signaled_file_read(signaled_file_name.c_str());
  if (signaled_file_read.peek() == std::ifstream::traits_type::eof()) {
    // file is empty, nothing to do
  } else {
    int next_signaled;
    while (! signaled_file_read.eof()) {
      next_signaled=-1;
      signaled_file_read >> next_signaled;
      if (next_signaled != -1)
	signaled_commands.push_back(next_signaled);
    }
  }
  signaled_file_read.close();
}


void 
commands_type::setup() {
  // read in all the commands
  read_commands();
  // read in the commands that have been processed
  read_processed();
  // read in all the signaled commands
  read_signaled();

  // read in all the exited commands
  read_exited();

  // setup the inital data structures
  init();
}


commands_type::commands_type(string& name,bool signaled,bool exited) {
  commands_file_name=name;
  rerun_signaled = signaled;
  rerun_exited = exited;

  printf("signaled=%d, exited=%d\n",rerun_signaled,rerun_exited);

  setup();

  // check if all commands have been processed, notify user if so.
  if (!has_next()) {
    cout << "\n\n===========================================================\n";
    cout << "All commands have been processed.\n" <<
      "If you think this is a mistake and/or want to redo your run\n"<<
      "remove directory .tamulauncher and run again\n";
    cout << "===========================================================\n\n\n";
  }

}


void commands_type::proceed_next() {
  // need to move the index at least 1;
  ++commands_index;
}


bool commands_type::has_next() {
  return (commands_index < commands.size());
}

int commands_type::num_commands() {

}

run_command_type&  commands_type::get_command() {
  return commands[commands_index];
}

run_command_type& commands_type::get_command(int index) {
  return all_commands[index];
}


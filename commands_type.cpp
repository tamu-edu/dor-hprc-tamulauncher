
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
  // by default all commands will be enabled
  enabled_commands.resize(commands.size());
  fill(enabled_commands.begin(),enabled_commands.end(),true);
  
  // disable all commands that have been processed
  for (vector<int>::iterator it=processed_commands.begin(); it != processed_commands.end(); ++it) {
    enabled_commands[(*it)]=false;
  }
  
  // enable/disable all commands in signal list, depending if kill commands need to be rerun
  for (vector<int>::iterator it=signaled_commands.begin(); it != signaled_commands.end(); ++it) {
    enabled_commands[(*it)]=rerun_signaled;
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
  vector<run_command_type> temp_commands(commands);
  commands.clear();
  for (int i=0;i<enabled_commands.size();++i) {
    if (enabled_commands[i]) {
      commands.push_back(temp_commands[i]);
    }
  }
  temp_commands.clear();
  // set commands_index to first enabled command
  proceed_next();
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
  // setup the inital data structures
  init();
}


commands_type::commands_type(string& name,bool rerun) {
  commands_file_name=name;
  rerun_signaled = rerun;
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
  return commands[index];
}


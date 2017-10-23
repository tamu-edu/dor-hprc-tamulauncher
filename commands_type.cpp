
#include <iostream>
#include <fstream>

#include "commands_type.hpp"
#include "run_command_type.hpp"


commands_type::commands_type(std::string& name) : dummy_command("",-1) {
  commands_file_name=name;
}


void 
commands_type::read() {
  std::ifstream in;
  in.open(commands_file_name.c_str());
  std::string line;
  getline(in,line);
  unsigned int counter=0;
  while (! in.eof()) {
      run_command_type cmd(line,counter);
      commands.push_back(cmd);
      ++counter;
      getline(in,line);
  }
}


run_command_type& commands_type::command_at(unsigned int index) {
  if ( index < commands.size() ) {
    return commands[index];
  } else {
    return dummy_command;
  }
}
 
unsigned int commands_type::num_commands() {
  return commands.size();
}



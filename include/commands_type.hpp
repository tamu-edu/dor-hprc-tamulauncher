#ifndef COMMANDS_TYPE_H
#define COMMANDS_TYPE_H

#include <vector>
#include <string>

#include "run_command_type.hpp"

using namespace std;

class commands_type {
  
private:
  
  run_command_type dummy_command;
  
  string commands_file_name;
  
  vector<run_command_type> commands;


  void read_commands();
 
  void init();
  void setup();

public:
  
  commands_type(string& name);

  void read();

  run_command_type& command_at(unsigned int index);

  unsigned int num_commands();
};
  
#endif




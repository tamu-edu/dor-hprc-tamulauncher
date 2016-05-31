#ifndef COMMANDS_TYPE_H
#define COMMANDS_TYPE_H

#include <vector>
#include <string>

#include "run_command_type.hpp"

using namespace std;

class commands_type {
  
private:


  const string processed_file_name = ".tamulauncher.processed";
  const string signaled_file_name  = ".tamulauncher.signaled";
  
  // note: commands_index should start at -1
  // proceed method will increase it to 0 during init
  int commands_index=-1;

  bool rerun_signaled=false;

  string commands_file_name;
  
  vector<run_command_type> commands;
  vector<bool> enabled_commands;
  vector<int> processed_commands;
  vector<int> signaled_commands;


  void read_commands();
  void read_processed();
  void read_signaled();
 
  void init();
  void setup();

public:
  
  commands_type(string& name,bool rerun);
  
  void proceed_next();
  bool has_next(); 
  int num_commands();

  run_command_type& get_command();
  run_command_type& get_command(int index);

};
  
#endif




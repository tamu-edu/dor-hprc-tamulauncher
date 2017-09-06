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
  const string exited_file_name    = ".tamulauncher.exited";

  // note: commands_index should start at -1
  // proceed method will increase it to 0 during init
  int commands_index=-1;

  bool rerun_signaled=false;
  bool rerun_exited=false;

  string commands_file_name;
  
  vector<run_command_type> commands;
  vector<run_command_type> all_commands;

  vector<bool> enabled_commands;
  vector<int> processed_commands;
  vector<int> signaled_commands;
  vector<int> exited_commands;

  void read_commands();
  void read_processed();
  void read_signaled();
  void read_exited();
 
  void init();
  void setup();

public:
  
  commands_type(string& name,bool signaled, bool exited);
  
  void proceed_next();
  bool has_next(); 
  int num_commands();

  run_command_type& get_command();
  run_command_type& get_command(int index);

};
  
#endif




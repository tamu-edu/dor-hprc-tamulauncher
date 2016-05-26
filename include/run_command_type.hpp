#ifndef RUN_COMMAND_TYPE_H
#define RUN_COMMAND_TYPE_H

#include <string>

using namespace std;

class run_command_type {

private:

  int command_index;
  string command;
  int return_code;
  double run_time;

  void fix_return_code();


public:

  run_command_type(string s,int index);

  string& get_command_string();

  int get_index();

  double get_runtime();

  int get_return_code();
  
  void execute(int task_id);

};

#endif
